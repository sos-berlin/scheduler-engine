// $Id: spooler.cxx,v 1.204 2003/05/23 06:26:12 jz Exp $
/*
    Hier sind implementiert

    Script_instance
    Spooler
    spooler_main()
    sos_main()
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


namespace sos {

extern const Bool _dll = false;

namespace spooler {

const char* default_factory_ini  = "factory.ini";
const string new_suffix          = "~new";  // Suffix für den neuen Spooler, der den bisherigen beim Neustart ersetzen soll
const double renew_wait_interval = 0.25;
const double renew_wait_time     = 30;      // Wartezeit für Brückenspooler, bis der alte Spooler beendet ist und der neue gestartet werden kann.
const double wait_for_thread_termination                 = latter_day;  // Haltbarkeit des Geduldfadens
const double wait_step_for_thread_termination            = 5.0;         // 1. Nörgelabstand
const double wait_step_for_thread_termination2           = 600.0;       // 2. Nörgelabstand
//const double wait_for_thread_termination_after_interrupt = 1.0;


static bool                     is_daemon               = false;
//static int                      daemon_starter_pid;
//bool                          spooler_is_running      = false;
volatile int                    ctrl_c_pressed          = 0;
Spooler*                        spooler_ptr             = NULL;


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

static void send_error_email( const string& subject, const string& body )
{
    try
    {
        string from = read_profile_string( default_factory_ini, "spooler", "log_mail_from"   );
        string to   = read_profile_string( default_factory_ini, "spooler", "log_mail_to"     );
        string cc   = read_profile_string( default_factory_ini, "spooler", "log_mail_cc"     );
        string bcc  = read_profile_string( default_factory_ini, "spooler", "log_mail_bcc"    );
        string smtp = read_profile_string( default_factory_ini, "spooler", "smtp"            );

        Sos_ptr<mail::Message> msg = mail::create_message(); // spooler_ptr->_java_vm );

        if( from != "" )  msg->set_from( from );
        if( to   != "" )  msg->set_to  ( to   );
        if( cc   != "" )  msg->set_cc  ( cc   );
        if( bcc  != "" )  msg->set_bcc ( bcc  );
        if( smtp != "" )  msg->set_smtp( smtp );

        msg->add_header_field( "X-SOS-Spooler", "" );
        msg->set_subject( subject );
        msg->set_body( body );
        msg->send(); 
    }
    catch( const exception& ) {}
}

//---------------------------------------------------------------------------------send_error_email

void send_error_email( const string& error_text, int argc, char** argv, Spooler* spooler )
{

    string body = "Der Spooler-Dienst konnte nicht gestartet werden.\n"
                  "\n"
                  "\n"
                  "Der Aufruf war:\n"
                  "\n";
                   
    for( int i = 0; i < argc; i++ )  body += argv[i], body += ' ';

    body += "\n\n\n"
            "Fehlermeldung:\n";
    body += error_text;

    string subject = "FEHLER BEI SPOOLER-START: " + error_text;

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
        if( isdigit( (uint)v[0] ) )  return as_int(v);
                               else  return as_bool(v);
    }
    catch( const Xc& ) { return deflt; }
}

//------------------------------------------------------------------read_profile_history_on_process

int read_profile_history_on_process( const string& profile, const string& section, const string& entry, int deflt )
{
    string result;
    string v = read_profile_string( profile, section, entry );

    if( v == "" )  return deflt;

    try
    {
        if( isdigit( (uint)v[0] ) )  return as_int(v);
                               else  return as_bool(v);
    }
    catch( const Xc& ) { return deflt; }
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
            fprintf( stderr, "Spooler wird wegen Ctrl-C beendet ...\n" );
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
            if( !is_daemon )  fprintf( stderr, "Spooler wird wegen kill -%d beendet ...\n", sig );

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
    LOG( "set_ctrl_c_handler(" << on << ")\n" );

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
                 LOG( "setsid()\n" );
                 setsid(); 
                 break;

        case -1: throw_errno( errno, "fork" );

        default: sleep(1);  // Falls der Daemon noch was ausgibt, sollte das vor dem Shell-Prompt sein.
                 //fprintf( stderr, "Daemon gestartet. pid=%d\n", pid ); 
                 //fflush( stderr );
                 _exit(0);
    }
}

#endif
//---------------------------------------------------------------------------------Spooler::Spooler

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

    Z_GNU_ONLY( time::empty_period = Period() );

    _pid = getpid();

    _tcp_port = 4444;
    _udp_port = 4444;
    _priority_max = 1000;       // Ein Wert > 1, denn 1 ist die voreingestelle Priorität der Jobs
            

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


    set_ctrl_c_handler( true );
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

    _spooler_thread_list.clear();


    _object_set_class_list.clear();

    _communication.close(0.0);
    _security.clear();

    _event.close();
    _wait_handles.close();

    //nicht nötig  Z_FOR_EACH( Job_chain_map, _job_chain_map, it )  it->second->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->close();
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

//--------------------------------------------------------------Spooler::wait_until_threads_stopped

void Spooler::wait_until_threads_stopped( Time until )
{
    assert( current_thread_id() == _thread_id );


#   ifdef Z_WINDOWS

        Wait_handles wait_handles ( this, &_log );

        Thread_list::iterator it = _thread_list.begin();
        while( it != _thread_list.end() )
        {
            Spooler_thread* thread = *it;
            if( thread->_free_threading  &&  !thread->_terminated  &&  thread->_thread_handle.valid() )  wait_handles.add( &(*it)->_thread_handle );
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

                    if( !thread->_terminated )
                    {
                        string msg = "Warten auf Thread " + thread->name() + " [" + thread->thread_as_text() + "]";
                        Job* job = thread->_current_job;
                        if( job )  msg += ", Job " + job->name() + " " + job->job_state();
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

            FOR_EACH( Thread_list, threads, it )  
            {
                Spooler_thread* thread = *it;
                if( thread->_terminated )
                {
                    _log.debug( "Thread " + thread->name() + " sollte gleich beendet sein ..." );
                    thread->thread_wait_for_termination();

                    _log.info( "Thread " + thread->name() + " beendet" );
                    it = threads.erase( it );
                }
                else
                    LOG( "Thread " << thread->name() << " läuft noch\n" );
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
    if( !thread )  throw_xc( "SPOOLER-128", thread_name );

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
    if( !c )  throw_xc( "SPOOLER-101", name );
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

//---------------------------------------------------------------------------------Spooler::get_job
// Anderer Thread

Job* Spooler::get_job( const string& job_name )
{
    Job* job = get_job_or_null( job_name );
    if( !job  ||  !job->state() )  throw_xc( "SPOOLER-108", job_name );
    return job;
}

//-------------------------------------------------------------------------Spooler::get_job_or_null
// Anderer Thread

Job* Spooler::get_job_or_null( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )
        {
            Job* job = (*it)->get_job_or_null( job_name );
            if( job )  return job;
        }
    }

    return NULL;
}

//---------------------------------------------------------------------Spooler::thread_by_thread_id

Spooler_thread* Spooler::thread_by_thread_id( Thread_id id )                    
{     
    Thread_id_map::iterator it;

    THREAD_LOCK( _thread_id_map_lock )  it = _thread_id_map.find(id); 

    return it != _thread_id_map.end()? it->second : NULL; 
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

//---------------------------------------------------------------------------Spooler::start_threads

void Spooler::start_threads()
{
    FOR_EACH( Thread_list, _thread_list, it )  
    {
        Spooler_thread* thread = *it;

        if( !thread->empty() ) 
        {
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
}

//---------------------------------------------------------------------------Spooler::close_threads

void Spooler::close_threads()
{
    signal_threads( "stop" );

    wait_until_threads_stopped( latter_day );

/*  Wir müssen warten, bis alle Threads beendet sind, denn sie benutzen _spooler. Also: Kein Timeout!
    wait_until_threads_stopped( Time::now() + wait_for_thread_termination );

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

//-----------------------------------------------------------------------------Spooler::run_threads

bool Spooler::run_threads()
{
    bool something_done = false;
    Time now            = Time::now();

    FOR_EACH( Spooler_thread_list, _spooler_thread_list, it )  
    {
        if( _state_cmd != sc_none )  break;

        Spooler_thread* thread = *it;
        if( thread->_next_start_time <= now  ||  thread->_wait_handles.signaled() )
        {
            bool ok = thread->process();

            something_done |= ok;

            //if( !ok )  thread->get_next_job_to_start();
        }

        //thread->_log.debug9( "_next_start_time=" + thread->_next_start_time.as_string() );
        //if( thread->_next_job  &&  _next_time > thread->_next_start_time )  _next_time = thread->_next_start_time, _next_job = thread->_next_job;
    }

    return something_done;
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

    for( Sos_option_iterator opt ( _argc, _argv ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "ini" ) )  _factory_ini = opt.value();
    }


    string log_level = as_string( _log_level );

    _spooler_id         = read_profile_string    ( _factory_ini, "spooler", "id"                 );
    _config_filename    = read_profile_string    ( _factory_ini, "spooler", "config"             );
    _log_directory      = read_profile_string    ( _factory_ini, "spooler", "log-dir"            );  // veraltet
    _log_directory      = read_profile_string    ( _factory_ini, "spooler", "log_dir"            , _log_directory );  _log_directory_as_option_set = !_log_directory.empty();
    _include_path       = read_profile_string    ( _factory_ini, "spooler", "include-path"       );  // veraltet
    _include_path       = read_profile_string    ( _factory_ini, "spooler", "include_path"       , _include_path );   _include_path_as_option_set  = !_include_path.empty();
    _spooler_param      = read_profile_string    ( _factory_ini, "spooler", "param"              );                   _spooler_param_as_option_set = !_spooler_param.empty();
    log_level           = read_profile_string    ( _factory_ini, "spooler", "log_level"          , log_level );   
    _history_columns    = read_profile_string    ( _factory_ini, "spooler", "history_columns"    );
    _history_yes        = read_profile_bool      ( _factory_ini, "spooler", "history"            , true );
    _history_on_process = read_profile_history_on_process( _factory_ini, "spooler", "history_on_process", 0 );
    _history_archive    = read_profile_archive   ( _factory_ini, "spooler", "history_archive"    , arc_no );
    _history_with_log   = read_profile_with_log  ( _factory_ini, "spooler", "history_with_log"   , arc_no );
    _db_name            = read_profile_string    ( _factory_ini, "spooler", "db"                 );
    _need_db            = read_profile_bool      ( _factory_ini, "spooler", "need_db"            , true                );
    _history_tablename  = read_profile_string    ( _factory_ini, "spooler", "db_history_table"   , "SPOOLER_HISTORY"   );
    _variables_tablename= read_profile_string    ( _factory_ini, "spooler", "db_variables_table" , "SPOOLER_VARIABLES" );

    _java_vm->set_filename      ( subst_env( read_profile_string( _factory_ini, "java"   , "vm"         , _java_vm->filename()       ) ) );
    _java_vm->prepend_class_path( subst_env( read_profile_string( _factory_ini, "java"   , "class_path" ) ) );
    _java_vm->set_javac_filename( subst_env( read_profile_string( _factory_ini, "java"   , "javac"      , _java_vm->javac_filename() ) ) );


    try
    {
        for( Sos_option_iterator opt ( _argc, _argv ); !opt.end(); opt.next() )
        {
            if( opt.flag      ( "V"                ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.flag      ( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "log"              ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "pid-file"         ) )  _pid_filename = opt.value();
            else
            if( opt.with_value( "ini"              ) )  ;   //
            else
            if( opt.with_value( "config"           )
             || opt.param(1)                         )  _config_filename = opt.value();
            else
            if( opt.with_value( "cd"               ) )  { string dir = opt.value(); if( chdir( dir.c_str() ) )  throw_errno( errno, "chdir", dir.c_str() ); }
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
            if( opt.with_value( "send-cmd"         ) )  _send_cmd = opt.value();
            else
                throw_sos_option_error( opt );
        }

        _temp_dir = read_profile_string( _factory_ini, "spooler", "tmp", get_temp_path() + Z_DIR_SEPARATOR "spooler" );
        _temp_dir = replace_regex( _temp_dir, "[\\/]+", Z_DIR_SEPARATOR );
        _temp_dir = replace_regex( _temp_dir, "\\" Z_DIR_SEPARATOR "$", "" );
        if( _spooler_id != "" )  _temp_dir += Z_DIR_SEPARATOR + _spooler_id;

        _manual = !_job_name.empty();
        if( _manual  &&  _log_directory.empty() )  _log_directory = "*stderr";

        _log_level = make_log_level( log_level );

        if( _log_level <= log_debug_spooler )  _debug = true;
        if( _config_filename.empty() )  throw_xc( "SPOOLER-115" );

        _java_work_dir = temp_dir() + Z_DIR_SEPARATOR "java";
        _java_vm->prepend_class_path( _java_work_dir );
        make_path( _java_work_dir );  // Verzeichnis muss beim Start von Java vorhanden sein, damit Java es in classpath berücksichtigt.
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
    _java_vm = get_java_vm( false );  //Z_NEW( java::Vm( false ) );

    load_arg();

    if( _pid_filename != "" )
    {
        File f ( _pid_filename, "w" );
        f.print( as_string( getpid() ) );
        f.print( "\n" );
        f.close();
    }

    _log.init( this );

    char hostname[200];  // Nach _communication.init() und nach _prefix_log.init()!
    if( gethostname( hostname, sizeof hostname ) == SOCKET_ERROR )  hostname[0] = '\0',  _log.warn( string("gethostname(): ") + strerror( errno ) );
    _hostname = hostname;

    Command_processor cp ( this );
    cp.execute_file( _config_filename );
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    assert( current_thread_id() == _thread_id );

    _mail_on_error   = read_profile_bool           ( _factory_ini, "spooler", "mail_on_error"  , _mail_on_error );
    _mail_on_process = read_profile_mail_on_process( _factory_ini, "spooler", "mail_on_process", _mail_on_process );
    _mail_on_success = read_profile_bool           ( _factory_ini, "spooler", "mail_on_success", _mail_on_success );
    _mail_queue_dir  = read_profile_string         ( _factory_ini, "spooler", "mail_queue_dir" , _mail_queue_dir );
    _mail_encoding   = read_profile_string         ( _factory_ini, "spooler", "mail_encoding"  , "base64"        );      // "quoted-printable": Jmail braucht 1s pro 100KB dafür
    _smtp_server     = read_profile_string         ( _factory_ini, "spooler", "smtp"           , _smtp_server );

    _log_mail_from      = read_profile_string( _factory_ini, "spooler", "log_mail_from"   );
    _log_mail_to        = read_profile_string( _factory_ini, "spooler", "log_mail_to"     );
    _log_mail_cc        = read_profile_string( _factory_ini, "spooler", "log_mail_cc"     );
    _log_mail_bcc       = read_profile_string( _factory_ini, "spooler", "log_mail_bcc"    );
    _log_mail_subject   = read_profile_string( _factory_ini, "spooler", "log_mail_subject");
    _log_collect_within = read_profile_uint  ( _factory_ini, "spooler", "log_collect_within", 0 );
    _log_collect_max    = read_profile_uint  ( _factory_ini, "spooler", "log_collect_max"   , 900 );

    _state_cmd = sc_none;
    set_state( s_starting );

    _base_log.set_directory( _log_directory );
    _base_log.open_new();
    
    _log.info( string( "Spooler (" VER_PRODUCTVERSION_STR ) + ") startet mit " + _config_filename );


    if( !_manual )  _communication.start_or_rebind();

    if( _has_java  ) 
    {
        try
        {
            init_java_vm();    // In spooler_module_java.cxx
        }
        catch( const exception& x )
        {
            _log.error( x.what() );
            _log.error( "Java kann nicht gestartet werden. Spooler startet ohne Java." );
        }
    }


    _db = SOS_NEW( Spooler_db( this ) );
    _db->open( _db_name );
    _db->spooler_start();

    set_ctrl_c_handler( false );
    set_ctrl_c_handler( true );       // Falls Java (über Dateityp jdbc) gestartet worden ist und den Signal-Handler verändert hat


    _spooler_start_time = Time::now();


    _spooler_thread_list.clear();

    FOR_EACH( Thread_list, _thread_list, it )
    {
        Spooler_thread* thread = *it;
        if( !thread->_free_threading )  _spooler_thread_list.push_back( thread );
        if( !thread->empty() )  thread->init();
    }


    if( _module.set() )
    {
        _module_instance = _module.create_instance();
        _module_instance->init();

        _module_instance->add_obj( (IDispatch*)_com_spooler, "spooler"     );
        _module_instance->add_obj( (IDispatch*)_com_log    , "spooler_log" );

        _module_instance->load();
        _module_instance->start();

        bool ok = check_result( _module_instance->call_if_exists( "spooler_init()Z" ) );
        if( !ok )  throw_xc( "SPOOLER-183" );
    }

    start_threads();

    
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

    set_state( _state_cmd == sc_let_run_terminate_and_restart? s_stopping_let_run : s_stopping );

    //_log.msg( "Spooler::stop" );

    close_threads();

    FOR_EACH( Thread_list, _thread_list, it )  it = _thread_list.erase( it );


/*  interrupt() lässt PerlScript abstürzen
    FOR_EACH( Thread_list, _thread_list, it )
    {
        Job* job = (*it)->current_job();
        if( job )  try { job->interrupt_script(); } catch(const Xc& x){_log.error(x.what());}
    }

    wait_until_threads_stopped( Time::now() + wait_for_thread_termination_after_interrupt );
*/
    _object_set_class_list.clear();
    _spooler_thread_list.clear();
    _thread_list.clear();

    if( _module_instance )  _module_instance->close();

    //_java_vm.close();  Erneutes _java.init() stürzt ab, deshalb lassen wird Java stehen und schließen es erst am Schluss

    if( _state_cmd == sc_terminate_and_restart 
     || _state_cmd == sc_let_run_terminate_and_restart )  spooler_restart( &_base_log, _is_service );

    _db->spooler_stop();
    _db->close();
    _db = NULL;

    set_state( s_stopped );     
    // Der Dienst ist hier beendet
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    assert( current_thread_id() == _thread_id );

    set_state( s_running );

    while(1)
    {
        // Threads ohne Jobs und nach Fehler gestorbene Threads entfernen:
        //FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->empty() )  THREAD_LOCK( _lock )  it = _thread_list.erase(it);
        bool valid_thread = false;
        FOR_EACH( Thread_list, _thread_list, it )  valid_thread |= !(*it)->_terminated;
        if( !valid_thread )  { _log.info( "Kein Thread vorhanden. Spooler wird beendet." ); break; }

        if( _state_cmd == sc_pause                 )  if( _state == s_running )  set_state( s_paused  ), signal_threads( "pause" );
        if( _state_cmd == sc_continue              )  if( _state == s_paused  )  set_state( s_running ), signal_threads( "continue" );
        if( _state_cmd == sc_load_config           )  break;
        if( _state_cmd == sc_reload                )  break;
        if( _state_cmd == sc_terminate             )  break;
        if( _state_cmd == sc_terminate_and_restart )  break;
        if( _state_cmd == sc_let_run_terminate_and_restart )  break;
        _state_cmd = sc_none;


        if( _state == Spooler::s_paused )
        {
            //_wait_handles.wait_until( latter_day );
            _event.wait();
        }

        _next_time = latter_day;
        _next_job  = NULL;


        //while( _state_cmd == sc_none  &&  !ctrl_c_pressed )    // Solange Jobs laufen, keine Umstände machen. 
        {
            bool something_done = run_threads();
        //    if( !something_done )  break;
        }

        int running_tasks_count = 0;
        FOR_EACH( Spooler_thread_list, _spooler_thread_list, it2 )  running_tasks_count += (*it2)->_running_tasks_count;

        //LOG( "spooler: running_tasks_count=" << running_tasks_count  << " _state_cmd=" << (int)_state_cmd << " ctrl_c_pressed=" << ctrl_c_pressed << " _next_time=" << _next_time << "\n" );
        if( running_tasks_count == 0  &&  _state_cmd == sc_none  &&  !ctrl_c_pressed )
        {

            FOR_EACH( Spooler_thread_list, _spooler_thread_list, it )  
            {
                Spooler_thread* thread = *it;
                thread->get_next_job_to_start();
                //thread->_log.debug9( "_next_start_time=" + thread->_next_start_time.as_string() );
                if( thread->_next_job  &&  _next_time > thread->_next_start_time )  _next_time = thread->_next_start_time, _next_job = thread->_next_job;
            }

            Time now = Time::now();

            if( _next_time > now )
            {
                string msg;
                
                Wait_handles wait_handles = _wait_handles;
                FOR_EACH( Spooler_thread_list, _spooler_thread_list, it )  wait_handles += (*it)->_wait_handles;

                if( _debug )  
                {
                    msg = _next_job? "Warten bis " + _next_time.as_string() + " für Job " + _next_job->name() 
                                   : "Kein Job zu starten";

                    //msg += " und " + wait_handles.as_string();

                    if( wait_handles.wait(0) == -1 )  _log.debug( msg ); //, wait_handles.wait_until( _next_time );
                }

                wait_handles.wait_until( _next_time );
                wait_handles.clear();
            }
        }


        _event.reset();

        if( ctrl_c_pressed )  _state_cmd = sc_terminate;
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

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv )
{
    int rc;

    if( !SOS_LICENCE( licence_spooler ) )  throw_xc( "SOS-1000", "Spooler" );

    _argc = argc;
    _argv = argv;

    tzset();

    _thread_id = current_thread_id();

#   ifdef Z_WINDOWS
        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#   endif

    //spooler_is_running = true;

    _event.set_name( "Spooler" );
    _event.create();
    _event.add_to( &_wait_handles );

    _communication.init();  // Für Windows
    //_communication.bind();  // Falls der Port belegt ist, gibt's hier einen Abbruch

    do
    {
        if( _state_cmd != sc_load_config )  load();

        THREAD_LOCK( _lock )  
        {
            if( _config_element_to_load == NULL )  throw_xc( "SPOOLER-116", _spooler_id );

            load_config( _config_element_to_load, _config_element_mod_time, _config_source_filename );

            _config_element_to_load = NULL;
            _config_document_to_load = NULL;
        }

        if( _send_cmd != "" )  { send_cmd();  return 0; }

        start();
        run();
        stop();

    } while( _state_cmd == sc_reload || _state_cmd == sc_load_config );


    //_java_vm->close();

    _log.info( "Spooler ordentlich beendet." );

    if( _pid_filename != "" )  unlink( _pid_filename.c_str() );


    rc = 0;

    //spooler_is_running = false;
    return rc;
}

//------------------------------------------------------------------------Spooler::send_error_email

void Spooler::send_error_email( const string& subject, const string& body )
{
    try
    {
        Sos_ptr<mail::Message> msg = mail::create_message(); // spooler_ptr->_java_vm );

        if( _log_mail_from != ""  &&  _log_mail_from != "-" )  msg->set_from( _log_mail_from );
        if( _log_mail_to   != ""  &&  _log_mail_to   != "-" )  msg->set_to  ( _log_mail_to   );
        if( _log_mail_cc   != ""  &&  _log_mail_cc   != "-" )  msg->set_cc  ( _log_mail_cc   );
        if( _log_mail_bcc  != ""  &&  _log_mail_bcc  != "-" )  msg->set_bcc ( _log_mail_bcc  );
        if( _smtp_server   != ""  &&  _smtp_server   != "-" )  msg->set_smtp( _smtp_server   );

        msg->add_header_field( "X-SOS-Spooler", "" );
        msg->set_subject( subject );
        msg->set_body( body );
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
            if( command_line.length() == 0 )  throw_xc( "SPOOLER-COMMANDLINE" );
            if( command_line[0] == '"' ) {
                pos = command_line.find( '"', 1 );  if( pos == string::npos )  throw_xc( "SPOOLER-COMMANDLINE" );
                pos++;                
            } else {
                pos = command_line.find( ' ' );  if( pos == string::npos )  throw_xc( "SPOOLER-COMMANDLINE" );
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
        if( log )  log->info( "Restart Spooler  " + command_line );
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
            catch( const Xc& x ) { 
                if( !is_service )  fprintf( stderr, "%s\n", x.what() );
                LOG( x.what() << '\n' );
            }

            if( error != ERROR_SHARING_VIOLATION )  return;
            sos_sleep( renew_wait_interval );
        }

        if( !is_service )  fprintf( stderr, "Der Spooler ist ausgetauscht und wird neu gestartet\n\n" );
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

int spooler_main( int argc, char** argv )
{
    int ret;

    Ole_initialize ole;
    Spooler my_spooler;

    try
    {
        ret = my_spooler.launch( argc, argv );
    }
    catch( const Xc& x )
    {
        SHOW_ERR( "Fehler " << x );     // Fehlermeldung vor ~Spooler ausgeben
        if( my_spooler.is_service() )  send_error_email( x.what(), argc, argv, &my_spooler );
        ret = 1;
    }

    return ret;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    LOG( "Spooler " VER_PRODUCTVERSION_STR "\n" );

    int  ret        = 99;
    bool is_service = false;

    try
    {
        bool    is_service_set = false;
        bool    do_install_service = false;
        bool    do_remove_service = false;
        string  id;
        string  service_name, service_display;
        string  service_description = "Hintergrund-Jobs der Document Factory";
        string  renew_spooler;
        string  command_line;
        bool    renew_service = false;
        string  send_cmd;
        string  log_filename;
        string  factory_ini = spooler::default_factory_ini;
        string  dependencies;

        for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
        {
          //if( opt.flag      ( "renew-spooler"    ) )  renew_spooler = program_filename();
          //else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
            else
            if( opt.with_value( "send-cmd"         ) )  send_cmd = opt.value();
            else
            if( opt.flag      ( "V"                ) )  fprintf( stderr, "Spooler %s\n", VER_PRODUCTVERSION_STR );
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
                    if( opt.with_value( "ini"              ) )  factory_ini = opt.value();
                    else
                    if( opt.with_value( "log"              ) )  log_filename = opt.value();

                    if( !command_line.empty() )  command_line += " ";
                    command_line += opt.complete_parameter( '"', '"' );
                }
            }
        }

        if( send_cmd != "" )  is_service = false;


#       ifdef Z_WINDOWS
            if( service_name != "" ) 
            {
                if( service_display == "" )  service_display = service_name;
            }
            else
            {
                service_name = spooler::make_service_name(id);
                if( service_display == "" )  service_display = spooler::make_service_display(id);
            }
#       endif

        if( log_filename.empty() )  log_filename = read_profile_string( factory_ini, "spooler", "log" );
        if( !log_filename.empty() )  log_start( log_filename );

#       ifdef Z_WINDOWS

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
                    ret = spooler::spooler_main( argc, argv );
                }
            }

#        else

            if( is_service )
            {
                spooler::is_daemon = true;

                LOG( "Spooler wird Daemon. Pid wechselt \n");
                spooler::be_daemon();
            }

            ret = spooler::spooler_main( argc, argv );

#       endif
    }
    catch( const exception& x )
    {
        LOG( x.what() );
        spooler::send_error_email( x.what(), argc, argv );
        ret = 1;
    }

    LOG( "Programm wird beendet\n" );

    return ret;
}

} //namespace sos

