// $Id: spooler.cxx,v 1.19 2001/01/10 14:47:37 jz Exp $




/*
    WAS FEHLT?

    Dienst.

*/

#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../kram/sleep.h"
#include "../kram/log.h"
#include "../file/anyfile.h"
#include "spooler.h"

//#if !defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
//#endif

#if defined SYSTEM_MICROSOFT
    //CComModule com_module;
    //CComModule& _Module = com_module;
#endif


namespace sos {

extern const Bool _dll = false;

namespace spooler {

using namespace std;

//---------------------------------------------------------------------------------------------now

Time now() 
{
#   if 1 //defined SYSTEM_WIN

        _timeb  tm;
        _ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3 - _timezone;

#    elif define SYSTEM_LINUX

        struct timeval tm;
        gettimeofday( &tm, NULL );
        return (double)tm.tv_sec + (double)tm.tv_usec / (double)1e6 - _timezone;

#   else

        return time(NULL) - _timezone;

#   endif
}

//----------------------------------------------------------------------------Script_instance::init

void Script_instance::init()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _script->_language;
    _script_site->init_engine();
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::add_obj( const CComPtr<IDispatch>& object, const string& name )
{
    _script_site->add_obj( object, SysAllocString_string( name ) );
}

//----------------------------------------------------------------------------Script_instance::load

void Script_instance::load()
{
    _script_site->parse( _script->_text );
}

//---------------------------------------------------------------------------Script_instance::close

void Script_instance::close()
{
    _script_site->close_engine();
    _script_site = NULL;
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

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_property_get( _dispatch, "level" );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _dispatch, "process", output_level );
}

//---------------------------------------------------------------------------Object_set::Object_set

Object_set::Object_set( Spooler* spooler, Task* task, const Sos_ptr<Object_set_descr>& descr ) 
: 
    _zero_(this+1),
    _spooler(spooler),
    _task(task),
    _object_set_descr(descr),
    _script_instance(&descr->_class->_script) 
{
}

//--------------------------------------------------------------------------Object_set::~Object_set

Object_set::~Object_set()
{
    _dispatch = NULL;
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    _script_instance.init();
    _script_instance.add_obj( new Com_task_log( _task ), "log" );
    _script_instance.load();

    CComVariant object_set_vt;

    try
    {
        CComVariant v = _script_instance.call( "spooler_dont_use_objects" );
        v.ChangeType( VT_BOOL );
        _use_objects = V_BOOL( &v ) == 0;
    }
    catch( const Xc& ) { _use_objects = true; }


    if( _use_objects )
    {
         object_set_vt = _script_instance.call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );
        _dispatch = object_set_vt.pdispVal;
    }
    else
    {
        _dispatch = _script_instance._script_site->_dispatch;
    }

    if( !_use_objects && stricmp( _script_instance._script->_language.c_str(), "PerlScript" ) == 0 )
    {
        _script_instance._script_site->parse( "$spooler_low_level="  + as_string( _object_set_descr->_level_interval._low_level ) + ";" );
        _script_instance._script_site->parse( "$spooler_high_level=" + as_string( _object_set_descr->_level_interval._high_level ) + ";" );
        _script_instance._script_site->parse( "$spooler_param="      + quoted_string( _spooler->_object_set_param, '\'', '\\' ) + ";" );
    }
    else
    {
        com_property_put( _dispatch, "spooler_low_level" , _object_set_descr->_level_interval._low_level );
        com_property_put( _dispatch, "spooler_high_level", _object_set_descr->_level_interval._high_level );
        com_property_put( _dispatch, "spooler_param"     , _spooler->_object_set_param.c_str() );
    }

    com_call( _dispatch, "spooler_open" );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    com_call( _dispatch, "spooler_close" );
    _dispatch = NULL;

    _script_instance.close();
}

//----------------------------------------------------------------------------------Object_set::eof

bool Object_set::eof()
{
    CComVariant eof;

    eof = com_call( _dispatch, "spooler_eof" );

    eof.ChangeType( VT_BOOL );

    return V_BOOL( &eof ) != 0;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_call( _dispatch, "spooler_get" );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH
         || obj.pdispVal == NULL  )  throw_xc( "SPOOLER-102", _object_set_descr->_class_name );
    
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;
    }

    return object;
}

//---------------------------------------------------------------------------------Object_set::step

bool Object_set::step( Level result_level )
{
    if( eof() ) 
    {
        return false;
    }
    else
    {
        if( _use_objects )
        {
            Spooler_object object = get();

            if( object.is_null() )  return false;

            object.process( result_level );
        }
        else
        {
            _script_instance.call( "spooler_get" );
            _script_instance.call( "spooler_process", result_level );
        }

        return true;
    }
}

//---------------------------------------------------------------------------Weekday_set::next_date

Time Weekday_set::next_date( Time tim )
{
    int day_no = tim / (24*60*60);

    int weekday = ( day_no + 4 ) % 7;

    for( int i = weekday; i < weekday+7; i++ )
    {
        if( _days[ i % 7 ] )  return day_no * (24*60*60);
        day_no++;
    }

    return latter_day;
}

//--------------------------------------------------------------------------Monthday_set::next_date

Time Monthday_set::next_date( Time tim )
{
    int                     day_no = tim / (24*60*60);
    Sos_optional_date_time  date   = tim;

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

    return latter_day;
}

//----------------------------------------------------------------------------Ultimo_set::next_date

Time Ultimo_set::next_date( Time tim )
{
/*
    int         day_no = tim / (24*60*60);
    struct tm*  tm = localtime(tim);
    Sos_date    date;

    date.assign_date( 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }
*/
    return latter_day;
}

//---------------------------------------------------------------------------------Run_time::check

void Run_time::check()
{
    if( _begin_time_of_day < 0                )  throw_xc( "SPOOLER-104" );
    if( _begin_time_of_day > _end_time_of_day )  throw_xc( "SPOOLER-104" );
    if( _end_time_of_day > 24*60*60           )  throw_xc( "SPOOLER-104" );
}

//----------------------------------------------------------------------------------Run_time::next

Time Run_time::next( Time tim_par )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    Time tim = tim_par;

    time_t time_only = (time_t)tim % (24*60*60);
    if( time_only > _end_time_of_day )  tim += 24*60*60;

    tim -= time_only;


    Time next;
 
    while(1)
    {
        next = latter_day;

        FOR_EACH( set<time_t>, _date_set, it )
        {
            if( *it < next  &&  *it >= tim )  next = *it;
        }

        next = min( next, _weekday_set .next_date( tim ) );
        next = min( next, _monthday_set.next_date( tim ) );
        next = min( next, _ultimo_set  .next_date( tim ) );

        if( _holiday_set.find( next ) == _holiday_set.end() )  break;

        tim += (24*60*60);
    }

    _next_start_time = next + _begin_time_of_day;
    _next_end_time = _next_start_time + ( _end_time_of_day - _begin_time_of_day );

    if( _next_start_time < tim_par )
    {
        //_next_start_time += int( (tim_par-_next_start_time) / _retry_period + _retry_period - 0.01 ) * _retry_period;
        _next_start_time = tim_par;
    }

    return _next_start_time;
}

//-----------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job),
    _script_instance(&job->_script),
    _log( &spooler->_log, this )
{
    set_new_start_time();
    _state = s_pending;
}

//--------------------------------------------------------------------------Task::set_new_start_time

void Task::set_new_start_time()
{
    _next_start_time = _job->_run_time.next();
}

//---------------------------------------------------------------------------------------Task::error

void Task::error( const Xc& x)
{
    _log.error( x.what() );

    _error = x;
    _state_cmd = sc_stop;

    _object_set = NULL;
}

//---------------------------------------------------------------------------------Task::start_error

void Task::start_error( const Xc& x )
{
    error( x );
}

//-----------------------------------------------------------------------------------Task::end_error

void Task::end_error( const Xc& x )
{
    error( x );
}

//----------------------------------------------------------------------------------Task::step_error

void Task::step_error( const Xc& x )
{
    error( x );
}

//---------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    _log.msg( "start" );

    _running_since = now();

    if( _job->_object_set_descr )  _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );

    try 
    {
        if( _object_set ) 
        {
            _object_set->open();
        }

        if( !_job->_script.empty() )
        {
            _script_instance.init();
            _script_instance.add_obj( new Com_task_log( this ), "log" );
            _script_instance.load();
        }

        _state = s_running;
        _step_count = 0;
        _run_until_end = false;
        _spooler->_running_jobs_count++;

        _next_start_time = max( _next_start_time + _job->_run_time._retry_period, now() );
        if( now() >= _job->_run_time._next_end_time )  set_new_start_time();
    }
    catch( const Xc& x ) { start_error(x); return false; }

    return true;
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    if( _state == s_suspended )  _state = s_running;
    if( _state != s_running )  return;

    _log.msg( "end" );

    try
    {
        if( _object_set )
        {
            _object_set->close();
        }

        if( !_job->_script.empty() ) 
        {
            _script_instance.close();
        }
    }
    catch( const Xc& x ) { end_error(x); }

    _state = s_pending;
    _spooler->_running_jobs_count--;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    _log.msg( "step" );

    Spooler_object object;

    try 
    {
        if( !_job->_script.empty() ) 
        {
            CComVariant result = _script_instance.call( "step" );
            result.ChangeType( VT_BOOL );
            if( !V_BOOL( &result ) )  return false;
        }
        else
        if( _object_set )
        {
            return _object_set->step( _job->_output_level );
        }
        else 
            return false; //?

        _step_count++;
    }
    catch( const Xc& x ) { step_error(x); return false; }

    return true;
}

//---------------------------------------------------------------------------------------Task::step

void Task::do_something()
{
    switch( _state_cmd.read_and_reset() )
    {
        case sc_stop:       end(); 
                            _state = s_stopped;
                            break;

        case sc_unstop:     _state = s_pending;
                            break;

        case sc_start:      start();
                            _run_until_end = true;
                            break;

        case sc_suspend:    _state = s_suspended;
                            _spooler->_running_jobs_count--;
                            break;

        case sc_continue:   _state = s_running;
                            _spooler->_running_jobs_count++;
                            break;
        default: ;
    }


    switch( _state )
    {
        case s_stopped:     break;

        case s_pending:     if( now() >= _next_start_time )
                            {
                                start();

                                bool ok = step();
                                if( !ok )  end();
                            }
                            break;

        case s_running:     if( _run_until_end | _job->_run_time.should_run_now() )
                            {
                                bool ok = step();
                                if( !ok )  end();
                            }
                            else
                            {
                                end();
                            }
                            break;

        case s_suspended:   break;

        default:            ;
    }
}

//----------------------------------------------------------------------------------Task::set_state

void Task::set_state( State new_state )
{ 
    switch( new_state )
    {
        case s_stopped:     if( _state & ( s_running | s_suspended ) )  set_state_cmd( sc_stop );  
                            break;

        case s_pending:     if( _state & s_stopped )  set_state_cmd( sc_unstop ); 
                            if( _state & s_running )  set_state_cmd( sc_end );      
                            break;
                            
        case s_running:     if( _state & s_pending   )  set_state_cmd( sc_start );
                            if( _state & s_suspended )  set_state_cmd( sc_continue );  
                            break;

        case s_suspended:   if( _state & s_running )  set_state_cmd( sc_suspend );
                            break;
        default: ;
    }
}

//------------------------------------------------------------------------------Task::set_state_cmd

void Task::set_state_cmd( State_cmd cmd )
{ 
    bool ok = false;

    switch( cmd )
    {
        case sc_stop:       ok = true;                  break;

        case sc_unstop:     ok = _state == s_stopped;   if(!ok) break;
                            _spooler->cmd_wake();
                            break;

        case sc_start:      ok = _state == s_pending;   if(!ok) break;
                            _next_start_time = now();  
                            _spooler->cmd_wake();
                            break;

        case sc_end:        ok = _state == s_running;   break;

        case sc_suspend:    ok = _state == s_running;   break;

        case sc_continue:   ok = _state == s_suspended; if(!ok) break;
                            _spooler->cmd_wake();
                            break;


        default:            ok = false;
    }

    if( ok )  _state_cmd = cmd;
      //else  throw_xc( "SPOOLER-109" );
}

//---------------------------------------------------------------------------------Task::state_name

string Task::state_name( State state )
{
    switch( state )
    {
        case s_stopped:     return "stopped";
        case s_pending:     return "pending";
        case s_running:     return "running";
        case s_suspended:   return "suspended";
        default:            return as_string( (int)state );
    }
}

//-----------------------------------------------------------------------------------Task::as_state

Task::State Task::as_state( const string& name )
{
    State state = (State)( s__max - 1 );

    while( state )
    {
        if( state_name(state) == name )  return state;
        state = (State)( state - 1 );
    }

    if( !name.empty() )  throw_xc( "SPOOLER-110", name );
    return s_none;
}

//-------------------------------------------------------------------------------Task::as_state_cmd

Task::State_cmd Task::as_state_cmd( const string& name )
{
    State_cmd cmd = (State_cmd)( sc__max - 1 );

    while( cmd )
    {
        if( state_cmd_name(cmd) == name )  return cmd;
        cmd = (State_cmd)( cmd - 1 );
    }

    if( !name.empty() )  throw_xc( "SPOOLER-106", name );
    return sc_none;
}

//-----------------------------------------------------------------------------Task::state_cmd_name

string Task::state_cmd_name( Task::State_cmd cmd )
{
    switch( cmd )
    {
        case Task::sc_stop:     return "stop";
        case Task::sc_unstop:   return "unstop";
        case Task::sc_start:    return "start";
        case Task::sc_end:      return "end";
        case Task::sc_suspend:  return "suspend";
        case Task::sc_continue: return "continue";
        default:                return as_string( (int)cmd );
    }
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg( int argc, char** argv )
{
    _config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    _log_directory    = read_profile_string( "factory.ini", "spooler", "log-dir" );
    _spooler_id       = read_profile_string( "factory.ini", "spooler", "spooler-id" );
    _object_set_param = read_profile_string( "factory.ini", "spooler", "object-set-param" );

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "log"              ) )  log_start( opt.value() );
        else
        if( opt.with_value( "config"           ) )  _config_filename = opt.value();
        else
        if( opt.with_value( "log-dir"          ) )  _log_directory = opt.value();
        else
        if( opt.with_value( "spooler-id"       ) )  _spooler_id = opt.value();
        else
        if( opt.with_value( "object-set-param" ) )  _object_set_param = opt.value();
        else
            throw_sos_option_error( opt );
    }
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    _log.msg( "Spooler::load" );

    tzset();

    {
        Thread_semaphore::Guard guard = &_semaphore;

        load_xml();
    }
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    _log.set_directory( _log_directory );
    _log.open_new();
    _log.msg( "Spooler::start" );
    
    FOR_EACH( Job_list, _job_list, it )
    {
        Sos_ptr<Task> task = SOS_NEW( Task( this, *it ) );

        _task_list.push_back( task );
    }

    _spooler_start_time = now();

    _communication.start_thread();
}

//------------------------------------------------------------------------------------Spooler::step

void Spooler::step()
{
    Thread_semaphore::Guard guard = &_semaphore;

    FOR_EACH( Task_list, _task_list, it )
    {
        Task* task = *it;

        task->do_something();
    }
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    tzset();

    if( _running_jobs_count == 0 )
    {
        _next_start_time = latter_day;
        Task* next_task = NULL;

        if( _paused )
        {
            _log.msg( "Spooler paused ");
        }
        else
        {
            Thread_semaphore::Guard guard = &_semaphore;

            _next_start_time = latter_day;
            FOR_EACH( Task_list, _task_list, it ) 
            {
                Task* task = *it;
                if( task->_state == Task::s_pending ) 
                {
                    if( _next_start_time > (*it)->_next_start_time )  next_task = *it, _next_start_time = next_task->_next_start_time;
                }
            }
        }

        _sleeping = true;  // Das muss in einen kritischen Abschnitt!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Time wait_time = _next_start_time - now();
        if( wait_time > 0 ) 
        {
            if( next_task )  next_task->_log.msg( "Nächster Start " + Sos_optional_date_time( _next_start_time ).as_string() );
                       else  _log.msg( "Kein Job zu starten" );

            while( _sleeping  &&  wait_time > 0 )
            {
                double sleep_time = 1.0;
                sos_sleep( min( sleep_time, wait_time ) );
                wait_time -= sleep_time;
            }

            _sleeping = false;
        }

        _next_start_time = 0;
        tzset();
    }
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    _log.msg( "Spooler::run" );

    while(1)
    {
        if( _pause     )  _pause = false, _paused = true;
        if( _stop      )  stop();
        if( _terminate )  break;
        if( _reload    )  reload();

        if( !_paused )  step();

        wait();
    }

    if( _terminate_and_restart )  restart();
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    _log.msg( "Spooler::stop" );
    _stop = false;

    {
        FOR_EACH( Task_list, _task_list, it ) 
        {
            Task* task = *it;
            task->end();
            it = _task_list.erase( it );
        }
    }

    _object_set_class_list.clear();
    _job_list.clear();
}

//----------------------------------------------------------------------------------Spooler::reload

void Spooler::reload()
{
    _log.msg( "Spooler::reload" );

    _reload = false;
    stop();
    load();
    start();
}

//---------------------------------------------------------------------------------Spooler::restart

void Spooler::restart()
{
    // Neuen Spooler-Prozess starten. Der wartet, bis dieser sich beendet hat. (pid übergeben und waitpid()? Sonst: TCP)

    // spawn( ... -restart -old-tcp-port=... )
    // exit()


    // Neuer Prozess (-restart):
    // Warten, bis -old-tcp-port frei wird, max. 30s.
}

//------------------------------------------------------------------------------Spooler::cmd_reload

void Spooler::cmd_reload()
{
    _reload = true;
    cmd_wake();
}

//--------------------------------------------------------------------------------Spooler::cmd_stop

void Spooler::cmd_stop()
{
    _stop = true;
    cmd_wake();
}

//---------------------------------------------------------------------------Spooler::cmd_terminate

void Spooler::cmd_terminate()
{
    _log.msg( "Spooler::cmd_terminate_and_restart" );

    _terminate = true;
    cmd_stop();
}

//---------------------------------------------------------------Spooler::cmd_terminate_and_restart

void Spooler::cmd_terminate_and_restart()
{
    _log.msg( "Spooler::cmd_terminate_and_restart" );

    _terminate_and_restart = true;
    cmd_terminate();
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    {
        spooler::Spooler spooler;

        
        spooler.load_arg( argc, argv );
        spooler.load();
        spooler.start();
        spooler.run();
    }

    CoUninitialize();
    return 0;
}


} //namespace sos

