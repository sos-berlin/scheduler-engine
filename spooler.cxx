// $Id: spooler.cxx,v 1.70 2001/07/16 08:51:31 jz Exp $
/*
    Hier sind implementiert

    Script_instance
    Spooler
    spooler_main()
    sos_main()
*/

#include "../kram/sos.h"
#include "spooler.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../kram/log.h"
#include "../file/anyfile.h"
#include "../kram/licence.h"

//#if !defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
//#endif



namespace sos {

extern const Bool _dll = false;

namespace spooler {

const string new_suffix          = "~new";  // Suffix für den neuen Spooler, der den bisherigen beim Neustart ersetzen soll
const double renew_wait_interval = 0.1;
const double renew_wait_time     = 30;      // Wartezeit für Brückenspooler, bis der alte Spooler beendet ist und der neue gestartet werden kann.
const double wait_for_thread_termination                 = latter_day;  // Haltbarkeit des Geduldfadens
const double wait_step_for_thread_termination            = 5.0;         // Nörgelabstand
//const double wait_for_thread_termination_after_interrupt = 1.0;

/*
struct Set_console_code_page
{
                                Set_console_code_page       ( uint cp )         { _cp = GetConsoleOutputCP(); SetConsoleOutputCP( cp ); }
                               ~Set_console_code_page       ()                  { SetConsoleOutputCP( _cp ); }

    uint                       _cp;
};
*/

//---------------------------------------------------------------------------------Spooler::Spooler

Spooler::Spooler() 
: 
    _zero_(this+1), 
    _security(this),
    _communication(this), 
    _prefix_log(&_log),
    _wait_handles(this,&_prefix_log),
    _log(this)
{
    _com_log     = new Com_log( &_prefix_log );
    _com_spooler = new Com_spooler( this );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    wait_until_threads_stopped( latter_day );

    _thread_list.clear();
    _object_set_class_list.clear();

    _communication.close(0.0);
    _security.clear();

    _event.close();
    _wait_handles.close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->close();
}

//--------------------------------------------------------------------------Spooler::security_level
// Anderer Thread

Security::Level Spooler::security_level( const Host& host )
{
    THREAD_LOCK( _lock )
    {
        return _security.level( host );
    }

    __assume(0);
}


//--------------------------------------------------------------------------Spooler::threads_as_xml
// Anderer Thread

xml::Element_ptr Spooler::threads_as_xml( xml::Document_ptr document )
{
    xml::Element_ptr threads = document->createElement( "threads" );

    dom_append_nl( threads );

    THREAD_LOCK_LOG( _lock, "threads_as_xml" )
    {
        FOR_EACH( Thread_list, _thread_list, it )
        {
            threads->appendChild( (*it)->xml( document ) );
            dom_append_nl( threads );
        }
    }

    return threads;
}

//--------------------------------------------------------------Spooler::wait_until_threads_stopped

void Spooler::wait_until_threads_stopped( Time until )
{
    assert( GetCurrentThreadId() == _thread_id );

    Wait_handles wait_handles ( this, &_prefix_log );

    Thread_list::iterator it = _thread_list.begin();
    while( it != _thread_list.end() )
    {
        if( (*it)->_thread_handle.handle() )  wait_handles.add_handle( (*it)->_thread_handle.handle() ),  it++;
                   else THREAD_LOCK( _lock )  it = _thread_list.erase( it );
    }

    while( _thread_list.size() > 0 )
    {
        Time until_step = Time::now() + wait_step_for_thread_termination;
        if( until_step > until )  until_step = until;

        int index = wait_handles.wait_until( until_step );
        if( index >= 0 ) 
        {
            Thread_list::iterator thread = _thread_list.begin();
            while( index-- > 0 )  thread++;
            _log.msg( "Thread " + (*thread)->name() + " beendet" );

            wait_handles.remove_handle( (*thread)->_thread_handle.handle() );
            THREAD_LOCK( _lock )  _thread_list.erase( thread );
        }

        if( Time::now() > until )  break;

        if( index < 0 )
        {
            FOR_EACH( Thread_list, _thread_list, it )  
            {
                string msg = "Warten auf Thread " + (*it)->name();
                Job* job = (*it)->_current_job;
                if( job )  msg += ", Job " + job->name() + " " + job->job_state();
                _log.msg( msg );
            }
        }
    }
}

//--------------------------------------------------------------------------Spooler::signal_threads

void Spooler::signal_threads( const string& signal_name )
{
    assert( GetCurrentThreadId() == _thread_id );

    FOR_EACH( Thread_list, _thread_list, it )  (*it)->signal( signal_name );
}

//------------------------------------------------------------------------------Spooler::get_thread
// Anderer Thread

Thread* Spooler::get_thread( const string& thread_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->name() == thread_name )  return *it;
    }

    throw_xc( "SPOOLER-128", thread_name );
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
    if( !job )  throw_xc( "SPOOLER-108", job_name );
    return job;
}

//-------------------------------------------------------------------------Spooler::get_job_or_null
// Anderer Thread

Job* Spooler::get_job_or_null( const string& job_name )
{
    THREAD_LOCK_LOG( _lock, "Spooler::get_job_or_null" )
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

Thread* Spooler::thread_by_thread_id( Thread_id id )                    
{     
    Thread_id_map::iterator it;

    THREAD_LOCK( _thread_id_map_lock )  it = _thread_id_map.find(id); 

    return it != _thread_id_map.end()? it->second : NULL; 
}

//---------------------------------------------------------------------------Spooler::signal_object
// Anderer Thread

void Spooler::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK_LOG( _lock, "Spooler::signal_object" )  FOR_EACH( Thread_list, _thread_list, t )  (*t)->signal_object( object_set_class_name, level );
}

//-------------------------------------------------------------------------------Spooler::set_state

void Spooler::set_state( State state )
{
    assert( GetCurrentThreadId() == _thread_id );

    if( _state == state )  return;

    _log.msg( state_name() );

    _state = state;
    if( _state_changed_handler )  (*_state_changed_handler)( this, NULL );
}

//------------------------------------------------------------------------------Spooler::state_name

string Spooler::state_name( State state )
{
    switch( state )
    {
        case s_stopped:     return "stopped";
        case s_starting:    return "starting";
        case s_running:     return "running";
        case s_paused:      return "paused";
        case s_stopping:    return "stopping";
        default:            return as_string( (int)state );
    }
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg()
{
    assert( GetCurrentThreadId() == _thread_id );

    _spooler_id       = read_profile_string( "factory.ini", "spooler", "id" );
    _config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    _log_directory    = read_profile_string( "factory.ini", "spooler", "log-dir" );        _log_directory_as_option_set = !_log_directory.empty();
    _include_path     = read_profile_string( "factory.ini", "spooler", "include-path" );   _include_path_as_option_set = !_include_path.empty();
    _spooler_param    = read_profile_string( "factory.ini", "spooler", "param" );          _spooler_param_as_option_set = !_spooler_param.empty();
    _debug            = read_profile_bool  ( "factory.ini", "spooler", "debug", _debug );  

    try
    {
        for( Sos_option_iterator opt ( _argc, _argv ); !opt.end(); opt.next() )
        {
            if( opt.flag      ( "service"          ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "log"              ) )  ;   // wurde in sos_main() bearbeitet
            else
            if( opt.with_value( "config"           ) )  _config_filename = opt.value();
            else
            if( opt.with_value( "id"               ) )  _spooler_id = opt.value();
            else
            if( opt.with_value( "log-dir"          ) )  _log_directory = opt.value(),  _log_directory_as_option_set = true;
            else
            if( opt.with_value( "include-path"     ) )  _include_path = opt.value(),  _include_path_as_option_set = true;
            else
            if( opt.with_value( "param"            ) )  _spooler_param = opt.value(),  _spooler_param_as_option_set = true;
            else
            if( opt.flag      ( "debug"            ) )  _debug = opt.set();
            else
                throw_sos_option_error( opt );
        }

        if( _config_filename.empty() )  throw_xc( "SPOOLER-115" );
    }
    catch( const Sos_option_error& )
    {
        if( !_is_service )
        {
            cerr << "usage: " << _argv[0] << "\n"
                    "       -config=XMLFILE\n"
                    "       -service-\n"
                    "       -log=HOSTWARELOGFILENAME\n"
                    "       -log-dir=DIRECTORY|*stderr\n"
                    "       -id=ID\n"
                    "       -param=PARAM\n";
        }

        throw;
    }
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    assert( GetCurrentThreadId() == _thread_id );

    set_state( s_starting );
    _log.msg( "Spooler::load " + _config_filename );

    Command_processor cp = this;

    tzset();

    _security.clear();             
    load_arg();

    cp.execute_2( file_as_string( _config_filename ) );
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    assert( GetCurrentThreadId() == _thread_id );

    _log.set_directory( _log_directory );
    _log.open_new();

    _communication.start_or_rebind();

    _state_cmd = sc_none;
    set_state( s_starting );
    //_log.msg( "Spooler::start" );

    _spooler_start_time = Time::now();

    FOR_EACH( Thread_list, _thread_list, it )  (*it)->start_thread();
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    assert( GetCurrentThreadId() == _thread_id );

    set_state( s_stopping );

    //_log.msg( "Spooler::stop" );

    signal_threads( "stop" );
    wait_until_threads_stopped( Time::now() + wait_for_thread_termination );

/*  interrupt() lässt PerlScript abstürzen
    FOR_EACH( Thread_list, _thread_list, it )
    {
        Job* job = (*it)->current_job();
        if( job )  try { job->interrupt_script(); } catch(const Xc& x){_log.error(x.what());}
    }

    wait_until_threads_stopped( Time::now() + wait_for_thread_termination_after_interrupt );
*/
    _object_set_class_list.clear();
    _thread_list.clear();

    if( _state_cmd == sc_terminate_and_restart )  spooler_restart( _is_service );
   
    set_state( s_stopped );     
    // Der Dienst ist hier beendet
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    assert( GetCurrentThreadId() == _thread_id );

    set_state( s_running );

    while(1)
    {
        // Threads ohne Jobs und nach Fehler gestorbene Threads entfernen:
        FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->empty() )  THREAD_LOCK( _lock )  it = _thread_list.erase(it);
        if( _thread_list.empty() )  { _log.error( "Kein Thread vorhanden. Spooler wird beendet." ); break; }

        _event.reset();

        if( _state_cmd == sc_pause                 )  set_state( s_paused  ), signal_threads( "pause" );
        if( _state_cmd == sc_continue              )  set_state( s_running ), signal_threads( "continue" );
        if( _state_cmd == sc_load_config           )  break;
        if( _state_cmd == sc_reload                )  break;
        if( _state_cmd == sc_terminate             )  break;
        if( _state_cmd == sc_terminate_and_restart )  break;
        _state_cmd = sc_none;

        _wait_handles.wait_until( latter_day );
    }
}

//-------------------------------------------------------------------------Spooler::cmd_load_config
// Anderer Thread

void Spooler::cmd_load_config( const xml::Element_ptr& config )  
{ 
    THREAD_LOCK( _lock )  
    {
        _config_document = config->ownerDocument; 
        _config_element  = config;
        _state_cmd = sc_load_config; 
    }

    signal( "load_config" ); 
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
    //_log.msg( "Spooler::cmd_terminate_and_restart" );

    //if( _is_service )  throw_xc( "SPOOLER-114" );

    _state_cmd = sc_terminate_and_restart;
    signal( "terminate_and_restart" );
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv )
{
    int rc;

    try
    {
        if( !SOS_LICENCE( licence_spooler ) )  throw_xc( "SOS-1000", "Spooler" );

        _argc = argc;
        _argv = argv;

        tzset();

        _thread_id = GetCurrentThreadId();
        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

        _event.set_name( "Spooler" );
        _event.add_to( &_wait_handles );

        _communication.init();  // Für Windows

        do
        {
            if( _state_cmd != sc_load_config )  load();
    
            THREAD_LOCK_LOG( _lock, "Spooler::launch load_config" )  
            {
                if( _config_element == NULL )  throw_xc( "SPOOLER-116", _spooler_id );
    
                load_config( _config_element );
        
                _config_element = NULL;
                _config_document = NULL;
            }

            start();
            run();
            stop();

        } while( _state_cmd == sc_reload || _state_cmd == sc_load_config );

        _log.msg( "Spooler ordentlich beendet." );

        rc = 0;
    }
    catch( const Xc& x )
    {
        SHOW_ERR( "Fehler " << x );
        rc = 9999;
    }

    return rc;
}

//------------------------------------------------------------------------------------start_process

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

//----------------------------------------------------------------------------make_new_spooler_path

static string make_new_spooler_path( const string& this_spooler )
{
    return directory_of_path(this_spooler) + DIR_SEP + basename_of_path( this_spooler ) + new_suffix + ".exe";
}

//----------------------------------------------------------------------------------spooler_restart

void spooler_restart( bool is_service )
{
    string this_spooler = program_filename();
    string command_line = GetCommandLine();
    string new_spooler  = make_new_spooler_path( this_spooler );

    if( GetFileAttributes( new_spooler.c_str() ) != -1 )      // spooler~new.exe vorhanden?
    {
        // Programmdateinamen aus command_line ersetzen
        int pos;
        if( command_line.length() == 0 )  throw_xc( "SPOOLER-COMMANDLINE" );
        if( command_line[0] == '"' ) {
            pos = command_line.find( 1, '"' );  if( pos == string::npos )  throw_xc( "SPOOLER-COMMANDLINE" );
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

    start_process( command_line + " -renew-spooler=" + quoted_string(this_spooler,'"','"') );
}

//------------------------------------------------------------------------------------spooler_renew

static void spooler_renew( const string& id, const string& renew_spooler, bool is_service, const string& command_line )
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
            if( spooler::service_state(id) == SERVICE_STOPPED )  break;    
            sos_sleep( renew_wait_interval );
        }

        if( spooler::service_state(id) != SERVICE_STOPPED )  return;
    }

    if( renew_spooler != this_spooler )
    {
        for( t; t > 0; t -= renew_wait_interval )  
        {
            LOG( "CopyFile " << this_spooler << ", " << renew_spooler << '\n' );

            copy_ok = CopyFile( this_spooler.c_str(), renew_spooler.c_str(), FALSE );
            if( copy_ok )  break;

            LOG( "CopyFile error=" << GetLastError() << '\n' );

            if( GetLastError() != ERROR_SHARING_VIOLATION )  return;
            sos_sleep( renew_wait_interval );
        }
    }

    if( is_service )  spooler::service_start( id );
                else  start_process( quoted_string(renew_spooler,'"','"') + " " + command_line );
}

//----------------------------------------------------------------------------------------full_path

static string full_path( const string& path )
{
    Sos_limited_text<MAX_PATH> full;

    char* p = _fullpath( full.char_ptr(), path.c_str(), full.size() );
    if( !p )  throw_xc( "fullpath", path );

    return full.char_ptr();
}

//-------------------------------------------------------------------------------delete_new_spooler

void delete_new_spooler( void* )
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
            LOG( "unlink " << copied_spooler << '\n' );

            int ret = _unlink( copied_spooler.c_str() );
            if( ret == 0  || errno != EACCES ) break;
            LOG( "errno=" << errno << ' ' << strerror(errno) << '\n' );
            sos_sleep( renew_wait_interval );
        }
    }
}

//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv )
{
    int ret;
    bool is_service = false;

    {
#       ifdef SYSTEM_WIN
            Ole_initialize ole;
#       endif

        spooler::Spooler spooler;

        ret = spooler.launch( argc, argv );
        
        is_service = spooler.is_service();
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    bool    is_service = false;
    bool    is_service_set = false;
    int     ret;
    string  log_filename = read_profile_string( "factory.ini", "spooler", "log" );
    bool    do_install_service = false;
    bool    do_remove_service = false;
    string  id;
    string  renew_spooler;
    string  command_line;
    bool    renew_service = false;

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
      //if( opt.flag      ( "renew-spooler"    ) )  renew_spooler = program_filename();
      //else
        if( opt.with_value( "renew-spooler"    ) )  renew_spooler = opt.value();
        else
        if( opt.flag      ( "renew-service"    ) )  renew_service = opt.set();
        else
        {
            if( opt.flag      ( "install-service"  ) )  do_install_service = opt.set();
            else
            if( opt.flag      ( "remove-service"   ) )  do_remove_service = opt.set();
            else
            {
                if( opt.flag      ( "service"          ) )  is_service = opt.set(), is_service_set = true;
                else
                if( opt.with_value( "id"               ) )  id = opt.value();
                else
                if( opt.with_value( "log"              ) )  log_filename = opt.value();

                if( !command_line.empty() )  command_line += " ";
                command_line += opt.complete_parameter( '"', '"' );
            }
        }
    }

    if( !log_filename.empty() )  log_start( log_filename );

    if( !renew_spooler.empty() )  
    { 
        spooler::spooler_renew( id, renew_spooler, renew_service, command_line ); 
        ret = 0;
    }
    else
    if( do_remove_service | do_install_service )
    {
        if( do_remove_service  )  spooler::remove_service( id );
        if( do_install_service ) 
        {
            if( !is_service )  command_line = "-service " + command_line;
            spooler::install_service( id, command_line );
        }
        ret = 0;
    }
    else
    {
        _beginthread( spooler::delete_new_spooler, 50000, NULL );

      //if( !is_service_set )  is_service = spooler::service_is_started(id);

        if( is_service )
        {
            ret = spooler::spooler_service( id, argc, argv );
        }
        else
        {
            ret = spooler::spooler_main( argc, argv );
        }
    }

    return ret;
}

} //namespace sos

