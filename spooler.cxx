// $Id: spooler.cxx,v 1.322 2004/02/11 10:41:55 jz Exp $
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

#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#ifdef Z_WINDOWS
#   include <process.h>
#   include <direct.h>
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

/*
#ifdef Z_WINDOWS
#   include <process.h>
#   include <direct.h>
#   define DEFAULT_VM_MODULE "msjava.dll"
#else
#   ifdef Z_HPUX
#       define DEFAULT_VM_MODULE "libjvm.sl"
#    else
#       define DEFAULT_VM_MODULE "libjvm.so"
#   endif
#endif
*/

//char** _argv = NULL;
//int    _argc = 0;


using namespace std;


namespace sos {

extern const Bool _dll = false;

namespace spooler {


const char*                     default_factory_ini                 = "factory.ini";
const string                    new_suffix                          = "~new";  // Suffix für den neuen Spooler, der den bisherigen beim Neustart ersetzen soll
const double                    renew_wait_interval                 = 0.25;
const double                    renew_wait_time                     = 30;      // Wartezeit für Brückenspooler, bis der alte Spooler beendet ist und der neue gestartet werden kann.
const double                    wait_for_thread_termination         = latter_day;  // Haltbarkeit des Geduldfadens
const double                    wait_step_for_thread_termination    = 5.0;         // 1. Nörgelabstand
const double                    wait_step_for_thread_termination2   = 600.0;       // 2. Nörgelabstand
//const double wait_for_thread_termination_after_interrupt = 1.0;

const char*                     temporary_process_class_name        = "(temporaries)";
static bool                     is_daemon                           = false;
//static t                      daemon_starter_pid;
//bool                          spooler_is_running      = false;
volatile int                    ctrl_c_pressed                      = 0;
Spooler*                        spooler_ptr                         = NULL;

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

static Error_settings           error_settings;



/*
struct Set_console_code_page
{
                                Set_console_code_page       ( uint cp )         { _cp = GetConsoleOutputCP(); SetConsoleOutputCP( cp ); }
                               ~Set_console_code_page       ()                  { SetConsoleOutputCP( _cp ); }

    uint                       _cp;
};
*/

static void set_ctrl_c_handler( bool on );

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

void send_error_email( const string& error_text, int argc, char** argv, const string& parameter_line, Spooler* spooler )
{

    string body = "Der Scheduler konnte nicht gestartet werden.\n"
                  "\n"
                  "\n"
                  "Der Aufruf war:\n"
                  "\n";
                   
    for( int i = 0; i < argc; i++ )  body += argv[i], body += ' ';
    body += parameter_line;

    body += "\n\n\n"
            "Fehlermeldung:\n";
    body += error_text;

    string subject = "FEHLER BEI SCHEDULER-START: " + error_text;

    if( spooler )  spooler->send_error_email( subject, body );
             else           send_error_email( subject, body );
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
                                        else  return as_bool(v);
    }
    catch( const exception& ) { return deflt; }
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
        if( dwCtrlType == CTRL_C_EVENT  &&  !ctrl_c_pressed )
        {
            ctrl_c_pressed++;
            //Kein Systemaufruf hier! (Aber bei Ctrl-C riskieren wir einen Absturz. Ich will diese Meldung sehen.)
            fprintf( stderr, "Scheduler wird wegen Ctrl-C beendet ...\n" );
            if( spooler_ptr )  spooler_ptr->async_signal( "Ctrl+C" );
            return true;
        }
        else
            return false;
    }

#else

    static void ctrl_c_handler( int sig )
    {
        set_ctrl_c_handler( false );

        //if( !ctrl_c_pressed )
        //{
            ctrl_c_pressed++;
            //Kein Systemaufruf hier! (Aber bei Ctrl-C riskieren wir einen Absturz. Ich will diese Meldung sehen.)
            if( !is_daemon )  fprintf( stderr, "Scheduler wird wegen kill -%d beendet ...\n", sig );

            // pthread_mutex_lock:
            // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
            // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
            // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.

            if( !is_daemon && spooler_ptr )  spooler_ptr->async_signal( "Ctrl+C" );
        //}
    }

#endif
//-------------------------------------------------------------------------------set_ctrl_c_handler

static void set_ctrl_c_handler( bool on )
{
    //LOG( "set_ctrl_c_handler(" << on << ")\n" );

#   ifdef Z_WINDOWS

        SetConsoleCtrlHandler( ctrl_c_handler, on );

#    else

        ::signal( SIGINT , on? ctrl_c_handler : SIG_DFL );
        ::signal( SIGTERM, on? ctrl_c_handler : SIG_DFL );

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
                 break;

        case -1: throw_errno( errno, "fork" );

        default: ::sleep(1);  // Falls der Daemon noch was ausgibt, sollte das vor dem Shell-Prompt sein.
                 //fprintf( stderr, "Daemon gestartet. pid=%d\n", pid ); 
                 //fflush( stderr );
                 _exit(0);
    }
}

#endif
//---------------------------------------------------------------------------------Spooler::Spooler
// Die Objektserver-Prozesse haben kein Spooler-Objekt.

Spooler::Spooler() 
: 
    _zero_(this+1), 
    _security(this),
    _communication(this), 
    _base_log(this),
    _log(1),
    _wait_handles(this,&_log),
    _module(this,&_log),
    _log_level( log_info ),
    _factory_ini( default_factory_ini ),

    _smtp_server   ("-"),   // Für spooler_log.cxx: Nicht setzen, damit Default aus sos.ini erhalten bleibt
    _log_mail_from ("-"),
    _log_mail_cc   ("-"),
    _log_mail_bcc  ("-"),
    _mail_queue_dir("-")
{
    if( spooler_ptr )  throw_xc( "spooler_ptr" );
    spooler_ptr = this;

    if( !SOS_LICENCE( licence_scheduler ) )  throw_xc( "SOS-1000", "Scheduler" );       // Früh prüfen, damit der Fehler auch auftritt, wenn die sos.ini fehlt.

    _pid = getpid();

    _tcp_port = 4444;
    _udp_port = 4444;
    _priority_max = 1000;       // Ein Wert > 1, denn 1 ist die voreingestelle Priorität der Jobs
            
    _max_threads = 1;

    _com_log     = new Com_log( &_log );
    _com_spooler = new Com_spooler( this );
    _variables   = new Com_variable_set();


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

    _module._dont_remote = true;

    _connection_manager = Z_NEW( object_server::Connection_manager );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    spooler_ptr = NULL;
    set_ctrl_c_handler( false );

    if( !_thread_list.empty() )  
    {
        close_threads();
        _thread_list.clear();
    }

    //_spooler_thread_list.clear();


    _object_set_class_list.clear();

    _communication.close( 5 );      // 5 Sekunden aufs Ende warten
    _security.clear();

    _event.close();
    _wait_handles.close();

    //nicht nötig  Z_FOR_EACH( Job_chain_map, _job_chain_map, it )  it->second->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->set_log( NULL );
}

//--------------------------------------------------------------------------Spooler::security_level
// Anderer Thread

Security::Level Spooler::security_level( const Host& host )
{
    Security::Level result = Security::seclev_none;

    THREAD_LOCK( _lock )
    {
        result = _security.level( host );
    }

    return result;
}

//-----------------------------------------------------------------------------Spooler::jobs_as_xml

xml::Element_ptr Spooler::jobs_as_xml( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr jobs_element = document.createElement( "jobs" );
    dom_append_nl( jobs_element );

    THREAD_LOCK( _lock )  FOR_EACH( Job_list, _job_list, it )  jobs_element.appendChild( (*it)->dom( document, show ) ), dom_append_nl( jobs_element );

    return jobs_element;
}

//----------------------------------------------------------------------Spooler::load_jobs_from_xml

void Spooler::load_jobs_from_xml( const xml::Element_ptr& element, const Time& xml_mod_time, bool init )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job" ) )
        {
            string spooler_id = e.getAttribute( "spooler_id" );

            if( _manual? e.getAttribute("name") == _job_name 
                       : spooler_id.empty() || spooler_id == id() )
            {
                string job_name = e.getAttribute("name");
                Sos_ptr<Job> job = get_job_or_null( job_name );
                if( job )
                {
                    job->set_dom( e, xml_mod_time );
                    if( init )  job->init0(),  job->init();
                }
                else
                {
                    job = SOS_NEW( Job( this ) );
                    job->set_dom( e, xml_mod_time );
                    if( init )  job->init0(),  job->init();
                    add_job( job );
                }
            }
        }
    }
}

//----------------------------------------------------------------------------Spooler::cmd_add_jobs
// Anderer Thread

void Spooler::cmd_add_jobs( const xml::Element_ptr& element )
{
    load_jobs_from_xml( element, true );

    signal( "add_jobs" );
}

//-------------------------------------------------------------------Spooler::remove_temporary_jobs

void Spooler::remove_temporary_jobs()
{
    THREAD_LOCK( _lock )
    {
        Job_list::iterator it = _job_list.begin();
        while( it != _job_list.end() )
        {
            Job* job = *it;

            if( job->should_removed() )    
            {
                if( _debug )  job->_log.debug( "Temporärer Job wird entfernt" );

                try
                {
                    job->close(); 
                }
                catch( exception &x )  { _log.warn( x.what() ); }   // Kann das überhaupt passieren?

                it = _job_list.erase( it );
                continue;
            }

            it++;
        }
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

xml::Element_ptr Spooler::threads_as_xml( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr threads = document.createElement( "threads" );

    dom_append_nl( threads );

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )
        {
            threads.appendChild( (*it)->dom( document, show ) );
            dom_append_nl( threads );
        }
    }

    return threads;
}

//-----------------------------------------------------------Spooler::load_process_classes_from_dom

void Spooler::load_process_classes_from_dom( const xml::Element_ptr& element, const Time& xml_mod_time )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "process_class" ) )
        {
            string spooler_id = e.getAttribute( "spooler_id" );

            if( spooler_id.empty() || spooler_id == id() )
            {
                add_process_class( Z_NEW( Process_class( this, e ) ) );
            }
        }
    }
}

//------------------------------------------------------------------Spooler::process_classes_as_dom
// Anderer Thread

xml::Element_ptr Spooler::process_classes_as_dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr element = document.createElement( "process_classes" );

    FOR_EACH( Process_class_list, _process_class_list, it )  element.appendChild( (*it)->dom( document, show ) );

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
    if( !pc )  throw_xc( "SCHEDULER-195", name );
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

void Spooler::init_process_classes()
{
    //while( _process_list.size() < _process_count_max )  new_process();

}

//---------------------------------------------------------------------Spooler::try_to_free_process

bool Spooler::try_to_free_process( Job* for_job, Process_class* process_class, const Time& now )
{
    return _single_thread->try_to_free_process( for_job, process_class, now );
}

//-----------------------------------------------------------------Spooler::register_process_handle

void Spooler::register_process_handle( Process_handle p )
{
    for( int i = 0; i < NO_OF( _process_handles ); i++ )
    {
        if( _process_handles[i] == 0 )  { _process_handles[i] = p;  return; }
    }
}

//-----------------------------------------------------------------Spooler::register_process_handle

void Spooler::unregister_process_handle( Process_handle p )
{
    for( int i = 0; i < NO_OF( _process_handles ); i++ )
    {
        if( _process_handles[i] == p )  { _process_handles[i] = 0;  return; }
    }
}

//--------------------------------------------------------------Spooler::wait_until_threads_stopped

void Spooler::wait_until_threads_stopped( Time until )
{
    assert( current_thread_id() == _thread_id );

    return;
    
/* Threads sind nicht implementiert    

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
*/
}

//--------------------------------------------------------------------------Spooler::signal_threads

void Spooler::signal_threads( const string& signal_name )
{
    assert( current_thread_id() == _thread_id );

    FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->_free_threading )  (*it)->signal( signal_name );
}

//------------------------------------------------------------------------------Spooler::get_thread
// Anderer Thread

Spooler_thread* Spooler::get_thread( const string& thread_name )
{
    Spooler_thread* thread = get_thread_or_null( thread_name );
    if( !thread )  throw_xc( "SCHEDULER-128", thread_name );

    return thread;
}

//----------------------------------------------------------------------Spooler::get_thread_or_null
// Anderer Thread

Spooler_thread* Spooler::get_thread_or_null( const string& thread_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )  if( stricmp( (*it)->name().c_str(), thread_name.c_str() ) == 0 )  return *it;
    }

    return NULL;
}

//--------------------------------------------------------------------Spooler::get_object_set_class
// Anderer Thread

Object_set_class* Spooler::get_object_set_class( const string& name )
{
    Object_set_class* c = get_object_set_class_or_null( name );
    if( !c )  throw_xc( "SCHEDULER-101", name );
    return c;
}

//-------------------------------------------------------------Spooler::get_object_set_class_or_null
// Anderer Thread

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

//---------------------------------------------------------------------------------Spooler::add_job

void Spooler::add_job( const Sos_ptr<Job>& job )
{
    THREAD_LOCK( _lock )
    {
        Job* j = get_job_or_null( job->name() );
        if( j )  throw_xc( "SCHEDULER-130", j->name() );

        _job_list.push_back( job );
    }
}

//---------------------------------------------------------------------------------Spooler::get_job
// Anderer Thread

Job* Spooler::get_job( const string& job_name )
{
    Job* job = get_job_or_null( job_name );
    if( !job  ||  !job->state() )  throw_xc( "SCHEDULER-108", job_name );
    return job;
}

//-------------------------------------------------------------------------Spooler::get_job_or_null
// Anderer Thread

Job* Spooler::get_job_or_null( const string& job_name )
{
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

//-------------------------------------------------------------------Spooler::get_next_job_to_start

Job* Spooler::get_next_job_to_start()
{
    Job* next_job = NULL;
    
    Time next_time = latter_day;      // Für HP_UX, gcc 3.1, eine lokale Variable. _next_start_time setzt der Compiler jedes zweite Mal auf 0. jz 7.3.03

    THREAD_LOCK( _lock )
    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;

            if( next_time > job->next_time() ) 
            {
                next_job = job; 
                next_time = next_job->next_time();
                if( next_time == 0 )  break;
            }

/*
            if( job->_state == Job::s_pending )
            {
                Time now = Time::now();

                if( job->_order_queue  
                 &&  job->_order_queue->has_order( now ) 
                 && ( job->_state != Job::s_pending || job->is_in_period(now) ) )
                {
                    next_job = job; 
                    next_time = 0; 
                    break; 
                }
            }
*/
        }
    }

    return next_job;
}

//------------------------------------------------------------------------Spooler::get_task_or_null
// Anderer Thread

Sos_ptr<Task> Spooler::get_task_or_null( int task_id )
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

Spooler_thread* Spooler::select_thread_for_task( Task* task )
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
    _log.info( state_name() );

    if( _state_changed_handler )  (*_state_changed_handler)( this, NULL );
}

//------------------------------------------------------------------------------Spooler::state_name

string Spooler::state_name( State state )
{
    switch( state )
    {
        case s_stopped:             return "stopped";
        case s_starting:            return "starting";
        case s_running:             return "running";
        case s_paused:              return "paused";
        case s_stopping:            return "stopping";
        case s_stopping_let_run:    return "stopping_let_run";
        default:                    return as_string( (int)state );
    }
}

//------------------------------------------------------------------------------Spooler::start_jobs

void Spooler::start_jobs()
{
    FOR_EACH_JOB( job )  (*job)->init();
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

//---------------------------------------------------------------------------Spooler::start_threads
/*
void Spooler::start_threads()
{
    FOR_EACH( Thread_list, _thread_list, it )  
    {
        Spooler_thread* thread = *it;

        if( thread->_free_threading )  
        {
            thread->thread_start();      // Eigener Thread
            thread->set_thread_termination_event( &_event );        // Signal zu uns, wenn Thread endet
        }
        else
        {
            thread->start( &_event );    // Unser Thread
        }
    }
}
*/
//---------------------------------------------------------------------------Spooler::close_threads

void Spooler::close_threads()
{
    signal_threads( "stop" );

    wait_until_threads_stopped( latter_day );

/*  Wir müssen warten, bis alle Threads beendet sind, denn sie benutzen _spooler. Also: Kein Timeout!
    wait_until_threads_stopped( Time::now() + wait_for_thread_termination );
*
    FOR_EACH( Thread_list, _thread_list, it )  
    {
        Spooler_thread* thread = *it;
        if( !thread->_free_threading ) 
        {
            thread->close1();
        }
    }
*/
}

//-----------------------------------------------------------------------Spooler::run_single_thread
/*
bool Spooler::run_single_thread()
{
    bool something_done = false;
    Time now            = Time::now();


    if( _state_cmd == sc_none ) 
    {
        Thread* thread = *_thread_list.begin();
    
        if( thread->_next_start_time <= now  ||  thread->_wait_handles.signaled() )
        {
            bool ok = thread->process();

            something_done |= ok;
        }

        //thread->_log.debug9( "_next_start_time=" + thread->_next_start_time.as_string() );
        //if( thread->_next_job  &&  _next_time > thread->_next_start_time )  _next_time = thread->_next_start_time, _next_job = thread->_next_job;
    }

    return something_done;
}
*/

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
        char buffer [100];

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
    _log_directory              =            read_profile_string    ( _factory_ini, "spooler", "log-dir"            );  // veraltet
    _log_directory              = subst_env( read_profile_string    ( _factory_ini, "spooler", "log_dir"            , _log_directory ) );  _log_directory_as_option_set = !_log_directory.empty();
    _include_path               =            read_profile_string    ( _factory_ini, "spooler", "include-path"       );  // veraltet
    _include_path               = subst_env( read_profile_string    ( _factory_ini, "spooler", "include_path"       , _include_path ) );   _include_path_as_option_set  = !_include_path.empty();
    _spooler_param              =            read_profile_string    ( _factory_ini, "spooler", "param"              );                   _spooler_param_as_option_set = !_spooler_param.empty();
    log_level                   =            read_profile_string    ( _factory_ini, "spooler", "log_level"          , log_level );   
    _tasks_tablename            =            read_profile_string    ( _factory_ini, "spooler", "db_tasks_table"     , "SCHEDULER_TASKS"   );
    _job_history_tablename      =            read_profile_string    ( _factory_ini, "spooler", "db_history_table"   , "SCHEDULER_HISTORY" );
    _job_history_columns        =            read_profile_string    ( _factory_ini, "spooler", "history_columns"    );
    _job_history_yes            =            read_profile_bool      ( _factory_ini, "spooler", "history"            , true );
    _job_history_on_process     =            read_profile_history_on_process( _factory_ini, "spooler", "history_on_process", 0 );
    _job_history_archive        =            read_profile_archive   ( _factory_ini, "spooler", "history_archive"    , arc_no );
    _job_history_with_log       =            read_profile_with_log  ( _factory_ini, "spooler", "history_with_log"   , arc_no );
    _order_history_yes          =            read_profile_bool      ( _factory_ini, "spooler", "order_history"      , true );
    _order_history_with_log     =            read_profile_with_log  ( _factory_ini, "spooler", "order_history_with_log", arc_no );
    _db_name                    =            read_profile_string    ( _factory_ini, "spooler", "db"                 );

    // need_db=yes|no|strict
  //_need_db                    =            read_profile_bool      ( _factory_ini, "spooler", "need_db"            , true                );
    string need_db_str          =            read_profile_string    ( _factory_ini, "spooler", "need_db"            , "no"                );
    if( stricmp( need_db_str.c_str(), "strict" ) == 0 )
    {
        _need_db = true; 
        _wait_endless_for_db_open = false;
    }
    else
    {
        try{ _wait_endless_for_db_open = _need_db = as_bool( need_db_str ); }
        catch( const exception& x ) { throw_xc( "SCHEDULER-206", need_db_str, x.what() ); }
    }

    _max_db_errors              =            read_profile_int       ( _factory_ini, "spooler", "max_db_errors"      , 5 );
    _order_history_tablename    =            read_profile_string    ( _factory_ini, "spooler", "db_order_history_table", "SCHEDULER_ORDER_HISTORY" );
    _orders_tablename           =            read_profile_string    ( _factory_ini, "spooler", "db_orders_table"    , "SCHEDULER_ORDERS"    );
    _variables_tablename        =            read_profile_string    ( _factory_ini, "spooler", "db_variables_table" , "SCHEDULER_VARIABLES" );
  //_interactive                = true;     // Kann ohne weiteres true gesetzt werden (aber _is_service setzt es wieder false)


    _mail_on_error   =            read_profile_bool           ( _factory_ini, "spooler", "mail_on_error"  , _mail_on_error );
    _mail_on_process =            read_profile_mail_on_process( _factory_ini, "spooler", "mail_on_process", _mail_on_process );
    _mail_on_success =            read_profile_bool           ( _factory_ini, "spooler", "mail_on_success", _mail_on_success );
    _mail_queue_dir  = subst_env( read_profile_string         ( _factory_ini, "spooler", "mail_queue_dir" , _mail_queue_dir ) );
    _mail_encoding   =            read_profile_string         ( _factory_ini, "spooler", "mail_encoding"  , "base64"        );      // "quoted-printable": Jmail braucht 1s pro 100KB dafür
    _smtp_server     =            read_profile_string         ( _factory_ini, "spooler", "smtp"           , _smtp_server );

    _log_mail_from      = read_profile_string( _factory_ini, "spooler", "log_mail_from"   );
    _log_mail_to        = read_profile_string( _factory_ini, "spooler", "log_mail_to"     );
    _log_mail_cc        = read_profile_string( _factory_ini, "spooler", "log_mail_cc"     );
    _log_mail_bcc       = read_profile_string( _factory_ini, "spooler", "log_mail_bcc"    );
    _log_mail_subject   = read_profile_string( _factory_ini, "spooler", "log_mail_subject");
    _log_collect_within = read_profile_uint  ( _factory_ini, "spooler", "log_collect_within", 0 );
    _log_collect_max    = read_profile_uint  ( _factory_ini, "spooler", "log_collect_max"   , 900 );

    _my_program_filename = _argv? _argv[0] : "(Programmdateiname fehlt)";

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
            if( opt.with_value( "sos.ini"          ) )  ;   // wurde in Hostware-main() bearbeitet
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
            if( opt.with_value( "include-path"     ) )  _include_path = opt.value(),  _include_path_as_option_set = true;
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
        if( _config_filename.empty() )  throw_xc( "SCHEDULER-115" );

    }
    catch( const Sos_option_error& )
    {
        if( !_is_service )
        {
            cerr << "usage: " << _argv[0] << "\n"
                    "       -cd=PATH\n"
                    "       -config=XMLFILE\n"
                    "       -service-\n"
                    "       -log=HOSTWARELOGFILENAME\n"
                    "       -log-dir=DIRECTORY|*stderr\n"
                    "       -id=ID\n"
                    "       -param=PARAM\n"
                    "       -include-path=PATH\n"
                    "       -log-level=error|warn|info|debug|debug1|...|debug9\n";
        }

        throw;
    }
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    assert( current_thread_id() == _thread_id );

    set_state( s_starting );
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
        //f.close();
    }

    _log.init( this );

    char hostname[200];  // Nach _communication.init() und nach _prefix_log.init()!
    if( gethostname( hostname, sizeof hostname ) == SOCKET_ERROR )  hostname[0] = '\0',  _log.warn( string("gethostname(): ") + strerror( errno ) );
    _hostname = hostname;


    // Die erste Prozessklasse ist für temporäre Prozesse
    ptr<Process_class> process_class = Z_NEW( Process_class( this, temporary_process_class_name ) );
    _process_class_list.push_back( process_class );         


    Command_processor cp ( this );
    _executing_command = false;             // Command_processor() hat es true gesetzt, aber noch läuft der Scheduler nicht. 
                                            // spooler_history.cxx verweigert das Warten auf die Datenbank, wenn _executing_command gesetzt ist,
                                            // damit der Scheduler nicht in einem TCP-Kommando blockiert.

    cp.execute_file( _config_filename );
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    assert( current_thread_id() == _thread_id );

    _state_cmd = sc_none;
    set_state( s_starting );

    _base_log.set_directory( _log_directory );
    _base_log.open_new();
    
    _log.info( string( "Scheduler (" VER_PRODUCTVERSION_STR ) + ") startet mit " + _config_filename );
    _spooler_start_time = Time::now();

    FOR_EACH_JOB( job )  (*job)->init0();

    try
    {
        if( _has_java_source )
        {
            string java_work_dir = temp_dir() + Z_DIR_SEPARATOR "java";
            _java_vm->set_work_dir( java_work_dir );
            _java_vm->prepend_class_path( java_work_dir );
        }

        if( _has_java )     // Nur True, wenn Java-Job nicht in separatem Prozess ausgeführt wird.
        {
            _java_vm->set_log( &_log );

            _java_vm->prepend_class_path( _config_java_class_path );        // Nicht so gut hier. Bei jedem Reload wird der Pfad verlängert. Aber Reload lässt Java sowieso nicht neu starten.
            _java_vm->set_options( _config_java_options );

            Java_module_instance::init_java_vm( _java_vm );
        }
    }
    catch( const exception& x )
    {
        _log.error( x.what() );
        _log.error( "Java kann nicht gestartet werden. Scheduler startet ohne Java." );
    }


    THREAD_LOCK( _lock )
    {
        if( _need_db  && _db_name.empty() )  throw_xc( "SCHEDULER-205" );

        _db = SOS_NEW( Spooler_db( this ) );
        _db->open( _db_name );
    }

    _db->spooler_start();

    // Thread _communication nach Java starten (auch implizit durch _db). Java muss laufen, wenn der Thread startet! (Damit attach_thread() greift)
    if( !_manual )  _communication.start_or_rebind();


    set_ctrl_c_handler( false );
    set_ctrl_c_handler( true );       // Falls Java (über Dateityp jdbc) gestartet worden ist und den Signal-Handler verändert hat



  //_spooler_thread_list.clear();

    FOR_EACH( Thread_list, _thread_list, it )
    {
        Spooler_thread* thread = *it;
      //if( !thread->_free_threading )  _spooler_thread_list.push_back( thread );
        thread->init();
    }


    if( _module.set() )
    {
        _module_instance = _module.create_instance();
      //_module_instance->_title = "Scheduler-Script";
        _module_instance->init();

        _module_instance->add_obj( (IDispatch*)_com_spooler, "spooler"     );
        _module_instance->add_obj( (IDispatch*)_com_log    , "spooler_log" );

        _module_instance->load();
        _module_instance->start();

        bool ok = check_result( _module_instance->call_if_exists( "spooler_init()Z" ) );
        if( !ok )  throw_xc( "SCHEDULER-183" );
    }

    init_process_classes();
  //start_threads();
    start_jobs();

    
    if( _is_service || is_daemon )
    {
/*
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
*/
    }
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    assert( current_thread_id() == _thread_id );

    //set_state( _state_cmd == sc_let_run_terminate_and_restart? s_stopping_let_run : s_stopping );

    //_log.msg( "Spooler::stop" );

    close_threads();
    close_jobs();

    //FOR_EACH( Thread_list, _thread_list, it )  it = _thread_list.erase( it );


/*  interrupt() lässt PerlScript abstürzen
    FOR_EACH( Thread_list, _thread_list, it )
    {
        Job* job = (*it)->current_job();
        if( job )  try { job->interrupt_script(); } catch(const exception& x){_log.error(x.what());}
    }

    wait_until_threads_stopped( Time::now() + wait_for_thread_termination_after_interrupt );
*/
    _object_set_class_list.clear();
  //_spooler_thread_list.clear();
    _thread_list.clear();
    _job_list.clear();
    _process_class_list.clear();

    if( _module_instance )  _module_instance->close();

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wird Java stehen und schließen es erst am Schluss

    if( _shutdown_cmd == sc_terminate_and_restart 
     || _shutdown_cmd == sc_let_run_terminate_and_restart )  spooler_restart( &_base_log, _is_service );

    _db->spooler_stop();

    THREAD_LOCK( _lock )
    {
        _db->close();
        _db = NULL;
    }

    set_state( s_stopped );     
    // Der Dienst ist hier beendet
}

//----------------------------------------------------------------------------Spooler::nichts_getan

void Spooler::nichts_getan( Spooler_thread* thread, int anzahl )
{
    if( anzahl == 1 )
    {
        _log.warn( "Nichts getan, state=" + state_name() + " _wait_handles=" + _wait_handles.as_string() );

        FOR_EACH( Job_list, _job_list, j )  
        {
            Job* job = *j;

            _log.warn( job->obj_name() 
                       + " state=" + job->state_name() ); 
                        //" queue_filled=" + ( (*it)->queue_filled()? "ja" : "nein" ) + 
                        //" running_tasks=" + as_string( (*it)->_running_tasks.size() ) );
        }

        thread->nichts_getan();
    }

    sos_sleep( min( 30, 1 << anzahl ) );
}

//-----------------------------------------------------------------------Spooler::execute_state_cmd

bool Spooler::execute_state_cmd()
{
    bool continue_spooler = true;

    if( _state_cmd )
    {
        if( _state_cmd == sc_pause                         )  if( _state == s_running )  set_state( s_paused  ), signal_threads( "pause" );
        if( _state_cmd == sc_continue                      )  if( _state == s_paused  )  set_state( s_running ), signal_threads( "continue" );

        if( _state_cmd == sc_load_config  
         || _state_cmd == sc_reload       
         || _state_cmd == sc_terminate             
         || _state_cmd == sc_terminate_and_restart 
         || _state_cmd == sc_let_run_terminate_and_restart )
        {
            if( _state_cmd != _shutdown_cmd )
            {
                set_state( _state_cmd == sc_let_run_terminate_and_restart? s_stopping_let_run : s_stopping );
                if( _state == s_stopping )  FOR_EACH( Thread_list, _thread_list, t )  (*t)->cmd_shutdown();
                continue_spooler = false;
            }

            _shutdown_cmd = _state_cmd;
        }

        _state_cmd = sc_none;
    }

    return continue_spooler;
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    set_state( s_running );


    if( !_xml_cmd.empty() )
    {
        Command_processor cp ( this );
        cout << cp.execute( _xml_cmd, Time::now(), true );                 // Bei einem Fehler Abbruch
        _xml_cmd = "";
    }

    _single_thread = _max_threads == 1? new_thread( false ) : NULL;


    int             nothing_done_count   = 0;
    int             nothing_done_max     = _job_list.size() * 2 + 3;
    int             nichts_getan_zaehler = 0;

  //Time            throttle_time        = Time::now();
  //int             throttle_loop_count  = 0;

  //bool            log_wait = _log.log_level() <= log_debug9;
    bool            log_wait = log_category_is_set( "scheduler.wait" );


    while(1)
    {
        bool something_done = false;

        _event.reset();

        // Threads ohne Jobs und nach Fehler gestorbene Threads entfernen:
        //FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->empty() )  THREAD_LOCK( _lock )  it = _thread_list.erase(it);
        
        if( _thread_list.size() > 0 )       // Beim Start gibt es noch keinen Thread.
        {
            bool valid_thread = false;
            FOR_EACH( Thread_list, _thread_list, it )  valid_thread |= !(*it)->terminated();
            if( !valid_thread )  { _log.info( "Kein Thread vorhanden. Der Scheduler wird beendet." ); break; }
        }


        //bool continue_spooler = execute_state_cmd();
        //if( !continue_spooler )  if( !_single_thread || !_single_thread->has_tasks() )  break;
        execute_state_cmd();
        if( _shutdown_cmd )  if( !_single_thread  ||  !_single_thread->has_tasks() )  break;

/*
        if( _state == Spooler::s_paused )
        {
            _event.wait();
            if( _state_cmd != sc_none )  continue;
        }
*/

        _next_time = latter_day;
        _next_job  = NULL;

        if( _single_thread )
        {
            if( _state != Spooler::s_paused )
            {
                FOR_EACH( Process_class_list, _process_class_list, pc )
                {
                    FOR_EACH( Process_list, (*pc)->_process_list, p )
                    {
                        something_done |= (*p)->async_continue();
                        _next_time = min( _next_time, Time( localtime_from_gmtime( (*p)->async_next_gmtime() ) ) );
                    }
                }
            }

            // spooler_communication.cxx:
            _connection_manager->async_continue();

#           ifdef Z_DEBUG
                Time earliest = Time::now(); // + 0.1;
                if( _next_time < earliest ) 
                {
                    static bool logged = false;
                    if( !logged )  LOG( "spooler.cxx: async_next_gmtime() von " << _next_time << " nach " << earliest << " korrigiert\n" ); 
                    logged = true;

                    _next_time = earliest;
                }
#           endif

            //LOG( "spooler.cxx: something_done=" << something_done << "    process_list \n" );
        }

/*      Wird von _single_thread->process() erledigt, denn dort wird die Jobkettenpriorität berücksichtigt!
        FOR_EACH_JOB( j )
        {
            Job* job = *j;
            something_done |= job->do_something();
            //LOG( "spooler.cxx: something_done=" ); 
            //LOG( something_done << "  " << job->obj_name() << "\n" );
        }
*/

        string       msg;

        if( log_wait )  msg = "Warten"; //"Kein Job und keine Task aktiv";

        //_next_time = latter_day;


        if( _state != Spooler::s_paused )
        {
            if( _single_thread )
            {
                something_done |= _single_thread->process();

                //LOG( "spooler.cxx: something_done=" << something_done << "   _single_thread->process()\n" );

                if( _single_thread->is_ready_for_termination() )  break;

                Task* task = _single_thread->get_next_task();
                if( task ) 
                {
                    _next_time = min( _next_time, task->next_time() );
                    if( log_wait )  if( task )  msg = "Warten bis " + _next_time.as_string() + " für Task " + task->name();
                                        //else  msg = "Keine Task aktiv";
                }

                nothing_done_max += _single_thread->task_count() * 3 + 3;    // Statt der Prozesse zählen wir die Tasks einmal mehr
            }


            Job* job = get_next_job_to_start();

            if( job  &&  _next_time > job->next_time() )  
            {
                _next_time = job->next_time();
                if( log_wait )  msg = "Warten bis " + _next_time.as_string() + " für Job " + job->name();
            }
        }



        if( something_done )  nothing_done_count = 0,  nichts_getan_zaehler = 0;
        else
        if( ++nothing_done_count > nothing_done_max )
        {
            nichts_getan( _single_thread, ++nichts_getan_zaehler );
            // geht nicht: _next_time = max( _next_time, Time::now() + min( 30.0, double( 1 << min( 5+2, nichts_getan_zaehler ) ) / 4 ) );    // Bremsen, mit 1/4s anfangen bis 30s
            _next_time = Time::now() + 0.5;
            //LOG( "Spooler _next_time nach 'nichts getan' = " << _next_time.as_string() << "\n" );
            something_done = false;  // Damit wait_until() gerufen wird.
        }

/*
        int ___SPOOLER_IST_GEDROSSELT______SPOOLER_IST_GEDROSSELT______SPOOLER_IST_GEDROSSELT___;
        if( ++throttle_loop_count > 1000 )
        {
#           ifndef Z_WINDOWS
                LOG( "Scheduler wird gedrosselt... something_done=" << something_done << " _next_time=" << _next_time.as_string() << "\n\n" );
                sos_sleep(0.1);                             // Erstmal alle 20 Durchläufe bremsen!
#           endif

            if( Time::now() < throttle_time + 0.1 )
            {
                LOG( "Scheduler wird gedrosselt... something_done=" << something_done << " _next_time=" << _next_time.as_string() << "\n\n" );
                sos_sleep(0.1);                         // Bei mehr als 1000 Schritten in 100ms
            }

            throttle_loop_count = 0;
            throttle_time = Time::now();
        }
*/

        if( !something_done  &&  _next_time > 0  &&  _state_cmd == sc_none  &&  _next_time > Time::now() )
        {
            //_next_time = min( _next_time, now + 10.0 );      // Wartezeit vorsichtshalber begrenzen

            Wait_handles wait_handles ( this, &_log );
            
            if( _state != Spooler::s_paused )
            {
                if( _single_thread )  wait_handles += _single_thread->_wait_handles;

#               ifdef SYSTEM_WIN
                    FOR_EACH( Process_class_list, _process_class_list, pc )
                    {
                        FOR_EACH( Process_list, (*pc)->_process_list, p )
                        {
                            object_server::Connection_to_own_server* server = dynamic_cast<object_server::Connection_to_own_server*>( +(*p)->_connection );
                            if( server  &&  server->_process_handle )  wait_handles.add_handle( server->_process_handle );        // Signalisiert Prozessende
                        }
                    }

#               endif
            }

#           ifdef SYSTEM_WIN
                // Events für spooler_communication.cxx
                vector<z::Event*> events;
                _connection_manager->get_events( &events );
                FOR_EACH( vector<z::Event*>, events, e )  wait_handles.add( *e );
#           endif

            wait_handles += _wait_handles;
            if( !wait_handles.signaled() )
            {
                _wait_counter++;

                if( log_wait )  
                {
                    if( !wait_handles.wait(0) )  { LOG( msg << "\n" ); wait_handles.wait_until( _next_time ); }    // Debug-Ausgabe der Wartezeit nur, wenn kein Ergebnis vorliegt
                }
                else
                {
                    wait_handles.wait_until( _next_time );
                }
            }

            wait_handles.clear();
        }
        else
        {
            // spooler_communication.cxx:
            _connection_manager->wait( 0.0 );       // select() rufen, damit die Signale der Kommunikations-Sockets gesetzt werden.
        }

        _next_time = 0;

        if( ctrl_c_pressed && _state != s_stopping )
        {
            _log.warn( "Abbruch-Signal (Ctrl-C) empfangen. Der Scheduler wird beendet.\n" );
            _state_cmd = sc_terminate;
        }

        _loop_counter++;
    }
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

void Spooler::cmd_stop()
{
    _state_cmd = sc_stop;
    signal( "stop" );
}

//---------------------------------------------------------------------------Spooler::cmd_terminate
// Anderer Thread

void Spooler::cmd_terminate()
{
    //_log.msg( "Spooler::cmd_terminate" );

    _state_cmd = sc_terminate;
    signal( "terminate" );
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart
// Anderer Thread

void Spooler::cmd_terminate_and_restart()
{
    _state_cmd = sc_terminate_and_restart;
    signal( "terminate_and_restart" );
}

//-------------------------------------------------------Spooler::cmd_let_run_terminate_and_restart
// Anderer Thread

void Spooler::cmd_let_run_terminate_and_restart()
{
    _state_cmd = sc_let_run_terminate_and_restart;
    signal( "let_run_terminate_and_restart" );
}

//--------------------------------------------------------------------------------abort_immediately

void Spooler::abort_immediately( bool restart )
{
    int exit_code = 99;


    for( int i = 0; i < NO_OF( _process_handles ); i++ )
    {
        if( _process_handles[i] )
        {
#           ifdef Z_WINDOWS
                LOG( "TerminateProcess(" << as_hex_string( (int)_process_handles[i] ) << ",exit_code)\n" );
                TerminateProcess( _process_handles[i], exit_code );
#            else
                LOG( "kill(" << _process_handles[i] << ",SIGKILL)\n" );
                kill( _process_handles[i], SIGKILL );
#           endif
        }
    }

    if( restart )
    {
        try{ spooler_restart( NULL, is_service() ); } catch(...) {}
    }

    try{ _log.close(); } catch(...){}


    // Point of no return

#   ifdef Z_WINDOWS
        LOG( "TerminateProcess( GetCurrentProcess() );\n" );
        TerminateProcess( GetCurrentProcess(), exit_code );
        _exit( exit_code );
#    else
        LOG( "kill( _spooler->_pid, SIGKILL );\n" );
        kill( _pid, SIGKILL );
        _exit( exit_code );
#   endif
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

#   ifdef Z_WINDOWS
        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#   endif

    //spooler_is_running = true;

    _event.set_name( "Scheduler" );
    _event.set_waiting_thread_id( current_thread_id() );
    _event.create();
    _event.add_to( &_wait_handles );

    _communication.init();  // Für Windows
    //_communication.bind();  // Falls der Port belegt ist, gibt's hier einen Abbruch

    do
    {
        if( _state_cmd != sc_load_config )  load();

        THREAD_LOCK( _lock )  
        {
            if( _config_element_to_load == NULL )  throw_xc( "SCHEDULER-116", _spooler_id );

            load_config( _config_element_to_load, _config_element_mod_time, _config_source_filename );

            _config_element_to_load = NULL;
            _config_document_to_load = NULL;
        }

        if( _send_cmd != "" )  { send_cmd();  return 0; }

        start();

        try
        {
            run();
        }
        catch( exception& )
        {
            try { stop(); } catch( exception& x ) { _log.error( x.what() ); }
            throw;
        }

        stop();

    } while( _state_cmd == sc_reload || _state_cmd == sc_load_config );


    _log.info( "Scheduler ordentlich beendet." );

    //if( _pid_filename != "" )  unlink( _pid_filename.c_str() );


    rc = 0;

    //spooler_is_running = false;
    return rc;
}

//------------------------------------------------------------------------Spooler::send_error_email

void Spooler::send_error_email( const string& subject, const string& text )
{
    try
    {
        Sos_ptr<mail::Message> msg = mail::create_message();

        if( _log_mail_from != ""  &&  _log_mail_from != "-" )  msg->set_from( _log_mail_from );
        if( _log_mail_to   != ""  &&  _log_mail_to   != "-" )  msg->set_to  ( _log_mail_to   );
        if( _log_mail_cc   != ""  &&  _log_mail_cc   != "-" )  msg->set_cc  ( _log_mail_cc   );
        if( _log_mail_bcc  != ""  &&  _log_mail_bcc  != "-" )  msg->set_bcc ( _log_mail_bcc  );
        if( _smtp_server   != ""  &&  _smtp_server   != "-" )  msg->set_smtp( _smtp_server   );

        msg->add_header_field( "X-SOS-Spooler", "" );
        msg->set_subject( remove_password( subject ) );
        msg->set_body( remove_password( text ) );
        msg->send(); 
    }
    catch( const exception& x ) 
    {
        _log.warn( "Fehler beim eMail-Versand: " + string(x.what()) );
    }
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
            if( command_line.length() == 0 )  throw_xc( "SCHEDULER-COMMANDLINE" );
            if( command_line[0] == '"' ) {
                pos = command_line.find( '"', 1 );  if( pos == string::npos )  throw_xc( "SCHEDULER-COMMANDLINE" );
                pos++;                
            } else {
                pos = command_line.find( ' ' );  if( pos == string::npos )  throw_xc( "SCHEDULER-COMMANDLINE" );
            }

            command_line = new_spooler + command_line.substr(pos);
                         //+ " -renew-spooler=" + quoted_string(this_spooler);
        }
        else
        {
            //command_line += " -renew-spooler";
        }

        if( is_service )  command_line += " -renew-service";

        command_line += " -renew-spooler=" + quoted_string(this_spooler,'"','"');
        if( log )  log->info( "Restart Scheduler " + command_line );
        start_process( command_line );

#   else

        switch( fork() )
        {
            case  0:
            {
                 int n = sysconf( _SC_OPEN_MAX );
                 for( int i = 3; i < n; i++ )  close(i);
                 execv( _argv[0], _argv ); 
                 fprintf( stderr, "Fehler bei execv %s: %s\n", _argv[0], strerror(errno) ); 
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
                else  start_process( quoted_string(renew_spooler,'"','"') + " " + command_line );
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

            msg = "errno=" + as_string(errno) + ' ' + strerror(errno) + '\n';
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

    Ole_initialize ole;
    Spooler my_spooler;

    my_spooler._is_service = spooler::is_daemon;

    try
    {
        ret = my_spooler.launch( argc, argv, parameter_line );
    }
    catch( const exception& x )
    {
        SHOW_ERR( "Fehler " << x.what() );     // Fehlermeldung vor ~Spooler ausgeben
        if( my_spooler.is_service() )  send_error_email( x.what(), argc, argv, parameter_line, &my_spooler );
        ret = 1;
    }

    return ret;
}

//------------------------------------------------------------------------------------object_server

int object_server( int argc, char** argv )
{
    zschimmer::com::object_server::Server server;

    server.register_class( spooler_com::CLSID_Remote_module_instance_server, Com_remote_module_instance_server::create_instance );
    server.register_class(              CLSID_Com_log_proxy                , Com_log_proxy                    ::create_instance );

    return server.main( argc, argv, true );
}
                   
//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv, const string& parameter_line )
{
    LOG( "Scheduler " VER_PRODUCTVERSION_STR "\n" );

    int  ret                = 99;
    bool is_service         = false;
    bool is_object_server   = false;

#   ifdef Z_WINDOWS
        SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );    // Das System soll sich Messageboxen verkneifen (außer beim Absturz)
#   endif

    set_log_category_default( "scheduler.call", true );   // Aufrufe von spooler_process() etc. protokollieren (Beginn und Ende)

    spooler::error_settings.read( spooler::default_factory_ini );

    try
    {
        bool    do_install_service = false;
        bool    do_remove_service  = false;
        bool    is_service_set     = false;
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
            if( opt.with_value( "sos.ini"          ) )  ;  //schon in sos_main0() geschehen.  set_sos_ini_filename( opt.value() );
            else
            //if( opt.flag      ( "renew-spooler"    ) )  renew_spooler = program_filename();
          //else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "send-cmd"         ) )  send_cmd = opt.value();
            else
          //if( opt.with_value( "execute-job"      ) )  is_object_server = true;       // Parameter ist nur für den Befehl ps
            if( opt.flag      ( 'O', "object-server" ) )  is_object_server = true;
            else
            if( opt.with_value( "title"            ) )  ;                               // Damit der Aufrufer einen Kommentar für ps übergeben kann (für -object-server)
            else
            if( opt.flag      ( "V"                ) )  fprintf( stderr, "Scheduler %s\n", VER_PRODUCTVERSION_STR );
            else
            {
                if( opt.flag      ( "install-service"  ) )  do_install_service = opt.set();
                else
                if( opt.with_value( "install-service"  ) )  do_install_service = true, service_name = opt.value();
                else
                if( opt.flag      ( "remove-service"   ) )  do_remove_service = opt.set();
                else
                if( opt.flag      ( "renew-service"    ) )  renew_service = opt.set();
                else
                if( opt.with_value( "service-name"     ) )  service_name = opt.value();
                else
                if( opt.with_value( "service-display"  ) )  service_display = opt.value();
                else
                if( opt.with_value( "service-descr"    ) )  service_description = opt.value();
                else
                if( opt.flag      ( "service"          ) )  is_service = opt.set(), is_service_set = true;
                else
                if( opt.with_value( "service"          ) )  is_service = true, is_service_set = true, service_name = opt.value();
                else
                if( opt.with_value( "need-service"     ) )  dependencies += opt.value(), dependencies += '\0';
                else
                {
                    if( opt.with_value( "id"               ) )  id = opt.value();
                    else
                    if( opt.with_value( "ini"              ) )  factory_ini = opt.value(), spooler::error_settings.read( factory_ini );
                    else
                    if( opt.with_value( "log"              ) )  log_filename = opt.value();

                    if( !command_line.empty() )  command_line += " ";
                    command_line += opt.complete_parameter( '"', '"' );
                }
            }
        }

        if( send_cmd != "" )  is_service = false;

        //Z_DEBUG_ONLY( MessageBox( 0, "spooler", "spooler -object-server", 0 ) );

        if( log_filename.empty() )  log_filename = subst_env( read_profile_string( factory_ini, "spooler", "log" ) );
        if( !log_filename.empty() )  log_start( log_filename );
/*
        if( log_category_is_set( "scheduler.cat" ) )
        {
            int n = 1000000;
            LOG( n << " mal log_category_is_set( \"scheduler.cat\" ) ...\n" );
            const char* cat = "scheduler.cat";
            for( int i = 0; i < n; i++ )  log_category_is_set( cat );
            LOG( "... fertig\n" );
        }
*/

        if( is_object_server )
        {
            ret = spooler::object_server( argc, argv );
        }
        else
        {

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
#           endif

#           ifdef Z_WINDOWS

                if( !renew_spooler.empty() )  
                { 
                    spooler::spooler_renew( service_name, renew_spooler, renew_service, command_line ); 
                    ret = 0;
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
                    ret = 0;
                }
                else
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

                if( is_service )
                {
                    spooler::is_daemon = true;

                    LOG( "Scheduler wird Daemon. Pid wechselt\n");
                    spooler::be_daemon();
                }

                ret = spooler::spooler_main( argc, argv, command_line );

#           endif
        }
    }
    catch( const exception& x )
    {
        LOG( x.what() << "\n" );
        if( is_service )  spooler::send_error_email( x.what(), argc, argv, parameter_line );
        cerr << x << "\n";
        ret = 1;
    }
    catch( const _com_error& x )
    {
        string what = string_from_ole( x.Description() );
        LOG( what << "\n" );
        if( is_service )  spooler::send_error_email( what, argc, argv, parameter_line );
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

    return sos::spooler_main( argc, argv, "" );
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
