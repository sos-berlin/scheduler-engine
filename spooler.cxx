// $Id: spooler.cxx,v 1.52 2001/02/14 22:06:54 jz Exp $
/*
    Hier sind implementiert

    Script_instance
    Spooler
    spooler_main()
    sos_main()
*/

#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../kram/log.h"
#include "../file/anyfile.h"

//#if !defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
//#endif



namespace sos {

extern const Bool _dll = false;

namespace spooler {

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
    _wait_handles(&_prefix_log),
    _log(this)
{
    _com_log     = new Com_log( &_prefix_log );
    _com_spooler = new Com_spooler( this );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    wait_until_threads_stopped();

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

//--------------------------------------------------------------Spooler::wait_until_threads_stopped

void Spooler::wait_until_threads_stopped()
{
    Time until = Time::now() + 10;

    { FOR_EACH( Thread_list, _thread_list, it )  (*it)->wait_until_thread_stopped( until ); }
}

//--------------------------------------------------------------------------Spooler::signal_threads

void Spooler::signal_threads( const string& signal_name )
{
    FOR_EACH( Thread_list, _thread_list, it )  (*it)->signal( signal_name );
}

//---------------------------------------------------------------------------------Spooler::get_job
// Anderer Thread

Job* Spooler::get_job( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Thread_list, _thread_list, it )
        {
            Job* job = (*it)->get_job_or_null( job_name );
            if( job )  return job;
        }
    }

    throw_xc( "SPOOLER-108", job_name );
    return NULL;
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
    if( _state == state )  return;

    _state = state;
    if( _state_changed_handler )  (*_state_changed_handler)( this, NULL );
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg()
{
    _spooler_id       = read_profile_string( "factory.ini", "spooler", "id" );
    _config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    _log_directory    = read_profile_string( "factory.ini", "spooler", "log-dir" );
    _spooler_param    = read_profile_string( "factory.ini", "spooler", "param" );

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
            if( opt.with_value( "log-dir"          ) )  _log_directory = opt.value();
            else
            if( opt.with_value( "id"               ) )  _spooler_id = opt.value();
            else
            if( opt.with_value( "param"            ) )  _spooler_param = opt.value();
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
    _log.set_directory( _log_directory );
    _log.open_new();

    _communication.start_or_rebind();

    _state_cmd = sc_none;
    set_state( s_starting );
    _log.msg( "Spooler::start" );

    _spooler_start_time = Time::now();

    FOR_EACH( Thread_list, _thread_list, it )  (*it)->start_thread();
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    set_state( s_stopping );

    _log.msg( "Spooler::stop" );

    signal_threads();
    wait_until_threads_stopped();

    _object_set_class_list.clear();
    _thread_list.clear();
    
    set_state( s_stopped );     
    // Der Dienst ist hier beendet
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    set_state( s_running );

    while(1)
    {
        // Threads ohne Jobs und nach Fehler gestorbene Threads entfernen:
        FOR_EACH( Thread_list, _thread_list, it )  if( (*it)->empty() )  THREAD_LOCK( _lock )  it = _thread_list.erase(it);
        if( _thread_list.empty() )  { _log.error( "Kein Thread vorhanden. Spooler wird beendet." ); break; }

        _event.reset();

        if( _state_cmd == sc_pause                 )  set_state( s_paused  ), signal_threads();
        if( _state_cmd == sc_continue              )  set_state( s_running ), signal_threads();
        if( _state_cmd == sc_load_config           )  break;
        if( _state_cmd == sc_reload                )  break;
        if( _state_cmd == sc_terminate             )  break;
        if( _state_cmd == sc_terminate_and_restart )  break;
        _state_cmd = sc_none;

        _wait_handles.wait_until( latter_day );
    }
}

//-------------------------------------------------------------------------Spooler::cmd_load_config

void Spooler::cmd_load_config( const xml::Element_ptr& config )  
{ 
    THREAD_LOCK( _lock )  
    {
        _config_document=config->ownerDocument; 
        _config_element=config;
        _state_cmd=sc_load_config; 
    }

    signal( "load_config" ); 
}

//----------------------------------------------------------------------------Spooler::cmd_continue

void Spooler::cmd_continue()
{ 
    if( _state == s_paused )  _state_cmd = sc_continue; 
    signal( "continue" ); 
}

//------------------------------------------------------------------------------Spooler::cmd_reload

void Spooler::cmd_reload()
{
    _state_cmd = sc_reload;
    signal( "reload" );
}

//--------------------------------------------------------------------------------Spooler::cmd_stop

void Spooler::cmd_stop()
{
    _state_cmd = sc_stop;
    signal( "stop" );
}

//---------------------------------------------------------------------------Spooler::cmd_terminate

void Spooler::cmd_terminate()
{
    _log.msg( "Spooler::cmd_terminate" );

    _state_cmd = sc_terminate;
    signal( "terminate" );
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart

void Spooler::cmd_terminate_and_restart()
{
    _log.msg( "Spooler::cmd_terminate_and_restart" );

    if( _is_service )  throw_xc( "SPOOLER-114" );

    _state_cmd = sc_terminate_and_restart;
    signal( "terminate_and_restart" );
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv )
{
    _argc = argc;
    _argv = argv;

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );

    _event.set_name( "Spooler" );
    _event.add_to( &_wait_handles );

    _communication.init();  // Für Windows

    do
    {
        if( _state_cmd != sc_load_config )  load();
        
        THREAD_LOCK( _lock )  
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

    Script_site::clear();

    _log.msg( "Spooler ordentlich beendet." );

    return _state_cmd == sc_terminate_and_restart? 1 : 0;
}

//----------------------------------------------------------------------------------spooler_restart

void spooler_restart()
{
#   ifdef SYSTEM_WIN

        PROCESS_INFORMATION process_info; 
        STARTUPINFO         startup_info; 
        BOOL                ok;

        memset( &process_info, 0, sizeof process_info );

        memset( &startup_info, 0, sizeof startup_info );
        startup_info.cb = sizeof startup_info; 

        ok = CreateProcess( NULL,                       // application name
                            GetCommandLine(),           // command line 
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

#    else

        //fork();
        //spawn();

#   endif
}
//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv )
{
    int ret;

    {
#       ifdef SYSTEM_WIN
            Ole_initialize ole;
#       endif

        spooler::Spooler spooler;

        ret = spooler.launch( argc, argv );
    }

    if( ret == 1 )  spooler_restart();

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

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "service"          ) )  is_service = opt.set(), is_service_set = true;
        else
        if( opt.flag      ( "install-service"  ) )  do_install_service = opt.set();
        else
        if( opt.flag      ( "remove-service"   ) )  do_remove_service = opt.set();
        else
        if( opt.with_value( "id"               ) )  id = opt.value();
        else
        if( opt.with_value( "log"              ) )  log_filename = opt.value();
    }

    if( !log_filename.empty() )  log_start( log_filename );

    if( do_remove_service | do_install_service )
    {
        if( do_remove_service  )  spooler::remove_service( id );
        if( do_install_service )  spooler::install_service( id, argc, argv );
        ret = 0;
    }
    else
    {
        if( !is_service_set )  is_service = spooler::service_is_started(id);

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



