// $Id: spooler.cxx,v 1.47 2001/02/04 22:51:36 jz Exp $
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
    _wait_handles(this),
    _log(this),
    _prefix_log(&_log)
{
    _com_log     = new Com_log( &_prefix_log );
    _com_spooler = new Com_spooler( this );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    _thread_list.clear();
    _object_set_class_list.clear();

    _communication.close(0.0);

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->close();
}

//---------------------------------------------------------------------------------Spooler::get_job

Job* Spooler::get_job( const string& job_name )
{
    FOR_EACH( Thread_list, _thread_list, it )
    {
        Job* job = (*it)->get_job_or_null( job_name );
        if( job )  return job;
    }

    throw_xc( "SPOOLER-108", job_name );
    return NULL;
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
    _config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    _log_directory    = read_profile_string( "factory.ini", "spooler", "log-dir" );
    _spooler_id       = read_profile_string( "factory.ini", "spooler", "id" );
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

    FOR_EACH( Thread_list, _thread_list, it )  (*it)->stop_thread();

    _object_set_class_list.clear();
    
    set_state( s_stopped );
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    set_state( s_running );

    while(1)
    {
        _event.reset();

        if( _state_cmd == sc_pause                 )  set_state( s_paused ); 
        if( _state_cmd == sc_continue              )  set_state( s_running );
        if( _state_cmd == sc_load_config           )  break;
        if( _state_cmd == sc_reload                )  break;
        if( _state_cmd == sc_terminate             )  break;
        if( _state_cmd == sc_terminate_and_restart )  break;
        _state_cmd = sc_none;

        if( _state == s_paused )  THREAD_SEMA( _pause_lock )  _wait_handles.wait();
                            else  _wait_handles.wait();
    }
}

//----------------------------------------------------------------------------Spooler::cmd_continue

void Spooler::cmd_continue()
{ 
    if( _state == s_paused )  _state_cmd = sc_continue; 
    signal(); 
}

//------------------------------------------------------------------------------Spooler::cmd_reload

void Spooler::cmd_reload()
{
    _state_cmd = sc_reload;
    signal();
}

//--------------------------------------------------------------------------------Spooler::cmd_stop

void Spooler::cmd_stop()
{
    _state_cmd = sc_stop;
    signal();
}

//---------------------------------------------------------------------------Spooler::cmd_terminate

void Spooler::cmd_terminate()
{
    _log.msg( "Spooler::cmd_terminate" );

    _state_cmd = sc_terminate;
    signal();
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart

void Spooler::cmd_terminate_and_restart()
{
    _log.msg( "Spooler::cmd_terminate_and_restart" );

    if( _is_service )  throw_xc( "SPOOLER-114" );

    _state_cmd = sc_terminate_and_restart;
    signal();
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv )
{
    _argc = argc;
    _argv = argv;

    _event.set_name( "Spooler" );
    _event.add_to( &_wait_handles );

    _communication.init();  // Für Windows

    do
    {
        if( _state_cmd != sc_load_config )  load();
        
        if( _config_element == NULL )  throw_xc( "SPOOLER-116", _spooler_id );
            
        load_config( _config_element ), _config_element = NULL, _config_document = NULL;

        start();
        run();
        stop();

    } while( _state_cmd == sc_reload || _state_cmd == sc_load_config );

    Script_site::clear();

    _log.msg( "Spooler ordentlich beendet." );

    return _state_cmd == sc_terminate_and_restart? 1 : 0;
}

//----------------------------------------------------------------------------------spooler_restart

static void spooler_restart()
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

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "service"          ) )  is_service = opt.set(), is_service_set = true;
        else
        if( opt.flag      ( "install-service"  ) )  { spooler::install_service();  return 0; }
        else
        if( opt.flag      ( "remove-service"   ) )  { spooler::remove_service();  return 0; }
        else
        if( opt.with_value( "log"              ) )  log_filename = opt.value();
    }

    if( !log_filename.empty() )  log_start( log_filename );



    if( !is_service_set )  is_service = spooler::service_is_started();

    if( is_service )
    {
        ret = spooler::spooler_service( argc, argv );
    }
    else
    {
        ret = spooler::spooler_main( argc, argv );
    }

    return ret;
}

} //namespace sos



