// $Id: spooler.cxx,v 1.28 2001/01/14 10:26:55 jz Exp $


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



namespace sos {

extern const Bool _dll = false;

namespace spooler {

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
    if( _script_site )
    {
        _script_site->close_engine();
        _script_site = NULL;
    }
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
    CComVariant object_set_vt;

    if( _script_instance.name_exists( "spooler_dont_use_objects" ) )
    {
        _use_objects = false;
        _script_instance.call( "spooler_dont_use_objects" );
    }
    else
        _use_objects = true;


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
/*
    if( !_use_objects && stricmp( _script_instance._script->_language.c_str(), "PerlScript" ) == 0 )
    {
      //_script_instance._script_site->parse( "$spooler_low_level="  + as_string( _object_set_descr->_level_interval._low_level ) + ";" );
      //_script_instance._script_site->parse( "$spooler_high_level=" + as_string( _object_set_descr->_level_interval._high_level ) + ";" );
      //_script_instance._script_site->parse( "$spooler_param="      + quoted_string( _spooler->_spooler_param, '\'', '\\' ) + ";" );
    }
    else
    {
        com_property_put( _dispatch, "spooler_low_level" , _object_set_descr->_level_interval._low_level );
        com_property_put( _dispatch, "spooler_high_level", _object_set_descr->_level_interval._high_level );
      //com_property_put( _dispatch, "spooler_param"     , _spooler->_spooler_param.c_str() );
    }
*/
    com_call( _dispatch, "spooler_open" );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    com_call( _dispatch, "spooler_close" );
    _dispatch = NULL;
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
    int                     day_no = tim / (24*60*60);
    Sos_date                date   = Sos_optional_date_time( tim );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ last_day_of_month( date ) - date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

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
    _job_script_instance(&job->_script),
    _log( &spooler->_log, this )
{
    set_new_start_time();
    _state = s_pending;
    _priority = _job->_priority;

    _com_log  = new Com_log( this );
    _com_task = new Com_task( this );
}

//-----------------------------------------------------------------------------------------Task::Task

Task::~Task()    
{
    if( _script_instance )  _script_instance->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set  )  _com_object_set->close();
    if( _com_task        )  _com_task->close();
    if( _com_log         )  _com_log->close();

    if( _directory_watcher )  _spooler->_wait_handles.remove( _directory_watcher._handle );
}

//--------------------------------------------------------------------------Task::set_new_start_time

void Task::set_new_start_time()
{
    _next_start_time = _job->_run_time.next();
}

//---------------------------------------------------------------------------------------Task::error

void Task::error( const Xc& x )
{
    _log.error( x.what() );

    _error = x;
    stop();
}

//---------------------------------------------------------------------------------------Task::error

void Task::error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    error( xc );
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

//------------------------------------------------------------------------------Task::prepare_script

void Task::prepare_script()
{
    Script_instance* s = NULL;

    if( _object_set            )  s = &_object_set->_script_instance;
    if( !_job->_script.empty() )  s = &_job_script_instance;

    if( !s )  throw_xc( "SPOOLER-111" );
    
    if( s->_script->_reuse == Script::reuse_task )  _script_instance = NULL;

    if( !_script_instance )
    {
        _script_instance = s;
        _script_instance->init();

      //if( !_spooler->_script_instance._script->empty() )  
      //    _script_instance->add_obj( _spooler->_script_instance.dispatch(), "spooler_script" );

        _script_instance->add_obj( (IDispatch*)_com_log              , "spooler_log"  );
        _script_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"      );
        _script_instance->add_obj( (IDispatch*)_com_task             , "spooler_task" );

        _script_instance->load();
        if( _script_instance->name_exists( "spooler_init" ) )  _script_instance->call( "spooler_init" );
    }
}

//---------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    _log.msg( "start" );

    _spooler->_task_count++;
    _running_since = now();

    try 
    {
        if( _job->_object_set_descr ) 
        {
            if( !_object_set  ||  _script_instance->_script->_reuse == Script::reuse_task ) 
            {
                _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
                _com_object_set = new Com_object_set( _object_set );
            }
        }

        prepare_script();

        if( _job->_object_set_descr )  _object_set->open();

        _step_count = 0;
        _next_start_time = max( _next_start_time + _job->_run_time._retry_period, now() );
        if( now() >= _job->_run_time._next_end_time )  set_new_start_time();
    }
    catch( const Xc& x        ) { start_error(x); return false; }
    catch( const exception& x ) { error(x); return false; }

    _state = s_running;
    _run_until_end = false;
    _spooler->_running_jobs_count++;

    return true;
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    if( _state == s_ending )  return;   // end() schon einmal gerufen und dabei abgebrochen?
    if( _state == s_suspended )  _state = s_running;
    if( _state != s_running )  return;

    _state = s_ending;

    _log.msg( "end" );

    try
    {
        if( _object_set )  _object_set->close();

        if( _script_instance  &&  _script_instance->_script->_reuse == Script::reuse_task )
        {
            _script_instance->close();
        }
    }
    catch( const Xc& x        ) { end_error(x); }
    catch( const exception& x ) { error(x); }

    _state = s_pending;
    _spooler->_running_jobs_count--;
}

//----------------------------------------------------------------------------------------Task::stop

void Task::stop()
{
    _log.msg( "stop" );

    end();

    try 
    {
        if( _script_instance )  _script_instance->close();
    }
    catch( const Xc& x        ) { end_error(x); }
    catch( const exception& x ) { error(x); }

    _state = s_stopped;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;

    _log.msg( "step" );

    Spooler_object object;

    try 
    {
        if( !_job->_script.empty() ) 
        {
            CComVariant result_vt = _job_script_instance.call( "step" );
            result_vt.ChangeType( VT_BOOL );
            result = V_BOOL( &result_vt ) != 0;
        }
        else
        if( _object_set )
        {
            result = _object_set->step( _job->_output_level );
        }
        else 
            return false; //?

        _spooler->_step_count++;
        _step_count++;
    }
    catch( const Xc& x ) { step_error(x); return false; }
    catch( const exception& x ) { error(x); }

    return result;
}

//---------------------------------------------------------------------------------------Task::step

bool Task::do_something()
{
    bool something_done = false;
    bool ok;


    switch( _state_cmd.read_and_reset() )
    {
        case sc_stop:       stop(); 
                            break;

        case sc_unstop:     _state = s_pending;
                            break;

        case sc_start:      ok = start();  if(!ok) break;
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

        case s_pending:     if( _directory_watcher._signaled || now() >= _next_start_time )
                            {
                                ok = start();  if(!ok) break;

                                ok = step();
                                if( !ok )  { end(); break; }

                                something_done = true;
                            }
                            break;

        case s_running:     if( _run_until_end | _directory_watcher._signaled | _job->_run_time.should_run_now() )
                            {
                                ok = step();
                                if( !ok )  { end(); break; }

                                something_done = true;
                            }
                            else
                            {
                                end();
                            }
                            break;

        case s_suspended:   break;

        default:            ;
    }

    if( _directory_watcher._signaled ) _directory_watcher.watch_again();

    return something_done;
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
                            break;

        case sc_start:      ok = _state == s_pending;   if(!ok) break;
                            _next_start_time = now();  
                            break;

        case sc_end:        ok = _state == s_running;   break;

        case sc_suspend:    ok = _state == s_running;   break;

        case sc_continue:   ok = _state == s_suspended; if(!ok) break;
                            break;


        default:            ok = false;
    }

    if( !ok )  return;    //throw_xc( "SPOOLER-109" );
    
    _state_cmd = cmd;
    _spooler->cmd_wake();
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

//----------------------------------------------------------------Task::wake_when_directory_changed 

void Task::wake_when_directory_changed( const string& directory_name )
{
#   ifdef SYSTEM_WIN

        _directory_watcher.watch_directory( directory_name );
        _spooler->_wait_handles.add( _directory_watcher._handle, this );

#    else

        throw_xc( "SPOOLER-112", "wake_when_directory_changed" );

#   endif
}

//---------------------------------------------------------------------------------Spooler::Spooler

Spooler::Spooler() 
: 
    _zero_(this+1), 
    _communication(this), 
    _script_instance(&_script),
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
    _communication.close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_spooler )  _com_spooler->close();
    if( _com_log     )  _com_log->close();
}

//--------------------------------------------------------------------------------Spooler::load_arg

void Spooler::load_arg()
{
    _config_filename  = read_profile_string( "factory.ini", "spooler", "config" );
    _log_directory    = read_profile_string( "factory.ini", "spooler", "log-dir" );
    _spooler_id       = read_profile_string( "factory.ini", "spooler", "spooler-id" );
    _spooler_param    = read_profile_string( "factory.ini", "spooler", "spooler-param" );

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
        if( opt.with_value( "spooler-id"       ) )  _spooler_id = opt.value();
        else
        if( opt.with_value( "spooler-param"    ) )  _spooler_param = opt.value();
        else
            throw_sos_option_error( opt );
    }
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    _state = s_starting;
    _log.msg( "Spooler::load" );

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

    if( _communication.started() )  _communication.rebind();
                              else  _communication.start_thread();

    _state_cmd = sc_none;
    _state = s_starting;
    _log.msg( "Spooler::start" );

    if( !_script_instance._script->empty() )
    {
        _script_instance.init();

        _script_instance.add_obj( (IDispatch*)_com_spooler, "spooler" );
        _script_instance.add_obj( (IDispatch*)_com_log, "spooler_log" );

        _script_instance.load();
      //_script_instance.optional_property_put( "spooler_param", _spooler_param.c_str() );
        if( _script_instance.name_exists( "spooler_init" ) )  _script_instance.call( "spooler_init" );
    }
    

    FOR_EACH( Job_list, _job_list, it )
    {
        Sos_ptr<Task> task = SOS_NEW( Task( this, *it ) );

        _task_list.push_back( task );
    }

    _spooler_start_time = now();
}

//------------------------------------------------------------------------------------Spooler::stop

void Spooler::stop()
{
    _state = s_stopping;

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

    _state = s_stopped;
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
    tzset();

    if( _running_jobs_count == 0 )
    {
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

        _sleeping = true;  // Das muss in einen kritischen Abschnitt!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        Time wait_time = _next_start_time - now();
        if( wait_time > 0 ) 
        {
            if( next_task )  next_task->_log.msg( "Nächster Start " + Sos_optional_date_time( _next_start_time ).as_string() );
                       else  _log.msg( "Kein Job zu starten" );

#           ifdef SYSTEM_WIN

                _wait_handles.wait( wait_time );
 
#            else

                while( _sleeping  &&  wait_time > 0 )
                {
                    double sleep_time = 1.0;
                    sos_sleep( min( sleep_time, wait_time ) );
                    wait_time -= sleep_time;
                }

#           endif

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
    
    _state = s_running;

    while(1)
    {
        if( _state_cmd == sc_pause                 )  _state = s_paused; 
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

    _state_cmd = sc_terminate_and_restart;
    cmd_wake();
}

//--------------------------------------------------------------------------------Spooler::cmd_wake

void Spooler::cmd_wake()
{
#   ifdef SYSTEM_WIN

        SetEvent( _command_arrived_event );

#    else

        _sleeping = false;

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
        
        if( _config_element )  load_config( _config_element ), _config_element = NULL;

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

    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    {
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

#       endif
    }

    CoUninitialize();

    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    bool is_service = false;
    int  ret;

    for( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "service"          ) )  is_service = opt.set();
        else
        if( opt.with_value( "log"              ) )  log_start( opt.value() );
    }

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



