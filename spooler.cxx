// $Id$
// §851: Weitere Log-Ausgaben zum Scheduler-Start eingebaut
// §1479

/*
    Hier sind implementiert

    Script_instance
    Spooler
    spooler_main()
    sos_main()


    Log-Kategorien:

    scheduler.call                  Beginn und Ende der Job-Methoden (spooler_process() etc.)
    scheduler.log                   Logs (Task-Log etc., open, unlink)
    scheduler.order                 payload = 
    scheduler.nothing_done
    scheduler.wait                  Warteaufrufe 
*/

#include "spooler.h"
#include "spooler_version.h"
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



using namespace std;


namespace sos {

extern const Bool _dll = false;

#ifdef Z_WINDOWS
    extern HINSTANCE _hinstance; 
#endif


namespace spooler {


const char*                     default_factory_ini                 = "factory.ini";
const string                    xml_schema_path                     = "scheduler.xsd";
const string                    scheduler_character_encoding        = "ISO-8859-1";     // Eigentlich Windows-1252, aber das ist weniger bekannt und wir sollten die Zeichen 0xA0..0xBF nicht benutzen.
const string                    new_suffix                          = "~new";           // Suffix für den neuen Spooler, der den bisherigen beim Neustart ersetzen soll
const double                    renew_wait_interval                 = 0.25;
const double                    renew_wait_time                     = 30;               // Wartezeit für Brückenspooler, bis der alte Spooler beendet ist und der neue gestartet werden kann.

const int                       before_suspend_wait_time            = 5;                // Diese Zeit vor Suspend auf Ereignis warten (eigentlich so kurz wie möglich)
const int                       inhibit_suspend_wait_time           = 10*60;            // Nur Suspend, wenn Wartezeit länger ist
const int                       show_message_after_seconds          = 15*60;            // Nach dieser Wartezeit eine Meldung ausgeben
const int                       show_message_after_seconds_debug    = 60;               // Nach dieser Wartezeit eine Meldung ausgeben

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
const double                    wait_for_thread_termination         = latter_day;       // Haltbarkeit des Geduldfadens
//const double                    wait_step_for_thread_termination    = 5.0;         // 1. Nörgelabstand
//const double                    wait_step_for_thread_termination2   = 600.0;       // 2. Nörgelabstand
//const double wait_for_thread_termination_after_interrupt = 1.0;

const char*                     temporary_process_class_name        = "(temporaries)";
const int                       no_termination_timeout              = UINT_MAX;
static bool                     is_daemon                           = false;
//static t                      daemon_starter_pid;
//bool                          spooler_is_running      = false;
volatile int                    ctrl_c_pressed                      = 0;
Spooler*                        spooler_ptr                         = NULL;

//-------------------------------------------------------------------------------------------------

extern zschimmer::Message_code_text  scheduler_messages[];            // messages.cxx, generiert aus messages.xml

//-----------------------------------------------------------------------------------Error_settings

struct Error_settings
{
    void read( const string& ini_file )
    {
        _from = read_profile_string( ini_file, "spooler", "log_mail_from", _from );
        _to   = read_profile_string( ini_file, "spooler", "log_mail_to"  , _to   );
        _cc   = read_profile_string( ini_file, "spooler", "log_mail_cc"  , _cc   );
        _bcc  = read_profile_string( ini_file, "spooler", "log_mail_bcc" , _bcc  );
        _smtp = read_profile_string( ini_file, "spooler", "smtp"         , _smtp );
    }

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
        Scheduler_event scheduler_event ( Scheduler_event::evt_scheduler_fatal_error, log_error, spooler );
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

    if( v == "" )  return deflt;

    try
    {
        if( isdigit( (unsigned char)v[0] ) )  return as_int(v);
                                        else  return as_bool(v)? 1 : 0;
    }
    catch( exception& x ) { z::throw_xc( "SCHEDULER-335", profile + " [" + section + "] " + entry, v, x ); }
}

//-----------------------------------------------------------------------------read_profile_archive

Archive_switch read_profile_archive( const string& profile, const string& section, const string& entry, Archive_switch deflt )
{
    string value = read_profile_string( profile, section, entry );

    if( value == "" )  return deflt;
    if( lcase(value) == "gzip" )  return arc_gzip;

    return read_profile_bool( profile, section, entry, false )? arc_yes : arc_no;
}

//----------------------------------------------------------------------------read_profile_with_log

With_log_switch read_profile_with_log( const string& profile, const string& section, const string& entry, With_log_switch deflt )
{
    return read_profile_archive( profile, section, entry, deflt );
}

//-----------------------------------------------------------------------------------ctrl_c_handler
#ifdef Z_WINDOWS

    static BOOL WINAPI ctrl_c_handler( DWORD dwCtrlType )
    {
        if( dwCtrlType == CTRL_C_EVENT )
        {
            ctrl_c_pressed++;

            if( ctrl_c_pressed >= 2 )
            {
                if( ctrl_c_pressed == 2  &&  spooler_ptr )  spooler_ptr->abort_now();  
                return false;
            }

            //Kein Systemaufruf hier! (Aber bei Ctrl-C riskieren wir einen Absturz. Ich will diese Meldung sehen.)
            //fprintf( stderr, "Scheduler wird wegen Ctrl-C beendet ...\n" );
            if( spooler_ptr )  spooler_ptr->async_signal( "Ctrl+C" );
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
                z::log_categories.toggle_all();         // Das kann vom Signal-Handler aufgerufen werden.
                ::signal( sig, ctrl_c_handler );        // Signal wieder zulassen.
                break;
            }

            case SIGINT:        // Ctrl-C
            case SIGTERM:       // Normales kill
            {
                ctrl_c_pressed++;

                if( ctrl_c_pressed >= 2  &&  spooler_ptr )  spooler_ptr->abort_now();  // Unter Linux ist ctrl_c_pressed meisten > 2, weil 
                                                                                       // jeder Thread diese Routine durchlaufen lässt. Lösung: sigaction()


                //Kein Systemaufruf hier! (Aber bei Ctrl-C riskieren wir einen Absturz. Ich will diese Meldung sehen.)
                //if( !is_daemon )  fprintf( stderr, "Scheduler wird wegen kill -%d beendet ...\n", sig );

                // pthread_mutex_lock:
                // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
                // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
                // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.

                if( !is_daemon && spooler_ptr )  spooler_ptr->async_signal( "Ctrl+C" );
                break;
            }

            default: 
                fprintf( stderr, "Unknown signal %d\n", sig );
        }

        set_ctrl_c_handler( ctrl_c_pressed < 2 );
    }

#endif
//-------------------------------------------------------------------------------set_ctrl_c_handler

static void set_ctrl_c_handler( bool on )
{
    //LOG( "set_ctrl_c_handler(" << on << ")\n" );

#   ifdef Z_WINDOWS

        SetConsoleCtrlHandler( ctrl_c_handler, on );

#    else

        ::signal( SIGINT , on? ctrl_c_handler : SIG_DFL );      // Ctrl-C
        ::signal( SIGTERM, on? ctrl_c_handler : SIG_DFL );      // Normales kill 
        ::signal( SIGUSR1, on? ctrl_c_handler : SIG_DFL );      // Log erweitern oder zurücknehmen

#   endif
}

//----------------------------------------------------------------------------------------be_daemon
#ifdef Z_UNIX

static void be_daemon()
{
    LOG( "fork()\n" );

    switch( fork() )
    {
        case  0: LOG( "pid=" << getpid() << "\n" );
                 zschimmer::main_pid = getpid();

                 LOG( "setsid()\n" );
                 setsid(); 

                 if( isatty( fileno(stdin) ) ) 
                 {
                     FILE* f = freopen( "/dev/null", "r", stdin );
                     if( !f )  throw_errno( errno, "/dev/null" );
                 }

                 break;

        case -1: throw_errno( errno, "fork" );

        default: ::sleep(1);  // Falls der Daemon noch was ausgibt, sollte das vor dem Shell-Prompt sein.
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
            string error_line = message_string( "SCHEDULER-256", _spooler->_single_thread->_task_list.size() );  // "Frist zur Beendigung des Schedulers ist abgelaufen, aber $1 Tasks haben sich nicht beendet
            
            _spooler->_log.error( error_line );

            {
                Scheduler_event scheduler_event ( Scheduler_event::evt_scheduler_kills, log_error, _spooler );
                scheduler_event.set_scheduler_terminates( true );

                Mail_defaults mail_defaults( _spooler );
                mail_defaults.set( "subject", error_line );
                mail_defaults.set( "body"   , "The tasks will be killed before the Scheduler terminates" );  // "Die Tasks werden abgebrochen, damit der Scheduler sich beenden kann."

                scheduler_event.send_mail( mail_defaults );
            }

            Z_FOR_EACH( Task_list, _spooler->_single_thread->_task_list, t )
            {
                //_spooler->_log.error( S() << "Kill " << (*t)->obj_name() );

                bool kill_immediately = true;
                (*t)->cmd_end( kill_immediately );      // Wirkt erst beim nächsten Task::do_something()
            }

            //_spooler->kill_all_processes();           Es reicht, wenn die Tasks gekillt werden. Die killen dann ihre abhängigigen Prozesse.

            set_async_next_gmtime( _timeout_at + kill_timeout_1 );

            _state = s_killing_1;
            something_done = true;
            break;
        }

        case s_killing_1:
        {
            int count = _spooler->_single_thread->_task_list.size();
            _spooler->_log.warn( message_string( "SCHEDULER-254", count, kill_timeout_1, kill_timeout_total ) );    // $1 Tasks haben sich nicht beendet trotz kill vor $2. Die $3s lange Nachfrist läuft weiter</title>
            //_spooler->_log.warn( S() << count << " Tasks haben sich nicht beendet trotz kill vor " << kill_timeout_1 << "s."
            //                     " Die " << kill_timeout_total << "s lange Nachfrist läuft weiter" ); 

            Z_FOR_EACH( Task_list, _spooler->_single_thread->_task_list, t )
            {
                _spooler->_log.warn( S() << "    " << (*t)->obj_name() );
            }

            set_async_next_gmtime( _timeout_at + kill_timeout_total );

            _state = s_killing_2;
            something_done = true;
            break;
        }

        case s_killing_2:
        {
            int count = _spooler->_single_thread->_task_list.size();
            _spooler->_log.error( message_string( "SCHEDULER-255", count, kill_timeout_total ) );  // "$1 Tasks haben sich nicht beendet trotz kill vor $2s. Scheduler bricht ab"
            //_spooler->_log.error( S() << count << " Tasks haben sich nicht beendet trotz kill vor " << kill_timeout_total << "s."
            //                                      " Scheduler bricht ab" ); 

            Z_FOR_EACH( Task_list, _spooler->_single_thread->_task_list, t )
            {
                _spooler->_log.error( S() << "    " << (*t)->obj_name() );
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

Spooler::Spooler() 
: 
    Scheduler_object( this, this, Scheduler_object::type_scheduler ),
    _zero_(this+1), 
    _version(VER_PRODUCTVERSION_STR),
    _security(this),
    _communication(this), 
    _base_log(this),
    _log(1),
    _wait_handles(this,&_log),
    _module(this,&_log),
    _log_level( log_info ),
    _factory_ini( default_factory_ini ),
    _mail_defaults(NULL),
    _termination_gmtimeout_at(no_termination_timeout),
    _web_services(this),
    _waitable_timer( "waitable_timer" ),
    _validate_xml(true)
{
    if( spooler_ptr )  throw_xc( "spooler_ptr" );
    spooler_ptr = this;

    if( !SOS_LICENCE( licence_scheduler ) )  throw_xc( "SOS-1000", "Scheduler" );       // Früh prüfen, damit der Fehler auch auftritt, wenn die sos.ini fehlt.

    _pid = getpid();

    _tcp_port = 0;
    _udp_port = 0;
    _priority_max = 1000;       // Ein Wert > 1, denn 1 ist die voreingestelle Priorität der Jobs
            
    _max_threads = 1;

    _com_log     = new Com_log( &_log );
    _com_spooler = new Com_spooler( this );
    _variables   = new Com_variable_set();


    //_dtd.read( dtd_string );
    if( _validate_xml )  _schema.read( xml::Document_ptr( embedded_files.string_from_embedded_file( xml_schema_path ) ) );


#   ifdef __GNUC__
/*
        sigset_t sigset;
    
        sigemptyset( &sigset );
        sigaddset( &sigset, SIGINT  );
        sigaddset( &sigset, SIGTERM );

        fprintf( stderr, "pthread_sigmask\n" );
        int err = pthread_sigmask( SIG_BLOCK, &sigset, NULL );
        if( err )  throw_errno( err, "pthread_sigmask" );
*/
#   endif

#   ifndef Z_WINDOWS
        ::signal( SIGPIPE, SIG_IGN );    // Fuer Linux eigentlich nicht erforderlich, weil com_remote.cxx() SIGPIPE selbst unterdrueckt
#   endif
    

    set_ctrl_c_handler( true );

    _connection_manager = Z_NEW( object_server::Connection_manager );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    spooler_ptr = NULL;
    set_ctrl_c_handler( false );

    if( !_thread_list.empty() )  
    {
        //close_threads();
        _thread_list.clear();
    }

    //_spooler_thread_list.clear();


    _object_set_class_list.clear();

    _security.clear();


    _waitable_timer.close();
    _event.close();
    _wait_handles.close();

    //nicht nötig  Z_FOR_EACH( Job_chain_map, _job_chain_map, it )  it->second->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->set_log( NULL );
}

//------------------------------------------------------------------------------------Spooler::name

string Spooler::name() const
{
    S result;
    
    result << "Scheduler ";
    result << _hostname;
    if( _tcp_port )  result << ":" << _tcp_port;

    if( _spooler_id != "" )  result << " -id=" << _spooler_id;

    return result;
}

//--------------------------------------------------------------------------Spooler::security_level
// Anderer Thread

Security::Level Spooler::security_level( const Ip_address& host )
{
    Security::Level result = Security::seclev_none;

    THREAD_LOCK( _lock )
    {
        result = _security.level( host.as_in_addr() );
    }

    return result;
}

//-----------------------------------------------------------------------Spooler::state_dom_element

xml::Element_ptr Spooler::state_dom_element( const xml::Document_ptr& dom, const Show_what& show )
{
    xml::Element_ptr state_element = dom.createElement( "state" );
 
    state_element.setAttribute( "time"                 , Sos_optional_date_time::now().as_string() );   // Veraltet (<answer> hat time).
    state_element.setAttribute( "id"                   , id() );
    state_element.setAttribute( "spooler_id"           , id() );
    state_element.setAttribute( "spooler_running_since", Sos_optional_date_time( (time_t)start_time() ).as_string() );
    state_element.setAttribute( "state"                , state_name() );
    state_element.setAttribute( "log_file"             , _base_log.filename() );
    state_element.setAttribute( "version"              , VER_PRODUCTVERSION_STR );
    state_element.setAttribute( "pid"                  , _pid );
    state_element.setAttribute( "config_file"          , _config_filename );
    state_element.setAttribute( "host"                 , _hostname );

  //if( _need_db )
  //state_element.setAttribute( "need_db"              , _need_db == need_db_yes? "yes" 
  //                            : "strict" 
  //                                                             : "no" );
    if( _need_db )
    state_element.setAttribute( "need_db"              , _need_db? _wait_endless_for_db_open? "yes" : "strict" 
                                                                 : "no" );

  //if( _wait_endless_for_db_open )
  //state_element.setAttribute( "wait_endless_for_db"  , _wait_endless_for_db_open? "yes" : "no" );

    if( _tcp_port )
    state_element.setAttribute( "tcp_port"             , _tcp_port );

    if( _udp_port )
    state_element.setAttribute( "udp_port"             , _udp_port );

    if( _db )
    {
        THREAD_LOCK( _lock )
        {
            state_element.setAttribute( "db"                   , trim( remove_password( _db->db_name() ) ) );

            if( _db->is_waiting() )
                state_element.setAttribute( "db_waiting", "yes" );

            if( _db->error() != "" )
                state_element.setAttribute( "db_error", trim( _db->error() ) );
        }
    }

    if( _waiting_errno )
    {
        state_element.setAttribute( "waiting_errno"         , _waiting_errno );
        state_element.setAttribute( "waiting_errno_text"    , "ERRNO-" + as_string( _waiting_errno ) + "  " + z_strerror( _waiting_errno ) );
        state_element.setAttribute( "waiting_errno_filename", _waiting_errno_filename );
    }

    double cpu_time = get_cpu_time();
    char buffer [30];
    sprintf( buffer, "%-.3lf", cpu_time ); 
#   ifdef Z_WINDOWS
        state_element.setAttribute( "cpu_time"             , buffer );
#   else
        LOG( "Command_processor::execute_show_state() cpu_time=" << cpu_time << "\n" );
#   endif

    state_element.setAttribute( "loop"                 , _loop_counter );
    state_element.setAttribute( "waits"                , _wait_counter );

    if( _last_wait_until )
    state_element.setAttribute( "wait_until", _last_wait_until.as_string() );

    if( _last_resume_at  &&  _last_resume_at != latter_day )
    state_element.setAttribute( "resume_at", _last_resume_at.as_string() );

#   ifdef Z_UNIX
    {
        // Offene file descriptors ermitteln. Zum Debuggen, weil das Gerücht geht, Dateien würden offen bleiben.
        S s;
        int n = sysconf( _SC_OPEN_MAX );
        for( int fd = 0; fd < n; fd++ )  if( fcntl( fd, F_GETFD ) != -1  ||  errno != EBADF )  s << ' ' << fd;
        state_element.setAttribute( "file_descriptors", s.str().substr( 1 ) );
    }
#   endif


    if( show & show_jobs )  state_element.appendChild( jobs_dom_element( dom, show ) );
                      else  state_element.append_new_comment( "<jobs> suppressed. Use what=\"jobs\"." );

  //state_element.appendChild( execute_show_threads( show ) );
    state_element.appendChild( process_classes_dom_element( dom, show ) );
    state_element.appendChild( job_chains_dom_element( dom, show ) );

    {
        xml::Element_ptr subprocesses_element = dom.createElement( "subprocesses" );
        for( int i = 0; i < NO_OF( _pids ); i++ )
        {
            int pid = _pids[ i ];
            if( pid )
            {
                xml::Element_ptr subprocess_element = dom.createElement( "subprocess" );
                subprocess_element.setAttribute( "pid", pid );

                subprocesses_element.appendChild( subprocess_element );
            }
        }
        
        state_element.appendChild( subprocesses_element );
    }

    state_element.appendChild( _remote_scheduler_register.dom_element( dom, show ) );
    state_element.appendChild( _communication.dom_element( dom, show ) );
    state_element.append_new_text_element( "operations", "\n" + _connection_manager->string_from_operations( "\n" ) + "\n" );
    state_element.appendChild( _web_services.dom_element( dom, show ) );

    return state_element;
}

//------------------------------------------------------------------------Spooler::jobs_dom_element

xml::Element_ptr Spooler::jobs_dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr jobs_element = document.createElement( "jobs" );
    dom_append_nl( jobs_element );

    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;

        if( job->visible()  &&  ( show._job_name == ""  ||  show._job_name == job->name() ) )
        {
            jobs_element.appendChild( job->dom_element( document, show ) ), dom_append_nl( jobs_element );
        }
    }

    return jobs_element;
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_jobs_from_xml( const xml::Element_ptr& element, const Time& xml_mod_time, bool init )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job" ) )
        {
            load_job_from_xml( e, xml_mod_time, init );
        }
    }
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_job_from_xml( const xml::Element_ptr& e, const Time& xml_mod_time, bool init )
{
    string spooler_id = e.getAttribute( "spooler_id" );

    if( _manual? e.getAttribute("name") == _job_name 
               : spooler_id.empty() || spooler_id == id() )
    {
        string job_name = e.getAttribute("name");
        ptr<Job> job = get_job_or_null( job_name );
        if( job )
        {
            job->set_dom( e, xml_mod_time );
            if( init )  init_job( job, true );
        }
        else
        {
            job = Z_NEW( Job( this ) );
            job->set_dom( e, xml_mod_time );
            if( init )
            {
                init_job( job, _jobs_initialized );     // Falls Job im Startskript über execute_xml() hingefügt wird: jetzt noch kein init()!
            }

            add_job( job );
        }
    }
}

//--------------------------------------------------------------------------------Spooler::init_job

void Spooler::init_job( Job* job, bool call_init_too )
{
    try
    {
        job->init0();
        if( call_init_too )  job->init();
    }
    catch( exception& )
    {
        _log.error( message_string( "SCHEDULER-330", job->obj_name() ) );
        throw;
    }
}

//----------------------------------------------------------------------------Spooler::cmd_add_jobs
// Anderer Thread

void Spooler::cmd_add_jobs( const xml::Element_ptr& element )
{
    load_jobs_from_xml( element, Time::now(), true );

    signal( "add_jobs" );
}

//---------------------------------------------------------------------------------Spooler::cmd_job

void Spooler::cmd_job( const xml::Element_ptr& element )
{
    load_job_from_xml( element, Time::now(), _state >= s_starting );

    signal( "add_job" );
}

//----------------------------------------------------------------Spooler::load_job_chains_from_xml

void Spooler::load_job_chains_from_xml( const xml::Element_ptr& element )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job_chain" ) )
        {
            // Siehe auch Command_processor::execute_job_chain()
            ptr<Job_chain> job_chain = new Job_chain( this );
            job_chain->set_dom( e );
            add_job_chain( job_chain );
        }
    }
}

//-------------------------------------------------------------------Spooler::remove_temporary_jobs

int Spooler::remove_temporary_jobs( Job* which_job )
{
    int count = 0;

    THREAD_LOCK( _lock )
    {
        Job_list::iterator it = _job_list.begin();
        while( it != _job_list.end() )
        {
            Job* job = *it;

            if( !which_job  ||  which_job == job )
            {
                if( job->should_removed() )    
                {
                    job->_log->info( message_string( "SCHEDULER-257" ) );   // "Job wird jetzt entfernt"
                    //job->_log->log( job->temporary()? log_debug : log_info, message_string( "SCHEDULER-257" ) );   // "Job wird jetzt entfernt"

                    try
                    {
                        job->close(); 
                    }
                    catch( exception &x )  { _log.warn( x.what() ); }   // Kann das überhaupt passieren?

                    it = _job_list.erase( it );

                    count++;

                    // Bei Auftragsjobs: _single_thread->build_prioritized_order_job_array();
                    continue;
                }
            }

            it++;
        }
    }

    return count;
}

//------------------------------------------------------------------------------Spooler::remove_job

void Spooler::remove_job( Job* job )
{
    job->set_remove( true );
    
    int removed = remove_temporary_jobs( job );
    if( !removed )
    {
        job->_log->debug( message_string( "SCHEDULER-258" ) );  // "Job wird entfernt sobald alle Tasks beendet sind" );
    }
}

//---------------------------------------------------------------------------Spooler::has_any_order

bool Spooler::has_any_order()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH_JOB( j )
        {
            Job* job = *j;
            if( job->order_queue()  &&  !job->order_queue()->empty() )  return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------Spooler::threads_as_xml
// Anderer Thread
/*
xml::Element_ptr Spooler::threads_as_xml( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr threads = document.createElement( "threads" );

    dom_append_nl( threads );

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )
        {
            threads.appendChild( (*it)->dom_element( document, show ) );
            dom_append_nl( threads );
        }
    }

    return threads;
}
*/
//-----------------------------------------------------------Spooler::load_process_classes_from_dom

void Spooler::load_process_classes_from_dom( const xml::Element_ptr& element, const Time& )
{
    if( !process_class_or_null( "" ) )
    {
        add_process_class( Z_NEW( Process_class( this, "" ) ) );
    }

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "process_class" ) )
        {
            string spooler_id = e.getAttribute( "spooler_id" );

            if( spooler_id.empty() || spooler_id == id() )
            {
                string process_class_name = e.getAttribute( "name" );
                ptr<Process_class> process_class = process_class_or_null( process_class_name );
                if( process_class )
                {
                    process_class->set_dom( e );
                }
                else
                {
                    add_process_class( Z_NEW( Process_class( this, e ) ) );
                }
            }
        }
    }
}

//-------------------------------------------------------------Spooler::process_classes_dom_element

xml::Element_ptr Spooler::process_classes_dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr element = document.createElement( "process_classes" );

    FOR_EACH( Process_class_list, _process_class_list, it )
    {
        if( (*it)->_module_use_count > 0 )  element.appendChild( (*it)->dom_element( document, show ) );
    }

    return element;
}

//-------------------------------------------------------------------Spooler::process_class_or_null

Process_class* Spooler::process_class_or_null( const string& name )
{
    FOR_EACH( Process_class_list, _process_class_list, pc )  if( (*pc)->_name == name )  return *pc;
    return NULL;
}

//-------------------------------------------------------------------Spooler::process_class_or_null

Process_class* Spooler::process_class( const string& name )
{
    Process_class* pc = process_class_or_null( name );
    if( !pc )  z::throw_xc( "SCHEDULER-195", name );
    return pc;
}

//-------------------------------------------------------------------Spooler::new_temporary_process

Process* Spooler::new_temporary_process()
{
    ptr<Process> process = Z_NEW( Process( this ) );

    process->set_temporary( true );

    temporary_process_class()->add_process( process );

    return process;
}

//-----------------------------------------------------------------------Spooler::add_process_class

void Spooler::add_process_class( Process_class* process_class )
{
    _process_class_list.push_back( process_class );         
}

//--------------------------------------------------------------------Spooler::init_process_classes
/*
void Spooler::init_process_classes()
{
    //while( _process_list.size() < _process_count_max )  new_process();
}
*/
//---------------------------------------------------------------------Spooler::try_to_free_process

bool Spooler::try_to_free_process( Job* for_job, Process_class* process_class, const Time& now )
{
    return _single_thread->try_to_free_process( for_job, process_class, now );
}

//-----------------------------------------------------------------Spooler::register_process_handle

void Spooler::register_process_handle( Process_handle p )
{
#   ifdef _DEBUG
        for( int i = 0; i < NO_OF( _process_handles ); i++ )
        {
            if( _process_handles[i] == p )  throw_xc( "register_process_handle" );              // Bereits registriert
        }
#   endif

    _process_count++;

    for( int i = 0; i < NO_OF( _process_handles ); i++ )
    {
        if( _process_handles[i] == 0 )  { _process_handles[i] = p;  return; }
    }
}

//---------------------------------------------------------------Spooler::unregister_process_handle

void Spooler::unregister_process_handle( Process_handle p )
{
    if( p )
    {
        _process_count--;

        for( int i = 0; i < NO_OF( _process_handles ); i++ )
        {
            if( _process_handles[i] == p )  { _process_handles[i] = 0;  return; }
        }
    }

#   ifdef _DEBUG
        throw_xc( "unregister_process_handle" );
#   endif
}

//-------------------------------------------------------------------------------Task::register_pid

void Spooler::register_pid( int pid )
{ 
    for( int i = 0; i < NO_OF( _pids ); i++ )
    {
        if( _pids[i] == 0 )  { _pids[i] = pid;  return; }
    }
}

//-----------------------------------------------------------------------------Task::unregister_pid

void Spooler::unregister_pid( int pid )
{ 
    for( int i = 0; i < NO_OF( _pids); i++ )
    {
        if( _pids[i] == pid )  { _pids[i] = 0;  return; }
    }
}

//--------------------------------------------------------------Spooler::wait_until_threads_stopped
/*
void Spooler::wait_until_threads_stopped( Time until )
{
    assert( current_thread_id() == _thread_id );

    return;
    
/ * Threads sind nicht implementiert    

#   ifdef Z_WINDOWS

        Wait_handles wait_handles ( this, &_log );

        Thread_list::iterator it = _thread_list.begin();
        while( it != _thread_list.end() )
        {
            Spooler_thread* thread = *it;
          //if( thread->_free_threading  &&  !thread->_terminated  &&  thread->_thread_handle.valid() )  wait_handles.add( &(*it)->_thread_handle );
            if( !thread->terminated()  &&  thread->_thread_handle.valid() )  wait_handles.add( &(*it)->_thread_handle );
            it++;
        }

        int c = 0;
        while( wait_handles.length() > 0 )
        {
            Time until_step = Time::now() + (++c < 10? wait_step_for_thread_termination : wait_step_for_thread_termination2 );
            if( until_step > until )  until_step = until;

            int index = wait_handles.wait_until( until_step );

            if( ctrl_c_pressed >= 2 )  set_state( s_stopping ),  signal_threads( "ctrl_c" );
            _event.reset();

            if( index >= 0 ) 
            {
                HANDLE h = wait_handles[index];
                FOR_EACH( Thread_list, _thread_list, it )  
                {
                    Spooler_thread* thread = *it;
                    if( thread->thread_handle() == h ) 
                    {
                        _log.info( "Thread " + thread->name() + " beendet" );
                        wait_handles.remove( &thread->_thread_handle );
                    }
                }
            }

            if( Time::now() > until )  break;

            if( index < 0 )
            {
                FOR_EACH( Thread_list, _thread_list, it )  
                {
                    Spooler_thread* thread = *it;

                    if( !thread->terminated() )
                    {
                        string msg = "Warten auf Thread " + thread->name() + " [" + thread->thread_as_text() + "]";
                        Task* task = thread->_current_task;
                        if( task )  msg += ", Task " + task->name() + " state=" + task->state_name();
                        _log.info( msg );
                    }
                }
            }

            if( wait_handles.length() > 0 )  sos_sleep( 0.01 );  // Zur Verkürzung des Protokolls: Nächsten Threads Zeit lassen, sich zu beenden

        }

#   else

        Thread_list threads;

        FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->_free_threading )  threads.push_back( *it );

        int c = 0;
        while( !threads.empty() )
        {
            Time until_step = Time::now() + ( ++c < 10? wait_step_for_thread_termination : wait_step_for_thread_termination2 );
            if( until_step > until )  until_step = until;

            while(1)
            {
                Time now = Time::now();
                if( now > until_step )  break;
                _event.wait( min( 1.0, (double)( until_step - now ) ) );
                if( ctrl_c_pressed >= 2 )  set_state( s_stopping ),  signal_threads( "ctrl_c" );
                _event.reset();
            }

            Thread_list::iterator it = threads.begin();
            while( it != threads.end() )
            {
                Spooler_thread* thread = *it;
                if( thread->terminated() )
                {
                    _log.debug( "Thread " + thread->name() + " sollte gleich beendet sein ..." );
                    thread->thread_wait_for_termination();

                    _log.info( "Thread " + thread->name() + " beendet" );
                    it = threads.erase( it );
                    continue;
                }
                else
                    LOG( "Thread " << thread->name() << " läuft noch\n" );

                it++;
            }

            if( threads.empty() )  break;


            if( Time::now() >= until_step )
            {
                sos_sleep( 0.01 );  // Zur Verkürzung des Protokolls: Nächsten Threads Zeit lassen, sich zu beenden

                FOR_EACH( Thread_list, threads, it )  
                {
                    Spooler_thread* thread = *it;

                    if( thread->thread_is_running() ) 
                    {
                        string msg = "Warten auf Thread " + thread->name() + " [" + thread->thread_as_text() + "]";
                        Job* job = thread->_current_job;
                        if( job )  msg += ", Job " + job->name() + " " + job->job_state();
                        _log.info( msg );
                    }
                }
            }

            if( Time::now() > until )  break;
        }

#   endif
* /
}
*/
//--------------------------------------------------------------------------Spooler::signal_threads
/*
void Spooler::signal_threads( const string& signal_name )
{
    assert( current_thread_id() == _thread_id );

    FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->_free_threading )  (*it)->signal( signal_name );
}
*/
//------------------------------------------------------------------------------Spooler::get_thread
// Anderer Thread
/*
Spooler_thread* Spooler::get_thread( const string& thread_name )
{
    Spooler_thread* thread = get_thread_or_null( thread_name );
    if( !thread )  z::throw_xc( "SCHEDULER-128", thread_name );

    return thread;
}
*/
//----------------------------------------------------------------------Spooler::get_thread_or_null
// Anderer Thread
/*
Spooler_thread* Spooler::get_thread_or_null( const string& thread_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )  if( stricmp( (*it)->name().c_str(), thread_name.c_str() ) == 0 )  return *it;
    }

    return NULL;
}
*/
//--------------------------------------------------------------------Spooler::get_object_set_class
// Anderer Thread
/*
Object_set_class* Spooler::get_object_set_class( const string& name )
{
    Object_set_class* c = get_object_set_class_or_null( name );
    if( !c )  z::throw_xc( "SCHEDULER-101", name );
    return c;
}
*/
//-------------------------------------------------------------Spooler::get_object_set_class_or_null
// Anderer Thread
/*
Object_set_class* Spooler::get_object_set_class_or_null( const string& name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Object_set_class_list, _object_set_class_list, it )
        {
            if( (*it)->_name == name )  return *it;
        }
    }

    return NULL;
}
*/
//---------------------------------------------------------------------------------Spooler::add_job

void Spooler::add_job( const ptr<Job>& job )
{
    THREAD_LOCK( _lock )
    {
        Job* j = get_job_or_null( job->name() );
        if( j )  z::throw_xc( "SCHEDULER-130", j->name() );

        _job_list.push_back( job );
    }
}

//---------------------------------------------------------------------------------Spooler::get_job
// Anderer Thread

Job* Spooler::get_job( const string& job_name, bool can_be_not_initialized )
{
    Job* job = get_job_or_null( job_name );
    if( !job  ||  ( !can_be_not_initialized && !job->state() ) )  z::throw_xc( "SCHEDULER-108", job_name );
    return job;
}

//-------------------------------------------------------------------------Spooler::get_job_or_null
// Anderer Thread

Job* Spooler::get_job_or_null( const string& job_name )
{
    if( job_name == "" )  return NULL;

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            Job* job = *it;
            if( stricmp( job->_name.c_str(), job_name.c_str() ) == 0 )  return job;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------Spooler::get_task
// Anderer Thread

ptr<Task> Spooler::get_task( int task_id )
{
    ptr<Task> task = get_task_or_null( task_id );
    if( !task )  z::throw_xc( "SCHEDULER-215", task_id );
    return task;
}

//------------------------------------------------------------------------Spooler::get_task_or_null
// Anderer Thread

ptr<Task> Spooler::get_task_or_null( int task_id )
{
    if( !_single_thread )  return NULL;
    return _single_thread->get_task_or_null( task_id );
}

//---------------------------------------------------------------------Spooler::thread_by_thread_id

Spooler_thread* Spooler::thread_by_thread_id( Thread_id id )                    
{     
    Thread_id_map::iterator it;

    THREAD_LOCK( _thread_id_map_lock )  it = _thread_id_map.find(id); 

    return it != _thread_id_map.end()? it->second : NULL; 
}

//----------------------------------------------------------------Spooler::select_thread_for_a_task

Spooler_thread* Spooler::select_thread_for_task( Task* )
{
    assert( current_thread_id() == thread_id() );

    if( _single_thread )  return _single_thread;

    throw_xc( "Spooler::select_thread_for_task" );

/*  Mutex _lock wird gesperrt. Nach Task::_lock. Wenn gleichzeitig ein TCP-Kommando kommt, dass in umgekehrter Reihenfolge sperrt, hängt das TCP-Kommando.

    // Kriterien: 
    // - Ein Thread, in dem die wenigsten Tasks desselben Jobs laufen
    // - Ein Thread, in dem die wenigsten Tasks laufen
    // ? Bei einem Java-Job: Ein Thread (Prozess) mit Java Virtual Machine - Das lösen wir über Thread-Klassen
    // - Ein Thread der Thread-Klasse (Prozess-Klasse)

    // Ggfs. Thread neu starten, wenn Maximum noch nicht erreicht.

    Spooler_thread* result = NULL;

    THREAD_LOCK( _lock )
    {
        int min_task_count_for_job = INT_MAX;   // Zahl der Tasks des Jobs für result
        int min_task_count         = INT_MAX;   // Zahl der Tasks aller Jobs für result

        FOR_EACH( Thread_list, _thread_list, t )
        {
            Spooler_thread* thread = *t;
            
            int c = thread->task_count( task->job() );
            if( min_task_count_for_job > c )  min_task_count_for_job = c, result = thread;
            
            c = thread->task_count();
            if( min_task_count > c )  min_task_count = c, result = thread;
        }

        if( _thread_list.size() < _max_threads  &&  min_task_count > 0 )  result = new_thread();
    }

    if( !result )  throw_xc( "select_thread_for_task" );
    return result;
*/
}

//---------------------------------------------------------------------------Spooler::signal_object
// Anderer Thread

void Spooler::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK( _lock )  FOR_EACH( Thread_list, _thread_list, t )  (*t)->signal_object( object_set_class_name, level );
}

//-------------------------------------------------------------------------------Spooler::set_state

void Spooler::set_state( State state )
{
    assert( current_thread_id() == _thread_id );

    if( _state == state )  return;

    _state = state;

    try
    {
        _log.log( state == s_loading || state == s_starting? log_debug3 : log_info, message_string( "SCHEDULER-902", state_name() ) );      // Nach _state = s_stopping aufrufen, damit's nicht blockiert!
    }
    catch( exception& ) {}      // ENOSPC bei s_stopping ignorieren wir

    if( _state_changed_handler )  (*_state_changed_handler)( this, NULL );
}

//------------------------------------------------------------------------------Spooler::state_name

string Spooler::state_name( State state )
{
    switch( state )
    {
        case s_stopped:             return "stopped";
        case s_loading:             return "loading";
        case s_starting:            return "starting";
        case s_running:             return "running";
        case s_paused:              return "paused";
        case s_stopping:            return "stopping";
        case s_stopping_let_run:    return "stopping_let_run";
        default:                    return as_string( (int)state );
    }
}

//-------------------------------------------------------------------------------Spooler::init_jobs

void Spooler::init_jobs()
{
    FOR_EACH_JOB( job )  init_job( *job, true );
    _jobs_initialized = true;
}

//-------------------------------------------------------------------------------Spooler::stop_jobs
/*
void Spooler::stop_jobs()
{
    FOR_EACH_JOB( it ) 
    {
        _current_job = *it;

        if( (*it)->state() != Job::s_stopped )  (*it)->stop( true );

        _current_job = NULL;
    }
}
*/
//------------------------------------------------------------------------------Spooler::close_jobs

void Spooler::close_jobs()
{
    FOR_EACH( Job_list, _job_list, it )
    {
        try
        {
            //if( !*it )  LOG( "Spooler_thread::close1: Ein Job ist NULL\n" );
            (*it)->close();
        }
        catch( const exception&  x ) { _log.error( x.what() ); }
        catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }
    }

    // Jobs erst bei Spooler-Ende freigeben, s. close()
    // Beim Beenden des Spooler noch laufende Threads können auf Jobs von bereits beendeten Threads zugreifen.
    // Damit's nicht knallt: Jobs schließen, aber Objekte halten.
}

//------------------------------------------------------------------------------Spooler::new_thread

Spooler_thread* Spooler::new_thread( bool free_threading )
{
    ptr<Spooler_thread>  thread;

    THREAD_LOCK( _lock )
    {
        thread = Z_NEW( Spooler_thread( this ) );
        thread->set_name( as_string( _thread_list.size() + 1 ) );

        _thread_list.push_back( thread );

        if( free_threading )  
        {
            thread->_free_threading = true;
            thread->thread_start();      // Eigener Thread
            thread->set_thread_termination_event( &_event );        // Signal zu uns, wenn Thread endet
        }
        else
        {
            thread->start( &_event );    // Unser Thread
        }
    }

    return thread;
}

//--------------------------------------------------------------------------------Spooler::send_cmd

void Spooler::send_cmd()
{
    xml::Document_ptr xml_doc;
    int ok = xml_doc.try_load_xml( _send_cmd );      // Haben wir ein gültiges XML-Dokument?
    if( !ok )
    {
        string text = xml_doc.error_text();
        _log.error( text );       // Log ist möglicherweise noch nicht geöffnet
        throw_xc( "XML-ERROR", text );
    }


    SOCKET sock = socket( PF_INET, SOCK_STREAM, 0 );
    if( sock == SOCKET_ERROR ) throw_sos_socket_error( "socket" );

    struct linger l; 
    l.l_onoff  = 1; 
    l.l_linger = 5;  // Sekunden
    setsockopt( sock, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof l );

    sockaddr_in addr;

    memset( &addr, 0, sizeof addr );
    addr.sin_family = PF_INET;
    int i = inet_addr( "127.0.0.1" );
    memcpy( &addr.sin_addr, &i, 4 );                        
    addr.sin_port = htons( _tcp_port );

    int ret = connect( sock, (sockaddr*)&addr, sizeof addr );
    if( ret == -1 )  throw_sos_socket_error( "connect" );

    const char* p     = _send_cmd.data();
    const char* p_end = p + _send_cmd.length();

    while( p < p_end )
    {
        ret = send( sock, p, p_end - p, 0 );
        if( ret == -1 )  throw_sos_socket_error( "send" );

        p += ret;
    }


    Xml_end_finder xml_end_finder;
    bool           last_was_nl = true;

    while(1)
    {
        char buffer [2000];

        int ret = recv( sock, buffer, sizeof buffer, 0 );
        if( ret == 0 )  break;
        if( ret < 0 )  throw_sos_socket_error( "recv" );
        fwrite( buffer, ret, 1, stdout );
        last_was_nl = buffer[ret-1] == '\n';
        if( xml_end_finder.is_complete( buffer, ret ) )  break;
    }

    if( !last_was_nl )  fputc( '\n', stdout );
    fflush( stdout );

    closesocket( sock );
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg()
{
    assert( current_thread_id() == _thread_id );

    for( Sos_option_iterator opt ( _argc, _argv, _parameter_line ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "ini" ) )  _factory_ini = opt.value();
        else
        if( opt.param() ) {}
    }


    string log_level = as_string( _log_level );

    _spooler_id                 =            read_profile_string    ( _factory_ini, "spooler", "id"                 );
    _config_filename            =            read_profile_string    ( _factory_ini, "spooler", "config"             );
    _html_directory             = subst_env( read_profile_string    ( _factory_ini, "spooler", "html_dir"           ) );
    _log_directory              =            read_profile_string    ( _factory_ini, "spooler", "log-dir"            );  // veraltet
    _log_directory              = subst_env( read_profile_string    ( _factory_ini, "spooler", "log_dir"            , _log_directory ) );  _log_directory_as_option_set = !_log_directory.empty();
    _include_path               =            read_profile_string    ( _factory_ini, "spooler", "include-path"       );  // veraltet
    _include_path               = subst_env( read_profile_string    ( _factory_ini, "spooler", "include_path"       , _include_path ) );   _include_path_as_option_set  = !_include_path.empty();
    _spooler_param              =            read_profile_string    ( _factory_ini, "spooler", "param"              );                   _spooler_param_as_option_set = !_spooler_param.empty();
    log_level                   =            read_profile_string    ( _factory_ini, "spooler", "log_level"          , log_level );   
    _tasks_tablename            = ucase(     read_profile_string    ( _factory_ini, "spooler", "db_tasks_table"     , "SCHEDULER_TASKS"   ) );
    _job_history_tablename      = ucase(     read_profile_string    ( _factory_ini, "spooler", "db_history_table"   , "SCHEDULER_HISTORY" ) );
    _job_history_columns        =            read_profile_string    ( _factory_ini, "spooler", "history_columns"    );
    _job_history_yes            =            read_profile_bool      ( _factory_ini, "spooler", "history"            , true );
    _job_history_on_process     =            read_profile_history_on_process( _factory_ini, "spooler", "history_on_process", 0 );
    _job_history_archive        =            read_profile_archive   ( _factory_ini, "spooler", "history_archive"    , arc_no );
    _job_history_with_log       =            read_profile_with_log  ( _factory_ini, "spooler", "history_with_log"   , arc_no );
    _order_history_yes          =            read_profile_bool      ( _factory_ini, "spooler", "order_history"      , true );
    _order_history_with_log     =            read_profile_with_log  ( _factory_ini, "spooler", "order_history_with_log", arc_no );
    _db_name                    =            read_profile_string    ( _factory_ini, "spooler", "db"                 );

    // need_db=yes|no|strict
    string need_db_str          =            read_profile_string    ( _factory_ini, "spooler", "need_db"            , "no"                );
    if( stricmp( need_db_str.c_str(), "strict" ) == 0 )
    {
        _need_db = true; 
        _wait_endless_for_db_open = false;
    }
    else
    {
        try{ _wait_endless_for_db_open = _need_db = as_bool( need_db_str ); }
        catch( const exception& x ) { z::throw_xc( "SCHEDULER-206", need_db_str, x.what() ); }
    }

    _max_db_errors              =            read_profile_int       ( _factory_ini, "spooler", "max_db_errors"         , 5 );
    _order_history_tablename    = ucase(     read_profile_string    ( _factory_ini, "spooler", "db_order_history_table", "SCHEDULER_ORDER_HISTORY" ) );
    _orders_tablename           = ucase(     read_profile_string    ( _factory_ini, "spooler", "db_orders_table"       , "SCHEDULER_ORDERS"    ) );
    _variables_tablename        = ucase(     read_profile_string    ( _factory_ini, "spooler", "db_variables_table"    , "SCHEDULER_VARIABLES" ) );
  //_interactive                = true;     // Kann ohne weiteres true gesetzt werden (aber _is_service setzt es wieder false)


    _mail_on_warning = read_profile_bool           ( _factory_ini, "spooler", "mail_on_warning", _mail_on_warning );
    _mail_on_error   = read_profile_bool           ( _factory_ini, "spooler", "mail_on_error"  , _mail_on_error   );
    _mail_on_process = read_profile_mail_on_process( _factory_ini, "spooler", "mail_on_process", _mail_on_process );
    _mail_on_success = read_profile_bool           ( _factory_ini, "spooler", "mail_on_success", _mail_on_success );
    _mail_encoding   = read_profile_string         ( _factory_ini, "spooler", "mail_encoding"  , "base64"        );      // "quoted-printable": Jmail braucht 1s pro 100KB dafür

    _mail_defaults.set( "queue_dir", subst_env( read_profile_string( _factory_ini, "spooler", "mail_queue_dir"   , "-" ) ) );
    _mail_defaults.set( "smtp"     ,            read_profile_string( _factory_ini, "spooler", "smtp"             , "-" ) );
    _mail_defaults.set( "from"     ,            read_profile_string( _factory_ini, "spooler", "log_mail_from"    ) );
    _mail_defaults.set( "to"       ,            read_profile_string( _factory_ini, "spooler", "log_mail_to"      ) );
    _mail_defaults.set( "cc"       ,            read_profile_string( _factory_ini, "spooler", "log_mail_cc"      ) );
    _mail_defaults.set( "bcc"      ,            read_profile_string( _factory_ini, "spooler", "log_mail_bcc"     ) );
    _mail_defaults.set( "subject"  ,            read_profile_string( _factory_ini, "spooler", "log_mail_subject" ) );

    _log_collect_within = read_profile_uint  ( _factory_ini, "spooler", "log_collect_within", 0 );
    _log_collect_max    = read_profile_uint  ( _factory_ini, "spooler", "log_collect_max"   , 900 );
    _zschimmer_mode     = read_profile_bool  ( _factory_ini, "spooler", "zschimmer", _zschimmer_mode );

    _my_program_filename = _argv? _argv[0] : "(missing program path)";

    //if( !_java_vm->running() )  // Für javac ist's egal, ob Java läuft (für scheduler.dll)
    {
      //_java_vm->set_filename      ( subst_env( read_profile_string( _factory_ini, "java"   , "vm"         , _java_vm->filename()       ) ) );
        _java_vm->prepend_class_path( subst_env( read_profile_string( _factory_ini, "java"   , "class_path" ) ) );
      //_java_vm->set_javac_filename( subst_env( read_profile_string( _factory_ini, "java"   , "javac"      , _java_vm->javac_filename() ) ) );
    }


    try
    {
        for( Sos_option_iterator opt ( _argc, _argv, _parameter_line ); !opt.end(); opt.next() )
        {
            if( opt.with_value( "sos.ini"          ) )  _sos_ini = opt.value();   // wurde in Hostware-main() bearbeitet
            else
            if( opt.flag      ( "V"                ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "log"              ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "i"                ) )  _interactive = opt.set();   // Nur für Joacim
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
             || opt.param(1)                         )  _config_filename = opt.value();
            else
            if( opt.with_value( "cd"               ) )  { string dir = opt.value(); if( chdir( dir.c_str() ) )  throw_errno( errno, "chdir", dir.c_str() ); } //_directory = dir; }
            else
            if( opt.with_value( "id"               ) )  _spooler_id = opt.value();
            else
            if( opt.with_value( "log-dir"          ) )  _log_directory = opt.value(),  _log_directory_as_option_set = true;
            else
            if( opt.with_value( "include-path"     ) )  subst_env( _include_path = opt.value() ),  _include_path_as_option_set = true;
            else
            if( opt.with_value( "param"            ) )  _spooler_param = opt.value(),  _spooler_param_as_option_set = true;
            else
            if( opt.with_value( "log-level"        ) )  log_level = opt.value();
            else
            if( opt.with_value( "job"              ) )  _job_name = opt.value();        // Nicht von SOS beauftragt
            else
            if( opt.with_value( "program-file"     ) )  _my_program_filename = opt.value();        // .../scheduler.exe
            else
            if( opt.with_value( "send-cmd"         ) )  _send_cmd = opt.value();
            else
            if( opt.with_value( "cmd"              ) )  _xml_cmd = opt.value();
            else
            if( opt.with_value( "port"             ) )  _tcp_port = _udp_port = opt.as_int(),  _tcp_port_as_option_set = _udp_port_as_option_set = true;
            else
            if( opt.flag      ( "reuse-port"       ) )  _reuse_port = opt.set();
            else
            if( opt.with_value( "tcp-port"         ) )  _tcp_port = opt.as_int(),  _tcp_port_as_option_set = true;
            else
            if( opt.with_value( "udp-port"         ) )  _udp_port = opt.as_int(),  _udp_port_as_option_set = true;
            else
            if( opt.flag      ( "ignore-process-classes" ) )  _ignore_process_classes = opt.set();
            else
            if( opt.flag      ( "validate-xml"           ) )  _validate_xml = opt.set();
            else
            if( opt.with_value( "env"                    ) )  ;  // Bereits von spooler_main() erledigt
            else
            if( opt.flag      ( 'z', "zschimmer"         ) )  _zschimmer_mode = opt.set();
          //else
          //if( opt.with_value( "now"                    ) )  _clock_difference = Time( Sos_date_time( opt.value() ) ) - Time::now();
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
        if( _temp_dir.empty() )  _temp_dir = z::get_temp_path() + Z_DIR_SEPARATOR "scheduler";
        _temp_dir = replace_regex( _temp_dir, "[\\/]+", Z_DIR_SEPARATOR );
        _temp_dir = replace_regex( _temp_dir, "\\" Z_DIR_SEPARATOR "$", "" );
        if( _spooler_id != "" )  _temp_dir += Z_DIR_SEPARATOR + _spooler_id;

        _manual = !_job_name.empty();
        if( _manual  &&  _log_directory.empty() )  _log_directory = "*stderr";

        _log_level = make_log_level( log_level );

        if( _log_level <= log_debug_spooler )  _debug = true;
        if( _config_filename.empty() )  z::throw_xc( "SCHEDULER-115" );

        if( _html_directory == "" )  _html_directory = directory_of_path( _config_filename ) + "/html";
    }
    catch( exception& )
    {
        if( !_is_service )
        {
            cerr << "\n"
                    "usage: " << _argv[0] << "\n"
                    "       [-config=]XMLFILE\n"
                    "       -validate-xml-\n"
                    "\n"
                    "       -V\n"
                    "       -show-xml-schema\n"
                    "\n"
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

        throw;
    }
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    assert( current_thread_id() == _thread_id );

    _log.init( this );              // Nochmal nach load_argv()
    _log.set_title( "Main log" );

    set_state( s_loading );
    //_log ist noch nicht geöffnet   _log.info( "Spooler::load " + _config_filename );


    tzset();

    _security.clear();             
    _java_vm = get_java_vm( false );
    _java_vm->set_destroy_vm( false );   //  Nicht DestroyJavaVM() rufen, denn das hängt manchmal (auch für Dateityp jdbc)


    load_arg();
    


    if( _pid_filename != ""  &&  !_pid_file.opened() )
    {
        _pid_file.open( _pid_filename, "w" );
        _pid_file.unlink_later();
        _pid_file.print( as_string( getpid() ) );
        _pid_file.print( "\n" );
        _pid_file.flush();
    }

    _log.open();

    char hostname[200];  // Nach _communication.init() und nach _prefix_log.init()!
    if( gethostname( hostname, sizeof hostname ) == SOCKET_ERROR )  hostname[0] = '\0',  _log.warn( "gethostname(): " + z_strerror( errno ) );
    _hostname = hostname;


    // Die erste Prozessklasse ist die Klasse für temporäre Prozesse
    ptr<Process_class> process_class = Z_NEW( Process_class( this, temporary_process_class_name ) );
    _process_class_list.push_back( process_class );         


    _web_services.init();    // Ein Job und eine Jobkette einrichten, s. spooler_web_service.cxx


    Command_processor cp ( this, Security::seclev_all );
    _executing_command = false;             // Command_processor() hat es true gesetzt, aber noch läuft der Scheduler nicht. 
                                            // spooler_history.cxx verweigert das Warten auf die Datenbank, wenn _executing_command gesetzt ist,
                                            // damit der Scheduler nicht in einem TCP-Kommando blockiert.

    cp.execute_file( _config_filename );


#   ifdef Z_WINDOWS
        if( _zschimmer_mode )
        {
            _waitable_timer.set_handle( CreateWaitableTimer( NULL, FALSE, NULL ) );
            if( !_waitable_timer )  z::throw_mswin( "CreateWaitableTimer" );

            _waitable_timer.add_to( &_wait_handles );


            set_next_daylight_saving_transition();
        }
#   endif
}

//-----------------------------------------------------Spooler::set_next_daylight_saving_transition
#ifdef Z_WINDOWS

static Time time_from_dst_systemtime( const SYSTEMTIME& dst, const SYSTEMTIME& now )
{
    if( dst.wYear != 0  ||  dst.wMonth == 0  ||  dst.wDay == 0 )  return 0;


    SYSTEMTIME result;
    SYSTEMTIME last_day; 
    BOOL       ok;
    
    memset( &result, 0, sizeof result );
    result.wYear = now.wYear;

    while(1)
    {
        result.wMonth        = dst.wMonth;
        result.wDay          = 1;
        result.wHour         = dst.wHour;
        result.wMinute       = dst.wMinute;
        result.wMilliseconds = dst.wMilliseconds;


        // result.wDayOfWeek errechnen

        int64 filetime;  // 100 Nanosekunden
        ok = SystemTimeToFileTime( &result, (FILETIME*)&filetime );
        if( !ok )  return Time(0);

        ok = FileTimeToSystemTime( (FILETIME*)&filetime, &result );   // Jetzt haben wir result.wDayOfWeek
        if( !ok )  return Time(0);


        // Letzten Tag des Monats bestimmen

        last_day = result;
        if( ++last_day.wMonth > 12 )  last_day.wMonth = 1, last_day.wYear++;

        ok = SystemTimeToFileTime( &last_day, (FILETIME*)&filetime );
        if( !ok )  return Time(0);

        filetime -= 24*3600*10000000LL;

        ok = FileTimeToSystemTime( (FILETIME*)&filetime, &last_day );  
        if( !ok )  return Time(0);


        // Wochentag setzen

        result.wDay += ( dst.wDayOfWeek - result.wDayOfWeek + 7 ) % 7;
        result.wDayOfWeek = dst.wDayOfWeek;


        // Woche setzen (dst.wDay gibt an, der wievielte Wochentag des Monats es ist)

        result.wDay += 7 * ( dst.wDay - 1 );


        // Aber nicht über den Monat hinaus

        while( result.wDay > last_day.wDay )  result.wDay -= 7;

        if( windows::compare_systemtime( now, result ) < 0 )  break;

        result.wYear++;  // Nächstes Jahr probieren
        assert( result.wYear <= now.wYear + 1 );
    }

    return Time( result );
}

//-----------------------------------------------------Spooler::set_next_daylight_saving_transition

void Spooler::set_next_daylight_saving_transition()
{
    if( _zschimmer_mode )  
    {
        TIME_ZONE_INFORMATION time_zone_information;
        DWORD result = GetTimeZoneInformation( &time_zone_information );
        if( result != TIME_ZONE_ID_INVALID )
        {
            SYSTEMTIME now;
            GetLocalTime( &now );

            if( time_zone_information.StandardDate.wMonth )
            {
                Time standard_date = time_from_dst_systemtime( time_zone_information.StandardDate, now );
                Time daylight_date = time_from_dst_systemtime( time_zone_information.DaylightDate, now );
                
                if( standard_date < daylight_date )
                {
                    _next_daylight_saving_transition_time = standard_date;  // Kann 0 sein
                    _next_daylight_saving_transition_name = "begin of standard time: " + string_from_ole( time_zone_information.StandardName );
                }
                else
                {
                    _next_daylight_saving_transition_time = daylight_date;  // Kann 0 sein
                    _next_daylight_saving_transition_name = "begin of daylight saving time: " + string_from_ole( time_zone_information.DaylightName );
                }
            }
            else
            {
                _next_daylight_saving_transition_time = latter_day;
                _next_daylight_saving_transition_name = "no daylight saving";
            }
        }
    }
}

#endif
//----------------------------------------------------------------------------Spooler::create_window
/*
#if defined Z_WINDOWS && defined Z_DEBUG
    
    static LRESULT CALLBACK window_callback( HWND, UINT, WPARAM, LPARAM )
    {
        return 0;
    }

#endif
*/

void Spooler::create_window()
{
#   if defined Z_WINDOWS

        string title = S() << name() << " " << _config_filename;
        SetConsoleTitle( title.c_str() );
/*
        const char* window_class_name = "Scheduler";
        WNDCLASSEX window_class;
 
        memset( &window_class, 0, sizeof window_class );
        window_class.cbSize        = sizeof window_class;
      //window_class.style         = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc   = window_callback;
        window_class.hInstance     = _hinstance;
        window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
        window_class.lpszClassName = window_class_name;
 
        RegisterClassEx( &window_class ); 


        HWND window = CreateWindow
        ( 
            window_class_name,
            name().c_str(),      // title-bar string 
            WS_OVERLAPPEDWINDOW, // top-level window 
            CW_USEDEFAULT,       // default horizontal position 
            CW_USEDEFAULT,       // default vertical position 
            CW_USEDEFAULT,       // default width 
            CW_USEDEFAULT,       // default height 
            (HWND)NULL,          // no owner window 
            (HMENU)NULL,         // use class menu 
            _hinstance,          // handle to application instance 
            NULL                 // no window-creation data 
        );

        ShowWindow( window, SW_SHOW ); 
*/
#   endif
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    assert( current_thread_id() == _thread_id );

    _state_cmd = sc_none;
    set_state( s_starting );

    _base_log.set_directory( _log_directory );
    _base_log.open_new();
    
    //_log.info( "Scheduler " + _version + " startet mit " + _config_filename + ", pid=" + as_string( getpid() ) );
    _log.info( message_string( "SCHEDULER-900", _version, _config_filename, getpid() ) );
    _spooler_start_time = Time::now();

    _web_services.load();   // Nicht in Spooler::load(), denn es öffnet schon -log-dir-Dateien (das ist nicht gut für -send-cmd=)

    FOR_EACH_JOB( job )  init_job( *job, false );

    try
    {
        _java_vm->set_log( &_log );
        _java_vm->prepend_class_path( _config_java_class_path );        // Nicht so gut hier. Bei jedem Reload wird der Pfad verlängert. Aber Reload lässt Java sowieso nicht neu starten.
        _java_vm->set_options( _config_java_options );

        if( _has_java_source )
        {
            string java_work_dir = temp_dir() + Z_DIR_SEPARATOR "java";
            _java_vm->set_work_dir( java_work_dir );
            _java_vm->prepend_class_path( java_work_dir );
        }

        if( _has_java )     // Nur True, wenn Java-Job nicht in separatem Prozess ausgeführt wird.
        {
            Java_module_instance::init_java_vm( _java_vm );
        }
    }
    catch( const exception& x )
    {
        _log.error( x.what() );
        _log.warn( message_string( "SCHEDULER-259" ) );  // "Java kann nicht gestartet werden. Scheduler startet ohne Java."
    }


    THREAD_LOCK( _lock )
    {
        if( _need_db  && _db_name.empty() )  z::throw_xc( "SCHEDULER-205" );

        _db = Z_NEW( Spooler_db( this ) );
        _db->open( _db_name );
    }

    _db->spooler_start();

    set_ctrl_c_handler( false );
    set_ctrl_c_handler( true );       // Falls Java (über Dateityp jdbc) gestartet worden ist und den Signal-Handler verändert hat

    // Thread _communication nach Java starten (auch implizit durch _db). Java muss laufen, wenn der Thread startet! (Damit attach_thread() greift)
    if( !_manual )  _communication.start_or_rebind();

    FOR_EACH( Job_chain_map, _job_chain_map, it ) 
    {
        Job_chain* job_chain = it->second;
        if( job_chain->_load_orders_from_database )
            job_chain->load_orders_from_database();  // Die Jobketten aus der XML-Konfiguration
    }

  //_spooler_thread_list.clear();

    FOR_EACH( Thread_list, _thread_list, it )
    {
        Spooler_thread* thread = *it;
      //if( !thread->_free_threading )  _spooler_thread_list.push_back( thread );
        thread->init();
    }


  //init_process_classes();
  //start_threads();

    init_jobs();

/*
    if( _is_service || is_daemon )
    {
        if( _log.fd() > 2 )   // Nicht -1, stdin, stdout, stderr?
        {
            FILE* new_stderr = fdopen( _log.fd(), "w" );
            if( !new_stderr )  throw_errno( errno, "fdopen stderr" );
            {
                _log.info( "stdout und stderr werden in diese Protokolldatei geleitet" ); 
                fclose( stderr ); stderr = new_stderr;
                fclose( stdout ); stdout = fdopen( _log.fd(), "w" );
            }
            //else  
            //    _log.error( string("stderr = fdopen(log): ") + strerror(errno) );
        }
    }
*/
}

//--------------------------------------------------------------------Spooler::run_scheduler_script

void Spooler::run_scheduler_script()
{
    try
    {
        if( _module.set() )
        {
            LOGI( "Startskript wird geladen und gestartet\n" );
        
            _module_instance = _module.create_instance();
          //_module_instance->_title = "Scheduler-Script";
            _module_instance->init();

            _module_instance->add_obj( (IDispatch*)_com_spooler, "spooler"     );
            _module_instance->add_obj( (IDispatch*)_com_log    , "spooler_log" );

            _module_instance->load();
            _module_instance->start();


            bool ok = check_result( _module_instance->call_if_exists( spooler_init_name ) );

            if( _log.highest_level() >= log_warn ) // &&  _log.mail_to() != ""  &&  _log.mail_from() != "" )
            {
                try
                {
                    string subject = name_of_log_level( _log.highest_level() ) + ": " + _log.highest_msg();
                    S      body;

                    body << Sos_optional_date_time::now().as_string() << "  " << name() << "\n\n";
                    body << "Scheduler started with ";
                    body << ( _log.highest_level() == log_warn? "warning" : "error" ) << ":\n\n";
                    body << subject << "\n\n";

                    Scheduler_event scheduler_event ( Scheduler_event::evt_scheduler_started, _log.highest_level(), this );

                    if( _log.highest_level() >= log_error )  scheduler_event.set_error( Xc( "SCHEDULER-227", _log.last( _log.highest_level() ).c_str() ) );
                    else
                    if( _log.highest_level() == log_warn )   scheduler_event.set_message( _log.last( log_warn ) );
                    

                    _log.set_mail_default( "subject"  , subject );
                    _log.set_mail_default( "body"     , body );
                    _log.send( -1, &scheduler_event );
                }
                catch( exception& x )  { _log.warn( S() << "Error on sending mail: " << x.what() ); }
            }

            if( !ok )  z::throw_xc( "SCHEDULER-183" );

            LOG( "Startskript ist gelaufen\n" );
        }
    }
    catch( exception& )
    {
        _log.error( message_string( "SCHEDULER-332" ) );
        throw;
    }
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop( const exception* )
{
    assert( current_thread_id() == _thread_id );

    //set_state( _state_cmd == sc_let_run_terminate_and_restart? s_stopping_let_run : s_stopping );

    //_log.msg( "Spooler::stop" );

    if( _module_instance )      // Scheduler-Skript zuerst beenden, damit die Finalizer die Tasks (von Job.start()) und andere Objekte schließen können.
    {
        Z_LOG2( "scheduler", "Scheduler-Skript wird beendet ...\n" );

        try
        {
            _module_instance->call_if_exists( spooler_exit_name );
        }
        catch( exception& x )  { _log.warn( message_string( "SCHEDULER-260", x.what() ) ); }  // "Scheduler-Skript spooler_exit(): $1"

        _module_instance->close();

        Z_LOG2( "scheduler", "Scheduler-Skript ist beendet.\n" );
    }


    for( Job_chain_map::iterator j = _job_chain_map.begin(); j != _job_chain_map.end(); j++ )  //j = _job_chain_map.erase( j ) )
    {
        j->second->close(); 
    }
    _job_chain_map.clear();

    //close_threads();
    close_jobs();

    if( _shutdown_ignore_running_tasks )  _spooler->kill_all_processes();   // Übriggebliebene Prozesse killen


    _object_set_class_list.clear();
  //_spooler_thread_list.clear();
    _thread_list.clear();
    _job_list.clear();
    _process_class_list.clear();

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wird Java stehen und schließen es erst am Schluss

    if( _main_scheduler_connection )
    {
        //_main_scheduler_connection->logoff( x );
    }

    _communication.close( 5.0 );      // Mit Wartezeit. Vor Restart, damit offene Verbindungen nicht vererbt werden.

    _db->spooler_stop();

    THREAD_LOCK( _lock )
    {
        _db->close();
        _db = NULL;
    }

    set_state( s_stopped );     
    // Der Dienst ist hier beendet

    if( _shutdown_cmd == sc_terminate_and_restart 
     || _shutdown_cmd == sc_let_run_terminate_and_restart )  
    {
        sleep( 5.0 + 1.0 );   // Etwas warten, damit der Browser nicht hängenbleibt. Sonst wird die HTTP-Verbindung nicht richtig geschlossen. Warum bloß?
                              // Siehe auch Spooler_communication::close(): set_linger( true, 1 );
        spooler_restart( &_base_log, _is_service );
    }
}

//----------------------------------------------------------------------------Spooler::nichts_getan

void Spooler::nichts_getan( int anzahl, const string& str )
{
    if( anzahl == 1  
     || anzahl >= scheduler_261_second &&  ( anzahl - scheduler_261_second ) % scheduler_261_intervall == 0 )
    {
        S tasks;
        S jobs;

        FOR_EACH( Task_list, _single_thread->_task_list, t )
        {
            Task* task = *t;
            if( tasks.length() > 0 )  tasks << ", ";
            tasks << task->obj_name() << " " << task->state_name();
            Time next_time = task->next_time();
            if( next_time < latter_day )  tasks << " until " << next_time;
        }
        if( tasks.length() == 0 )  tasks << "no tasks";

        FOR_EACH( Job_list, _job_list, j )  
        {
            Job* job = *j;
            if( jobs.length() > 0 )  jobs << ", ";
            jobs << job->obj_name() << " " << job->state_name();
            Time next_time = job->next_time();
            if( next_time < latter_day )  jobs << " until " << next_time; 
        }
        if( jobs.length() == 0 )  jobs << "no jobs";

        _log.warn( message_string( "SCHEDULER-261", str, _connection_manager->string_from_operations(), tasks, jobs ) );  // "Nichts getan, state=$1, _wait_handles=$2"

        // Wenn's ein System-Ereignis ist, das, jedenfalls unter Windows, immer wieder signalisiert wird,
        // dann kommen die anderen Ereignisse, insbesondere der TCP-Verbindungen, nicht zum Zuge.
        // Gut wäre, in einer Schleife alle Ereignisse zu prüfen und dann Event::_signaled zu setzen.
        // Aber das ganze sollte nicht vorkommen.
    }

    double t = 1;
    Z_LOG2( "scheduler", "sleep(" << t << ")...\n" );
    sos_sleep( t );
}

//-----------------------------------------------------------------------Spooler::execute_state_cmd

void Spooler::execute_state_cmd()
{
    if( _state_cmd )
    {
        if( _state_cmd == sc_pause )     if( _state == s_running )  set_state( s_paused  ); //, signal_threads( "pause" );
        if( _state_cmd == sc_continue )  if( _state == s_paused  )  set_state( s_running ); //, signal_threads( "continue" );

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
                    Z_FOR_EACH( Job_list, _job_list, j )
                    {
                        Job* job = *j;
                        //_log.info( message_string( "SCHEDULER-903", job->obj_name() ) );        // "Stopping"
                        bool end_all_tasks = true;
                        job->stop( end_all_tasks );
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

                if( _termination_gmtimeout_at != no_termination_timeout ) 
                {
                    _log.info( message_string( "SCHEDULER-904", ( _termination_gmtimeout_at - ::time(NULL) ) ) );
                    //_log.info( S() << "Die Frist zum Beenden der Tasks endet in " << ( _termination_gmtimeout_at - ::time(NULL) ) << "s" );
                    _termination_async_operation = Z_NEW( Termination_async_operation( this, _termination_gmtimeout_at ) );
                    _termination_async_operation->set_async_manager( _connection_manager );
                    _connection_manager->add_operation( _termination_async_operation );
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
    FOR_EACH( Task_list, _single_thread->_task_list, t )
    {
        Task* task = *t;

        if( task->state() == Task::s_running_waiting_for_order ) 
        {
            //_log.info( S() << "end " << task->obj_name() );
            task->cmd_end();
        }
    }
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    set_state( s_running );

#   ifdef Z_WINDOWS
        _print_time_every_second = log_directory() == "*stderr"  &&  isatty( fileno( stderr ) );
#   endif

    if( !_xml_cmd.empty() )
    {
        Command_processor cp ( this, Security::seclev_all );
        cout << cp.execute( _xml_cmd, Time::now(), true );                 // Bei einem Fehler Abbruch
        _xml_cmd = "";
    }

    _single_thread = _max_threads == 1? new_thread( false ) : NULL;


    int     nothing_done_count   = 0;
    int     nichts_getan_zaehler = 0;
    int     log_wait_id          = 0;
    bool    log_wait_0           = false;
    string  catched_event_string;

    //---------------------------------------------------------------------------------------------

    while(1)  // Die große Haupt-Schleife
    {
        bool    log_wait          = _print_time_every_second || log_categories.update_flag_if_modified( "scheduler.wait", &log_wait_0, &log_wait_id );
        Time    wait_until        = latter_day;
        Object* wait_until_object = NULL;    
        Time    resume_at         = latter_day;
        Object* resume_at_object  = NULL;

        _loop_counter++;

        if( _thread_list.size() > 0 )       // Beim Start gibt es noch keinen Thread.
        {
            bool valid_thread = false;
            FOR_EACH( Thread_list, _thread_list, it )  valid_thread |= !(*it)->terminated();
            if( !valid_thread )  { _log.info( "Kein Thread vorhanden. Der Scheduler wird beendet." ); break; }
        }

        //---------------------------------------------------------------------------------CONTINUE
        // Hier werden die asynchronen Operationen fortgesetzt, die eigentliche Scheduler-Arbeit

        _event.reset();

        execute_state_cmd();
        if( _shutdown_cmd )  if( !_single_thread  ||  !_single_thread->has_tasks()  ||  _shutdown_ignore_running_tasks )  break;

        bool something_done = run_continue();
        if( _single_thread->is_ready_for_termination() )  break;

        if( something_done )  wait_until = 0;   // Nicht warten, wir drehen noch eine Runde

        //----------------------------------------------------------------------------NICHTS GETAN?

        int nothing_done_max = 2;   // Ein überflüssiges Signal wird toleriert wegen Race condition, und dann gibt es manch voreiliges Signal (vor allem zu Beginn einer Operation)
//      int nothing_done_max = _job_list.size() * 2 + _single_thread->task_count() * 3 + 6;  // Seien wir großzügig

        if( something_done )
        {
            Z_DEBUG_ONLY( if( nichts_getan_zaehler )  Z_LOG2( "scheduler", "nichts_getan_zaehler=" << nichts_getan_zaehler << "\n" ); )
            nothing_done_count = 0;
            nichts_getan_zaehler = 0;
        }
        else
        if( ++nothing_done_count > nothing_done_max )
        {
            nichts_getan( ++nichts_getan_zaehler, catched_event_string );
            if( wait_until == 0 )  wait_until = Time::now() + 1;
        }
        else
        {
            Z_DEBUG_ONLY( Z_LOG2( "scheduler.nothing_done", "nothing_done_count=" << nothing_done_count << " nichts_getan_zaehler=" << nichts_getan_zaehler << "\n" ); )
        }

        //----------------------------------------------------------------------WARTEZEIT ERMITTELN

        if( _state != Spooler::s_paused )
        {
            // NÄCHSTE (JETZT NOCH WARTENDE) TASK ERMITTELN

            if( wait_until > 0 )
            {
                FOR_EACH( Task_list, _single_thread->_task_list, t )
                {
                    Task* task = *t;
                    Time  task_next_time = task->next_time();

                    if( task->job()->is_machine_resumable()  &&  resume_at > task_next_time )  resume_at = task_next_time,  resume_at_object = task;

                    if( wait_until > task_next_time )
                    {
                        wait_until = task_next_time; 
                        wait_until_object = task;
                        if( wait_until == 0 )  break;
                    }
                }
            }


            // NÄCHSTEN JOB ERMITTELN, ALSO DEN NÄCHSTEN TASK-START ODER PERIODEN-ENDE

            if( wait_until > 0 )
            {
                FOR_EACH_JOB( it )
                {
                    Job* job = *it;

                    Time next_job_time = job->next_time();

                    if( job->is_machine_resumable()  &&  resume_at > next_job_time )  resume_at = next_job_time,  resume_at_object = job;

                    if( wait_until > next_job_time ) 
                    {
                        wait_until = next_job_time;
                        wait_until_object = job;
                        if( wait_until == 0 )  break;
                    }
                }
            }
        }


      //if( !something_done  &&  wait_until > 0  &&  _state_cmd == sc_none  &&  wait_until > Time::now() )   Immer wait() rufen, damit Event.signaled() gesetzt wird!
        {
            Wait_handles wait_handles ( this, &_log );
            
            if( _state != Spooler::s_paused )
            {
                // PROCESS-HANDLES EINSAMMELN
    
#               ifndef Z_WINDOWS
                    if( wait_until > 0 )
#               endif 
                FOR_EACH( Process_class_list, _process_class_list, pc )
                {
                    FOR_EACH( Process_list, (*pc)->_process_list, p )
                    {
#                       ifdef Z_WINDOWS

                            object_server::Connection_to_own_server* server = dynamic_cast<object_server::Connection_to_own_server*>( +(*p)->_connection );
                            if( server  &&  server->_process_handle )  wait_handles.add( &server->_process_handle );        // Signalisiert Prozessende

#                        else

                            double t = (*p)->async_next_gmtime();
                            Time next_time = Time( t == 0? 0 : localtime_from_gmtime( t ) );
                            //Z_LOG( **p << "->async_next_gmtime() => " << next_time << "\n" );
                            if( next_time < wait_until )
                            {
                                wait_until = next_time;
                                wait_until_object = *p;
                                if( wait_until == 0 )  break;
                            }

#                       endif
                    }
 
                    if( wait_until == 0 )  break;
                }
            }
            

            // TCP- und UDP-HANDLES EINSAMMELN, für spooler_communication.cxx

            vector<System_event*> events;
            _connection_manager->get_events( &events );
            FOR_EACH( vector<System_event*>, events, e )  wait_handles.add( *e );


            // Termination_async_operation etc.

            if( wait_until > 0 )
            {
                if( ptr<Async_operation> operation = _connection_manager->async_next_operation() )
                {
                    Time next_time = Time( localtime_from_gmtime( operation->async_next_gmtime() ) );
                    //Z_LOG( **p << "->async_next_gmtime() => " << next_time << "\n" );
                    if( next_time < wait_until )
                    {
                        wait_until = next_time;
                        wait_until_object = operation;
                    }
                }
            }

            //-------------------------------------------------------------------------------WARTEN

            wait_handles += _wait_handles;

            if( nothing_done_count > 0  ||  !wait_handles.signaled() )   // Wenn "nichts_getan" (das ist schlecht), dann wenigstens alle Ereignisse abfragen, damit z.B. ein TCP-Verbindungsaufbau erkannt wird.
            {
                if( wait_until == 0 )
                {
                    wait_handles.wait_until( 0, wait_until_object, 0, NULL );   // Signale checken
                }
                else
                {
                    bool signaled = false;

                    _wait_counter++;
                    _last_wait_until = wait_until;
                    _last_resume_at  = resume_at;

                    if( _zschimmer_mode  &&  _should_suspend_machine  &&  is_machine_suspendable() )  // &&  !_single_thread->has_tasks() )
                    {
#                       ifdef Z_WINDOWS
                            if( !IsSystemResumeAutomatic() )  _should_suspend_machine = false;  // Rechner ist nicht automatisch gestartet, sondern durch Benutzer? Dann kein Suspend

                            if( _should_suspend_machine )
                            {
                                Time now = Time::now();
                                if( now + inhibit_suspend_wait_time < resume_at )
                                {
                                    signaled = wait_handles.wait_until( min( now + before_suspend_wait_time, wait_until ), wait_until_object, resume_at, resume_at_object );
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
#                       endif
                    }


                    if( !signaled  &&  !_print_time_every_second )  //!string_begins_with( _log.last_line(), "SCHEDULER-972" ) )
                    {
                        Time first_wait_until = _base_log.last_time() + ( _log.log_level() <= log_debug3? show_message_after_seconds_debug : show_message_after_seconds );
                        if( first_wait_until < wait_until )
                        {
                            string msg = message_string( "SCHEDULER-972", wait_until.as_string(), wait_until_object );
                            if( msg != _log.last_line() ) 
                            {
                                String_object o ( msg );
                                signaled = wait_handles.wait_until( first_wait_until, &o, resume_at, resume_at_object );
                                if( !signaled  &&  msg != _log.last_line() )  _log.info( msg );
                            }
                        }
                    }

                    if( !signaled )
                    {
                        wait_handles.wait_until( wait_until, wait_until_object, resume_at, resume_at_object );
                    }

                    //vielleicht: if( Time::now() - time_before_wait >= 0.001 )  nothing_done_count = 0, nichts_getan_zaehler = 0;
                }
            }

            catched_event_string = "";
            if( wait_handles._catched_event )
            {
                catched_event_string = wait_handles._catched_event->name();
                if( wait_handles._catched_event->_signal_name != "" )
                {
                    catched_event_string += " (";
                    catched_event_string += wait_handles._catched_event->_signal_name;
                    catched_event_string += ')';
                }
            }

            if( log_wait )  Z_LOG2( "scheduler.loop", "-------------scheduler loop " << _loop_counter << "-------------> " << catched_event_string << "\n" );  

            wait_handles.clear();
        }

        //-----------------------------------------------------------------------------------CTRL-C

        run_check_ctrl_c();
    }
}

//----------------------------------------------------------------------------Spooler::run_continue

bool Spooler::run_continue()
{
    bool something_done = false;

    if( _state != Spooler::s_paused )
    {
        // PROZESSE FORTSETZEN

        FOR_EACH( Process_class_list, _process_class_list, pc )
        {
            FOR_EACH( Process_list, (*pc)->_process_list, p )
            {
                something_done |= (*p)->async_continue();
            }
        }

        // TASKS FORTSETZEN

        something_done |= _single_thread->process();    
    }

    if( something_done )  _last_wait_until = 0, _last_resume_at = 0;


    // TCP- UND UDP-VERBINDUNGEN IN SPOOLER_COMMUNICATION.CXX FORTSETZEN
    something_done |= _connection_manager->async_continue();

    return something_done;
}

//------------------------------------------------------------------------Spooler::run_check_ctrl_c

void Spooler::run_check_ctrl_c()
{
    if( ctrl_c_pressed )
    {
        if( _state != s_stopping )
        {
            _log.warn( message_string( "SCHEDULER-262" ) );   // "Abbruch-Signal (Ctrl-C) empfangen. Der Scheduler wird beendet.\n" );
            cmd_terminate();

            ctrl_c_pressed = 0;
            set_ctrl_c_handler( true );
        }
        else
        {
            _log.warn( message_string( "SCHEDULER-263" ) );  // "Abbruch-Signal (Ctrl-C) beim Beenden des Schedulers empfangen. Der Scheduler wird abgebrochen, sofort.\n" );
            abort_now();
        }
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

        Z_LOG2( "scheduler", "SetSystemPowerState(TRUE,FALSE) => " << (ok? "ok" : message_string( printf_string( "MSWIN-%08lX", error ) ) ) << "\n" );

#   endif
}

//-------------------------------------------------------------------------Spooler::cmd_load_config
// Anderer Thread

void Spooler::cmd_load_config( const xml::Element_ptr& config, const Time& xml_mod_time, const string& source_filename )  
{ 
    THREAD_LOCK( _lock )  
    {
        _config_document_to_load = config.ownerDocument(); 
        _config_element_to_load  = config;
        _config_element_mod_time = xml_mod_time;
        _config_source_filename  = source_filename;
        _state_cmd = sc_load_config; 
    }

    if( current_thread_id() != _thread_id )  signal( "load_config" ); 
}

//----------------------------------------------------------------------------Spooler::cmd_continue
// Anderer Thread

void Spooler::cmd_continue()
{ 
    if( _state == s_paused )  _state_cmd = sc_continue; 
    
    //if( _waiting_errno )  _waiting_errno_continue = true;       // Siehe spooler_log.cxx: Warten bei ENOSPC

    signal( "continue" ); 
}

//------------------------------------------------------------------------------Spooler::cmd_reload
// Anderer Thread

void Spooler::cmd_reload()
{
    _state_cmd = sc_reload;
    signal( "reload" );
}

//--------------------------------------------------------------------------------Spooler::cmd_stop
// Anderer Thread
/*
void Spooler::cmd_stop()
{
    _state_cmd = sc_stop;
    signal( "stop" );
}
*/
//---------------------------------------------------------------------------Spooler::cmd_terminate
// Anderer Thread (spooler_service.cxx)

void Spooler::cmd_terminate( int timeout )
{
    if( timeout < 0 )  timeout = 0;

    _state_cmd                = sc_terminate;
    _termination_gmtimeout_at = timeout < 999999999? ::time(NULL) + timeout : no_termination_timeout;

    signal( "terminate" );
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart
// Anderer Thread

void Spooler::cmd_terminate_and_restart( int timeout )
{
    if( timeout < 0 )  timeout = 0;

    _state_cmd                = sc_terminate_and_restart;
    _termination_gmtimeout_at = timeout < 999999999? ::time(NULL) + timeout : no_termination_timeout;
    
    signal( "terminate_and_restart" );
}

//-------------------------------------------------------Spooler::cmd_let_run_terminate_and_restart
// Anderer Thread

void Spooler::cmd_let_run_terminate_and_restart()
{
    _state_cmd = sc_let_run_terminate_and_restart;
    signal( "let_run_terminate_and_restart" );
}

//-----------------------------------------------------------------------Spooler::abort_immediately

void Spooler::abort_immediately( bool restart )
{
    try
    { 
        _log.close(); 
        _communication.close( 0.0 );   // Damit offene HTTP-Logs ordentlich schließen (denn sonst ersetzt ie6 das Log durch eine Fehlermeldung)
    } 
    catch( ... ) {}

    abort_now( restart );
}

//-------------------------------------------------------------------------------Spooler::abort_now
// KANN VON EINEM ANDEREN THREAD GERUFEN WERDEN (

void Spooler::abort_now( bool restart )
{
    // So schnell wie möglich abbrechen!

    int exit_code = 99;


    kill_all_processes();

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
        LOG( "TerminateProcess( GetCurrentProcess() );\n" );
        TerminateProcess( GetCurrentProcess(), exit_code );
        _exit( exit_code );
#    else
        LOG( "kill(" << _pid << ",SIGKILL);\n" );
        kill( _pid, SIGKILL );
        _exit( exit_code );
#   endif
}

//----------------------------------------------------------------------Spooler::kill_all_processes

void Spooler::kill_all_processes()
{
    for( int i = 0; i < NO_OF( _process_handles ); i++ )  if( _process_handles[i] )  try_kill_process_immediately( _process_handles[i] );
    for( int i = 0; i < NO_OF( _pids            ); i++ )  if( _pids[i]            )  try_kill_process_immediately( _pids[i]            );
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv, const string& parameter_line )
{
    int rc;

    _argc = argc;
    _argv = argv;
    _parameter_line = parameter_line;


    tzset();

    _thread_id = current_thread_id();

//#   ifdef Z_WINDOWS
//        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
//#   endif

    //spooler_is_running = true;

    _event.set_name( "Scheduler" );
    _event.set_waiting_thread_id( current_thread_id() );
    _event.create();
    _event.add_to( &_wait_handles );

    _communication.init();  // Für Windows
    //_communication.bind();  // Falls der Port belegt ist, gibt's hier einen Abbruch



    //do
    {
        if( _state_cmd != sc_load_config )  load();

        THREAD_LOCK( _lock )  
        {
            if( _config_element_to_load == NULL )  z::throw_xc( "SCHEDULER-116", _spooler_id );

            load_config( _config_element_to_load, _config_element_mod_time, _config_source_filename );

            //Erst muss noch _config_commands_element ausgeführt werden: _config_element_to_load = NULL;
            //Erst muss noch _config_commands_element ausgeführt werden: _config_document_to_load = NULL;
        }



        // Nachdem argv und profile gelesen sind, und config geladen ist:

        _mail_defaults.set( "from_name", name() );      // Jetzt sind _hostname und _tcp_port bekannt
        _log.init( this );                              // Neue Einstellungen übernehmen: Default für from_name


        create_window();


        _module._dont_remote = true;
        if( _module.set() )  _module.init();



        if( _send_cmd != "" )  { send_cmd();  return 0; }

        start();

        // <commands> aus <config> ausführen:
        if( xml::Element_ptr commands_element = _config_element_to_load.select_node( "commands" ) )
        {
            Command_processor command_processor ( this, Security::seclev_all );
            command_processor.set_log( &_log );
            
            DOM_FOR_EACH_ELEMENT( commands_element, command_element )
            {
                xml::Element_ptr result = command_processor.execute_command( command_element, _config_element_mod_time );
                if( !result.select_node( "ok [ count(*) = 0  and  count(@*) = 0 ]" ) )
                {
                    Message_string m ( "SCHEDULER-966" );
                    m.set_max_insertion_length( INT_MAX );
                    m.insert( 1, result.xml( true ) );
                    _log.info( m );
                }
            }
            //command_processor.execute_commands( commands_element, _config_element_mod_time );
        }

        _config_element_to_load = NULL;
        _config_document_to_load = NULL;


        run_scheduler_script();

        if( _main_scheduler_connection )  _main_scheduler_connection->set_socket_manager( _connection_manager );


        try
        {
            run();
        }
        catch( exception& x )
        {
            set_state( s_stopping );        // Wichtig, damit _log wegen _waiting_errno nicht blockiert!

            try
            {
                _log.error( "" );
                _log.error( x.what() );
                _log.error( message_string( "SCHEDULER-264" ) );  // "SCHEDULER TERMINATES AFTER SERIOUS ERROR"
            }
            catch( exception& ) {}

            try 
            { 
                try
                {
                    Command_processor cp ( this, Security::seclev_all );
                    bool indent = true;
                    string xml = cp.execute( "<show_state what='task_queue orders remote_schedulers' />", Time::now(), indent );
                    try
                    {
                        _log.info( xml );  // Blockiert bei ENOSPC nicht wegen _state == s_stopping
                    }
                    catch( exception& ) { Z_LOG( "\n\n" << xml << "\n\n" ); }
                } 
                catch( exception& ) {}

                stop( &x ); 
            } 
            catch( exception& x ) { _log.error( x.what() ); }

            throw;
        }

        stop();

    }// while( _shutdown_cmd == sc_reload  ||  _shutdown_cmd == sc_load_config );


    _log.info( message_string( "SCHEDULER-999" ) );  // "Scheduler ordentlich beendet"
    //_log.info( "Scheduler ordentlich beendet." );
    _log.close();

   
    //if( _pid_filename != "" )  unlink( _pid_filename.c_str() );


    rc = 0;

    //spooler_is_running = false;
    return rc;
}

//------------------------------------------------------------------------------------start_process
#ifdef Z_WINDOWS

static void start_process( const string& command_line )
{
    LOG( "start_process(\"" << command_line << "\")\n" );

    PROCESS_INFORMATION process_info; 
    STARTUPINFO         startup_info; 
    BOOL                ok;
    Dynamic_area        my_command_line;
    
    my_command_line.assign( command_line.c_str(), command_line.length() + 1 );

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
            int pos;
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

        switch( fork() )
        {
            case  0:
            {
                 int n = sysconf( _SC_OPEN_MAX );
                 for( int i = 3; i < n; i++ )  close(i);
                 execv( _argv[0], _argv ); 
                 fprintf( stderr, "Error in execv %s: %s\n", _argv[0], strerror(errno) ); 
                 _exit(99);
            }

            case -1: 
                throw_errno( errno, "execv", _argv[0] );

            default: ;
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
            if( spooler::service_state(service_name) == SERVICE_STOPPED )  break;    
            sos_sleep( renew_wait_interval );
        }

        if( spooler::service_state(service_name) != SERVICE_STOPPED )  return;
    }

    if( renew_spooler != this_spooler )
    {
        for( t; t > 0; t -= renew_wait_interval )  
        {
            string msg = "CopyFile " + this_spooler + ", " + renew_spooler + '\n';
            if( !is_service )  fprintf( stderr, "%s", msg.c_str() );  // stderr, weil wir kein Log haben.
            LOG( msg );

            copy_ok = CopyFile( this_spooler.c_str(), renew_spooler.c_str(), FALSE );
            if( copy_ok )  break;

            int error = GetLastError();
            try 
            { 
                throw_mswin_error( error, "CopyFile" ); 
            }
            catch( const exception& x ) { 
                if( !is_service )  fprintf( stderr, "%s\n", x.what() );
                LOG( x.what() << '\n' );
            }

            if( error != ERROR_SHARING_VIOLATION )  return;
            sos_sleep( renew_wait_interval );
        }

        if( !is_service )  fprintf( stderr, "Der Scheduler ist ausgetauscht und wird neu gestartet\n\n" );
    }

    if( is_service )  spooler::service_start( service_name );
                else  start_process( quoted_windows_process_parameter( renew_spooler ) + " " + command_line );
}

#endif
//----------------------------------------------------------------------------------------full_path
#ifdef Z_WINDOWS

static string full_path( const string& path )
{
    Sos_limited_text<MAX_PATH> full;

    char* p = _fullpath( full.char_ptr(), path.c_str(), full.size() );
    if( !p )  throw_xc( "fullpath", path );

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
            LOG( msg );

            int ret = _unlink( copied_spooler.c_str() );
            if( ret == 0  || errno != EACCES ) break;

            msg = "errno=" + as_string(errno) + ' ' + z_strerror(errno) + '\n';
            fprintf( stderr, "%s", msg.c_str() );
            LOG( msg.c_str() );
            
            sos_sleep( renew_wait_interval );
        }
    }
}

#endif
//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv, const string& parameter_line )
{
    int ret;

//#   if defined _DEBUG  && defined __GNUC__
//        mtrace();   // Memory leak detectiopn
//#   endif

    Ole_initialize  ole;

    while(1)
    {
        Spooler my_spooler;

        try
        {
            my_spooler._is_service = spooler::is_daemon;

            ret = my_spooler.launch( argc, argv, parameter_line );

            if( my_spooler._shutdown_cmd == Spooler::sc_reload 
             || my_spooler._shutdown_cmd == Spooler::sc_load_config )  continue;        // Dasselbe in spooler_service.cxx!
        }
        catch( const exception& x )
        {
            //my_spooler._log ist vielleicht noch nicht geöffnet oder schon geschlossen
            my_spooler._log.error( x.what() );
            my_spooler._log.error( message_string( "SCHEDULER-331" ) );

            SHOW_ERR( "Error " << x.what() );     // Fehlermeldung vor ~Spooler ausgeben
            if( my_spooler.is_service() )  send_error_email( x, argc, argv, parameter_line, &my_spooler );
            ret = 1;
        }

        break;
    }

    return ret;
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
    zschimmer::com::object_server::Server server;

    server.register_class( spooler_com::CLSID_Remote_module_instance_server, Com_remote_module_instance_server::Create_instance );
    server.register_class(              CLSID_Com_log_proxy                , Com_log_proxy                    ::Create_instance );
    server.register_class( spooler_com::CLSID_Task_proxy                   , Com_task_proxy                   ::Create_instance );
    server.register_class( spooler_com::CLSID_Spooler_proxy                , Com_spooler_proxy                ::Create_instance );
  //server.register_class( spooler_com::CLSID_Xslt_stylesheet              , Xslt_stylesheet                  ::Create_instance );

    return server.main( argc, argv, true );
}
                   
//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv, const string& parameter_line )
{
    add_message_code_texts( sos::spooler::scheduler_messages );

    set_log_category_default( "scheduler"     , true );
  //set_log_category_default( "scheduler.*"   , true );
  //set_log_category_default( "scheduler.wait", false );
    set_log_category_default( "scheduler.loop", false );
    set_log_category_default( "scheduler.call", true );   // Aufrufe von spooler_process() etc. protokollieren (Beginn und Ende)
    set_log_category_default( "scheduler.order", true );



    Z_LOG2( "scheduler", "Scheduler " VER_PRODUCTVERSION_STR "\n" );

    int     ret                = 0;
    bool    is_service         = false;
    bool    is_object_server   = false;
    bool    is_scheduler_client= false;
    bool    kill_pid_file      = false;
    int     kill_pid           = 0;
    string  pid_filename;


#   ifdef Z_WINDOWS
        SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );    // Das System soll sich Messageboxen verkneifen (außer beim Absturz)
#   endif

    spooler::error_settings.read( spooler::default_factory_ini );

    try
    {
        bool    need_call_scheduler = true;
        bool    call_scheduler      = false;
        bool    do_install_service  = false;
        bool    do_remove_service   = false;
        bool    is_service_set      = false;
        string  id;
        string  service_name, service_display;
        string  service_description = "Job scheduler for process automation";
        string  renew_spooler;
        string  command_line;
        bool    renew_service = false;
        string  send_cmd;
        string  log_filename;
        string  factory_ini = spooler::default_factory_ini;
        string  dependencies;

        for( Sos_option_iterator opt ( argc, argv, parameter_line ); !opt.end(); opt.next() )
        {
            if( opt.with_value( "scheduler" ) )     // Stichwort für scheduler_client
            {
                is_scheduler_client = true;
                break;  // scheduler_client wertet argc und argv erneut aus, deshalb brechen wir hier ab.
            }
            else
            //if( opt.flag      ( "renew-spooler"    ) )  renew_spooler = program_filename();
          //else
          //if( opt.flag      ( "show-dtd"         ) )  { if( opt.set() )  need_call_scheduler = false, fprintf( stdout, "%s", spooler::dtd_string ); }
          //else
            if( opt.with_value( "expand-classpath" ) )  { cout << java::expand_class_path( opt.value() ) << '\n'; need_call_scheduler = false; }
            else
            if( opt.flag      ( "show-xml-schema"  ) )  { if( opt.set() )  need_call_scheduler = false, fprintf( stdout, "%s", spooler::embedded_files.string_from_embedded_file( spooler::xml_schema_path ).c_str() ); }
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
            if( opt.flag      ( "V"                ) )  fprintf( stderr, "Scheduler %s\n", VER_PRODUCTVERSION_STR );
            else
            if( opt.flag      ( "kill"             ) )  kill_pid_file = true;
            else
            if( opt.with_value( "kill"             ) )  kill_pid = opt.as_int();
            else
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
            if( opt.flag      ( "service"          ) )  call_scheduler = true, is_service = opt.set(), is_service_set = true;
            else
            if( opt.with_value( "service"          ) )  call_scheduler = true, is_service = true, is_service_set = true, service_name = opt.value();
            else
            if( opt.with_value( "need-service"     ) )  dependencies += opt.value(), dependencies += '\0';
            else
            {
                if( opt.with_value( "sos.ini"          ) )  ;  //schon in sos_main0() geschehen.  set_sos_ini_filename( opt.value() );
                else
                if( opt.with_value( "id"               ) )  id = opt.value();
                else
                if( opt.with_value( "ini"              ) )  factory_ini = opt.value(), spooler::error_settings.read( factory_ini );
                else
                if( opt.with_value( "log"              ) )  log_filename = opt.value();
                else
                if( opt.with_value( "pid-file"         ) )  pid_filename = opt.value();
                else
                if( opt.with_value( "env"              ) )  
                {
                    string value = opt.value();
                    size_t eq = value.find( '=' ); if( eq == string::npos )  z::throw_xc( "SCHEDULER-318", value );
                    set_environment_variable( value.substr( 0, eq ), value.substr( eq + 1 ) );
                }
                else
                    call_scheduler = true;     // Aber is_scheduler_client hat Vorrang!

                if( !command_line.empty() )  command_line += " ";
                command_line += opt.complete_parameter( '"', '"' );
            }
        }

        if( send_cmd != "" )  is_service = false;

        if( log_filename.empty() )  log_filename = subst_env( read_profile_string( factory_ini, "spooler", "log" ) );
        if( !log_filename.empty() )  log_start( log_filename );


        if( is_scheduler_client )
        {
            ret = spooler::scheduler_client_main( argc, argv );
        }
        else
        if( is_object_server )
        {
            ret = spooler::object_server( argc, argv );
        }
        else
        {
            if( kill_pid )
            {
                kill_process_immediately( kill_pid );
                need_call_scheduler = false;
            }

            if( kill_pid_file )
            {
                int pid = as_int( replace_regex( string_from_file( pid_filename ), "[\r\n]", "" ) ); 
                kill_process_immediately( pid, true );   // kill_childs = true
                need_call_scheduler = false;
            }            


#           ifdef Z_WINDOWS
                if( service_name != "" ) 
                {
                    if( service_display == "" )  service_display = service_name;
                }
                else
                {
                    service_name = spooler::make_service_name(id);
                    if( service_display == "" )  service_display = spooler::make_service_display(id);
                }

                if( !renew_spooler.empty() )  
                { 
                    spooler::spooler_renew( service_name, renew_spooler, renew_service, command_line ); 
                }
                else
                if( do_remove_service | do_install_service )
                {
                    if( do_remove_service  )  spooler::remove_service( service_name );
                    if( do_install_service ) 
                    {
                        //if( !is_service )  command_line = "-service " + command_line;
                        command_line = "-service=" + service_name + " " + command_line;
                        dependencies += '\0';
                        spooler::install_service( service_name, service_display, service_description, dependencies, command_line );
                    }
                }
                else
                if( call_scheduler || need_call_scheduler )
                {
                    _beginthread( spooler::delete_new_spooler, 50000, NULL );

                //if( !is_service_set )  is_service = spooler::service_is_started(service_name);

                    if( is_service )
                    {
                        ret = spooler::spooler_service( service_name, argc, argv );   
                    }
                    else
                    {
                        ret = spooler::spooler_main( argc, argv, parameter_line );
                    }
                }

#            else

                if( call_scheduler || need_call_scheduler )
                {
                    if( is_service )
                    {
                        spooler::is_daemon = true;

                        LOG( "Scheduler wird Daemon. Pid wechselt\n");
                        spooler::be_daemon();
                    }

                    ret = spooler::spooler_main( argc, argv, command_line );
                }

#           endif
        }
    }
    catch( const exception& x )
    {
        LOG( x.what() << "\n" );
        if( is_service )  spooler::send_error_email( x, argc, argv, parameter_line );
        cerr << x << "\n";
        ret = 1;
    }
    catch( const _com_error& x )
    {
        string what = string_from_ole( x.Description() );
        LOG( what << "\n" );
        if( is_service )  spooler::send_error_email( zschimmer::Xc( x ), argc, argv, parameter_line );
        cerr << what << "\n";
        ret = 1;
    }


    if( !is_object_server )  LOG( "Programm wird beendet\n" );

    return ret;
}

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    _argc = argc;
    _argv = argv;

    int ret = sos::spooler_main( argc, argv, "" );



#   ifdef SCHEDULER_WITH_HOSTJAVA

        // HP-UX und eingebundenes Hostjava: Irgendein atexit() stürzt in InterlockedIncrement() (AddRef()?") ab.
        // Deshalb beenden wir den Scheduler hier mit _exit(), schließen aber alle Dateien vorher

        Z_LOG( "_exit(" << ret << ") für Hostjava\n" );

        int n = sysconf( _SC_OPEN_MAX );
        for( int i = 0; i < n; i++ )  ::close(i);

        _exit( ret );   // Kein atexit() wird gerufen, keine Standard-I/O-Puffer werden geschrieben

#   endif

    return ret;
}


} //namespace sos

//-------------------------------------------------------------------------------------------------
/*
#ifdef Z_WINDOWS

//extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )

extern "C" int __cdecl mainCRTStartup();
extern "C" BOOL WINAPI _DllMainCRTStartup( HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved );


extern "C" int __stdcall entry_point( HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved )
{
    MessageBox( NULL, "Hier ist der Scheduler!", "Scheduler", 0 );

    //return mainCRTStartup();
    return _DllMainCRTStartup( hDllHandle, dwReason, lpreserved );
}

#endif
*/
//-------------------------------------------------------------------------------------------------
