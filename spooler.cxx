// $Id: spooler.cxx,v 1.36 2001/01/21 16:59:05 jz Exp $
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

//----------------------------------------------------------------------------Script_instance::init

void Script_instance::init( const string& language )
{
    _script_site = new Script_site;
    _script_site->_engine_name = language;
    _script_site->init_engine();
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::add_obj( const CComPtr<IDispatch>& object, const string& name )
{
    CComBSTR name_bstr;
    name_bstr.Attach( SysAllocString_string( name ) );

    _script_site->add_obj( object, name_bstr );
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::load( const Script& script )
{
    if( !_script_site )  init( script._language );
                   else  if( _script_site->_engine_name != script._language )  throw_xc( "SPOOLER-117" );

    _script_site->parse( script._text );

    _loaded = true;
}

//---------------------------------------------------------------------------Script_instance::close

void Script_instance::close()
{
    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }

    _loaded = false;
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name )
{
    return _script_site->call( name );
}

//----------------------------------------------------------------------------Script_instance::call

CComVariant Script_instance::call( const char* name, int param )
{
    return _script_site->call( name, param );
}

//--------------------------------------------------------------------Script_instance::property_get

CComVariant Script_instance::property_get( const char* name )
{
    return _script_site->property_get( name );
}

//-----------------------------------------------------------Script_instance::optional_property_put

void Script_instance::optional_property_put( const char* name, const CComVariant& v )
{
    try 
    {
        property_put( name, v );
    }
    catch( const Xc& )
    {
        // Ignorieren, wenn das Objekt die Eigenschaft nicht kennt
    }
}

//---------------------------------------------------------------------------------Spooler::Spooler

Spooler::Spooler() 
: 
    _zero_(this+1), 
    _communication(this), 
    _log(this)
{
    _com_log     = new Com_log( &_log );
    _com_spooler = new Com_spooler( this );
}

//--------------------------------------------------------------------------------Spooler::~Spooler

Spooler::~Spooler() 
{
    _task_list.clear();
    _job_list.clear();
    _object_set_class_list.clear();

    _script_instance.close();
    _communication.close(0.0);

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->close();
}

//---------------------------------------------------------------------------------Spooler::get_job

Job* Spooler::get_job( const string& job_name )
{
    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;
        if( job->_name == job_name )  return job;
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

    {
        Thread_semaphore::Guard guard = &_semaphore;
        Command_processor       cp = this;

        tzset();
        load_arg();

        cp.execute_2( file_as_string( _config_filename ) );
      //load_xml();
    }
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

    if( !_script.empty() )
    {
        _script_instance.init( _script._language );

        _script_instance.add_obj( (IDispatch*)_com_spooler, "spooler"     );
        _script_instance.add_obj( (IDispatch*)_com_log    , "spooler_log" );

        _script_instance.load( _script );

        if( _script_instance.name_exists( "spooler_init" ) )  _script_instance.call( "spooler_init" );
    }
    

    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;

        Sos_ptr<Task> task = SOS_NEW( Task( this, job ) );
        _task_list.push_back( task );
    }

    _spooler_start_time = Time::now();
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    set_state( s_stopping );

    _log.msg( "Spooler::stop" );

    {
        FOR_EACH( Task_list, _task_list, it ) 
        {
            Task* task = *it;
            task->stop();
            it = _task_list.erase( it );  it--;
        }
    }

    _object_set_class_list.clear();
    _job_list.clear();

    _script_instance.close();

    set_state( s_stopped );
}

//------------------------------------------------------------------------------------Spooler::step

void Spooler::step()
{

  //int  pri_sum = 0;
    bool something_done = false;

  //FOR_EACH( Task_list, _task_list, it )  pri_sum += (*it)->task_priority;


    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:

    {
        Thread_semaphore::Guard guard = &_semaphore;

        FOR_EACH( Task_list, _task_list, it )
        {
            Task* task = *it;
            if( task->_priority >= _priority_max )  something_done |= task->do_something();
        }
    }


    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        Thread_semaphore::Guard guard = &_semaphore;

        FOR_EACH( Task_list, _task_list, it )
        {
            Task* task = *it;
            for( int i = 0; i < task->_priority; i++ )  something_done |= task->do_something();
        }
    }


    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        Thread_semaphore::Guard guard = &_semaphore;

        FOR_EACH( Task_list, _task_list, it )
        {
            Task* task = *it;
            if( task->_priority == 0 )  task->do_something();
        }
    }
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    if( _running_jobs_count > 0  && _state != s_paused )  return;

    tzset();

    _next_start_time = latter_day;
    Task* next_task = NULL;

    if( _state == s_paused )
    {
        _log.msg( "Spooler paused ");
    }
    else
    {
        Thread_semaphore::Guard guard = &_semaphore;

        FOR_EACH( Task_list, _task_list, it ) 
        {
            Task* task = *it;
            if( task->_state == Task::s_pending ) 
            {
                if( _next_start_time > (*it)->_next_start_time )  next_task = *it, _next_start_time = next_task->_next_start_time;
            }
        }
    }


    Time wait_time = _next_start_time - Time::now();
    if( wait_time > 0 ) 
    {
        if( next_task )  next_task->_log.msg( "Nächster Start " + _next_start_time.as_string() );
                   else  _log.msg( "Kein Job zu starten" );

        _sleeping = true;

#       ifdef SYSTEM_WIN

            _wait_handles.wait( wait_time );

#        else

            while( !_wake  &&  wait_time > 0 )
            {
                double sleep_time = 1.0;
                sos_sleep( min( sleep_time, wait_time ) );
                wait_time -= sleep_time;
            }

#       endif

        _sleeping = false;
    }

    _next_start_time = 0;
    tzset();
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
  //_log.msg( "Spooler::run" );
    
    set_state( s_running );

    while(1)
    {
        if( _state_cmd == sc_pause                 )  set_state( s_paused ); 
        if( _state_cmd == sc_continue              )  set_state( s_running );
        if( _state_cmd == sc_load_config           )  break;
        if( _state_cmd == sc_reload                )  break;
        if( _state_cmd == sc_terminate             )  break;
        if( _state_cmd == sc_terminate_and_restart )  break;
        _state_cmd = sc_none;

        if( _state == s_running )  step();

        wait();
    }
}

//------------------------------------------------------------------------------Spooler::cmd_reload

void Spooler::cmd_reload()
{
    _state_cmd = sc_reload;
    cmd_wake();
}

//--------------------------------------------------------------------------------Spooler::cmd_stop

void Spooler::cmd_stop()
{
    _state_cmd = sc_stop;
    cmd_wake();
}

//---------------------------------------------------------------------------Spooler::cmd_terminate

void Spooler::cmd_terminate()
{
    _log.msg( "Spooler::cmd_terminate" );

    _state_cmd = sc_terminate;
    cmd_wake();
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart

void Spooler::cmd_terminate_and_restart()
{
    _log.msg( "Spooler::cmd_terminate_and_restart" );

    if( _is_service )  throw_xc( "SPOOLER-114" );

    _state_cmd = sc_terminate_and_restart;
    cmd_wake();
}

//--------------------------------------------------------------------------------Spooler::cmd_wake

void Spooler::cmd_wake()
{
#   ifdef SYSTEM_WIN

        SetEvent( _command_arrived_event );

#    else

        Thread_semaphore::Guard guard = &_sleep_semaphore;

        _wake = true;

#   endif
}

//----------------------------------------------------------------------------------Spooler::launch

int Spooler::launch( int argc, char** argv )
{
    _argc = argc;
    _argv = argv;

#   ifdef SYSTEM_WIN
        _command_arrived_event = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( !_command_arrived_event )  throw_mswin_error( "CreateEvent" );
        _wait_handles.add( _command_arrived_event );
#   endif


    do
    {
        if( _state_cmd != sc_load_config )  load();
        
        if( _config_element == NULL )  throw_xc( "SPOOLER-116", _spooler_id );
            
        load_config( _config_element ), _config_element = NULL, _config_document = NULL;

        start();
        run();
        stop();

    } while( _state_cmd == sc_reload || _state_cmd == sc_load_config );

    _log.msg( "Spooler ordentlich beendet." );

    return _state_cmd == sc_terminate_and_restart? 1 : 0;
}

//-------------------------------------------------------------------------------------spooler_main

int spooler_main( int argc, char** argv )
{
    int ret;

    {
#       ifdef SYSTEM_WIN
            Ole_initialize ole;
          //static Set_console_code_page cp ( 1252 );              // Für die richtigen Umlaute
#       endif


        spooler::Spooler spooler;

        ret = spooler.launch( argc, argv );
    }

    if( ret == 1 )
    {
#       ifdef SYSTEM_WIN

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

#        else

            //fork();
            //spawn();

#       endif
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



