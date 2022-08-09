// $Id: spooler.cxx 15045 2011-08-26 07:09:06Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "scheduler_client.h"

#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#ifdef Z_WINDOWS
#   include <process.h>
#   include <direct.h>
#   include <io.h>
#endif

#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../kram/log.h"
#include "../file/anyfile.h"
#include "../file/stdfile.h"    // make_path
#include "../kram/licence.h"
#include "../kram/sos_mail.h"
#include "../kram/sos_java.h"
#include "../zschimmer/com_remote.h"
#include "../zschimmer/xml_end_finder.h"
#include "../zschimmer/z_signals.h"
#include "../zschimmer/not_in_recursion.h"
#include "../zschimmer/file_path.h"
#include "jni__register_native_classes.h"
#include "../javaproxy/java__io__File.h"
#include "../javaproxy/java__net__URI.h"
#include "../javaproxy/java__net__URL.h"
#include "../javaproxy/com__sos__scheduler__engine__kernel__async__CppCall.h"

using namespace std;
using namespace zschimmer::file;


namespace sos {

extern const Bool _dll = false;

#ifdef Z_WINDOWS
    extern HINSTANCE _hinstance; 
#endif

static ptr<javabridge::Vm> start_java(const string& options, const string& class_path);

namespace scheduler {


const char*                     default_factory_ini                 = "factory.ini";
const int                       max_open_log_files                  = 50;               // Anzahl der offenzuhaltenden Log-Dateien. Wenn's mehr wird, wird die älteste geschlossen.
const int                       windows_maxstdio                    = 2048;             // Anzahl stdio-Handles für Windows
const string                    new_suffix                          = "~new";           // Suffix für den neuen Spooler, der den bisherigen beim Neustart ersetzen soll
const double                    renew_wait_interval                 = 0.25;
const double                    renew_wait_time                     = 30;               // Wartezeit für Brückenspooler, bis der alte Spooler beendet ist und der neue gestartet werden kann.

const Duration                  before_suspend_wait_time            = Duration(5);      // Diese Zeit vor Suspend auf Ereignis warten (eigentlich so kurz wie möglich)
const Duration                  inhibit_suspend_wait_time           = Duration(10*60);  // Nur Suspend, wenn Wartezeit länger ist
const Duration                  show_message_after_seconds          = Duration(15*60);  // Nach dieser Wartezeit eine Meldung ausgeben
const Duration                  show_message_after_seconds_debug    = Duration(60);     // Nach dieser Wartezeit eine Meldung ausgeben

const int                       nothing_done_max                    = 10;  //2+1;        // Ein überflüssiges Signal wird toleriert wegen Race condition, und dann gibt es manch voreiliges Signal (vor allem zu Beginn einer Operation)
                                                                                        // +1 für Job::start_when_directory_changed() unter Windows
const double                    nichts_getan_bremse                 = 1.0;              // Wartezeit in nichts_getan(), Meldung SCHEDULER-261
#ifdef Z_DEBUG
    const int                   scheduler_261_second                = 5;                // Nach sovielen leeren Schleifendurchläufen SCHEDULER-261 wiederholen
    const int                   scheduler_261_intervall             = 10;               // Meldung SCHEDULER-261 wiederholen (s.a. nichts_getan_bremse), sonst unterdrücken
#else
    const int                   scheduler_261_second                = 10;               // Nach sovielen leeren Schleifendurchläufen SCHEDULER-261 wiederholen
    const int                   scheduler_261_intervall             = 1000;             // ca. 20 Minuten
#endif

const int                       kill_timeout_1                      = 10;               // < kill_timeout_total
const int                       kill_timeout_total                  = 30;               // terminate mit timeout: Nach timeout und kill noch soviele Sekunden warten

const int                       tcp_restart_close_delay             = 3;                // Wartezeit, damit der Browser nicht hängenbleibt.
                                                                                        // Sonst wird die HTTP-Verbindung nicht richtig geschlossen. Warum bloß?
                                                                                        // Client (Internet Explorer) bekommt so Gelegenheit, selbst die Verbindung zu schließen.
                                                                                        // Siehe auch Spooler_communication::close(): set_linger( true, 1 );

const int                       const_order_id_length_max           = 250;              // Die Datenbankspalte _muss_ so groß sein, sonst bricht Scheduler mit SCHEDULER-303, SCHEDULER-265 ab!

const Duration                  delete_temporary_files_delay        = Duration(2);                                
const Duration                  delete_temporary_files_retry        = Duration(0.1);                              

static bool                     is_daemon                           = false;

volatile int                    ctrl_c_pressed                      = 0;                // Tatsächliches Signal ist in last_signal
volatile int                    ctrl_c_pressed_handled              = 0;
#ifndef Z_WINDOWS
    volatile int                last_signal                         = 0;                // Signal für ctrl_c_pressed
#endif

bool                            static_ld_library_path_changed      = false;
string                          static_original_ld_library_path     = "";               // Inhalt der Umgebungsvariablen LD_LIBRARY_PATH
#ifdef Z_HPUX
    string                      static_ld_preload                   = "";               // Inhalt der Umgebungsvariablen LD_PRELOAD
#endif

static Spooler*                 spooler_ptr                         = NULL;

const string                    variable_set_name_for_substitution  = "$";              // Name der Variablenmenge für die ${...}-Ersetzung

//-------------------------------------------------------------------------------------------------

extern zschimmer::Message_code_text  scheduler_messages[];            // messages.cxx, generiert aus messages.xml
extern const char               _author_[]                          = "\n\n" "Scheduler, 2000-2007 Joacim Zschimmer, Zschimer GmbH, http://www.zschimmer.com\n\n";

//-------------------------------------------------------------------------------------------------

//DEFINE_SIMPLE_CALL(Spooler, Pause_scheduler_call)
//DEFINE_SIMPLE_CALL(Spooler, Continue_scheduler_call)
//DEFINE_SIMPLE_CALL(Spooler, Reload_scheduler_call)
//DEFINE_SIMPLE_CALL(Spooler, Terminate_scheduler_call)
//DEFINE_SIMPLE_CALL(Spooler, Let_run_terminate_and_restart_scheduler_call);

//-----------------------------------------------------------------------------------Error_settings

struct Error_settings
{
    Error_settings() : _zero_(this+1) {}

    void read( const string& ini_file )
    {
        _from = read_profile_string( ini_file, "spooler", "log_mail_from", _from );
        _to   = read_profile_string( ini_file, "spooler", "log_mail_to"  , _to   );
        _cc   = read_profile_string( ini_file, "spooler", "log_mail_cc"  , _cc   );
        _bcc  = read_profile_string( ini_file, "spooler", "log_mail_bcc" , _bcc  );
        _smtp = read_profile_string( ini_file, "spooler", "smtp"         , _smtp );
    }

    Fill_zero                  _zero_;
    string                     _from;
    string                     _to;
    string                     _cc;
    string                     _bcc;
    string                     _smtp;
};

//-------------------------------------------------------------------------------------------------

static Error_settings           error_settings;

//-------------------------------------------------------------------------------------------------

/*
struct Set_console_code_page
{
                                Set_console_code_page       ( uint cp )         { _cp = GetConsoleOutputCP(); SetConsoleOutputCP( cp ); }
                               ~Set_console_code_page       ()                  { SetConsoleOutputCP( _cp ); }

    uint                       _cp;
};
*/

static void set_ctrl_c_handler( bool on );

//----------------------------------------------------------------------Termination_async_operation

struct Termination_async_operation : Async_operation
{
    enum State
    {
        s_ending,
        s_killing_1,
        s_killing_2,
        s_finished
    };

                                Termination_async_operation ( Spooler*, time_t timeout_at );

    virtual bool                async_continue_             ( Continue_flags );
    virtual bool                async_finished_             () const                                { return false; }
    virtual string              async_state_text_           () const                                { return "Termination_async_operation"; }


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    State                      _state;
    time_t                     _timeout_at;
};

//--------------------------------------------------------------------------------------print_usage

static void print_usage()
{
    cerr << "\n"
            "usage: " << (_argc <= 0? "(scheduler)" : _argv[0]) << "\n"
            "       [-config=]XMLFILE\n"
            "       -validate-xml-\n"
            "\n"
            "       -?\n"
            "       -V\n"
            "\n"
            "       -java-classpath=...\n"
            "       -java-options=...\n"
            "       -cd=PATH\n"
            "       -log=HOSTWARELOGFILE\n"
            "       -log-dir=DIRECTORY|*stderr\n"
            "       -log-level=error|warn|info|debug|debug1|...|debug9\n"
            "       -id=ID\n"
            "       -param=PARAM\n"
            "       -cmd=\"<xml_command/>\"\n"
            "       -include-path=PATH\n"
            "       -ini=FILE\n"
            "       -sos.ini=FILE\n"
            "\n"
            "       -port=N\n"
            "       -tcp-port=N\n"
            "       -udp-port=N\n"
            "       -reuse-port\n"
            "\n"
            "       -exclusive\n"
            "       -backup\n"
            "       -backup-precedence=N\n"
            "       -distributed-orders\n"
            "\n"
            "       -send-cmd=\"<xml_command/>\"\n"
            "\n"
            "       -pid-file=FILE\n"
            "       -kill[=PID]\n"
            "       -service\n"
#               ifdef Z_WINDOWS
            "       -install-service[=NAME]\n"
            "       -remove-service[=NAME]\n"
            "       -renew-service[=NAME]\n"
            "       -service-name=NAME\n"
            "       -service-display=STRING\n"
            "       -install-descr=STRING\n"
            "       -need-service=SERVICE\n"
#               endif
            "\n"
            "SCHEDULER CLIENT\n"
            "       -scheduler=HOST:PORT\n"
            "       -language=shell|javascript|vbscript|perlscript\n"
            "       -process-class=NAME\n"
            "       -at='DATE TIME'\n"
            "       -job-chain=NAME\n"
            "       -order-id=ID\n"
            "\n";
}

//----------------------------------------------------------------------------------test_read_value

static void test_read_value()
{
    assert( make_history_on_process( "1", 0 ) == 1 );
    assert( make_history_on_process( "yes", 1 ) == 1 );
    assert( make_history_on_process( "no", 1 ) == 0 );
    assert( make_history_on_process( "", 1 ) == 1 );
    assert( make_history_on_process( "", 0 ) == 0 );

    assert( make_archive( "yes" , arc_yes ) == arc_yes );
    assert( make_archive( "no"  , arc_yes ) == arc_no );
    assert( make_archive( "gzip", arc_yes ) == arc_gzip );
    assert( make_archive( ""    , arc_no  ) == false );
}

//----------------------------------------------------------------------------------------self_test

static void self_test()
{
    zschimmer::Log_categories::self_test();
    zschimmer::file::File_path::self_test();
    Path::self_test();
    Absolute_path::self_test();
    test_read_value();
}

//---------------------------------------------------------------------------------send_error_email

static void send_error_email( const string& subject, const string& text )
{
    try
    {
        Sos_ptr<mail::Message> msg = mail::create_message();

        error_settings.read( default_factory_ini );

        if( error_settings._from != "" )  msg->set_from( error_settings._from );
        if( error_settings._to   != "" )  msg->set_to  ( error_settings._to   );
        if( error_settings._cc   != "" )  msg->set_cc  ( error_settings._cc   );
        if( error_settings._bcc  != "" )  msg->set_bcc ( error_settings._bcc  );
        if( error_settings._smtp != "" )  msg->set_smtp( error_settings._smtp );

        string body = remove_password( text );

        msg->add_header_field( "X-SOS-Spooler", "" );
        msg->set_subject( remove_password( subject ) );
        msg->set_body( remove_password( text ) );
        msg->send(); 
    }
    catch( const exception& ) {}
}

//---------------------------------------------------------------------------------send_error_email

void send_error_email( const exception& x, int argc, char** argv, const string& parameter_line, Spooler* spooler )
{
    string body = "Scheduler could not start.\n"
                  "\n"
                  "\n"
                  "The command line was:\n"
                  "\n";
                   
    for( int i = 0; i < argc; i++ )  body += argv[i], body += ' ';
    body += parameter_line;

    body += "\n\n\n"
            "Error message:\n";
    body += x.what();

    string subject = "ERROR ON SCHEDULER START: " + string( x.what() );

    if( spooler )
    {
        Scheduler_event scheduler_event ( scheduler::evt_scheduler_fatal_error, log_error, spooler );
        scheduler_event.set_error( x );
        scheduler_event.set_scheduler_terminates( true );

        Mail_defaults mail_defaults( spooler );
        mail_defaults.set( "subject", subject );
        mail_defaults.set( "body"   , body    );

        scheduler_event.send_mail( mail_defaults );
    }
    else           
        send_error_email( subject, body );
}

//---------------------------------------------------------------------read_profile_mail_on_process

int read_profile_mail_on_process( const string& profile, const string& section, const string& entry, int deflt )
{
    string v = read_profile_string( profile, section, entry );

    if( v == "" )  return deflt;

    try
    {
        if( isdigit( (unsigned char)v[0] ) )  return as_int(v);
                                        else  return as_bool(v);
    }
    catch( const exception& ) { return deflt; }
}

//------------------------------------------------------------------read_profile_history_on_process

int read_profile_history_on_process( const string& profile, const string& section, const string& entry, int deflt )
{
    string result;
    string v = read_profile_string( profile, section, entry );

    try
    {
        return make_history_on_process( v, deflt );
    }
    catch( exception& x ) { z::throw_xc( "SCHEDULER-335", profile + " [" + section + "] " + entry, v, x ); }
}

//--------------------------------------------------------------------make_history_on_process

int make_history_on_process( const string& v, int deflt )
{
    string result;

    if( v == "" )  return deflt;

    if( isdigit( (unsigned char)v[0] ) )  return as_int(v);
                                    else  return as_bool(v)? 1 : 0;
}

//-----------------------------------------------------------------------------read_profile_archive

Archive_switch read_profile_archive( const string& profile, const string& section, const string& entry, Archive_switch deflt )
{
    string value = read_profile_string( profile, section, entry );

    if( value == "" )  return deflt;
    if( lcase(value) == "gzip" )  return arc_gzip;

    return read_profile_bool( profile, section, entry, false )? arc_yes : arc_no;
}

//-------------------------------------------------------------------------------make_archive

Archive_switch make_archive( const string& value, Archive_switch deflt )
{
    if( value == "" )  return deflt;
    if( lcase(value) == "gzip" )  return arc_gzip;

    return as_bool( value )? arc_yes : arc_no;
}

//----------------------------------------------------------------------------read_profile_with_log

With_log_switch read_profile_with_log( const string& profile, const string& section, const string& entry, With_log_switch deflt )
{
    return read_profile_archive( profile, section, entry, deflt );
}

//--------------------------------------------------------------------read_profile_yes_no_last_both

First_and_last read_profile_yes_no_last_both( const string& profile, const string& section, const string& entry, First_and_last deflt )
{
    return make_yes_no_last_both( entry, read_profile_string( profile, section, entry ), deflt );
}

//----------------------------------------------------------------------make_yes_no_last_both

First_and_last make_yes_no_last_both( const string& setting_name, const string& value, First_and_last deflt )
{
    First_and_last result;

    if( value == ""                    )  result = deflt;
    else
    if( value == "all"                 )  result = fl_all;
    else
    if( value == "first_only"          )  result = fl_first_only;
    else
    if( value == "last_only"           )  result = fl_last_only;
    else
    if( value == "first_and_last_only" )  result = fl_first_and_last_only;
    else
        z::throw_xc( "SCHEDULER-391", setting_name, value, "all, first_only, last_only, first_and_last_only" );

    return result;
}

//-----------------------------------------------------------------------------------ctrl_c_handler
#ifdef Z_WINDOWS

    static BOOL WINAPI ctrl_c_handler( DWORD dwCtrlType )
    {
        if( dwCtrlType == CTRL_C_EVENT )
        {
            ctrl_c_pressed++;

            if( ctrl_c_pressed - ctrl_c_pressed_handled > 4 )
            {
                if( spooler_ptr )  spooler_ptr->abort_now();  
                return false;
            }

            //Kein Systemaufruf hier in der Interrupt-Routine!
            if( spooler_ptr )  spooler_ptr->signal(); //_call_register.call<Ctrl_c_scheduler_call>();
            return true;
        }
        else
            return false;
    }

#else

    static void ctrl_c_handler( int sig )
    {
        switch( sig )
        {
            case SIGUSR1:
            {
                z::static_log_categories.toggle_all();         // Das kann vom Signal-Handler aufgerufen werden.
                ::signal( sig, ctrl_c_handler );        // Signal wieder zulassen.
                break;
            }

            case SIGINT:        // Ctrl-C
            case SIGTERM:       // Normales kill
            {
                ctrl_c_pressed++;
                last_signal = sig;

                if( ctrl_c_pressed - ctrl_c_pressed_handled > 4 )  spooler_ptr->abort_now();

                //Kein Systemaufruf hier! (Aber bei Ctrl-C riskieren wir einen Absturz. Ich will diese Meldung sehen.)
                //if( !is_daemon )  fprintf( stderr, "Scheduler wird wegen kill -%d beendet ...\n", sig );

                // pthread_mutex_lock:
                // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
                // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
                // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.

                if( spooler_ptr )  spooler_ptr->async_signal( "Ctrl+C" );
                break;
            }

            default: 
                fprintf( stderr, "Unknown signal %d\n", sig );
        }

        set_ctrl_c_handler( ctrl_c_pressed <= 5 );
    }

#endif
//-------------------------------------------------------------------------------set_ctrl_c_handler

static void set_ctrl_c_handler( bool on )
{
    //Z_LOG2( "scheduler", "set_ctrl_c_handler(" << on << ")\n" );

#   ifdef Z_WINDOWS

        SetConsoleCtrlHandler( ctrl_c_handler, on );

#    else

        ::signal( SIGINT , on? ctrl_c_handler : SIG_DFL );      // Ctrl-C
        ::signal( SIGTERM, on? ctrl_c_handler : SIG_DFL );      // Normales kill 
        ::signal( SIGUSR1, on? ctrl_c_handler : SIG_DFL );      // Log erweitern oder zurücknehmen

#   endif
}

//----------------------------------------------------------------------------------------setmaxstdio

static void setmaxstdio() {
    #ifdef Z_WINDOWS
        Z_LOG2("scheduler", "_getmaxstdio()=" << _getmaxstdio() << "\n");
        if (_getmaxstdio() < scheduler::windows_maxstdio) {
            _setmaxstdio(scheduler::windows_maxstdio);
            Z_LOG2("scheduler", "_setmaxstdio(" << scheduler::windows_maxstdio << "), _getmaxstdio()=" << _getmaxstdio() << "\n");
        }
    #endif
}

//----------------------------------------------------------------------------------------be_daemon
#ifdef Z_UNIX

static void be_daemon()
{
    Z_LOG2( "scheduler", "fork()\n" );

    switch( fork() )
    {
        case  0: Z_LOG2( "scheduler", "pid=" << getpid() << "\n" );
                 zschimmer::main_pid = getpid();

                 Z_LOG2( "scheduler", "setsid()\n" );
                 setsid(); 

                 if( isatty( fileno(stdin) ) ) 
                 {
                     FILE* f = freopen( "/dev/null", "r", stdin );
                     if( !f )  throw_errno( errno, "/dev/null" );
                 }

                 break;

        case -1: throw_errno( errno, "fork" );

        default: ::sleep(1);  // Falls der Daemon noch was ausgibt, sollte das vor dem Shell-Prompt sein.
            
                 // 2008-06-12 Besser: Warten, bis der Daemon ein Okay gegeben oder sich beendet hat.
                 // Wann ist es Okay? Nach der Datenbank, vor dem TCP-Port? 
                 // Oder doch besser nach dem Port, also bei s_waiting_for_activation? Das wäre am saubersten.

                 //fprintf( stderr, "Daemon gestartet. pid=%d\n", pid ); 
                 //fflush( stderr );
                 _exit(0);
    }
}

#endif

//-----------------------------------------Termination_async_operation::Termination_async_operation

Termination_async_operation::Termination_async_operation( Spooler* spooler, time_t timeout_at )
:
    _zero_(this+1),
    _spooler(spooler),
    _timeout_at( timeout_at )
{
    set_async_next_gmtime( timeout_at );
}

//-----------------------------------------------------Termination_async_operation::async_continue_

bool Termination_async_operation::async_continue_( Continue_flags flags )
{
    bool something_done = false;

    if( !( flags & cont_next_gmtime_reached ) )  return false;


    switch( _state )
    {
        case s_ending:
        {
            string error_line = message_string( "SCHEDULER-256", _spooler->_task_subsystem->_task_set.size() );  // "Frist zur Beendigung des Schedulers ist abgelaufen, aber $1 Tasks haben sich nicht beendet
            
            _spooler->_log->error( error_line );

            {
                Scheduler_event scheduler_event ( evt_scheduler_kills, log_error, _spooler );
                scheduler_event.set_scheduler_terminates( true );

                Mail_defaults mail_defaults( _spooler );
                mail_defaults.set( "subject", error_line );
                mail_defaults.set( "body"   , "The tasks will be killed before the Scheduler terminates" );  // "Die Tasks werden abgebrochen, damit der Scheduler sich beenden kann."

                scheduler_event.send_mail( mail_defaults );
            }

            Z_FOR_EACH(Task_set, _spooler->_task_subsystem->_task_set, t ) {
                (*t)->cmd_end(task_end_kill_immediately);      // Wirkt erst beim nächsten Task::do_something()
            }

            //_spooler->kill_all_processes();           Es reicht, wenn die Tasks gekillt werden. Die killen dann ihre abhängigigen Prozesse.

            set_async_next_gmtime( _timeout_at + kill_timeout_1 );

            _state = s_killing_1;
            something_done = true;
            break;
        }

        case s_killing_1:
        {
            int count = int_cast(_spooler->_task_subsystem->_task_set.size());
            _spooler->_log->warn( message_string( "SCHEDULER-254", count, kill_timeout_1, kill_timeout_total ) );    // $1 Tasks haben sich nicht beendet trotz kill vor $2. Die $3s lange Nachfrist läuft weiter</title>
            //_spooler->_log->warn( S() << count << " Tasks haben sich nicht beendet trotz kill vor " << kill_timeout_1 << "s."
            //                     " Die " << kill_timeout_total << "s lange Nachfrist läuft weiter" );

            Z_FOR_EACH(Task_set, _spooler->_task_subsystem->_task_set, t) {
                _spooler->_log->warn( S() << "    " << (*t)->obj_name() );
            }

            set_async_next_gmtime( _timeout_at + kill_timeout_total );

            _state = s_killing_2;
            something_done = true;
            break;
        }

        case s_killing_2:
        {
            int count = int_cast(_spooler->_task_subsystem->_task_set.size());
            _spooler->_log->error( message_string( "SCHEDULER-255", count, kill_timeout_total ) );  // "$1 Tasks haben sich nicht beendet trotz kill vor $2s. Scheduler bricht ab"
            //_spooler->_log->error( S() << count << " Tasks haben sich nicht beendet trotz kill vor " << kill_timeout_total << "s."
            //                                      " Scheduler bricht ab" ); 

            Z_FOR_EACH(Task_set, _spooler->_task_subsystem->_task_set, t) {
                _spooler->_log->error( S() << "    " << (*t)->obj_name() );
            }

            _spooler->_shutdown_ignore_running_tasks = true;
            _state = s_finished;

            something_done = true;
            break;
        }

        case s_finished:
        {
            break;
        }
    }

    return something_done;
}

//---------------------------------------------------------------------------------Spooler::Spooler
// Die Objektserver-Prozesse haben kein Spooler-Objekt.

/*!
 * \change 2.1.2 - JS-559: new licence type scheduler-agent
 */
Spooler::Spooler(jobject java_main_context) 
: 
    Abstract_scheduler_object( this, this, Scheduler_object::type_scheduler ),
    _zero_(this+1), 
    _java_main_context(java_main_context),
    _subsystem_register(this),
    _version(version_string),
    _security(this),
    _communication(this), 
    _base_log(this),
    _wait_handles(this),
    _settings(Z_NEW(Settings)),
    _log_level( log_info ),
    _log_to_stderr_level( log_unknown ),
    _log_file_cache(log::cache::Request_cache::new_instance()),
    _db_log_level( log_none ),
    _scheduler_wait_log_category ( "scheduler.wait" ),
    _factory_ini( default_factory_ini ),
    _mail_defaults(NULL),
    _mail_on_delay_after_error  ( fl_first_and_last_only ),
    _termination_gmtimeout_at(time_max),
    _waitable_timer( "waitable_timer" ),
    _validate_xml(true),
    _environment( variable_set_from_environment() ),
    _holidays(this),
    _next_process_id(1 + rand() % 1000000000),
    _configuration_directories(confdir__max+1),
    _configuration_directories_as_option_set(confdir__max+1),
    _max_micro_step_time(Duration(10)),
    _call_register(this),
    _reuse_addr(true)
{
    _base_log.set_corresponding_prefix_log(_log);
    _log->init( this );              // Nochmal nach load_argv()
    _log->set_title( "Main log" );

    Z_DEBUG_ONLY( self_test() );

    if( spooler_ptr )  z::throw_xc( "spooler_ptr", "The JobScheduler Engine is already running in this process");

    check_licence();

    _pid          = getpid();
    _tcp_port     = 0;
    _udp_port     = 0;
  //_priority_max = 1000;       // Ein Wert > 1, denn 1 ist die voreingestelle Priorität der Jobs
    _com_log      = new Com_log( _log );
    _com_spooler  = new Com_spooler( this );
    _variables    = new Com_variable_set();
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    try
    {
        _base_log.set_corresponding_prefix_log(NULL);
        close();
    }
    catch( exception& x )
    {
        cerr << Z_FUNCTION << " " << x.what() << "\n";
    }
}

//---------------------------------------------------------------------Spooler::modifiable_settings

Settings* Spooler::modifiable_settings() const {
    if (_settings->is_freezed())  z::throw_xc("modifiable_settings");
    return _settings;
}

//--------------------------------------------------------------------------------Spooler::settings

const Settings* Spooler::settings() const {
    if (!_settings->is_freezed())  z::throw_xc("SETTINGS-NOT-FREEZED");
    return _settings;
}

//---------------------------------------------------------------------------Spooler::check_licence

void Spooler::check_licence() 
{
    /** \change 2.1.2 - JS-559: new licence type "scheduler agent" */
    if( !SOS_LICENCE( licence_scheduler) && !SOS_LICENCE( licence_scheduler_agent ) )  sos::throw_xc( "SOS-1000", "Scheduler" );       // Früh prüfen, damit der Fehler auch auftritt, wenn die sos.ini fehlt.
    _remote_commands_allowed_for_licence = SOS_LICENCE(licence_scheduler_agent) != NULL;
    if (!_remote_commands_allowed_for_licence) Z_LOG2( "scheduler", "executing of remote commands are not allowed (licence key for agent is required. sales@sos-berlin.com).\n" );
    if ( Log_ptr::is_demo_version() )  
       Z_LOG2( "scheduler", "JobScheduler is running with open source licence.\n" );
    else
       Z_LOG2( "scheduler", "JobScheduler is running with commercial licence.\n" );
    sos_static_ptr()->_licence->log_licence_keys();
}

//-----------------------------------------------------------------------------------Spooler::close

void Spooler::close()
{
    spooler_ptr = NULL;

    set_ctrl_c_handler( false );

    destroy_subsystems();
    
    _security.clear();

    _waitable_timer.close();
    _scheduler_event.close();
    _wait_handles.close();

    release_com_objects(); // falls noch jemand eine Referenz darauf hat

    //update_console_title( 0 );
    // _log offenhalten, weil es noch vom Destruktor genutzt werden könnte.
}

//---------------------------------------------------------------------Spooler::release_com_objects

void Spooler::release_com_objects() 
{
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->set_log( NULL );
}

//------------------------------------------------------------------------------------Spooler::name

string Spooler::name() const
{
    S result;
    
    result << "Scheduler ";
    result << _complete_hostname;
    if( _tcp_port )  result << ":" << _tcp_port;

    if( _spooler_id != "" )  result << " -id=" << _spooler_id;

    return result;
}

//-------------------------------------------------------------------------Spooler::truncate_head

string Spooler::truncate_head(const string& str)
{
    int max_length = _spooler->settings()->_max_length_of_blob_entry;
    assert(max_length>=0);

    if (str.length() <= max_length)
        return str;
    else {
        string msg = message_string("SCHEDULER-722", max_length) + "\n";
        _spooler->log()->debug(msg);
        msg += "...";
        size_t tail_start = str.length() - max_length + msg.length();
        return msg + str.substr( min(tail_start,str.length()) );
    }
}

//--------------------------------------------------------------------------Spooler::security_level

Security::Level Spooler::security_level( const Ip_address& host )
{
    return _security.level( host.as_in_addr() );
}

//----------------------------------------------------------------------------------Spooler::set_id

void Spooler::set_id( const string& id )
{
    if( id.find( '/' ) != string::npos )  z::throw_xc( "SCHEDULER-365", id );
    _spooler_id = id;
}

//--------------------------------------------------------------------------------Spooler::http_url

string Spooler::http_url() const
{
    if (!settings()->_https_port.empty()) {
        return "https://" + interface_and_port_to_uri_authority(settings()->_https_port);
    } else
    if (!settings()->_http_port.empty()) {
        return "http://" + interface_and_port_to_uri_authority(settings()->_http_port);
    } else
    if (_tcp_port) {
        S result;
        result << "http://";
        if (_ip_address_as_option_set) {
            if (_ip_address.ip_string() == "127.0.0.1")
                result << _ip_address.ip_string();   // "127.0.0.1" für die Tests (gerne immer IP-Nummer, aber it 
            else 
                result << _ip_address.name();
        }
        else result << _complete_hostname;

        result << ":" << _tcp_port;
        return result;
    } else
        return "";
}

string Spooler::interface_and_port_to_uri_authority(const string& interface_and_port) const {
    string host = _complete_hostname;
    const char* s = interface_and_port.c_str();
    if (const char* colon = strchr(s, ':')) {
        string iface = string(s, colon - s);
        if (iface != "0.0.0.0") {
            host = iface;
        }
    } 
    return host + ":" + Settings::extract_port_number(interface_and_port);
}

//-----------------------------------------------------------------------Spooler::state_dom_element

xml::Element_ptr Spooler::state_dom_element( const xml::Document_ptr& dom, const Show_what& show_what )
{
    xml::Element_ptr state_element = dom.createElement( "state" );
 
    state_element.setAttribute( "time"                 , Time::now().xml_value());   // Veraltet (<answer> hat time).
    state_element.setAttribute( "time_zone"            , _time_zone_name);
    state_element.setAttribute( "id"                   , id() );
    state_element.setAttribute( "spooler_id"           , id() );
    state_element.setAttribute( "spooler_running_since", start_time().xml_value(time::without_ms));
    state_element.setAttribute( "state"                , state_name() );
    state_element.setAttribute( "log_file"             , _base_log.filename() );
    state_element.setAttribute( "version"              , version_string );
    state_element.setAttribute_optional( "version_commit_hash", SchedulerJ::versionCommitHash() );    
    state_element.setAttribute( "pid"                  , _pid );
    state_element.setAttribute( "config_file"          , _configuration_file_path );
    state_element.setAttribute( "configuration_directory", local_configuration_directory());
    state_element.setAttribute( "host"                 , _short_hostname );
    state_element.setAttribute_optional("http_port", _settings->_http_port);
    state_element.setAttribute_optional("https_port", _settings->_https_port);
    if( _tcp_port )
    state_element.setAttribute( "tcp_port"             , _tcp_port );

    if( _udp_port )
    state_element.setAttribute( "udp_port"             , _udp_port );

    if( _ip_address )
    state_element.setAttribute( "ip_address"           , _ip_address.ip_string() );

    if( _db ) {
        string db_name = _db->db_name();
        db_name = remove_password( db_name );
        state_element.setAttribute( "db", trim( db_name ) );

        if( _db->is_waiting() )
            state_element.setAttribute( "db_waiting", "yes" );

        if( _db->error() != "" )
            state_element.setAttribute( "db_error", trim( _db->error() ) );
    }

    if( _waiting_errno )
    {
        state_element.setAttribute( "waiting_errno"         , _waiting_errno );
        state_element.setAttribute( "waiting_errno_text"    , "ERRNO-" + as_string( _waiting_errno ) + "  " + z_strerror( _waiting_errno ) );
        state_element.setAttribute( "waiting_errno_filename", _waiting_errno_filename );
    }

    state_element.setAttribute( "loop"                 , _loop_counter );
    state_element.setAttribute( "waits"                , _wait_counter );

    if( _last_wait_until.not_zero() )
        state_element.setAttribute( "wait_until", _last_wait_until.xml_value() );

    if( _last_resume_at.not_zero()  &&  !_last_resume_at.is_never() )
        state_element.setAttribute( "resume_at", _last_resume_at.xml_value() );

//#   ifdef Z_UNIX
//    {
//        // Offene file descriptors ermitteln. Zum Debuggen, weil das Gerücht geht, Dateien würden offen bleiben.
//        // Das war nur ein Gerücht.
//        S s;
//        int n = sysconf( _SC_OPEN_MAX );
//        for( int fd = 0; fd < n; fd++ )  if( fcntl( fd, F_GETFD ) != -1  ||  errno != EBADF )  s << ' ' << fd;
//        state_element.setAttribute( "file_descriptors", s.str().substr( 1 ) );
//    }
//#   endif

    if( !show_what.is_set( show_folders ) )
    {
        if (_lock_subsystem && !_lock_subsystem->is_empty() )  state_element.appendChild(_lock_subsystem->file_baseds_dom_element( dom, show_what ) );
        if (_monitor_subsystem && !_monitor_subsystem->is_empty())  state_element.appendChild(_monitor_subsystem->file_baseds_dom_element(dom, show_what));
        if (_job_subsystem && show_what.is_set( show_jobs ) )  state_element.appendChild(_job_subsystem->file_baseds_dom_element( dom, show_what ) );
        else  state_element.append_new_comment( "<jobs> suppressed. Use what=\"jobs\"." );

        if( _process_class_subsystem )  state_element.appendChild( _process_class_subsystem->file_baseds_dom_element( dom, show_what ) );

        if( show_what.is_set( show_schedules ) && _schedule_subsystem )  state_element.appendChild( _schedule_subsystem->file_baseds_dom_element( dom, show_what ) );

        if (_order_subsystem) 
            state_element.appendChild(_order_subsystem->file_baseds_dom_element( dom, show_what ) );
    }
    else
    if( _folder_subsystem )  
    {
        if( Folder* folder = folder_subsystem()->folder_or_null( show_what._folder_path ) )
        {
            state_element.appendChild( folder->dom_element( dom, show_what ) );
        }
    }


    if (_order_subsystem && !_order_subsystem->order_id_spaces_interface()->is_empty() )
         state_element.appendChild( _order_subsystem->order_id_spaces_interface()->dom_element( dom, show_what ) );


    {
        xml::Element_ptr subprocesses_element = dom.createElement( "subprocesses" );
        for( int i = 0; i < NO_OF( _pids ); i++ )
        {
            int pid = _pids[ i ]._pid;
            if( pid )
            {
                xml::Element_ptr subprocess_element = dom.createElement( "subprocess" );
                subprocess_element.setAttribute( "pid", pid );

                subprocesses_element.appendChild( subprocess_element );
            }
        }
        
        state_element.appendChild( subprocesses_element );
    }


    if (_supervisor) state_element.appendChild( _supervisor->dom_element( dom, show_what ) );
    if( _cluster )  state_element.appendChild( _cluster->dom_element( dom, show_what ) );
    if (_web_services) state_element.appendChild( _web_services->dom_element( dom, show_what ) );
    if (!_api_process_register.empty()) {
        xml::Element_ptr processes_element = state_element.append_new_element("remote_processes");
        Z_FOR_EACH(Api_process_register, _api_process_register, it) {
            xml::Element_ptr process_element = processes_element.append_new_element("remote_process");
            process_element.setAttribute("id", it->second->process_id());
            process_element.setAttribute("pid", it->second->pid());
        }
    }

    state_element.appendChild( _communication.dom_element( dom, show_what ) );

    if( show_what.is_set( show_operations ) )
    {
        if (_connection_manager)
            state_element.append_new_cdata_or_text_element( "operations", "\n" + _connection_manager->string_from_operations( "\n" ) + "\n" );
        if (_java_subsystem)  state_element.appendChild(_java_subsystem->dom_element(dom));   // Das sollte vielleicht eingeordnet werden unter <subsystems>

        Memory_allocator::Allocation_map allocation_map = Memory_allocator::allocation_map();
        if( !allocation_map.empty() ) {
            xml::Element_ptr element = state_element.append_new_element( "allocations" );
            long size_total = 0;
            int count_total = 0;

            Z_FOR_EACH( Memory_allocator::Allocation_map, allocation_map, it ) {
                xml::Element_ptr e = element.append_new_element( "allocation" );
                long size = (long)it->first._size;
                int count = it->second._count;

                e.setAttribute( "count", count );
                e.setAttribute( "name", it->first._name );
                e.setAttribute( "size", size );

                size_total += size * count;
                count_total += count;
            }

            element.setAttribute( "count", count_total );
            element.setAttribute( "size", size_total );

#ifdef Z_WINDOWS
            MEMORYSTATUS m = memory_status_init();
            element.setAttribute( "uom_memory_values", "MB");
            element.setAttribute( "reserved_virtual", mb_formatted(memory_status_calculate_reserved_virtual(m)));
            element.setAttribute( "total_virtual", mb_formatted(m.dwTotalVirtual));
            element.setAttribute( "avail_virtual", mb_formatted(m.dwAvailVirtual));
            element.setAttribute( "total_physical", mb_formatted(m.dwTotalPhys));
            element.setAttribute( "avail_physical", mb_formatted(m.dwAvailPhys));
            element.setAttribute( "total_pagefile", mb_formatted(m.dwTotalPageFile));
            element.setAttribute( "avail_pagefile", mb_formatted(m.dwAvailPageFile));
            element.setAttribute( "memoryload", mb_formatted(m.dwMemoryLoad));
#endif

        }
    }

    return state_element;
}

#ifdef Z_WINDOWS
MEMORYSTATUS Spooler::memory_status_init()
{
    MEMORYSTATUS m;
    memset( &m, 0, sizeof m );
    m.dwLength = sizeof m;
    GlobalMemoryStatus( &m );
    return m;
}

SIZE_T Spooler::memory_status_calculate_reserved_virtual(MEMORYSTATUS m)
{
  return m.dwTotalVirtual - m.dwAvailVirtual;      // Das sollte der belegte Adressraum sein
}

string Spooler::mb_formatted(SIZE_T value)
{
    char buffer [ 30 ];
    int len = snprintf( buffer, sizeof buffer - 1, "%-.3f", (double)value / 1024 / 1024 );
    return string(buffer,len);
}
#endif

//------------------------------------------------------Spooler::print_xml_child_elements_for_event

void Spooler::print_xml_child_elements_for_event( String_stream* s, Scheduler_event* )
{
    *s << "<state";
    *s << " state=\"" << state_name() << '"';
    *s << "/>";
}


void Spooler::register_api_process(Api_process* process) {
    assert(process->process_id());
    _api_process_register[process->process_id()] = process;
}


void Spooler::unregister_api_process(Process_id process_id) {
    assert(process_id);
    Api_process* process = task_process(process_id);
    process->close_async();
    _api_process_register.erase(process_id);
}


Api_process* Spooler::task_process(Process_id process_id) {
    assert(process_id);
    Api_process_register::iterator it = _api_process_register.find(process_id);
    if (it == _api_process_register.end())  z::throw_xc(Z_FUNCTION, "unknown process id", process_id);
    return it->second;
}

//-----------------------------------------------------------------Spooler::register_process_handle
// registrierung aller Kindprozesse (wird beim beenden benötigt)
void Spooler::register_process_handle( Process_handle p )
{
#   ifdef _DEBUG
        for( int i = 0; i < NO_OF( _process_handles ); i++ )
        {
//            if( _process_handles[i] == p )  z::throw_xc( "register_process_handle" );              // Bereits registriert
            if( _process_handles[i] == p ) _log->warn( message_string("SCHEDULER-713", hex_from_int((int)p) ) );         // JS-471: process-handle bereits registriert
        }
#   endif


    for( int i = 0; i < NO_OF( _process_handles ); i++ )
    {
        if( _process_handles[i] == 0 )  
        { 
            _process_handles[i] = p;  
            _process_count++;
            break; 
        }
    }

    update_console_title( 2 );
}

//---------------------------------------------------------------Spooler::unregister_process_handle

void Spooler::unregister_process_handle( Process_handle p )
{
    if( p )
    {

        for( int i = 0; i < NO_OF( _process_handles ); i++ )
        {
            if( _process_handles[i] == p )  
            { 
                _process_handles[i] = 0;  
                _process_count--;
                break; 
            }
        }
    }

    update_console_title( 2 );
}

//-------------------------------------------------------------------------------Task::register_pid

void Spooler::register_pid( int pid, bool is_process_group )
{ 
    for( int i = 0; i < NO_OF( _pids ); i++ )
    {
        Killpid* p = &_pids[i];

        if( p->_pid == 0  ||  p->_pid == pid )
        { 
            p->_pid = pid;
            p->_is_process_group = is_process_group;
            return; 
        }
    }
}

//-----------------------------------------------------------------------------Task::unregister_pid

void Spooler::unregister_pid( int pid )
{ 
    for( int i = 0; i < NO_OF( _pids); i++ )
    {
        Killpid* p = &_pids[i];

        if( p->_pid == pid )  { _pids[i]._pid = 0;  return; }
    }
}

//---------------------------------------------------------------------------Spooler::name_is_valid

bool Spooler::name_is_valid( const string& name )
{
    bool result = true;

    if( name == ""                       )  result = false;
    if( name.find( '/' ) != string::npos )  result = false;

    return result;
}

//------------------------------------------------------------------------------Spooler::check_name

void Spooler::check_name( const string& name )
{
    if( !name_is_valid( name ) )  z::throw_xc( "SCHEDULER-417", name );
}

//-----------------------------------------------------------------Spooler::process_class_subsystem

Process_class_subsystem* Spooler::process_class_subsystem() const
{
    if( !_process_class_subsystem )  z::throw_xc( Z_FUNCTION, "Process_class_subsystem is not initialized" );

    return _process_class_subsystem; 
}

//-------------------------------------------------------------------------Spooler::order_subsystem

Order_subsystem* Spooler::order_subsystem() const
{ 
    if( !_order_subsystem )  assert(0), z::throw_xc( Z_FUNCTION, "Order_subsystem_impl is not initialized" );

    return _order_subsystem; 
}

//----------------------------------------------------------------Spooler::standing_order_subsystem

Standing_order_subsystem* Spooler::standing_order_subsystem() const
{ 
    if( !_order_subsystem )  assert(0), z::throw_xc( Z_FUNCTION, "Standing_order_subsystem is not initialized" );

    return _standing_order_subsystem; 
}

//----------------------------------------------------------------------Spooler::schedule_subsystem

Schedule_subsystem_interface* Spooler::schedule_subsystem() const
{ 
    if( !_schedule_subsystem )  assert(0), z::throw_xc( Z_FUNCTION, "Schedule_subsystem is not initialized" );

    return _schedule_subsystem; 
}

//---------------------------------------------------------------------------Spooler::job_subsystem

Job_subsystem* Spooler::job_subsystem() const
{ 
    if( !_job_subsystem )  assert(0), z::throw_xc( Z_FUNCTION, "Job_subsystem is not initialized" );

    return _job_subsystem; 
}

//--------------------------------------------------------------------------Spooler::task_subsystem

Task_subsystem* Spooler::task_subsystem() const
{ 
    if( !_task_subsystem )  assert(0), z::throw_xc( Z_FUNCTION, "Task_subsystem is not initialized" );

    return _task_subsystem; 
}

//-----------------------------------------------------------------------Spooler::monitor_subsystem

Monitor_subsystem* Spooler::monitor_subsystem() const
{ 
    if (!_monitor_subsystem)  assert(0), z::throw_xc(Z_FUNCTION, "Monitor_subsystem is not initialized");
    return _monitor_subsystem; 
}

//-------------------------------------------------------------------------Spooler::order_subsystem

supervisor::Supervisor_client_interface* Spooler::supervisor_client()
{ 
    return _supervisor_client;
}

string Spooler::supervisor_uri() {
    if (_supervisor_client) {
        return "tcp://" +_supervisor_client->address();
    } else
        return "";
}

//----------------------------------------------------------------------------Spooler::has_any_task

bool Spooler::has_any_task()
{
    if (_task_subsystem  &&  !_task_subsystem->_task_set.empty())  return true;

    return _job_subsystem->is_any_task_queued();
}

//--------------------------------------------------------------------------------Spooler::get_task

ptr<Task> Spooler::get_task( int task_id )
{
    ptr<Task> task = get_task_or_null( task_id );
    if( !task )  z::throw_xc( "SCHEDULER-215", task_id );
    return task;
}

//------------------------------------------------------------------------Spooler::get_task_or_null

ptr<Task> Spooler::get_task_or_null( int task_id )
{
    if( !_task_subsystem )  return NULL;
    return _task_subsystem->get_task_or_null( task_id );
}

//-------------------------------------------------------------------------------Spooler::set_state

void Spooler::set_state( State state )
{
    assert( current_thread_id() == _thread_id );

    self_check();
    
    if (_pause_at_start) {
        if (state == s_running) {
            _pause_at_start = false;
            state = s_paused;
        } else if (state == s_waiting_for_activation) {
            _pause_at_start = false;
            state = s_waiting_for_activation_paused;
        } 
    }


    if( _state == state )  return;

    State old_state = _state;
    _state = state;

    try
    {
        Log_level log_level = state == s_loading || state == s_starting? log_debug3 : log_info;
        Z_DEBUG_ONLY( _log->log( log_level, "--------------------------------------------" ) );
        _log->log( log_level, message_string( "SCHEDULER-902", state_name() ) );      // Nach _state = s_stopping aufrufen, damit's nicht blockiert!
    }
    catch( exception& ) {}      // ENOSPC bei s_stopping ignorieren wir

    update_console_title();

    try
    {
        Scheduler_event event ( evt_scheduler_state_changed, log_info, this );
        report_event( &event );
    }
    catch( exception& ) {}      // ENOSPC bei s_stopping ignorieren wir

    if( _state_changed_handler )  (*_state_changed_handler)( this, NULL );

    if( _state == s_running  &&  old_state != s_paused  ||
        _state == s_paused   ||
        _state == s_stopping )
    {
        log_show_state();
    }
    if (_cluster) {
        if (_state == s_paused) {
            _cluster->set_paused(true);
        } else
        if (_state == s_running) {
            _cluster->set_paused(false);
        }
    }
    Scheduler_object::report_event(CppEventFactoryJ::newSchedulerStateChanged(_state));
}

//------------------------------------------------------------------------------Spooler::state_name

string Spooler::state_name( State state )
{
    switch( state )
    {
        case s_stopped:             return "stopped";
        case s_loading:             return "loading";
        case s_starting:            return "starting";
        case s_waiting_for_activation: return "waiting_for_activation";
        case s_waiting_for_activation_paused: return "waiting_for_activation_paused";
        case s_running:             return "running";
        case s_paused:              return "paused";
        case s_stopping:            return "stopping";
        case s_stopping_let_run:    return "stopping_let_run";
        default:                    return as_string( (int)state );
    }
}

//------------------------------------------------------------------------------Spooler::self_check

void Spooler::self_check()
{
    if (Order_subsystem* o = _order_subsystem)  o->self_check();
}

//--------------------------------------------------------------------------------Spooler::send_cmd

void Spooler::send_cmd()
{
    xml::Document_ptr xml_doc;
    try {
        xml_doc.load_xml_bytes( _send_cmd_xml_bytes );      // Haben wir ein gültiges XML-Dokument?
    } catch (exception& x) {
        _spooler->log()->error(x.what());       // Log ist möglicherweise noch nicht geöffnet
        throw;
    }

    if (_settings->_http_port != "") {
        send_cmd_via_http();
    } else {
        send_cmd_via_tcp();
    }
}


void Spooler::send_cmd_via_tcp() {
    SOCKET sock = socket( PF_INET, SOCK_STREAM, 0 );
    if( sock == SOCKET_ERROR )  z::throw_socket( socket_errno(), "socket" );

    struct linger l; 
    l.l_onoff  = 1; 
    l.l_linger = 5;  // Sekunden
    setsockopt( sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );

    sockaddr_in addr;

    memset( &addr, 0, sizeof addr );
    addr.sin_family = PF_INET;
    //int i = inet_addr( "127.0.0.1" );
    //memcpy( &addr.sin_addr, &i, 4 );                        
    Ip_address ip = _ip_address;
    if( !ip )  ip = Ip_address( 127, 0, 0, 1 );
    addr.sin_addr = ip.as_in_addr();
    addr.sin_port = htons( _tcp_port );
    string ip_string = S() << ip.as_string() << ":" << _tcp_port;

    int ret = connect( sock, (sockaddr*)&addr, sizeof addr );
    if( ret == -1 )  z::throw_socket( socket_errno(), "connect", ip_string.c_str() );

    const char* p     = _send_cmd_xml_bytes.data();
    const char* p_end = p + _send_cmd_xml_bytes.length();

    while( p < p_end )
    {
        ret = send( sock, p, int_cast(p_end - p), 0 );
        if( ret == -1 )  z::throw_socket( socket_errno(), "send" );

        p += ret;
    }


    Xml_end_finder xml_end_finder;
    bool           last_was_nl = true;
    bool           end = false;

    while( !end )
    {
        char buffer [2000];

        int ret = recv( sock, buffer, sizeof buffer, 0 );
        if( ret == 0 )  break;
        if( ret < 0 )  z::throw_socket( socket_errno(), "recv" );
        if( buffer[ret-1] == '\0' )  ret--, end = true;
        fwrite( buffer, ret, 1, stdout );
        last_was_nl = ret > 0  &&  buffer[ret-1] == '\n';
        if( xml_end_finder.is_complete( buffer, ret ) )  break;
    }

    if( !last_was_nl )  fputc( '\n', stdout );
    fflush( stdout );

    closesocket( sock );
}


void Spooler::send_cmd_via_http() {
    string host = !_ip_address.is_empty() ? _ip_address.as_string() : "127.0.0.1";
    string host_port = strchr(_settings->_http_port.c_str(), ':') ? _settings->_http_port : host + ":" + _settings->_http_port;
    string uri = S() << "http://" << host_port << "/";
    java_subsystem()->schedulerJ().sendCommandAndReplyToStout(uri, _send_cmd_xml_bytes);
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg()
{
    read_ini_filename();
    read_ini_file();
    read_command_line_arguments();
    set_home_directory();
    handle_configuration_directories();

    if( _zschimmer_mode  &&  string_ends_with( _configuration_file_path, ".js" ) ) {
        _configuration_is_job_script = true;
        _configuration_job_script_language = "javascript";
        if( !_log_directory_as_option_set )  _log_directory = "*stderr";
    }

    //_manual = !_job_name.empty();
    //if( _manual  &&  !_log_directory_as_option_set )  _log_directory = "*stderr";
}

//-----------------------------------------------------------------------Spooler::read_ini_filename

void Spooler::read_ini_filename()
{
    for( Sos_option_iterator opt ( _argc, _argv, _parameter_line ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "ini" ) )  _factory_ini = opt.value();
        else
        if( opt.param() ) {}
    }

    sos::mail::Mail_static::instance()->set_factory_ini_path( _factory_ini );
}

//---------------------------------------------------------------------------Spooler::read_ini_file

void Spooler::read_ini_file()
{
    set_id                      (            read_profile_string    ( _factory_ini, "spooler", "id"                 ) );
    _configuration_file_path    =            read_profile_string    ( _factory_ini, "spooler", "config"             );
    _log_directory              =            read_profile_string    ( _factory_ini, "spooler", "log-dir"            );  // veraltet
    _log_directory              = subst_env( read_profile_string    ( _factory_ini, "spooler", "log_dir"            , _log_directory ) );  _log_directory_as_option_set = !_log_directory.empty();
    _include_path               =            read_profile_string    ( _factory_ini, "spooler", "include-path"       );  // veraltet
    _include_path               = subst_env( read_profile_string    ( _factory_ini, "spooler", "include_path"       , _include_path ) );   _include_path_as_option_set  = !_include_path.empty();
    _spooler_param              =            read_profile_string    ( _factory_ini, "spooler", "param"              );                   _spooler_param_as_option_set = !_spooler_param.empty();
    _log_level                  = make_log_level(read_profile_string( _factory_ini, "spooler", "log_level"          , as_string(_log_level) ));   
    _job_history_columns        =            read_profile_string    ( _factory_ini, "spooler", "history_columns"    );
    _job_history_yes            =            read_profile_bool      ( _factory_ini, "spooler", "history"            , true );
    _job_history_on_process     =            read_profile_history_on_process( _factory_ini, "spooler", "history_on_process", 0 );
    _job_history_archive        =            read_profile_archive   ( _factory_ini, "spooler", "history_archive"    , arc_no );
    _job_history_with_log       =            read_profile_with_log  ( _factory_ini, "spooler", "history_with_log"   , arc_no );
    _order_history_yes          =            read_profile_bool      ( _factory_ini, "spooler", "order_history"      , true );
    _order_history_with_log     =            read_profile_with_log  ( _factory_ini, "spooler", "order_history_with_log", arc_no );
    modifiable_settings()->_db_name =        read_profile_string    ( _factory_ini, "spooler", "db"                 );
    _db_check_integrity         =            read_profile_bool      ( _factory_ini, "spooler", "db_check_integrity" , _db_check_integrity );
    _db_log_level               = make_log_level(read_profile_string( _factory_ini ,"spooler", "db_log_level"       , as_string( _db_log_level ) ) );
    modifiable_settings()->_html_dir = subst_env(read_profile_string( _factory_ini, "spooler", "html_dir" ) );

    _mail_on_warning = read_profile_bool           ( _factory_ini, "spooler", "mail_on_warning", _mail_on_warning );
    _mail_on_error   = read_profile_bool           ( _factory_ini, "spooler", "mail_on_error"  , _mail_on_error   );
    _mail_on_process = read_profile_mail_on_process( _factory_ini, "spooler", "mail_on_process", _mail_on_process );
    _mail_on_success = read_profile_bool           ( _factory_ini, "spooler", "mail_on_success", _mail_on_success );
    _mail_on_delay_after_error   = read_profile_yes_no_last_both( _factory_ini, "spooler", "mail_on_delay_after_error"  , _mail_on_delay_after_error   );

    _mail_encoding   = read_profile_string         ( _factory_ini, "spooler", "mail_encoding"  , "base64"        );      // "quoted-printable": Jmail braucht 1s pro 100KB dafür

    _mail_defaults.set( "queue_dir", subst_env( read_profile_string( _factory_ini, "spooler", "mail_queue_dir"   , "-" ) ) );
    _mail_defaults.set( "queue_only",           read_profile_bool  ( _factory_ini, "spooler", "mail_queue_only", false )? "1" : "0" );
    _mail_defaults.set( "smtp"     ,            read_profile_string( _factory_ini, "spooler", "smtp"             , "-" ) );
    _mail_defaults.set( "from"     ,            read_profile_string( _factory_ini, "spooler", "log_mail_from"    ) );
    _mail_defaults.set( "to"       ,            read_profile_string( _factory_ini, "spooler", "log_mail_to"      ) );
    _mail_defaults.set( "cc"       ,            read_profile_string( _factory_ini, "spooler", "log_mail_cc"      ) );
    _mail_defaults.set( "bcc"      ,            read_profile_string( _factory_ini, "spooler", "log_mail_bcc"     ) );
    _mail_defaults.set( "subject"  ,            read_profile_string( _factory_ini, "spooler", "log_mail_subject" ) );
    _mail_defaults.set("mail_on_error", _mail_on_error ? "1" : "0");  // Only for Java
    _mail_defaults.set("mail_on_warning", _mail_on_warning ? "1" : "0");  // Only for Java
    _mail_defaults.set("mail.smtp.port",        read_profile_string(_factory_ini, "smtp", "mail.smtp.port"    ));  // Only for Java
    _mail_defaults.set("mail.smtp.user",        read_profile_string(_factory_ini, "smtp", "mail.smtp.user"    ));  // Only for Java
    _mail_defaults.set("mail.smtp.password",    read_profile_string(_factory_ini, "smtp", "mail.smtp.password"));  // Only for Java

    _subprocess_own_process_group_default = read_profile_bool( _factory_ini, "spooler", "subprocess.own_process_group", _subprocess_own_process_group_default );
    _log_collect_within = Duration(read_profile_uint  ( _factory_ini, "spooler", "log_collect_within", 0 ));
    _log_collect_max    = Duration(read_profile_uint  ( _factory_ini, "spooler", "log_collect_max"   , 900 ));
  //_zschimmer_mode     = read_profile_bool  ( _factory_ini, "spooler", "zschimmer", _zschimmer_mode );
    modifiable_settings()->_job_java_options = trim(
        modifiable_settings()->_job_java_options + " " + read_profile_string(_factory_ini, "java", "job_options"));
}

//-------------------------------------------------------------Spooler::read_command_line_arguments

void Spooler::read_command_line_arguments()
{

    _my_program_filename = _argv? _argv[0] : "(missing program path)";

    try
    {
        for( Sos_option_iterator opt ( _argc, _argv, _parameter_line ); !opt.end(); opt.next() )
        {
            if( opt.with_value( "sos.ini"          ) )  _sos_ini = opt.value();   // wurde in Hostware-main() bearbeitet
            else
            if( opt.flag      ( 'V', "version"     ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "?"                ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "h"                ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "log"              ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "i"                ) )  _interactive = opt.set();   // Nur für Joacim Zschimmer
            else
            if( opt.with_value( "pid-file"         ) )  _pid_filename = opt.value();
            else
            if( opt.flag      ( "kill"             ) )  ;
            else
            if( opt.with_value( "kill"             ) )  ;
            else
            if( opt.with_value( "ini"              ) )  ;   //
            else
            if( opt.with_value( "config"           )
             || opt.param(1)                         )  _configuration_file_path = opt.value();
            else
            if( opt.param()  &&  opt.value().find('=') != string::npos ) {  // 1. Parameter ist _configuration_file_path, die folgenden sind Parameter name=wert
                // Nicht offiziell. Verbessert werden könnte, dass <params> diese Werte nicht überschreibt, sondern umgekehrt den Default vorgibt.
                // Sollte vielleicht auch mit -config funktionieren. Jetzt muss die Konfigurationsdatei als 1. Parameter (ohne -config=) angegeben werden.
                string value = opt.value();
                size_t eq = value.find( '=' );
                _variables->set_var( value.substr( 0, eq ), value.substr( eq + 1 ) );
            }
            else
            if( opt.with_value( "cd"               ) )  {}  // Bereits von spooler_main() erledigt
            else
            if( opt.with_value( "id"               ) )  set_id( opt.value() );
            else
            if( opt.with_value( "log-dir"          ) )  _log_directory = opt.value(),  _log_directory_as_option_set = true;
            else
            if( opt.flag      ( 'e', "log-to-stderr" ) )  _log_to_stderr = opt.set();   // Nur für Joacim Zschimmer
            else
            if( opt.with_value( "stderr-level"     ) )  _log_to_stderr = true,  _log_to_stderr_level = make_log_level( opt.value() );
            else
            if( opt.with_value( "include-path"     ) )  _include_path = opt.value(),  _include_path_as_option_set = true;
            else
            if( opt.with_value( "param"            ) )  _spooler_param = opt.value(),  _spooler_param_as_option_set = true;
            else
            if( opt.with_value( "log-level"        ) )  _log_level = make_log_level(opt.value());
            else
            //if( opt.with_value( "job"              ) )  _job_name = opt.value();        // Nicht von SOS beauftragt
            //else
            if( opt.with_value( "program-file"     ) )  _my_program_filename = opt.value();        // .../scheduler.exe
            else
            if( opt.with_value( "send-cmd"         ) )  _send_cmd_xml_bytes = opt.value();
            else
            if (opt.with_value( "cmd"              ) )  _cmd_xml_bytes = opt.value();
            else
            if( opt.with_value( "port"             ) )  _tcp_port = _udp_port = opt.as_int(),  _tcp_port_as_option_set = _udp_port_as_option_set = true;
            else
            if( opt.flag      ( "reuse-port"       ) )  _reuse_addr = opt.set();
            else
            if (opt.with_value( "http-port")) { 
                modifiable_settings()->set(setting_http_port, opt.value()); 
                _http_port_as_option_set = true;
            } else
            if (opt.with_value( "https-port")) { 
                modifiable_settings()->set(setting_https_port, opt.value()); 
                _https_port_as_option_set = true;
            } else
            if( opt.with_value( "tcp-port"         ) )  _tcp_port = opt.as_int(),  _tcp_port_as_option_set = true;
            else
            if( opt.with_value( "udp-port"         ) )  _udp_port = opt.as_int(),  _udp_port_as_option_set = true;
            else
            if( opt.with_value( "ip-address"       ) )  _ip_address = opt.value(),  _ip_address.resolve_name(),  _ip_address_as_option_set = !opt.value().empty();
            else
            if( opt.flag      ( "ignore-process-classes" ) )  _ignore_process_classes = opt.set(),  _ignore_process_classes_set = true;
            else
            if( opt.flag      ( "validate-xml"           ) )  _validate_xml = opt.set();
            else
            if( opt.flag      ( "exclusive"              ) )  _cluster_configuration._demand_exclusiveness   = opt.set();
            else
            if( opt.flag      ( "backup"                 ) )  _cluster_configuration._is_backup_member       = opt.set();
            else if (opt.flag("pause")) {
                _pause_at_start = true;
            } else
            if( opt.with_value( "backup-precedence"      ) )  _cluster_configuration._backup_precedence      = opt.as_int();
            else
            if( opt.flag      ( "distributed-orders"     ) )  _cluster_configuration._orders_are_distributed = opt.set();
            else
            if( opt.with_value( "env"                    ) )  ;  // Bereits von spooler_main() erledigt
            else
            if( opt.with_value( "test-env"               ) )  ;  // Bereits von spooler_main() erledigt
            else
            if( opt.flag      ( "zschimmer"              ) )  _zschimmer_mode = opt.set();
            else
            if( opt.flag      ( "test"                   ) )  set_log_category( "self_test" ), set_log_category( "self_test.exception" ),  self_test();
            else
            if( opt.flag      ( "suppress-watchdog-thread" ) )  _cluster_configuration._suppress_watchdog_thread = opt.set();
          //else
          //if( opt.with_value( "now"                    ) )  _clock_difference = Time( Sos_date_time( opt.value() ) ) - Time::now();
            else
            if( opt.flag      ( "check-memory-leak"     ) )  set_check_memory_leak(opt.set());
#           ifdef Z_WINDOWS
                else if( opt.flag( "debug-break"        ) )  ;   // Bereits von spooler_main() verarbeitet
#           endif
            else
            if( opt.with_value( "use-xml-schema"        ) ) {
                _xml_schema_url = javaproxy::java::io::File::new_instance(opt.value()).toURI().toURL().toExternalForm();
                Z_LOG2( "scheduler", "Using dynamic schema: " << _xml_schema_url << "\n" );
            }
            else
            if (opt.with_value("db"))  modifiable_settings()->_db_name = opt.value();
            else
                if(opt.with_value("configuration-directory")) _opt_configuration_directory = opt.value(); // JS-462
            else
            if (opt.with_value("java-options")) { _java_options = opt.value(); }  // wird in sos::spooler_main vearbeitet
            else
            if (opt.with_value("java-classpath")) {}   // wird in sos::spooler_main vearbeitet
            else
            if (opt.with_value("job-java-options")) { 
                modifiable_settings()->_job_java_options = trim(modifiable_settings()->_job_java_options + " " + opt.value()); 
            }
            else
            if (opt.with_value("job-java-classpath")) { modifiable_settings()->_job_java_classpath = opt.value(); }
            else
            if (opt.with_value("time-zone")) { _time_zone_name = opt.value(); }
            else
            if (opt.with_value("roles")) { _settings->set(setting_roles, opt.value()); }
            else
            if (opt.flag("pause-after-failure")) { _settings->set(setting_pause_after_failure, as_string(opt.set())); }
            else
                throw_sos_option_error( opt );
        }

        if( _is_service )  _interactive = false;

        if( _directory.empty() )    // Nur beim ersten Mal setzen!
        {
            char dir [ 1024 + 2 ];
            char* ok = getcwd( dir, sizeof dir - 1 );
            if( !ok )  throw_errno( errno, "getcwd" );
            strcat( dir, Z_DIR_SEPARATOR );
            _directory = dir;
        }
        else
        {
            //if( *_directory.rbegin() != '/'  &&  *_directory.rbegin() != '\\' )  _directory += Z_DIR_SEPARATOR;
        }

        _temp_dir = subst_env( read_profile_string( _factory_ini, "spooler", "tmp" ) );
        if( _temp_dir.empty() )  _temp_dir = get_temp_path() + Z_DIR_SEPARATOR "scheduler";
        _temp_dir = replace_regex( _temp_dir, "[\\/]+", Z_DIR_SEPARATOR );
        _temp_dir = replace_regex( _temp_dir, "\\" Z_DIR_SEPARATOR "$", "" );
        if( _spooler_id != "" )  _temp_dir += Z_DIR_SEPARATOR + _spooler_id;

        if( _log_level <= log_debug_spooler )  _debug = true;

        if( _configuration_file_path.empty() )  z::throw_xc( "SCHEDULER-115" );
    }
    catch( exception& )
    {
        if( !_is_service )  print_usage();
        throw;
    }
}


//----------------------------------------------------------------------Spooler::set_home_directory

void Spooler::set_home_directory() {
    File_path p = File_path(_my_program_filename);
    _home_directory = p.directory().directory();
    Z_DEBUG_ONLY( _log->debug("Home directory is " + _home_directory ) );
}

//-------------------------------------------------------------------Spooler::set_check_memory_leak

void Spooler::set_check_memory_leak(bool) {
    // Schon in sos_main0 codiert.
    //if (on) {
    //    #ifdef Z_WINDOWS
    //        // Bei Programmende Speicherzellen ausgeben:
    //        // http://msdn.microsoft.com/de-de/library/5at7yxcs%28v=VS.100%29.aspx
    //        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF);
    //    #endif
    //}
}

//--------------------------------------------------------Spooler::handle_configuration_directories

void Spooler::handle_configuration_directories()
{
    if( _configuration_directories[ confdir_local ] == "" )
    {
        if( file::File_info( _configuration_file_path ).is_directory() )
        {
            _configuration_directories[ confdir_local ] = File_path( _configuration_file_path, "" );
            _configuration_directories_as_option_set[ confdir_local ] = true;
            _configuration_file_path = File_path( _configuration_file_path, "scheduler.xml" );
        }
        else
        {
            _configuration_directories[ confdir_local ] = File_path( File_path( _configuration_file_path.directory(), "live" ), "" );
        }

        if(!_opt_configuration_directory.empty()) // JS-462
        {
            if(!file::File_info(_opt_configuration_directory).is_directory())
            { 
                z::throw_xc( "SCHEDULER-715", _opt_configuration_directory.c_str() );
            }
            _configuration_directories[ confdir_local ] = File_path( _opt_configuration_directory );
            _configuration_directories_as_option_set[ confdir_local ] = false;                        
        }

    }

    _configuration_directories[ confdir_cache ] = File_path( File_path( _configuration_file_path.directory(), "cache" ), "" );        // "live" bis es richtig gemacht ist

    if( _central_configuration_directory == "" )
        _central_configuration_directory = File_path( File_path( _configuration_file_path.directory(), "remote" ), "" );
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    set_state( s_loading );
    tzset();
    _security.clear();             
    load_arg();
    _java_subsystem->initialize_java_sister();
    open_pid_file();
    _log->open_dont_cache();
    fetch_hostname();

    if( _validate_xml ) {
        _schema = xml::Schema_ptr(_xml_schema_url);
    }

    read_xml_configuration();
    _db = Z_NEW(Database(this));

    new_subsystems();
    _java_subsystem->switch_subsystem_state( subsys_initialized );
    modifiable_settings()->set_defaults(this);

    initialize_subsystems();
    load_config( _config_element_to_load, _config_source_filename );
    modifiable_settings()->set_from_variables(*_variables);
    _settings->freeze(_db); // Von Scheduler.java befüllt
    if (_time_zone_name == "") {
        _time_zone_name = SchedulerJ::defaultTimezoneId();
    }
    initialize_subsystems_after_base_processing();

    if( _zschimmer_mode )  initialize_sleep_handler();
    _java_subsystem->on_scheduler_loaded();
}

//---------------------------------------------------------------------------Spooler::open_pid_file

void Spooler::open_pid_file() 
{
    if( _pid_filename != ""  &&  !_pid_file.opened() ) {
        _pid_file.open( _pid_filename, "w" );
        #ifdef Z_UNIX
            struct stat stat;
            if (fstat(_pid_file, &stat) == 0) {
                fchmod(_pid_file, stat.st_mode | 0444);
            }
        #endif
        _pid_file.unlink_later();
        _pid_file.print( as_string( getpid() ) );
        _pid_file.print( "\n" );
        _pid_file.flush();
    }
}

//--------------------------------------------------------------------------Spooler::fetch_hostname

void Spooler::fetch_hostname()
{
    char hostname[200];  // Nach _communication.init() und nach _prefix_log.init()!
    if( gethostname( hostname, sizeof hostname ) == SOCKET_ERROR )  hostname[0] = '\0',  _log->warn( "gethostname(): " + z_strerror( errno ) );
    _short_hostname = hostname;
    _complete_hostname = complete_computer_name();
}

//------------------------------------------------------------------Spooler::read_xml_configuration

void Spooler::read_xml_configuration()
{
    assert(!_config_element_to_load);

    Command_processor cp ( this, Security::seclev_all );
    _executing_command = false;             // Command_processor() hat es true gesetzt, aber noch läuft der Scheduler nicht.
                                            // database.cxx verweigert das Warten auf die Datenbank, wenn _executing_command gesetzt ist,
                                            // damit der Scheduler nicht in einem TCP-Kommando blockiert.

    if( _configuration_is_job_script )
        cp.execute_xml_string( configuration_for_single_job_script() );
    else
        cp.execute_config_file( _configuration_file_path );

    if (!_config_element_to_load)
       if (this->id().empty() )
          z::throw_xc( "SCHEDULER-480" );
       else
         z::throw_xc( "SCHEDULER-479", this->id() );

}

//--------------------------------------------------------------------------Spooler::new_subsystems

void Spooler::new_subsystems()
{
    // nicht die Reihenfolge ändern !!!!!!
    _event_subsystem            = new_event_subsystem( this );
    _scheduler_event_manager    = Z_NEW( Scheduler_event_manager( this ) );
    _folder_subsystem           = new_folder_subsystem( this );
    _scheduler_script_subsystem = new_scheduler_script_subsystem( this );
    _schedule_subsystem         = schedule::new_schedule_subsystem( this );
    _process_class_subsystem    = Z_NEW( Process_class_subsystem( this ) );
    _lock_subsystem             = Z_NEW( lock::Lock_subsystem( this ) );
    _monitor_subsystem          = new_monitor_subsystem(this);
    _job_subsystem              = new_job_subsystem( this );
    _task_subsystem             = Z_NEW( Task_subsystem( this ) );
    _order_subsystem            = new_order_subsystem( this );
    _standing_order_subsystem   = new_standing_order_subsystem( this );
    _http_server                = http::new_http_server( this );
    _web_services               = new_web_services( this );
    _supervisor                 = supervisor::new_supervisor( this );
}

//----------------------------------------------------------------------Spooler::destroy_subsystems

void Spooler::destroy_subsystems()
{
    // In der Reihenfolge der Abhängigkeiten löschen:
    // nicht die Reihenfolge ändern !!!!!!
    _standing_order_subsystem   = NULL;
    _order_subsystem            = NULL;
    _task_subsystem             = NULL;
    _job_subsystem              = NULL;
    _monitor_subsystem          = NULL;
    _lock_subsystem             = NULL;
    _process_class_subsystem    = NULL;
    _schedule_subsystem         = NULL;
    _scheduler_script_subsystem = NULL;
    _folder_subsystem           = NULL;
    _db                         = NULL;
}

//-------------------------------------------------------------------Spooler::initialize_subsystems

void Spooler::initialize_subsystems()
{
    _event_subsystem           ->switch_subsystem_state( subsys_initialized );
    _folder_subsystem          ->switch_subsystem_state( subsys_initialized );
    //_supervisor                ->switch_subsystem_state( subsys_initialized );
    _schedule_subsystem        ->switch_subsystem_state( subsys_initialized );
    _process_class_subsystem   ->switch_subsystem_state( subsys_initialized );
    _lock_subsystem            ->switch_subsystem_state( subsys_initialized );
    _monitor_subsystem         ->switch_subsystem_state( subsys_initialized );
    //_job_subsystem             ->switch_subsystem_state( subsys_initialized );
    //_scheduler_script_subsystem->switch_subsystem_state( subsys_initialized );
    _order_subsystem           ->switch_subsystem_state( subsys_initialized );
    _standing_order_subsystem  ->switch_subsystem_state( subsys_initialized );
    _http_server               ->switch_subsystem_state( subsys_initialized );
    _web_services              ->switch_subsystem_state( subsys_initialized );        // Ein Job und eine Jobkette einrichten, s. spooler_web_service.cxx
}

//---------------------------------------------Spooler::initialize_subsystems_after_base_processing
// Nachdem load_config() die <base> ausgeführt hat.
// <base> erlaubt mehrere mischende set_dom() aufs selbe File_based und muss vor der Initialisierung gelaufen sein.
// Andererseits müssen einige andere Subsysteme bereits vorher initialisiert sein.

void Spooler::initialize_subsystems_after_base_processing()
{
    initialize_cluster();
    _connection_manager = Z_NEW(object_server::Connection_manager);
    _supervisor                ->switch_subsystem_state( subsys_initialized );
    _job_subsystem             ->switch_subsystem_state( subsys_initialized );
    _scheduler_script_subsystem->switch_subsystem_state( subsys_initialized );
}

//-------------------------------------------------------------------------Spooler::load_subsystems

void Spooler::load_subsystems()
{
    _java_subsystem            ->switch_subsystem_state( subsys_loaded );
    _event_subsystem           ->switch_subsystem_state( subsys_loaded );
    _folder_subsystem          ->switch_subsystem_state( subsys_loaded );

    if( !_ignore_process_classes )
    _process_class_subsystem   ->switch_subsystem_state( subsys_loaded );

    _schedule_subsystem        ->switch_subsystem_state( subsys_loaded );
    _lock_subsystem            ->switch_subsystem_state( subsys_loaded );
    _monitor_subsystem         ->switch_subsystem_state( subsys_loaded );
    _job_subsystem             ->switch_subsystem_state( subsys_loaded );         // Datenbank muss geöffnet sein
    _order_subsystem           ->switch_subsystem_state( subsys_loaded );
    _standing_order_subsystem  ->switch_subsystem_state( subsys_loaded );
    _scheduler_script_subsystem->switch_subsystem_state( subsys_loaded );
    _http_server               ->switch_subsystem_state( subsys_loaded );
}

//---------------------------------------------------------------------Spooler::activate_subsystems

void Spooler::activate_subsystems()
{
    _java_subsystem          ->switch_subsystem_state( subsys_active );

    // Job- und Order-<run_time> benutzen das geladene Scheduler-Skript
    _scheduler_script_subsystem->switch_subsystem_state( subsys_active );       // ruft spooler_init()
    detect_warning_and_send_mail();

    _event_subsystem         ->switch_subsystem_state( subsys_active );
    _folder_subsystem        ->switch_subsystem_state( subsys_active );

    if( !_ignore_process_classes )
    _process_class_subsystem ->switch_subsystem_state( subsys_active );

    _schedule_subsystem      ->switch_subsystem_state( subsys_active );
    _lock_subsystem          ->switch_subsystem_state( subsys_active );
    _monitor_subsystem       ->switch_subsystem_state( subsys_active );
    _job_subsystem           ->switch_subsystem_state( subsys_active );
    _order_subsystem         ->switch_subsystem_state( subsys_active );
    _standing_order_subsystem->switch_subsystem_state( subsys_active );
    _web_services            ->switch_subsystem_state( subsys_active );         // Nicht in Spooler::load(), denn es öffnet schon -log-dir-Dateien (das ist nicht gut für -send-cmd=)
}

//-------------------------------------------------------------------------Spooler::stop_subsystems

void Spooler::stop_subsystems()
{
    _scheduler_script_subsystem->switch_subsystem_state( subsys_stopped ); // Scheduler-Skript zuerst beenden, damit die Finalizer die Tasks (von Job.start()) und andere Objekte schließen können.
    _web_services              ->switch_subsystem_state( subsys_stopped );
    _standing_order_subsystem  ->switch_subsystem_state( subsys_stopped );
    _order_subsystem           ->switch_subsystem_state( subsys_stopped );
    _job_subsystem             ->switch_subsystem_state( subsys_stopped );
    _task_subsystem            ->switch_subsystem_state( subsys_stopped );
    _monitor_subsystem         ->switch_subsystem_state( subsys_stopped );
    _lock_subsystem            ->switch_subsystem_state( subsys_stopped );
    _process_class_subsystem   ->switch_subsystem_state( subsys_stopped );
    _schedule_subsystem        ->switch_subsystem_state( subsys_stopped );
    _folder_subsystem          ->switch_subsystem_state( subsys_stopped );
    _event_subsystem           ->switch_subsystem_state( subsys_stopped );
    _supervisor                ->switch_subsystem_state( subsys_stopped );

    if( _cluster ) {
        _cluster->switch_subsystem_state( subsys_stopped );
        _cluster = NULL;
    }

    if( _supervisor_client )  _supervisor_client->switch_subsystem_state( subsys_stopped );
}

//----------------------------------------------------------------------------Spooler::create_window

void Spooler::update_console_title( int level )
{
#   if defined Z_WINDOWS

#       ifndef Z_DEBUG
            if( !_zschimmer_mode && level != 1 )  return;
#       endif

        if( _has_windows_console )
        {
            S title;
            
            if( level )
            {
                title << name() << "  ";

                if( _configuration_directories_as_option_set[ confdir_local ] )  title << _configuration_directories[ confdir_local ];
                                                        else  title << _configuration_file_path;
                title << "  pid=" << getpid() << "  ";
                title << state_name();

                if( level == 2 )    // Aktuelles im laufenden Betrieb zeigen
                {
                    if( _task_subsystem  &&  _task_subsystem->finished_tasks_count() )
                    {
                        title << ", " <<_task_subsystem->finished_tasks_count() << " finished tasks";
                    }

                    if( _order_subsystem  &&  _order_subsystem->finished_orders_count() )
                    {
                        title << ", " << _order_subsystem->finished_orders_count() << " finished orders";     
                    }

                    int process_count = 0;
                    for( int i = 0; i < NO_OF( _process_handles ); i++ )  if( _process_handles[i] )  process_count++;
                    if( _state == s_running  ||  process_count > 0 )  title << ", " << process_count << " processes";
                }
            }

            BOOL ok = SetConsoleTitle( title.to_string().c_str() );
            if( !ok )  _has_windows_console = false;
        }

#   endif
}

//----------------------------------------------------------------Spooler::initialize_sleep_handler

void Spooler::initialize_sleep_handler()
{
#ifdef Z_WINDOWS
    _waitable_timer.set_handle( CreateWaitableTimer( NULL, FALSE, NULL ) );
    if( !_waitable_timer )  z::throw_mswin( "CreateWaitableTimer" );

    _waitable_timer.add_to( &_wait_handles );
#endif
}

//-----------------------------------------------------Spooler::configuration_for_single_job_script

string Spooler::configuration_for_single_job_script()
{
    return S() <<
        "<?xml version='1.0'?>\n" 
        "<spooler>\n" 
        "    <config>\n"
        "        <jobs>\n"
        "            <job name='" << xml::encode_attribute_value( _configuration_file_path.base_name() ) << "'>\n"
        "                <script language='" << xml::encode_attribute_value( _configuration_job_script_language ) << "'>\n"
        "                   <include file='" << xml::encode_attribute_value( _configuration_file_path ) << "'/>\n"
        "                </script>\n"
        "                <run_time once='yes'/>\n"
        "            </job>\n"
        "        </jobs>\n"
        "    </config>\n"
        "</spooler>\n";
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    static_log_categories.save_to( &_original_log_categories );
    _max_micro_step_time = Duration((double)_variables->get_int64("scheduler.message.SCHEDULER-721.timeout", _max_micro_step_time.seconds()));

    _state_cmd = sc_none;
    set_state( s_starting );

    _base_log.set_directory( _log_directory );
    _base_log.open_new();

    _log->info(message_string("SCHEDULER-900", (string)SchedulerJ::buildVersion(), _configuration_file_path, getpid()));
    _spooler_start_time = Time::now();

    if( _cluster )  _cluster->switch_subsystem_state( subsys_loaded );
    _web_services->switch_subsystem_state( subsys_loaded );

    if (_spooler->settings()->has_role_scheduler()) {
        _db->open();
        schedulerJ().onDatabaseOpened();
    }
    
    assert( !_cluster || _cluster->my_member_id() != "" );
    _db->spooler_start();


    set_ctrl_c_handler( false );
    set_ctrl_c_handler( true );       // Falls Java (über Dateityp jdbc) gestartet worden ist und den Signal-Handler verändert hat
    setmaxstdio();

#   ifdef Z_WINDOWS
        _print_time_every_second = log_directory() == "*stderr"  &&  isatty( fileno( stderr ) )
                                   ||  _log_to_stderr  &&  _log_to_stderr_level <= log_info  &&  isatty( fileno( stderr ) );
#   endif

    _communication.start_or_rebind();

    if( _supervisor_client )  _supervisor_client->switch_subsystem_state( subsys_initialized );
    if( _cluster           )  _cluster          ->switch_subsystem_state( subsys_active );
}

//--------------------------------------------------------------------------------Spooler::activate

void Spooler::activate(State state)
{
    load_subsystems();
    activate_subsystems();

    if (!_cmd_xml_bytes.empty())
    {
        Command_processor cp ( this, Security::seclev_all );
        cout << cp.execute_xml_bytes(_cmd_xml_bytes, "  ");          // Bei einem Fehler Abbruch
        _cmd_xml_bytes = "";
    }

    execute_config_commands();                                                                          

    set_state(state);
    _java_subsystem->on_scheduler_activated();

    if (settings()->_pause_after_failure) {
        if (_db->last_scheduler_run_failed()) {
            set_state(s_paused);
        }
    }
}

//-----------------------------------------------------------------Spooler::execute_config_commands

void Spooler::execute_config_commands()

// <commands> aus <config> ausführen:

{
    if( _commands_document )
    {
        Command_processor command_processor ( this, Security::seclev_all );
        command_processor.set_log( _log );
        
        DOM_FOR_EACH_ELEMENT( _commands_document.documentElement(), command_element )
        {
            xml::Element_ptr result = command_processor.execute_command( command_element );

            if( result  &&  !result.select_node( "ok [ count(*) = 0  and  count(@*) = 0 ]" ) )
            {
                Message_string m ( "SCHEDULER-966" );
                m.set_max_insertion_length( INT_MAX );
                m.insert( 1, result.xml_string() );
                _log->info( m );
            }
        }
    }


    // Jetzt brauchen wir die Konfiguration nicht mehr
    _config_element_to_load = xml::Element_ptr();
    _config_document_to_load = xml::Document_ptr();
}

//----------------------------------------------------------------------Spooler::initialize_cluster

void Spooler::initialize_cluster() 
{
    if (_cluster_configuration.is_cluster()) {
        _cluster = cluster::new_cluster_subsystem(this, _cluster_configuration);
        _cluster->switch_subsystem_state( subsys_initialized );
    }
}

//----------------------------------------------------------------------------Spooler::stop_cluster

void Spooler::stop_cluster()
{
    if( _cluster )
    {
        _assert_is_active = false;

        try
        {
            _cluster->set_continue_exclusive_operation( _terminate_continue_exclusive_operation );
        }
        catch( exception& x ) { _log->error( S() << x.what() << ", in Cluster::set_continue_exclusive_operation\n" ); }

        if( _terminate_all_schedulers )
        {
            // Das Kommando wird in <cluster_command> eingepackt und muss von scheduler.xsd zugelassen sein
            S cmd;
            cmd << "<terminate";
            if( _termination_gmtimeout_at  < time_max )  cmd << " timeout='" << ( _termination_gmtimeout_at - ::time(NULL) ) << "'";
            if( _terminate_all_schedulers_with_restart )  cmd << " restart='yes'";
            cmd << "/>";

            _cluster->set_command_for_all_schedulers_but_me( (Transaction*)NULL, cmd );
        }
    }
}

//-----------------------------------------------------------------------Spooler::cluster_is_active

bool Spooler::cluster_is_active()
{
    return !_cluster || _cluster->is_active();
}

//------------------------------------------------------------------Spooler::orders_are_distributed

bool Spooler::orders_are_distributed()
{
    return _cluster_configuration._orders_are_distributed;
}

//-----------------------------------------------------------Spooler::assert_are_orders_distributed

void Spooler::assert_are_orders_distributed( const string& text )
{
    if( !orders_are_distributed() )  z::throw_xc( "SCHEDULER-370", text );
}

//-----------------------------------------------------------------------Spooler::cluster_member_id

string Spooler::cluster_member_id()
{
    return _cluster? _cluster->my_member_id() : "";
}

//-------------------------------------------------------------------Spooler::distributed_member_id
// Liefert Member-ID nur für verteilten Scheduler,
// nicht aber im Backup-Betrieb.

string Spooler::distributed_member_id()
{
    string result;

    if( orders_are_distributed() )
    {
        result = cluster_member_id();
    }

    return result;
}

//----------------------------------------------------------------Spooler::db_distributed_member_id

string Spooler::db_distributed_member_id()
{
    string result = distributed_member_id();
    if( result == "" )  result = "-";
    return result;
}

//-----------------------------------------------------------------------Spooler::has_exclusiveness

bool Spooler::has_exclusiveness()
{
    return !_cluster || _cluster->has_exclusiveness();
}

//----------------------------------------------------------------Spooler::assert_has_exclusiveness

//void Spooler::assert_has_exclusiveness( const string& text )
//{
//    if( !has_exclusiveness() )
//    {
//        z::throw_xc( "SCHEDULER-366", text );
//    }
//}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop( const exception* )
{
    assert( current_thread_id() == _thread_id );

    bool restart = _shutdown_cmd == sc_terminate_and_restart 
                || _shutdown_cmd == sc_let_run_terminate_and_restart;

    if( _shutdown_ignore_running_tasks )  _spooler->kill_all_processes( kill_task_subsystem );   // Übriggebliebene Prozesse killen
    stop_cluster();
    stop_subsystems();

    if( _scheduler_event_manager )  _scheduler_event_manager->close_responses();
    _communication.finish_responses( 5.0 );
    _communication.close( restart? tcp_restart_close_delay : 0.0 );      // Mit Wartezeit. Vor Restart, damit offene Verbindungen nicht vererbt werden.

    _db->spooler_stop();
    _db->close();

    _java_subsystem->switch_subsystem_state( subsys_stopped );
    set_state( s_stopped );     
    // Der Dienst ist hier beendet

    update_console_title( 0 );

    if( restart )  
        spooler_restart( &_base_log, _is_service );
}

//----------------------------------------------------------------------------Spooler::nichts_getan

void Spooler::nichts_getan( int anzahl, const string& str )
{
    if( anzahl == 1  
     || anzahl >= scheduler_261_second &&  ( anzahl - scheduler_261_second ) % scheduler_261_intervall == 0 )
    {
        S tasks;
        S jobs;

        if( _task_subsystem )  
        {
            FOR_EACH(Task_set, _task_subsystem->_task_set, t ) {
                Task* task = *t;
                if( tasks.length() > 0 )  tasks << ", ";
                tasks << task->obj_name() << " " << task->state_name();
            }
        }
        if( tasks.length() == 0 )  tasks << "no tasks";


        //FOR_EACH_JOB( job )  
        //{
        //    if( jobs.length() > 0 )  jobs << ", ";
        //    jobs << job->obj_name() << " " << job->state_name();
        //    if( !job->is_in_period( Time::now() ) )  jobs << " (not in period)";
        //    if( job->waiting_for_process() )  jobs << " (waiting for process)";
        //}
        //if( jobs.length() == 0 )  jobs << "no jobs";

        _log->log( anzahl <= 1? log_debug9 :
                   anzahl <= 2? log_info
                              : log_warn,
                  message_string( "SCHEDULER-261", str, _connection_manager->string_from_operations(), tasks, jobs ) );  // "Nichts getan, state=$1, _wait_handles=$2"

        // Wenn's ein System-Ereignis ist, das, jedenfalls unter Windows, immer wieder signalisiert wird,
        // dann kommen die anderen Ereignisse, insbesondere der TCP-Verbindungen, nicht zum Zuge.
        // Gut wäre, in einer Schleife alle Ereignisse zu prüfen und dann Event::_signaled zu setzen.
        // Aber das ganze sollte nicht vorkommen.
    }

    double t = 1;
    Z_LOG2( "scheduler", Z_FUNCTION << " sleep(" << t << ")...\n" );
    sos_sleep( t );
}

//----------------------------------------------------------------Spooler::is_termination_state_cmd

bool Spooler::is_termination_state_cmd() 
{ 
    return _state_cmd == sc_terminate || 
           _state_cmd == sc_terminate_and_restart || 
           _state_cmd == sc_let_run_terminate_and_restart; 
}

//-----------------------------------------------------------------------Spooler::execute_state_cmd

void Spooler::execute_state_cmd()
{
    if( _state_cmd )
    {
        if (_state_cmd == sc_pause) {
            if (_state == s_running) set_state(s_paused);
            else 
            if (_state == s_waiting_for_activation) set_state(s_waiting_for_activation_paused);
        } else 
        if (_state_cmd == sc_continue) {
            if (_state == s_paused) set_state(s_running);
            else 
            if (_state == s_waiting_for_activation_paused) set_state(s_waiting_for_activation);
        }

        if( _state_cmd == sc_load_config  
         || _state_cmd == sc_reload       
         || _state_cmd == sc_terminate             
         || _state_cmd == sc_terminate_and_restart 
         || _state_cmd == sc_let_run_terminate_and_restart )
        {
            if( _state_cmd != _shutdown_cmd )
            {
                set_state( _state_cmd == sc_let_run_terminate_and_restart? s_stopping_let_run : s_stopping );

                if( _state == s_stopping )
                {
                    FOR_EACH_JOB( job )
                    {
                        //_log->info( message_string( "SCHEDULER-903", job->obj_name() ) );        // "Stopping"
                        bool end_all_tasks = true;
                        job->stop_simply( end_all_tasks );
                    }
                }

                if( _state == s_stopping_let_run )
                {
                    end_waiting_tasks();
                }

                if( _termination_async_operation )
                {
                    _connection_manager->remove_operation( _termination_async_operation );
                    _termination_async_operation = NULL;
                }

                if( _termination_gmtimeout_at != time_max ) 
                {
                    _log->info( message_string( "SCHEDULER-904", ( _termination_gmtimeout_at - ::time(NULL) ) ) );
                    //_log->info( S() << "Die Frist zum Beenden der Tasks endet in " << ( _termination_gmtimeout_at - ::time(NULL) ) << "s" );

                    if( _task_subsystem )
                    {
                        _termination_async_operation = Z_NEW( Termination_async_operation( this, _termination_gmtimeout_at ) );
                        _termination_async_operation->set_async_manager( _connection_manager );
                        _connection_manager->add_operation( _termination_async_operation );
                    }
                }


                _shutdown_cmd = _state_cmd;
            }
        }

        _state_cmd = sc_none;
    }
}

//-----------------------------------------------------------------------Spooler::end_waiting_tasks

void Spooler::end_waiting_tasks()
{
    if( _task_subsystem )
    {
        FOR_EACH(Task_set, _task_subsystem->_task_set, t) {
            Task* task = *t;

            if( task->state() == Task::s_running_waiting_for_order )  {
                //_log->info( S() << "end " << task->obj_name() );
                task->cmd_end();
            }
        }
    }
}

//---------------------------------------------------------------------------------Spooler::try_run

void Spooler::try_run() 
{
    try {
        run();
    }
    catch( exception& x ) {
        set_state( s_stopping );        // Wichtig, damit _log wegen _waiting_errno nicht blockiert!
        if (_cluster) {
            _cluster->set_scheduler_stops_because_of_error();
        }

        try {
            _log->error( x.what() );
            _log->error( message_string( "SCHEDULER-264" ) );  // "SCHEDULER TERMINATES AFTER SERIOUS ERROR"
        }
        catch( exception& ) {}

        try { 
            log_show_state( _log );
            stop( &x ); 
        } 
        catch( exception& x ) { _log->error( x.what() ); }

        throw;
    }
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    int                 nothing_done_count          = 0;
    int                 nichts_getan_zaehler        = 0;
    Cached_log_category scheduler_loop_log_category ( "scheduler.loop" );
    Cached_log_category scheduler_wait_log_category ( "scheduler.wait" );

    //---------------------------------------------------------------------------------------------

    while(1)  // Die große Hauptschleife
    {
        Time    wait_until        = Time::never;
        Object* wait_until_object = NULL;    
        Time    resume_at         = Time::never;
        Object* resume_at_object  = NULL;

        _loop_counter++;

        //-----------------------------------------------------------------------------------------

        if( !_is_activated ) {
            if( cluster_is_active()  &&  
                ( !_supervisor_client  ||  _supervisor_client->is_ready()  ||  _supervisor_client->connection_failed() )  &&
                ( !_cluster_configuration._demand_exclusiveness  ||  _cluster && _cluster->has_exclusiveness() ) )
            {
                _is_activated = true;
                activate(_cluster && _cluster->is_paused() ? s_paused : s_running);
                _assert_is_active = true;
            }
            else
            if( _state == s_starting )
                set_state( s_waiting_for_activation );
        }

        //---------------------------------------------------------------------------------CONTINUE
        // Hier werden die asynchronen Operationen fortgesetzt, die eigentliche Scheduler-Arbeit

        _scheduler_event.reset();

        execute_state_cmd();
        if( _shutdown_cmd )  if( !_task_subsystem  ||  !_task_subsystem->has_tasks()  ||  _shutdown_ignore_running_tasks )  break;

        Time now = Time::now();
        Time micro_step_start_time = now;
        
        bool something_done = run_continue( now );
        
        if( _cluster )  check_cluster();
        _order_subsystem->check_exception();

        if( _task_subsystem  &&  _task_subsystem->is_ready_for_termination() )  break;

        if( something_done )  wait_until = Time(0);   // Nicht warten, wir drehen noch eine Runde

        int64 next_millis = schedulerJ().onEnteringSleepState();
        something_done |= next_millis < 0;  // negativ
        wait_until = min(wait_until, Time(min(abs(next_millis) / 1000.0, Time::never.as_double())));

        //----------------------------------------------------------------------------NICHTS GETAN?

        if( something_done ) {
            if( nichts_getan_zaehler )  Z_LOG2( "scheduler.nothing_done", "something_done!, nichts_getan_zaehler=" << nichts_getan_zaehler << "\n" );
            nothing_done_count = 0;
            nichts_getan_zaehler = 0;
        } else {
            if( ++nothing_done_count > nothing_done_max ) {
                nichts_getan( ++nichts_getan_zaehler, "" );
                if( wait_until.is_zero() )  wait_until = Time::now() + Duration(1);
            }
            if( nothing_done_count > 1 ) {
                Z_LOG2( "scheduler.nothing_done", "nothing_done_count=" << nothing_done_count << " nichts_getan_zaehler=" << nichts_getan_zaehler << "\n" );
            }
        }

        //----------------------------------------------------------------------WARTEZEIT ERMITTELN

      //if( !something_done  &&  wait_until > 0  &&  _state_cmd == sc_none  &&  wait_until > Time::now() )   Immer wait() rufen, damit Event.signaled() gesetzt wird!
        {
            Wait_handles wait_handles ( this );

            // TCP- und UDP-HANDLES EINSAMMELN, für spooler_communication.cxx
            vector<System_event*> events;
            _connection_manager->get_events( &events );  // JS-471 TCP und UDP-Verbindungen am Anfang der Handles
            FOR_EACH( vector<System_event*>, events, e )  wait_handles.add( *e );

            // PROCESS-HANDLES EINSAMMELN
    
            #ifndef Z_WINDOWS
                if( !wait_until.is_zero() )
            #endif 
            FOR_EACH_FILE_BASED( Process_class, process_class ) {
                Z_FOR_EACH( Process_class::Process_set, process_class->process_set(), p ) {
                    #ifdef Z_WINDOWS
                        if( object_server::Connection* server = (*p)->connection())
                            if( server->process_event() && *server->process_event() )  wait_handles.add( server->process_event() );        // Signalisiert Prozessende
                    #else
                        Time next_time;
                        next_time.set_utc( (*p)->async_next_gmtime() );
                        //Z_LOG2( "scheduler", **p << "->async_next_gmtime() => " << next_time << "\n" );
                        if( next_time < wait_until )
                        {
                            wait_until = next_time;
                            wait_until_object = *p;
                            if( wait_until.is_zero() )  break;
                        }
                    #endif
                }
 
                if( wait_until.is_zero() )  break;
            }

            {
                Time n = Time::now();
                if (n >= micro_step_start_time + _max_micro_step_time)  _log->warn(message_string("SCHEDULER-721", (n - micro_step_start_time).as_string()));
            }

            //-------------------------------------------------------------------------------WARTEN

            wait_handles += _wait_handles;

            if( nothing_done_count > 0  ||  !wait_handles.signaled() )   // Wenn "nichts_getan" (das ist schlecht), dann wenigstens alle Ereignisse abfragen, damit z.B. ein TCP-Verbindungsaufbau erkannt wird.
            {
                if( wait_until.is_zero() )
                {
                    wait_handles.wait_until( Time(), wait_until_object, Time(), NULL );   // Signale checken
                }
                else
                {
                    Time now_before_wait = nothing_done_count == nothing_done_max? Time::now() : Time(0);

                    wait( &wait_handles, wait_until, wait_until_object, resume_at, resume_at_object );

                    if( nothing_done_count == nothing_done_max  &&  Time::now() - now_before_wait >= Duration(0.010) )  nothing_done_count = 0, nichts_getan_zaehler = 0;
                }
            }

            if( zschimmer::Log_ptr log = scheduler_loop_log_category ) {
                S line;
                line << "-------------scheduler loop " << _loop_counter << "-------------> " << 
                        "  wait_until=" << wait_until.as_string(_time_zone_name);
                if( wait_until_object )  line << "  for " << wait_until_object->obj_name();
                line << ", something_done=" << something_done << "\n";  
                log << line;
            }

            wait_handles.clear();
        }

        //-----------------------------------------------------------------------------------CTRL-C

        run_check_ctrl_c();
    }
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    Wait_handles wait_handles = _wait_handles;
    wait( &wait_handles, Time::never, NULL, Time::never, NULL );
    wait_handles.clear();
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait( Wait_handles* wait_handles, const Time& wait_until_, Object* wait_until_object, const Time& resume_at, Object* resume_at_object )
{
    Time wait_until = wait_until_;
    bool signaled   = false;

    _wait_counter++;
    _last_wait_until = wait_until;
    _last_resume_at  = resume_at;


    // Termination_async_operation etc.

    if( !wait_until.is_zero() )
    {
        if( ptr<Async_operation> operation = _connection_manager->async_next_operation() )
        {
            Time next_time;
            next_time.set_utc( operation->async_next_gmtime() );
            //Z_LOG2( "scheduler", **p << "->async_next_gmtime() => " << next_time << "\n" );
            if( next_time < wait_until )
            {
                wait_until = next_time;
                wait_until_object = operation;
            }
        }
    }


    if( _zschimmer_mode  &&  _should_suspend_machine  &&  is_machine_suspendable() )  // &&  !_task_subsystem->has_tasks() )
    {
#       ifdef Z_WINDOWS
            if( !IsSystemResumeAutomatic() )  _should_suspend_machine = false;  // Rechner ist nicht automatisch gestartet, sondern durch Benutzer? Dann kein Suspend

            if( _should_suspend_machine )
            {
                Time now = Time::now();
                if( now + inhibit_suspend_wait_time < resume_at )
                {
                    signaled = wait_handles->wait_until( min( now + before_suspend_wait_time, wait_until ), wait_until_object, resume_at, resume_at_object );
                    if( !signaled )   // Nichts passiert?
                    {
                        if( IsSystemResumeAutomatic() )   // Benutzer schläft noch?
                        {
                            suspend_machine();
                        }

                        _should_suspend_machine = false;
                    }
                }
            }
#       endif
    }


//#   ifndef Z_UNIX   // Unter Unix mit Verzeichnisüberwachung gibt der Scheduler alle show_message_after_seconds Sekunden die Meldung SCHEDULER-972 aus
//        if( !signaled  &&  !_cluster  &&  !_print_time_every_second )
//        {
//            Time first_wait_until = _base_log.last_time() + ( _log->log_level() <= log_debug3? show_message_after_seconds_debug : show_message_after_seconds );
//            if( first_wait_until < wait_until )
//            {
//                string msg = message_string( "SCHEDULER-972", wait_until.as_string(), wait_until_object );
//                if( msg != _log->last_line() ) 
//                {
//                    String_object o ( msg );
//                    signaled = wait_handles->wait_until( first_wait_until, &o, resume_at, resume_at_object );
//                    if( !signaled  &&  msg != _log->last_line() )  _log->info( msg );
//                }
//            }
//        }
//#   endif

    if( !signaled )
    {
        wait_handles->wait_until( wait_until, wait_until_object, resume_at, resume_at_object );
    }
}

//----------------------------------------------------------------------------Spooler::run_continue

bool Spooler::run_continue( const Time& now )
{
    bool something_done = false;

    // PROZESSE FORTSETZEN, durch zentrales _scheduler_event signalisiert
    something_done |= _process_class_subsystem->async_continue();
    
    if( something_done )  _last_wait_until = Time(0), _last_resume_at = Time(0);

    // TCP- UND UDP-VERBINDUNGEN IN SPOOLER_COMMUNICATION.CXX FORTSETZEN
    something_done |= _connection_manager->async_continue();

    if (_settings->_use_old_microscheduling_for_jobs && _job_subsystem) {
        _job_subsystem->do_something();
    }
    if (_settings->_use_old_microscheduling_for_tasks && _task_subsystem) {  
        something_done |= _task_subsystem->do_something();
    }

    return something_done;
}

//----------------------------------------------------------------------------------Spooler::signal
// Thread-fähig
void Spooler::signal()       
{ 
    _scheduler_event.signal(""); 
    #ifdef Z_UNIX
        if (::pthread_self() != _spooler->thread_id()) {
            _communication.signal();
        }
    #endif
}

//---------------------------------------------------------------------------Spooler::check_cluster

void Spooler::check_cluster()
{
    _cluster->check();
    check_is_active();
}


void Spooler::do_a_heart_beat_when_needed(const string& debug_text) {
    if (_cluster) {
        _cluster->do_a_heart_beat_when_needed(Z_FUNCTION);
    } 
}

//------------------------------------------------------------------Spooler::assert_is_still_active

bool Spooler::assert_is_still_active( const string& debug_function, const string& message_text, Transaction* outer_transaction )
{
    bool result = true;

    if( !check_is_active( outer_transaction ) )  
    {
        cmd_terminate_after_error( debug_function, message_text );
        result = false;
    }

    return result;
}

//-------------------------------------------------------------------------Spooler::check_is_active

bool Spooler::check_is_active( Transaction* outer_transaction )

// Kann nach jeder vermutlich längeren Operation aufgerufen werden, vor allem bei externer Software: Datenbank, eMail, xslt etc.

{
    // Wir sind mitten in irgendeiner Verarbeitung. Also nur bestimmte Sachen verändern!

    bool result = true;

    if( _assert_is_active  &&  _cluster )
    {
        if( Not_in_recursion not_in_recursion = &_is_in_check_is_active )
        {
            if( !_cluster->check_is_active( outer_transaction )  
            ||  _cluster_configuration._demand_exclusiveness && !_cluster->has_exclusiveness() )
            {
                _assert_is_active = false;

                kill_all_processes( kill_task_subsystem );
                
                _log->error( message_string( _cluster_configuration._demand_exclusiveness? "SCHEDULER-367" : "SCHEDULER-362" ) );

                _cluster->show_active_schedulers( outer_transaction );

                //_cluster offen lassen, damit belegte Aufträge freigegeben werden können.
                // Außerdem sind wir mitten in irgendeiner Verarbeitung, da sollten wir _cluster weder schließen noch entfernen

                //_cluster->close();     // Scheduler-Mitglieds-Eintrag entfernen
                //_cluster = NULL;       // aber Eintrag für verteilten Scheduler lassen, Scheduler ist nicht herunterfahren (wird ja vom anderen aktiven Scheduler fortgesetzt)

                cmd_terminate( false, INT_MAX, cluster::continue_exclusive_any );
                result = false;
            }
        }
    }

    return result;
}

//---------------------------------------------------------------------Spooler::assert_is_activated

void Spooler::assert_is_activated( const string& function )
{
    if( !_is_activated )  z::throw_xc( "SCHEDULER-381", function );
}

//----------------------------------------------Spooler::abort_immediately_after_distribution_error

void Spooler::abort_immediately_after_distribution_error( const string& debug_text )
{
    zschimmer::Xc x ( _cluster_configuration._demand_exclusiveness? "SCHEDULER-367" : "SCHEDULER-362", debug_text );
    
    _log->error( x.what() );
    send_error_email( x, _argc, _argv, _parameter_line, this );
    abort_immediately();
}

//------------------------------------------------------------------------Spooler::run_check_ctrl_c

void Spooler::run_check_ctrl_c()
{
    while( ctrl_c_pressed_handled < ctrl_c_pressed )
    {
#       ifdef Z_WINDOWS
            string signal_text = "ctrl-C";
#        else
            string signal_text = S() << "signal " << last_signal << " " << signal_name_from_code( last_signal ) << " " << signal_title_from_code( last_signal );
#       endif


    
        switch( ctrl_c_pressed_handled + 1 )
        {
            case 1:
            {
                string m = message_string( "SCHEDULER-263", signal_text, ctrl_c_pressed_handled + 1, "Stopping Scheduler" ); 
                _log->warn( m );
                if( !_log_to_stderr &&  !is_daemon )  cerr << m << endl;

                if( _state != s_stopping )
                {
                    _log->warn( message_string( "SCHEDULER-262", signal_text ) );       // "Abbruch-Signal (Ctrl-C) empfangen. Der Scheduler wird beendet.\n" );
                    cmd_terminate( false, INT_MAX, cluster::continue_exclusive_any );
                }

                set_ctrl_c_handler( true );
                ctrl_c_pressed_handled = 1;
                break;
            }

            case 2:
            {
                string m = message_string( "SCHEDULER-263", signal_text, ctrl_c_pressed_handled + 1, "Killing all processes" );
                _log->warn( m );
                if( !_log_to_stderr && !is_daemon )  cerr << m << endl;

                kill_all_processes( kill_task_subsystem );

                set_ctrl_c_handler( true );
                ctrl_c_pressed_handled = 2;
                break;
            }

            case 3:
            {
                string m = message_string( "SCHEDULER-263", signal_text, ctrl_c_pressed_handled + 1, "Ignoring running tasks" );
                _log->warn( m );
                if( !_log_to_stderr && !is_daemon )  cerr << m << endl;

                _spooler->_shutdown_ignore_running_tasks = true;
                ctrl_c_pressed_handled = 3;
                break;
            }

            case 4:
            default:
            {
                string m = message_string( "SCHEDULER-263", signal_text, ctrl_c_pressed, "ABORTING IMMEDIATELY" );
                _log->warn( m );
                if( !_log_to_stderr && !is_daemon )  cerr << m << endl;

                abort_now();
                ctrl_c_pressed_handled = 4;
            }
        }

        //ctrl_c_pressed_handled = ctrl_c_pressed;
    }
}

//--------------------------------------------------------------Spooler::begin_dont_suspend_machine

void Spooler::begin_dont_suspend_machine()
{
    if( _dont_suspend_machine_counter == 0 )
    {
#       ifdef Z_WINDOWS
            Z_LOG2( "scheduler", "SetThreadExecutionState(ES_CONTINUOUS|ES_SYSTEM_REQUIRED);\n" );
            SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED );
#       endif
    }

    _dont_suspend_machine_counter++;

    _should_suspend_machine = false;
}

//----------------------------------------------------------------Spooler::end_dont_suspend_machine

void Spooler::end_dont_suspend_machine()
{
    _dont_suspend_machine_counter--;

    if( _dont_suspend_machine_counter == 0 )
    {
#       ifdef Z_WINDOWS
            Z_LOG2( "scheduler", "SetThreadExecutionState(ES_CONTINUOUS);\n" );
            SetThreadExecutionState( ES_CONTINUOUS );
#       endif

        if( _suspend_after_resume )  _should_suspend_machine = true;
    }
}

//-------------------------------------------------------------------------Spooler::suspend_machine

void Spooler::suspend_machine()
{
#   ifdef Z_WINDOWS

        BOOL ok;


        Z_LOG2( "scheduler", "SetSystemPowerState(TRUE,FALSE) ...\n" );

        bool suspend_flag = true;
        bool force_flag   = false;
        ok = SetSystemPowerState( suspend_flag, force_flag );  // Dazu brauchen wir ein Recht

        int error = GetLastError();
        if( !ok  &&  error == ERROR_PRIVILEGE_NOT_HELD )
        {
            // Erlaubnis für SetSystemPowerState() einholen
        
            windows::Handle process_token;

            ok = OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, process_token.addr_of() );
            if( ok )
            {
                TOKEN_PRIVILEGES token_privileges;

                token_privileges.PrivilegeCount = 1;
                token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                ok = LookupPrivilegeValue( "", SE_SHUTDOWN_NAME, &token_privileges.Privileges[0].Luid );
                if( ok )
                {
                    ok = AdjustTokenPrivileges( process_token, FALSE, &token_privileges, 0, NULL, NULL );
                }
            }

            ok = SetSystemPowerState( suspend_flag, force_flag );
            int error = GetLastError();
        }

        Z_LOG2( "scheduler", "SetSystemPowerState(TRUE,FALSE) => " << (ok? "ok" : message_string( printf_string( "MSWIN-%08X", error ) ) ) << "\n" );

#   endif
}

//----------------------------------------------------------------------Spooler::execute_xml_string

string Spooler::execute_xml_string(const string& xml_command) {
    return execute_xml_string_with_security_level(xml_command, Security::seclev_all, Host());
}

//--------------------------------------------------Spooler::execute_xml_string_with_security_level

string Spooler::execute_xml_string_with_security_level(const string& xml_command, const string& security_level, const string& client_host) {
    return execute_xml_string_with_security_level(xml_command, Security::as_level(security_level), Host(client_host));
}

//--------------------------------------------------Spooler::execute_xml_string_with_security_level

string Spooler::execute_xml_string_with_security_level(const string& xml_command, Security::Level security_level, const Host& client_host) {
    Command_processor cp ( _spooler, security_level, client_host);
    cp.set_log(log());
    return cp.execute_xml_string(xml_command);
}

//-----------------------------------------------------------------------Spooler::java_execute_http

http::Java_response* Spooler::java_execute_http(const SchedulerHttpRequestJ& requestJ, const SchedulerHttpResponseJ& responseJ) {
   return java_execute_http_with_security_level(requestJ, responseJ, as_string(Security::seclev_all) );
}

//-----------------------------------------------------------------------Spooler::java_execute_http

http::Java_response* Spooler::java_execute_http_with_security_level(const SchedulerHttpRequestJ& requestJ, const SchedulerHttpResponseJ& responseJ, const string& security_level) {
    Command_processor command_processor ( this, Security::as_level(security_level));
    command_processor.set_log(log());
    ptr<http::Request> request = http::new_java_request(requestJ);
    ptr<http::Java_response> response = Z_NEW(http::Java_response(request, responseJ));
    command_processor.execute_http(request, response, (Http_file_directory*)NULL);
    response->set_ready();
    response->finish();
    return response.copy();  // Java muss Java_response::Release aufrufen, um Speicherleck zu vermeiden!
}

//-------------------------------------------------------------------------Spooler::cmd_load_config

void Spooler::cmd_load_config( const xml::Element_ptr& config, const string& source_filename )  
{ 
    _config_document_to_load = config.ownerDocument(); 
    _config_element_to_load  = config;
    _config_source_filename  = source_filename;
    _state_cmd = sc_load_config; 
}

//----------------------------------------------------------------------------Spooler::cmd_continue
// Anderer Thread (spooler_service.cxx)

void Spooler::cmd_continue()
{
    if (_state == s_paused || _state == s_waiting_for_activation_paused) {
        _state_cmd = sc_continue;
    }
    
    //if( _waiting_errno )  _waiting_errno_continue = true;       // Siehe spooler_log.cxx: Warten bei ENOSPC

    signal(); //_call_register.call<Continue_scheduler_call>();
}

//-------------------------------------------------------------------------------Spooler::cmd_pause

void Spooler::cmd_pause() 
{
    _state_cmd = sc_pause; 
    signal(); //_call_register.call<Pause_scheduler_call>();
}

//------------------------------------------------------------------------------Spooler::cmd_reload

void Spooler::cmd_reload()
{
    _state_cmd = sc_reload;
    signal(); //_call_register.call<Reload_scheduler_call>();
}

//---------------------------------------------------------------Spooler::cmd_terminate_after_error

void Spooler::cmd_terminate_after_error( const string& debug_function, const string& debug_text )
{
    _log->error( message_string( "SCHEDULER-264", "in " + debug_function, debug_text ) );

    cmd_terminate( false, INT_MAX, cluster::continue_exclusive_any );  
}

//---------------------------------------------------------------------------Spooler::cmd_terminate
// Anderer Thread (spooler_service.cxx)

void Spooler::cmd_terminate( bool restart, int timeout, const string& continue_exclusive_operation, bool terminate_all_schedulers )
{
    if( timeout < 0 )  timeout = 0;

    _state_cmd                = restart? sc_terminate_and_restart : sc_terminate;
    _terminate_all_schedulers_with_restart = restart;
    _termination_gmtimeout_at = timeout < 999999999? ::time(NULL) + timeout : time_max;
    _terminate_all_schedulers = terminate_all_schedulers;
    _terminate_continue_exclusive_operation = continue_exclusive_operation == "non_backup"? cluster::continue_exclusive_non_backup
                                                                                          : continue_exclusive_operation;

    signal(); //_call_register.call<Terminate_scheduler_call>();
}

//-------------------------------------------------------Spooler::cmd_let_run_terminate_and_restart

void Spooler::cmd_let_run_terminate_and_restart()
{
    _state_cmd = sc_let_run_terminate_and_restart;
    signal(); //_call_register.call<Let_run_terminate_and_restart_scheduler_call>();
}

//----------------------------------------------------------------------------Spooler::cmd_add_jobs

void Spooler::cmd_add_jobs( const xml::Element_ptr& element )
{
    //_job_subsystem->set_dom( element, Time::now(), true );
    root_folder()->job_folder()->set_dom( element );

    //signal( "add_jobs" );
}

//---------------------------------------------------------------------------------Spooler::cmd_job

void Spooler::cmd_job( const xml::Element_ptr& element )
{
    //_job_subsystem->load_job_from_xml( element, Time::now(), _spooler->state() >= Spooler::s_starting );
    root_folder()->job_folder()->add_or_replace_file_based_xml( element );

    //signal( "add_job" );
}

//-----------------------------------------------------------------------Spooler::abort_immediately

//void Spooler::abort_immediately( const string& message_text )
//{
//    _log->error( message_text );
//    abort_immediately( false );
//}

//-----------------------------------------------------------------------Spooler::abort_immediately

void Spooler::abort_immediately( bool restart, const string& message_text )
{
    try
    { 
        if( message_text != "" )  _log->error( message_text );

        kill_all_processes( kill_task_subsystem );
        _log->close(); 

     //?_communication.finish_responses( 5.0 );
        _communication.close( restart? tcp_restart_close_delay : 0 );    // Mit Wartezeit. Vor Restart, damit offene Verbindungen nicht vererbt werden. 
                                            // close(), damit offene HTTP-Logs ordentlich schließen (denn sonst ersetzt ie6 das Log durch eine Fehlermeldung)
    } 
    catch( ... ) {}

    abort_now( restart );
}

//-------------------------------------------------------------------------------Spooler::abort_now
// KANN VON EINEM ANDEREN THREAD GERUFEN WERDEN

void Spooler::abort_now( bool restart )
{
    // So schnell wie möglich abbrechen!

    int exit_code = 99;


    kill_all_processes( kill_registered_pids_only );

    if( restart )
    {
        try{ spooler_restart( NULL, is_service() ); } catch(...) {}
    }

    try
    {
        _pid_file.~File();
    } 
    catch( ... ) {}



    // Point of no return

#   ifdef Z_WINDOWS
        Z_LOG2( "scheduler", "TerminateProcess( GetCurrentProcess() );\n" );
        TerminateProcess( GetCurrentProcess(), exit_code );
        _exit( exit_code );
#    else
        // Das bricht auch den restart-Scheduler ab:  try_kill_process_group_immediately( _pid );
        try_kill_process_immediately( _pid );
        _exit( exit_code );
#   endif

}

////------------------------------------------------------------Spooler::on_call Pause_scheduler_call
//
//void Spooler::on_call(const Pause_scheduler_call&) {
//}
//
////---------------------------------------------------------Spooler::on_call Continue_scheduler_call
//
//void Spooler::on_call(const Continue_scheduler_call&) {
//}
//
////-----------------------------------------------------------Spooler::on_call Reload_scheduler_call
//
//void Spooler::on_call(const Reload_scheduler_call&) {
//}
//
////--------------------------------------------------------Spooler::on_call Terminate_scheduler_call
//
//void Spooler::on_call(const Terminate_scheduler_call&) {
//}
//
////------------------------------------Spooler::on_call Let_run_terminate_and_restart_scheduler_call
//
//void Spooler::on_call(const Let_run_terminate_and_restart_scheduler_call&) {
//}
//
//----------------------------------------------------------------------Spooler::kill_all_processes

void Spooler::kill_all_processes( Kill_all_processs_option option )
{
    if( option == kill_task_subsystem  &&  _task_subsystem )  
    {
        _task_subsystem->end_all_tasks(task_end_kill_immediately);

        // Auf "ps -ef" warten, bevor Spooler::kill_all_processes() ausgeführt wird. Dann kann ps den Prozess und seine Nachfahren zeigen
        sleep( 0.5 );  
    }


    for( int i = 0; i < NO_OF( _process_handles ); i++ )  
    {
        if( _process_handles[i] )  
        {
            try_kill_process_immediately( _process_handles[i] );
            _process_handles[i] = 0;
        }
    }

    for( int i = 0; i < NO_OF( _pids ); i++ )
    {
        if( _pids[i]._pid )  
        {
#           ifdef Z_UNIX
                if( _pids[i]._is_process_group )  posix::try_kill_process_group_immediately( _pids[i]._pid );
                else  
#           endif
                try_kill_process_immediately( _pids[i]._pid );

            _pids[i]._pid = 0;
        }
    }
    
    _are_all_tasks_killed = true;
}

//------------------------------------------------------------Spooler::detect_warning_and_send_mail

void Spooler::detect_warning_and_send_mail()
{
    // Wenn eine Warnung ins _Hauptprotokoll_ ausgegeben worden ist, eMail versenden
    if( _log->highest_level() >= log_warn ) // &&  _log->mail_to() != ""  &&  _log->mail_from() != "" )
    {
        try
        {
            string subject = name_of_log_level( _log->highest_level() ) + ": " + _log->highest_msg();
            S      body;

            body << Sos_optional_date_time::now().as_string() << "  " << name() << "\n\n";
            body << "Scheduler started with ";
            body << ( _log->highest_level() == log_warn? "warning" : "error" ) << ":\n\n";
            body << subject << "\n\n";

            Scheduler_event scheduler_event ( evt_scheduler_started, _log->highest_level(), this );

            if( _log->highest_level() >= log_error )  scheduler_event.set_error( Xc( "SCHEDULER-227", _log->last( _log->highest_level() ).c_str() ) );
            else
            if( _log->highest_level() == log_warn )   scheduler_event.set_message( _log->last( log_warn ) );
            

            _log->set_mail_default( "subject"  , subject );
            _log->set_mail_default( "body"     , body );
            _log->send( &scheduler_event );
        }
        catch( exception& x )  { _log->warn( S() << "Error on sending mail: " << x.what() ); }
    }
}

//--------------------------------------------------------------------------Spooler::log_show_state

void Spooler::log_show_state( Prefix_log* log )
{
    try
    {
        Command_processor cp     ( this, Security::seclev_all );
        string xml = cp.execute_xml_string( "<show_state what='folders jobs job_params job_commands tasks task_queue job_chains orders remote_schedulers operations' />", "  " );

        if( log )
        {
            try
            {
                log->info( xml );   // Blockiert bei ENOSPC nicht wegen _state == s_stopping
            }
            catch( exception& ) 
            { 
                log = NULL; 
            }
        }

        if( !log )  Z_LOG2( "scheduler", "\n\n" << xml << "\n\n" );
    } 
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//----------------------------------------------------------------------------Spooler::enqueue_call

Timed_call* Spooler::enqueue_call(Timed_call* call) {
    schedulerJ().enqueueCall(call->java_sister());
    return call;
}

//-----------------------------------------------------------------------------Spooler::cancel_call

void Spooler::cancel_call(Timed_call* call) {
    if (call) 
        if (SchedulerJ& o = schedulerJ()) 
            o.cancelCall(call->java_sister());
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv, const string& parameter_line)
{
    spooler_ptr = this;

    _argc = argc;
    _argv = argv;
    _parameter_line = parameter_line;

    _java_subsystem = new_java_subsystem(this);

    _variable_set_map[ variable_set_name_for_substitution ] = _environment;


#   ifndef Z_WINDOWS
        ::signal( SIGPIPE, SIG_IGN );    // Fuer Linux eigentlich nicht erforderlich, weil com_remote.cxx() SIGPIPE selbst unterdrueckt
#   endif
    
    set_ctrl_c_handler( true );

    tzset();    // Timezone

    _thread_id = current_thread_id();

    _scheduler_event.set_name( "Scheduler" );
    _scheduler_event.set_waiting_thread_id( current_thread_id() );
    _scheduler_event.create();
    _scheduler_event.add_to( &_wait_handles );

#   ifdef Z_WINDOWS
        if( !_is_service )  _has_windows_console = true;
#   endif

    _communication.init();  // Initialisiert Windows-Sockets

    if( _state_cmd != sc_load_config )  load();
    if( !_config_element_to_load )  z::throw_xc( "SCHEDULER-116", _spooler_id );

    if (_is_service) {
        assign_stdout();
    }
    //Erst muss noch _config_commands_element ausgeführt werden: _config_element_to_load = NULL;
    //Erst muss noch _config_commands_element ausgeführt werden: _config_document_to_load = NULL;

    // Nachdem argv und profile gelesen sind und config geladen ist:

    _mail_defaults.set( "from_name", name() );      // Jetzt sind _complete_hostname und _tcp_port bekannt
    _log->init( this );                              // Neue Einstellungen übernehmen: Default für from_name


    if( _send_cmd_xml_bytes != "" ) { 
        send_cmd();  
        stop(); 
    }
    else {
        update_console_title();
        start();

        if( !_shutdown_cmd )
            try_run();

        stop();
        close();
        _log->info( message_string( "SCHEDULER-999" ) );        // "Scheduler ordentlich beendet"
        _log->close();

    }
    return 0;
}

//---------------------------------------------------------------------------Spooler::assign_stdout

void Spooler::assign_stdout()
{
    Path stdout_path = "scheduler.out";
    if( !string_begins_with( _log_directory, "*" ) )  stdout_path = Path( _log_directory, stdout_path );

    File new_stdout ( stdout_path, "w" );

    dup2( new_stdout.file_no(), fileno( stdout ) );
    dup2( new_stdout.file_no(), fileno( stderr ) );
}

//--------------------------------------------------------------------------Spooler::backup_logfile

string Spooler::backup_logfile( const File_path path )
{
   string msg = "";
   if( path.file_exists() )
   {
         size_t i = path.find_last_of(".");
         File_path scheduler_old = (path.extension().empty() || i == string::npos) ? path + "-old" : path.substr(0,i) + "-old." + path.extension();
         try
         {
            scheduler_old.try_unlink();
            path.move_to(scheduler_old);
            msg = S() << "File " << path << " moved to " << scheduler_old << "\n"; 
         }
         catch( exception& x ) { 
            try {
               msg = S() << x.what() << ", while renaming file " << path << " to " << scheduler_old << " - trying to copy\n"; 
               copy_file(path,scheduler_old);
               msg += S() << "File " << path << " copied to " << scheduler_old << "\n"; 
            }
            catch( exception& x1 ) {
               msg += S() << x1.what() << ", while copying file " << path << " to " << scheduler_old << "\n"; 
            }
         }

   }
   return msg;
}

//------------------------------------------------------------------------------------start_process
#ifdef Z_WINDOWS

static void start_process( const string& command_line )
{
    Z_LOG2( "scheduler", "start_process(\"" << command_line << "\")\n" );

    PROCESS_INFORMATION process_info; 
    STARTUPINFO         startup_info; 
    BOOL                ok;
    Dynamic_area        my_command_line;
    
    my_command_line.assign( command_line.c_str(), int_cast(command_line.length() + 1) );

    memset( &process_info, 0, sizeof process_info );

    memset( &startup_info, 0, sizeof startup_info );
    startup_info.cb = sizeof startup_info; 

    ok = CreateProcess( NULL,                       // application name
                        my_command_line.char_ptr(), // command line 
                        NULL,                       // process security attributes 
                        NULL,                       // primary thread security attributes 
                        FALSE,                      // handles are inherited?
                        0,                          // creation flags 
                        NULL,                       // use parent's environment 
                        NULL,                       // use parent's current directory 
                        &startup_info,              // STARTUPINFO pointer 
                        &process_info );            // receives PROCESS_INFORMATION 

    if( !ok )  throw_mswin_error("CreateProcess");

    CloseHandle( process_info.hThread );
    CloseHandle( process_info.hProcess );
}

#endif
//----------------------------------------------------------------------------make_new_spooler_path
#ifdef Z_WINDOWS

static string make_new_spooler_path( const string& this_spooler )
{
    return directory_of_path(this_spooler) + DIR_SEP + basename_of_path( this_spooler ) + new_suffix + ".exe";
}

#endif
//----------------------------------------------------------------------------------spooler_restart

void spooler_restart( Log* log, bool is_service )
{
#   ifdef Z_WINDOWS
    
        string this_spooler = program_filename();
        string command_line = GetCommandLine();
        string new_spooler  = make_new_spooler_path( this_spooler );

        if( GetFileAttributes( new_spooler.c_str() ) != -1 )      // spooler~new.exe vorhanden?
        {
            // Programmdateinamen aus command_line ersetzen
            size_t pos;
            if( command_line.length() == 0 )  z::throw_xc( "SCHEDULER-COMMANDLINE" );
            if( command_line[0] == '"' ) {
                pos = command_line.find( '"', 1 );  if( pos == string::npos )  z::throw_xc( "SCHEDULER-COMMANDLINE" );
                pos++;                
            } else {
                pos = command_line.find( ' ' );  if( pos == string::npos )  z::throw_xc( "SCHEDULER-COMMANDLINE" );
            }

            command_line = new_spooler + command_line.substr(pos);
                         //+ " -renew-spooler=" + quoted_string(this_spooler);
        }
        else
        {
            //command_line += " -renew-spooler";
        }

        if( is_service )  command_line += " -renew-service";

        command_line += " " + quoted_windows_process_parameter( "-renew-spooler=" + this_spooler );
        if( log )  log->info( message_string( "SCHEDULER-906", command_line ) );        // "Restart Scheduler "
        start_process( command_line );

#   else

        switch( int pid = fork() )
        {
            case 0:
            {
            //    setpgrp();   // Neue process group id

            //    switch( int pid2 = fork() )
            //    {
            //        case 0: 
            //        {
                        int n = sysconf( _SC_OPEN_MAX );
                        for( int i = 3; i < n; i++ )  close(i);
                        ::sleep( 1 );     // Warten bis Aufrufer sich beendet hat
                        if (static_ld_library_path_changed) {
                            set_environment_variable("LD_LIBRARY_PATH", static_original_ld_library_path);
                        }
                        execv( _argv[0], _argv ); 
                        fprintf( stderr, "Error in execv %s: %s\n", _argv[0], strerror(errno) ); 
                        _exit(99);
            //        }

            //        case -1: 
            //        {
            //            int errn = errno; 
            //            sprintf( stderr, "Error %d in fork(): %s\n", errn, strerror( errn ) );
            //            _exit(1);
            //        }

            //        default:
            //        {
            //            _exit(0);
            //        }
            //    }
            }       

            case -1: 
                throw_errno( errno, "execv", _argv[0] );

            default: 
            {
                Z_LOG2( "scheduler", "waitpid(" << pid << ")  JobScheduler restart\n" );
                //waitpid( pid, NULL, 0 );
                Z_LOG2( "scheduler", "waitpid(" << pid << ")  OK\n" );
            }
        }

#   endif
}

//------------------------------------------------------------------------------------spooler_renew
#ifdef Z_WINDOWS

static void spooler_renew( const string& service_name, const string& renew_spooler, bool is_service, const string& command_line )
{

    string this_spooler = program_filename();
    BOOL   copy_ok      = true;
    double t            = renew_wait_time;

    if( is_service ) 
    {
        // terminate_and_restart: Erst SERVICE_STOP_PENDING, dann SERVICE_STOPPED
        // abort_immediately_and_restart: SERVICE_RUNNING, vielleicht SERVICE_PAUSED o.a.

        for( t; t > 0; t -= renew_wait_interval )
        {
            if( scheduler::service_state(service_name) == SERVICE_STOPPED )  break;    
            sos_sleep( renew_wait_interval );
        }

        if( scheduler::service_state(service_name) != SERVICE_STOPPED )  return;
    }

    if( renew_spooler != this_spooler )
    {
        for( t; t > 0; t -= renew_wait_interval )  
        {
            string msg = "CopyFile " + this_spooler + ", " + renew_spooler + '\n';
            if( !is_service )  fprintf( stderr, "%s", msg.c_str() );  // stderr, weil wir kein Log haben.
            Z_LOG2( "scheduler", msg );

            copy_ok = CopyFile( this_spooler.c_str(), renew_spooler.c_str(), FALSE );
            if( copy_ok )  break;

            int error = GetLastError();
            try 
            { 
                throw_mswin_error( error, "CopyFile" ); 
            }
            catch( const exception& x ) { 
                if( !is_service )  fprintf( stderr, "%s\n", x.what() );
                Z_LOG2( "scheduler", x.what() << '\n' );
            }

            if( error != ERROR_SHARING_VIOLATION )  return;
            sos_sleep( renew_wait_interval );
        }

        if( !is_service )  fprintf( stderr, "Scheduler has been changed and will be restarted now\n\n" );
    }

    if( is_service )  scheduler::service_start( service_name );
                else  start_process( quoted_windows_process_parameter( renew_spooler ) + " " + command_line );
}

#endif
//----------------------------------------------------------------------------------------full_path
#ifdef Z_WINDOWS

static string full_path( const string& path )
{
    Sos_limited_text<MAX_PATH> full;

    char* p = _fullpath( full.char_ptr(), path.c_str(), full.size() );
    if( !p )  z::throw_xc( "fullpath", path );

    return full.char_ptr();
}

#endif
//-------------------------------------------------------------------------------delete_new_spooler
#ifdef Z_WINDOWS

void __cdecl delete_new_spooler( void* )
{
    string          this_spooler   = program_filename();
    string          basename       = basename_of_path( this_spooler );
    string          copied_spooler = make_new_spooler_path( this_spooler );
    struct _stat    ths;
    struct _stat    cop;
    int             err;

    if( full_path( this_spooler ) == full_path( copied_spooler ) )  return;

    err = _stat( this_spooler  .c_str(), &ths );  if(err)  return;
    err = _stat( copied_spooler.c_str(), &cop );  if(err)  return;

    if( ths.st_size == cop.st_size  &&  ths.st_mtime == cop.st_mtime )  // spooler.exe == spooler~new.exe?
    {
        for( double t = renew_wait_time; t > 0; t -= renew_wait_interval )  
        {
            string msg = "remove " + copied_spooler + '\n';
            fprintf( stderr, "%s", msg.c_str() );
            Z_LOG2( "scheduler", msg );

            int ret = _unlink( copied_spooler.c_str() );
            if( ret == 0  || errno != EACCES ) break;

            msg = "errno=" + as_string(errno) + ' ' + z_strerror(errno) + '\n';
            fprintf( stderr, "%s", msg.c_str() );
            Z_LOG2( "scheduler", msg.c_str() );
            
            sos_sleep( renew_wait_interval );
        }
    }
}

#endif
//---------------------------------------------------------------------sos::scheduler::spooler_main

int spooler_main( int argc, char** argv, const string& parameter_line, jobject java_main_context )
{
    int ret;

//#   if defined _DEBUG  && defined __GNUC__
//        mtrace();   // Memory leak detectiopn
//#   endif

    while(1)
    {
        Spooler my_spooler (java_main_context);

        try
        {
            if (argc < 1)  throw_xc("Missing arguments, argc=0");

            my_spooler._is_service = scheduler::is_daemon;

            ret = my_spooler.launch( argc, argv, parameter_line );

            if( my_spooler._shutdown_cmd == Spooler::sc_reload 
             || my_spooler._shutdown_cmd == Spooler::sc_load_config )  continue;        // Dasselbe in spooler_service.cxx!
        }
        catch(exception& x)
        {
            //my_spooler._log ist vielleicht noch nicht geöffnet oder schon geschlossen
            my_spooler._log->error( x.what() );
            my_spooler._log->error( message_string( "SCHEDULER-331" ) );
            if (!java_main_context) {
            string line = S() << "Error " << x.what();
                if (!isatty(fileno(stderr)))  cerr << line << "\n" << flush;
            else show_msg( line );     // Fehlermeldung vor ~Spooler ausgeben
            }

            if( my_spooler.is_service() )  send_error_email( x, argc, argv, parameter_line, &my_spooler );
            if (java_main_context)  throw;
            ret = 1;
        }

        break;
    }

    return ret;
}

//---------------------------------------------------------------------Object_server::Object_server

Object_server::Object_server()
{
    register_class( spooler_com::CLSID_Remote_module_instance_server, Com_remote_module_instance_server::Create_instance );
    register_class(              CLSID_Com_log_proxy                , Com_log_proxy                    ::Create_instance );
    register_class( spooler_com::CLSID_Task_proxy                   , Com_task_proxy                   ::Create_instance );
    register_class( spooler_com::CLSID_Spooler_proxy                , Com_spooler_proxy                ::Create_instance );
  //register_class( spooler_com::CLSID_Xslt_stylesheet              , Xslt_stylesheet                  ::Create_instance );
}

//------------------------------------------------------------------------------------object_server

int object_server( int argc, char** argv )
{
#   ifdef Z_WINDOWS
        SetConsoleCtrlHandler( NULL, true );    // Wir sind ein Kind-Prozess und Ctrl-C soll ignoriert werden (darum kümmert sich der Hauptprozess, wie unter Unix)
#    else
        ::signal( SIGINT, SIG_IGN );            // Ctrl-C ignorieren
#   endif

    //show_msg("object_server");
    Object_server server;
    int rc = server.main( argc, argv );
    return rc;
}
                   
//-------------------------------------------------------------------------------------------------

} //namespace scheduler

//-----------------------------------------------------------------------redirect_stdout_and_stderr
//#ifdef Z_WINDOWS
//
//static void redirect_stdout_and_stderr() 
//{
//    // -cd sollte aufgerufen sein! Sonst landen die Dateien in c:\windwows\system32
//    #ifdef Z_DEBUG  // Erstmal nur Debug, solange -log-dir nicht schon beim Start bekannt ist.
//        if (windows::Handle stdout_file = CreateFile("stdout.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) {
//            SetStdHandle(STD_OUTPUT_HANDLE, stdout_file);
//            stdout_file.take();
//        }
//        if (windows::Handle stderr_file = CreateFile("stderr.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) {
//            SetStdHandle(STD_ERROR_HANDLE, stderr_file);
//            stderr_file.take();
//        }
//    #endif
//}
//
//#endif
//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv, const string& parameter_line, jobject java_main_context )
{
    Ole_initialize  ole;

    add_message_code_texts( sos::scheduler::scheduler_messages );

#   ifdef Z_DEBUG
        set_log_category_default ( "java.stackTrace"     , true );
#   endif
    set_log_category_default ( "log4j.*"             , true );      // Fürs Loggen aus Java
    set_log_category_default ( "scheduler"           , true );
  //set_log_category_default ( "scheduler.*"         , true );
    set_log_category_explicit( "scheduler.wait"      );
    set_log_category_explicit( "scheduler.loop"      );
    set_log_category_implicit( "scheduler.call"      , true );      // Aufrufe von spooler_process() etc. protokollieren (Beginn und Ende)
    set_log_category_implicit( "scheduler.order"     , false );
    set_log_category_implicit( "scheduler.service"   , true );      // Windows-Dienstesteuerung
  //set_log_category_default ( "scheduler.file_order", true );
  //set_log_category_default ( "scheduler.cluster"   , true );      

    int     ret                = 0;
    bool    is_service         = false;
    bool    is_object_server   = false;
    bool    is_scheduler_client= false;
    bool    kill_pid_file      = false;
    int     kill_pid           = 0;
    string  pid_filename;

#   ifdef Z_WINDOWS
        SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );    // Das System soll sich Messageboxen verkneifen (außer beim Absturz)
        #if defined Z_DEBUG
            windows::create_mini_dump_on_unhandled_exception();
        #endif
#   endif

    Log_ptr::set_demo_version( sos_static_ptr()->_licence->is_demo_version() );
    scheduler::error_settings.read( scheduler::default_factory_ini );

    try
    {
        int     relevant_arg_count  = 0;
        bool    need_call_scheduler = true;
        bool    print_version       = false;
        bool    call_scheduler      = false;
#ifdef Z_WINDOWS
        bool    do_install_service  = false;
        bool    do_remove_service   = false;
        bool    renew_service = false;
        string  service_name, service_display;
        string  service_description = "Job scheduler for process automation";
#endif
        bool    is_backup           = false;
        string  id;
        string  renew_spooler;
        string  command_line;
        string  send_cmd;
        string  log_filename;
        string  factory_ini = scheduler::default_factory_ini;
        string  java_options;
        string  java_classpath;
        string  dependencies;


        for( Sos_option_iterator opt ( argc, argv, parameter_line ); !opt.end(); opt.next() )
        {
            relevant_arg_count++;

#           ifdef Z_WINDOWS
                if( opt.flag( "debug-break"        ) )  assert( !( opt.flag( "debug-break" ) && opt.set() ) );   // Lässt Windows eine Diagbox zeigen
                else
#           endif
            if( opt.with_value( "scheduler" ) )     // Stichwort für scheduler_client
            {
                is_scheduler_client = true;
                break;  // scheduler_client wertet argc und argv erneut aus, deshalb brechen wir hier ab.
            }
            else
            //if( opt.flag      ( "renew-spooler"    ) )  renew_spooler = program_filename();
          //else
            if( opt.with_value( "expand-classpath" ) )  { cout << javabridge::expand_class_path( opt.value() ) << '\n'; need_call_scheduler = false; }
            else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "send-cmd"         ) )  call_scheduler = true,  send_cmd = opt.value();
            else
            if( opt.flag      ( 'O', "object-server" ) )  is_object_server = true;
            else
            if( opt.with_value( "title"            ) )  ;                               // Damit der Aufrufer einen Kommentar für ps übergeben kann (für -object-server)
            else
            if( opt.flag      ( 'V', "version"     ) )  need_call_scheduler = false, print_version = true, fprintf( stderr, "JobScheduler engine %s\n", scheduler::version_string );
            else
            if( opt.flag      ( "?"                )
             || opt.flag      ( "h"                ) )  need_call_scheduler = false, fprintf( stderr, "JobScheduler engine %s\n", scheduler::version_string ), scheduler::print_usage();
            else
            if( opt.flag      ( "kill"             ) )  kill_pid_file = true;
            else
            if( opt.with_value( "kill"             ) )  kill_pid = opt.as_int();
            else
#ifdef Z_WINDOWS
            if( opt.flag      ( "install-service"  ) )  do_install_service = opt.set();
            else
            if( opt.with_value( "install-service"  ) )  do_install_service = true, service_name = opt.value();
            else
            if( opt.flag      ( "remove-service"   ) )  do_remove_service = opt.set();
            else
            if( opt.with_value( "remove-service"   ) )  do_remove_service = true, service_name = opt.value();
            else
            if( opt.flag      ( "renew-service"    ) )  renew_service = opt.set();
            else
            if( opt.with_value( "renew-service"    ) )  renew_service = true, service_name = opt.value();
            else
            if( opt.with_value( "service-name"     ) )  service_name = opt.value();
            else
            if( opt.with_value( "service-display"  ) )  service_display = opt.value();
            else
            if( opt.with_value( "service-descr"    ) )  service_description = opt.value();
            else
            if( opt.with_value( "service"          ) )  call_scheduler = true, is_service = true, service_name = opt.value();
            else
#endif
            if( opt.flag      ( "service"          ) )  call_scheduler = true, is_service = opt.set();
            else
            if( opt.with_value( "need-service"     ) )  dependencies += opt.value(), dependencies += '\0';
            else
            {
                // Hier Optionen, die für install-service relevant sind.
                if( opt.with_value( "sos.ini"          ) )  ;  //schon in sos_main0() geschehen.  set_sos_ini_filename( opt.value() );
                else
                if( opt.with_value( "cd"               ) )  { string dir = opt.value(); if (chdir(dir.c_str())) throw_errno(errno, "chdir", dir.c_str()); }
                else
                if( opt.with_value( "id"               ) )  id = opt.value();
                else
                if( opt.with_value( "ini"              ) )  factory_ini = opt.value(), scheduler::error_settings.read( factory_ini );
                else
                if( opt.with_value( "log"              ) )  log_filename = opt.value();
                else
                if( opt.with_value( "pid-file"         ) )  pid_filename = opt.value();
                else
                if( opt.with_value( "test-env"         ) )  // To allow the JVM test framework to change the process' environment variable LD_LIBRARY_PATH
                {
                    string value = opt.value();
                    size_t eq = value.find( '=' ); if( eq == string::npos )  z::throw_xc( "SCHEDULER-318", value );
                    string name = value.substr( 0, eq );
                    value = value.substr( eq + 1 );
                    set_environment_variable( name, value );
                }
                else
                if( opt.with_value( "env"              ) )  
                {
                    string value = opt.value();
                    size_t eq = value.find( '=' ); if( eq == string::npos )  z::throw_xc( "SCHEDULER-318", value );
                    string name = value.substr( 0, eq );
                    value = value.substr( eq + 1 );
                    if (name == "LD_LIBRARY_PATH" && !scheduler::static_ld_library_path_changed) {
                        scheduler::static_ld_library_path_changed = true;
                        if (const char* value = getenv("LD_LIBRARY_PATH")) {
                            scheduler::static_original_ld_library_path = value;  // This is the original environment variable
                        }
                    }
                    set_environment_variable( name, value );
#                   ifdef Z_HPUX
                        if( name == "LD_PRELOAD" )  scheduler::static_ld_preload = value;
#                   endif
                }
                else
                if( opt.flag      ( "backup"           ) )  is_backup = opt.set();
                else
                if (opt.flag("pause")) {}
                else
                if (opt.with_value("configuration-directory")) {
                    string d = opt.value();
                    if (!d.empty() && !file::File_info(d).is_directory()) z::throw_xc("SCHEDULER-715", d);
                }
                if (opt.with_value("java-options")) java_options = opt.value();
                else
                if (opt.with_value("java-classpath")) java_classpath = opt.value();
                else
                if (opt.with_value("job-java-classpath")) {}
                else
                if (opt.with_value("log-dir")) {}
                else
                  call_scheduler = true;     // Aber is_scheduler_client hat Vorrang!

                if( !command_line.empty() )  command_line += " ";
                command_line += opt.complete_parameter( '"', '"' );
            }
        }

        if( send_cmd != "" )  is_service = false;

        java_options = subst_env(read_profile_string(factory_ini, "java", "options")) + " " + java_options;
        java_classpath = java_classpath + Z_PATH_SEPARATOR + subst_env(read_profile_string(factory_ini, "java", "class_path"));
        
        // scheduler.log
        if( log_filename.empty() )  log_filename = subst_env( read_profile_string( factory_ini, "spooler", "log" ) );
        if( !log_filename.empty()  &&  renew_spooler == "" )  
        {
            size_t pos = log_filename.find( '>' );
            File_path path = pos == string::npos? log_filename : log_filename.substr( pos + 1 );
            string msg = sos::scheduler::Spooler::backup_logfile( path );  // Does not backup for "-log=+scheduler.log" due to '+' in path.
            log_start( log_filename );
            if (!msg.empty()) Z_LOG2("scheduler",msg);
        }
        Z_LOG2( "scheduler", "JobScheduler engine " << scheduler::version_string << "\n" );

        //Z_WINDOWS_ONLY(if (is_service) redirect_stdout_and_stderr();) 

        if( is_scheduler_client )
        {
            start_java(java_options, java_classpath);
            ret = scheduler::scheduler_client_main( argc, argv );
        }
        else
        if( is_object_server )
        {
            start_java(java_options, java_classpath);
            ret = scheduler::object_server( argc, argv );   // Ruft _exit()
        }
        else
        {
            if( kill_pid )
            {
                kill_process_immediately( kill_pid, Z_FUNCTION );
                need_call_scheduler = false;
            }

            if( kill_pid_file )
            {
                int pid = as_int( replace_regex( string_from_file( pid_filename ), "[\r\n]", "" ) ); 

#               ifdef Z_WINDOWS
                    windows::try_kill_process_with_descendants_immediately( pid, (Has_log*)NULL, (Message_string*)NULL, Z_FUNCTION );
#                else
                    if( !posix::try_kill_process_group_immediately( pid, Z_FUNCTION ) )
                        kill_process_immediately( pid, Z_FUNCTION );
#               endif

                need_call_scheduler = false;
            }            

#           ifdef Z_WINDOWS
                if( service_name != "" ) 
                {
                    if( service_display == "" )  service_display = service_name;
                }
                else
                {
                    service_name = scheduler::make_service_name( id, is_backup );
                    if( service_display == "" )  service_display = scheduler::make_service_display( id, is_backup );
                }

                if( !renew_spooler.empty() )  
                { 
                    scheduler::spooler_renew( service_name, renew_spooler, renew_service, command_line ); 
                }
                else
                if( do_remove_service | do_install_service )
                {
                    if( do_remove_service  )  scheduler::remove_service( service_name );
                    if( do_install_service ) 
                    {
                        //if( !is_service )  command_line = "-service " + command_line;
                        command_line = "-service=" + service_name + " " + command_line;
                        dependencies += '\0';
                        scheduler::install_service( service_name, service_display, service_description, dependencies, command_line );
                    }
                }
                else
                if( call_scheduler || need_call_scheduler )
                {
                    start_java(java_options, java_classpath);

                    _beginthread( scheduler::delete_new_spooler, 50000, NULL );

                    if( is_service )
                    {
                        ret = scheduler::spooler_service( service_name, argc, argv );
                    }
                    else
                    {
                        ret = scheduler::spooler_main( argc, argv, parameter_line, java_main_context );
                    }
                }

#            else

                if( call_scheduler || need_call_scheduler )
                {
                    if( is_service )
                    {
                        scheduler::is_daemon = true;

                        Z_LOG2( "scheduler", "JobScheduler becomes daemon, and process ID changes\n");
                        scheduler::be_daemon();
                    }

                    start_java(java_options, java_classpath);

                    ret = scheduler::spooler_main( argc, argv, command_line, java_main_context );
                }

#           endif
            else
            if (print_version) {
                start_java(java_options, java_classpath);
                fprintf(stdout, "JobScheduler engine %s\n", ((string)sos::scheduler::SchedulerJ::buildVersion()).c_str());
            }

        }
    }
    catch( const exception& x )
    {
        Z_LOG2( "scheduler", x.what() << "\n" );
        if( is_service )  scheduler::send_error_email( x, argc, argv, parameter_line );
        if (java_main_context) throw;
        cerr << x << "\n";
        ret = 1;
    }
    catch( const _com_error& x )
    {
        string what = string_from_ole( x.Description() );
        Z_LOG2( "scheduler", what << "\n" );
        if( is_service )  scheduler::send_error_email( zschimmer::Xc( x ), argc, argv, parameter_line );
        if (java_main_context) throw;
        cerr << what << "\n";
        ret = 1;
    }

    if (is_object_server) {
        //#ifdef Z_WINDOWS
        //    //JS-709: Visual Studio 2010, Windows 2003, Dienst unter nicht-SYSTEM-Konto: API-Prozess bleibt bei bloßem return oder exit() hängen.
        //    log_stop();
        //    _flushall();
        //    _exit(ret);  
        //#endif
    } else {
        Z_LOG2( "scheduler", "Executable will be terminated.\n" );
    }

    return ret;
}

//---------------------------------------------------------------------------------------start_java

static ptr<javabridge::Vm> start_java(const string& options, const string& class_path) {
    ptr<javabridge::Vm> java_vm = get_java_vm(false);
    java_vm->set_destroy_vm(false);   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal (auch für Dateityp jdbc), wahrscheinlich wegen Hostware ~Sos_static.
    java_vm->set_options(options);
    java_vm->prepend_class_path(class_path);
    ::sos::scheduler::init_java_vm(java_vm);
    register_native_classes();
    return java_vm;
}

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
#   ifdef Z_HPUX
        if( const char* value = getenv( "LD_PRELOAD" ) )  scheduler::static_ld_preload = value;
        putenv( "LD_PRELOAD=" );
#   endif


    _argc = argc;
    _argv = argv;

    bool has_ld_library_path = false;
    string ld_library_path;
    if (const char* o = getenv("LD_LIBRARY_PATH")) {
        ld_library_path = o;
        has_ld_library_path = true;
    }

    int ret = sos::spooler_main( argc, argv, "", (jobject)NULL );



#   ifdef SCHEDULER_WITH_HOSTJAVA

        // Wegen Hostware ~Sos_static?
        // HP-UX und eingebundenes Hostjava: Irgendein atexit() stürzt in InterlockedIncrement() (AddRef()?") ab.
        // Deshalb beenden wir den Scheduler hier mit _exit(), schließen aber alle Dateien vorher

        Z_LOG2( "scheduler", "_exit(" << ret << ") for Hostjava\n" );

        int n = sysconf( _SC_OPEN_MAX );
        for( int i = 0; i < n; i++ )  ::close(i);

        _exit( ret );   // Kein atexit() wird gerufen, keine Standard-I/O-Puffer werden geschrieben

#   endif

    if (has_ld_library_path) {
        // Restore for multiple calls by integration tests
        set_environment_variable("LD_LIBRARY_PATH", ld_library_path);
    } else {
        #if defined Z_WINDOWS
            set_environment_variable("LD_LIBRARY_PATH", "");
        #else
            unsetenv("LD_LIBRARY_PATH");
        #endif
    }
 
    return ret;
}


} //namespace sos

//-------------------------------------------------------------------------------------------------
