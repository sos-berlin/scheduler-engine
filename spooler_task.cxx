// $Id: spooler_task.cxx,v 1.18 2001/02/04 17:12:43 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/



#include "../kram/sos.h"
#include "spooler.h"


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

static const char spooler_init_name      [] = "spooler_init";
static const char spooler_open_name      [] = "spooler_open";
static const char spooler_close_name     [] = "spooler_close";
static const char spooler_get_name       [] = "spooler_get";
static const char spooler_process_name   [] = "spooler_process";
static const char spooler_level_name     [] = "spooler_level";
static const char spooler_on_error_name  [] = "spooler_on_error";
static const char spooler_on_success_name[] = "spooler_on_success";

//---------------------------------------------------------------------check_spooler_process_result

bool check_spooler_process_result( const CComVariant& vt )
{
    if( vt.vt == VT_EMPTY    )  return true;                       // Keine Rückgabe? True, also weiter machen
    if( vt.vt == VT_DISPATCH )  return vt.pdispVal != NULL;        // Nothing => False, also Ende
    if( vt.vt == VT_BOOL     )  return vt.bVal != NULL;            // Nothing => False, also Ende

    CComVariant v = vt;

    HRESULT hr = v.ChangeType( VT_BOOL );
    if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", spooler_process_name );
    return vt.bVal != 0;
}

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_property_get( _idispatch, spooler_level_name );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _idispatch, spooler_process_name, output_level );
}

//---------------------------------------------------------------------------Object_set::Object_set

Object_set::Object_set( Spooler* spooler, Task* task, const Sos_ptr<Object_set_descr>& descr ) 
: 
    _zero_(this+1),
    _spooler(spooler),
    _task(task),
    _object_set_descr(descr),
    _class(descr->_class)
{
}

//--------------------------------------------------------------------------Object_set::~Object_set

Object_set::~Object_set()
{
    _idispatch = NULL;
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    CComVariant object_set_vt;

    if( _class->_object_interface )
    {
        object_set_vt = _task->_job->_script_instance.call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );

        _idispatch = object_set_vt.pdispVal;
    }
    else
    {
        _idispatch = _task->_job->_script_instance._script_site->dispatch();
    }

    if( com_name_exists( _idispatch, spooler_open_name ) )  com_call( _idispatch, spooler_open_name );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _idispatch, spooler_close_name ) )  com_call( _idispatch, spooler_close_name );
    _idispatch = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_call( _idispatch, spooler_get_name );

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
  //if( eof() )  return false;

    if( _class->_object_interface )
    {
        Spooler_object object = get();
        if( object.is_null() )  return false;

        if( _task->_job->_error )  return false;       // spooler_task.error() gerufen?

        object.process( result_level );
        return true;
    }
    else
    {
        return check_spooler_process_result( _task->_job->_script_instance.call( spooler_process_name, result_level ) );
    }
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Thread* thread )
: 
    _zero_(this+1),
    _thread(thread),
    _spooler(thread->_spooler),
    _directory_watcher(this),
    _script_instance(thread->_spooler),
    _wait_handles(thread->_spooler),
    _log( &thread->_spooler->_log )
{
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    close_task();
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    THREAD_SEMA( _task_lock )  stop();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job )  _com_job->close();
    if( _com_log )  _com_log->close();
}

//--------------------------------------------------------------------------------Job::close_engine

void Job::close_engine()
{
    _com_task = new Com_task();

    try 
    {
        _script_instance.close();
    }
    catch( const Xc& x        ) { error(x); }
    catch( const exception& x ) { error(x); }
}

//----------------------------------------------------------------------------------Job::close_task

void Job::close_task()
{
    //THREAD_SEMA( _task_lock )
    {
        if( _task )  _task->close();
        _task = NULL; 
        _params = NULL; 
        if( _state != s_stopped )  _state = s_ended;
    }

    if( _script_ptr->_reuse == Script::reuse_task )  close_engine();
}

//----------------------------------------------------------------------------------------Job::init

void Job::init()
{
    _log.set_prefix( "Job " + _name );

    _state = s_pending;

    _script_ptr = _object_set_descr? &_object_set_descr->_class->_script
                                   : &_script;

    _event.set_name( "Job " + _name );
    _event.add_to( &_wait_handles );

    _period          = _run_time.next_period( Time::now() );
    _next_start_time = _period.begin();

    _com_job  = new Com_job( this );
    _com_log  = new Com_log( &_log );
    _com_task = new Com_task();
}

//---------------------------------------------------------------------------------Job::create_task

void Job::create_task()
{
    THREAD_SEMA( _task_lock ) 
    {
        if( _task ) 
        {
            //if( !( _task->_state & (Task::s_pending|Task::s_stopped) ) )  
                throw_xc( "SPOOLER-118", _name, state_name() );
        }

        _error = NULL;
        _repeat = 0;
        _state = s_task_created;
        if( !_params )  _params = new Com_variable_set;

        _task = SOS_NEW( Task( _spooler, this ) );
        _com_task->set_task( _task );
    }
}

//---------------------------------------------------------------------------------------Job::start

void Job::start( const CComPtr<spooler_com::Ivariable_set>& params )
{
    THREAD_SEMA( _task_lock )
    {
        if( _task )  throw_xc( "SPOOLER-118", _name, state_name() );

        _params = params;

        set_state_cmd( sc_start );
    }
}

//----------------------------------------------------------------Job::start_when_directory_changed

void Job::start_when_directory_changed( const string& directory_name )
{
    _log.msg( "start_when_directory_changed " + directory_name + "\"" );

#   ifdef SYSTEM_WIN

        _directory_watcher.watch_directory( directory_name );
        _directory_watcher.add_to( &_thread->_wait_handles );

#    else

        throw_xc( "SPOOLER-112", "Job::start_when_directory_changed" );

#   endif
}

//----------------------------------------------------------------------------------------Job::load

void Job::load()
{
    _script_instance.init( _script_ptr->_language );

    _script_instance.add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"      );
    _script_instance.add_obj( (IDispatch*)_com_log              , "spooler_log"  );
    _script_instance.add_obj( (IDispatch*)_com_job              , "spooler_job"  );
    _script_instance.add_obj( (IDispatch*)_com_task             , "spooler_task" );

    _script_instance.load( *_script_ptr );
    if( _error )  return;

    _script_instance.call_if_exists( spooler_init_name );
    if( _error )  return;

    _has_spooler_process = _script_instance.name_exists( spooler_process_name );

    _state = s_loaded;
}

//-----------------------------------------------------------------------------------------Job::stop

void Job::stop()
{
    if( _state != s_stopped )  _log.msg( "stop" );

    if( _task )  _task->end();


    _directory_watcher.close();

    close_task();
    _state = s_stopped;
}

//--------------------------------------------------------------------------Job::set_next_start_time

void Job::set_next_start_time( Time now )
{
    if( _repeat > 0 ) {
        _next_start_time = now + _repeat;
        _repeat = 0;
        if( !_period.let_run()  &&  _next_start_time > _period.end() )  _next_start_time = latter_day;
    }
    else
    if( _period.is_single_start() )  _next_start_time = latter_day;
                               else  _next_start_time = _period.next_try( now );

    if( _next_start_time == latter_day ) 
    {
        Time new_time = _period.is_single_start()? now : max( now, _period.end() );

        _period          = _run_time.next_period( new_time );
        _next_start_time = _period.begin();
    }
}

//--------------------------------------------------------------------------------Job::do_something

bool Job::do_something()
{
    bool something_done = false;
    bool ok;

    THREAD_SEMA( _task_lock )
    {

        switch( _state_cmd.read_and_reset() )
        {
            case sc_stop:       stop(); 
                                break;

            case sc_unstop:     if( _state == s_pending )  _state = s_pending;     // Sollte nicht durchlaufen werden.
                                break;

            case sc_start:      create_task();
                                _task->_let_run = true;
                                _task->start();
                                break;

            case sc_suspend:    _state = s_suspended;
                                _thread->_running_tasks_count--;
                                break;

            case sc_continue:   _state = s_running;
                                _thread->_running_tasks_count++;
                                break;
            default: ;
        }


        if( !_error )
        {
            bool do_a_step = false;
            Time now = Time::now();

            if( _state == s_pending  &&  ( _directory_watcher._signaled || Time::now() >= _next_start_time ) )
            {
                if( _directory_watcher._signaled )  _directory_watcher.watch_again();

                create_task();
                _task->start();
            }

            if( _state == s_running )
            {
                ok = do_a_step | _task->_let_run || _period.is_in_time(now);
                if( !ok )  set_next_start_time(now),  ok = _task->_let_run || _period.is_in_time(now);

                if( ok )   ok = _task->step();
                if( !ok )  _task->end();

                something_done = true;
            }

            if( _state == s_ended  &&  _task ) 
            {
                close_task();
                set_next_start_time();
                _state = s_pending;
            }
        }

        if( _error )  
        {
            stop();
            if( _repeat > 0 )  _state = s_pending;
        }
    }

    return something_done;
}

//---------------------------------------------------------------------------------------Job::error

void Job::error( const Xc& x )
{
    _log.error( x.what() );

    _error = x;
    _repeat = 0;
}

//---------------------------------------------------------------------------------------Job::error

void Job::error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    error( xc );
}

//-----------------------------------------------------------------------------------Job::set_state

void Job::set_state( State new_state )
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

//-------------------------------------------------------------------------------Job::set_state_cmd

void Job::set_state_cmd( State_cmd cmd )
{ 
    bool ok = false;

    switch( cmd )
    {
        case sc_stop:       ok = true;                  break;

        case sc_unstop:     ok = _state == s_stopped;   if(!ok) break;
                            break;

        case sc_start:      ok = ( _state & (s_pending|s_stopped) ) != 0;   if(!ok) break;
                            _params = NULL;
                            break;

        case sc_end:        ok = _state == s_running;   break;

        case sc_suspend:    ok = _state == s_running;   break;

        case sc_continue:   ok = _state == s_suspended; if(!ok) break;
                            break;


        default:            ok = false;
    }

    if( !ok )  return;    //throw_xc( "SPOOLER-109" );
    
    _state_cmd = cmd;

    _thread->signal();
}

//----------------------------------------------------------------------------------Job::state_name

string Job::state_name( State state )
{
    switch( state )
    {
        case s_stopped:     return "stopped";
        case s_loaded:      return "loaded";
        case s_pending:     return "pending";
        case s_task_created:return "task_created";
        case s_running:     return "running";
        case s_suspended:   return "suspended";
        case s_ending:      return "ending";
        case s_ended:       return "ended";
        default:            return as_string( (int)state );
    }
}

//------------------------------------------------------------------------------------Job::as_state

Job::State Job::as_state( const string& name )
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

//--------------------------------------------------------------------------------Job::as_state_cmd

Job::State_cmd Job::as_state_cmd( const string& name )
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

//------------------------------------------------------------------------------Job::state_cmd_name

string Job::state_cmd_name( Job::State_cmd cmd )
{
    switch( cmd )
    {
        case Job::sc_stop:     return "stop";
        case Job::sc_unstop:   return "unstop";
        case Job::sc_start:    return "start";
        case Job::sc_end:      return "end";
        case Job::sc_suspend:  return "suspend";
        case Job::sc_continue: return "continue";
        default:               return as_string( (int)cmd );
    }
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job)
{
    //_priority = _job->_priority;
    _let_run  = _job->_period.let_run();
}

//---------------------------------------------------------------------------------------Task::Task

Task::~Task()    
{
    //_job->_log.msg( "~Task" );

    try{ close(); } catch(const Xc&) {}
}

//--------------------------------------------------------------------------------------Task::close

void Task::close()
{
    //_params = NULL;

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set  )  _com_object_set->close();
    //_job = NULL;

    THREAD_SEMA( _terminated_events_lock )  FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal();

  //FOR_EACH( Task_list, _spooler->_task_list, it )  if( +*it == this )  { _spooler->_task_list.erase( it );  break; }
}

//--------------------------------------------------------------------------------------Task::start

void Task::start()
{
    _job->_log.msg( "start" );

    _job->_thread->_task_count++;
    _running_since = Time::now();

    try 
    {
        if( _job->_object_set_descr ) 
        {
            if( !_object_set ) 
            {
                _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
                _com_object_set = new Com_object_set( _object_set );
            }
        }

        if( !_job->_script_instance.loaded() )
        {
            _job->load();
            if( _job->_error )  return;
        }

        if( _job->_object_set_descr )  _object_set->open();
                                 else  _job->_script_instance.call_if_exists( spooler_open_name );

        _opened = true;
    }
    catch( const Xc& x        ) { _job->error(x); return; }
    catch( const exception& x ) { _job->error(x); return; }

    _job->_state = Job::s_running;
    _job->_thread->_running_tasks_count++;
}

//-------------------------------------------------------------------------Task::on_error_on_success

void Task::on_error_on_success()
{
    if( _job->_error )
    {
        try
        {
            _job->_script_instance.call_if_exists( spooler_on_error_name );
        }
        catch( const Xc& x        ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
        catch( const exception& x ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
    }
    else
    {
        try
        {
            _job->_script_instance.call_if_exists( spooler_on_success_name );
        }
        catch( const Xc& x        ) { _job->error(x); }
        catch( const exception& x ) { _job->error(x); }
    }
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    if( _job->_state == Job::s_ending    )  return;   // end() schon einmal gerufen und dabei abgebrochen?
    if( _job->_state == Job::s_ended     )  return;
    if( _job->_state == Job::s_suspended )  _job->_state = Job::s_running;
    if( _job->_state != Job::s_running   )  return;
    if( !_opened                         )  return;

    _job->_state = Job::s_ending;

    try
    {
        if( _object_set )  _object_set->close();
                     else  _job->_script_instance.call_if_exists( spooler_close_name );
    }
    catch( const Xc& x        ) { _job->error(x); }
    catch( const exception& x ) { _job->error(x); }


    on_error_on_success();

    _object_set = NULL;
    _opened = false;
    _step_count = 0;

    _job->_state = Job::s_ended;
    _job->_thread->_running_tasks_count--;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;

    try 
    {
        if( _object_set )
        {
            result = _object_set->step( _job->_output_level );
        }
        else 
        {
            if( _job->_has_spooler_process )  result = check_spooler_process_result( _job->_script_instance.call( spooler_process_name ) );
                                        else  result = false;
            
        }

        _job->_thread->_step_count++;
        _step_count++;
    }
    catch( const Xc& x        ) { _job->error(x); return false; }
    catch( const exception& x ) { _job->error(x); return false; }

    return result;
}

//----------------------------------------------------------------------Task::wait_until_terminated

bool Task::wait_until_terminated( double wait_time )
{
    if( GetCurrentThread() == _job->_thread->_thread.handle() )  throw_xc( "SPOOLER-125" );     // Deadlock

    Event event ( "Task " + _job->_name + " wait_until_terminated" );
    int   i = 0;
    
    THREAD_SEMA( _terminated_events_lock ) 
    {
        i = _terminated_events.size();
        _terminated_events.push_back( &event );
    }

    bool result = event.wait( wait_time );

    { THREAD_SEMA( _terminated_events_lock )  _terminated_events.erase( &_terminated_events[i] ); }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
