// $Id: spooler_task.cxx,v 1.17 2001/01/30 13:32:37 jz Exp $
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
    CComVariant level = com_property_get( _dispatch, spooler_level_name );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _dispatch, spooler_process_name, output_level );
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
    _dispatch = NULL;
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    CComVariant object_set_vt;

    if( _class->_object_interface )
    {
        object_set_vt = _task->_script_instance_ptr->call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );

        _dispatch = object_set_vt.pdispVal;
    }
    else
    {
        _dispatch = _task->_script_instance_ptr->_script_site->_dispatch;
    }

    if( com_name_exists( _dispatch, spooler_open_name ) )  com_call( _dispatch, spooler_open_name );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _dispatch, spooler_close_name ) )  com_call( _dispatch, spooler_close_name );
    _dispatch = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_call( _dispatch, spooler_get_name );

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

        if( _task->_error )  return false;       // spooler_task.error() gerufen?

        object.process( result_level );
        return true;
    }
    else
    {
        return check_spooler_process_result( _task->_script_instance_ptr->call( spooler_process_name, result_level ) );
    }
}

//-----------------------------------------------------------------------------------------Job::Job
// Spooler-Thread

Job::Job( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler),
    _script_instance(spooler)
{
    _com_job = new Com_job( this );
    //_com_current_task = new Com_task();
}

//----------------------------------------------------------------------------------------Job::~Job
// Spooler-Thread

Job::~Job()
{
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job )  _com_job->close();
}

//---------------------------------------------------------------------------------Job::create_task
// Spooler-Thread

Task* Job::create_task( const CComPtr<spooler_com::Ivariable_set>& params )
{
    if( _task ) 
    {
        //if( !( _task->_state & (Task::s_pending|Task::s_stopped) ) )  
            throw_xc( "SPOOLER-118", _name, _task->state_name() );
    }

    Sos_ptr<Task> task = SOS_NEW( Task( _spooler, this ) );

    _task = task;
    THREAD_SEMA( _spooler->_task_list_lock )  _spooler->_task_list.push_back( task );

    if( _spooler->_use_threads )  task->start_thread();

    return task;
}

//---------------------------------------------------------------------------------------Job::start
// Spooler-Thread

Task* Job::start( const CComPtr<spooler_com::Ivariable_set>& params )
{
    if( !_task )
    {
        Task* task = create_task( params );
    }

    _task->set_state_cmd( Task::sc_start );
    return _task;
}

//----------------------------------------------------------------Job::start_when_directory_changed
// Task-Thread

void Job::start_when_directory_changed( const string& directory_name )
{
    _task->_log.msg( "start_when_directory_changed " + directory_name + "\"" );

#   ifdef SYSTEM_WIN

        _task->_directory_watcher.watch_directory( directory_name );

#    else

        throw_xc( "SPOOLER-112", "Job::start_when_directory_changed" );

#   endif
}

//---------------------------------------------------------------------------------------Task::Task
// Spooler-Thread

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job),
    _log( &spooler->_log, "Job " + job->_name ),
    _script_instance(spooler),
    _wait_handles(spooler),
    _directory_watcher(this)
{
    Time now = Time::now();
    _period = _job->_run_time.next_period( now );
    _next_start_time = _period.begin();
    _let_run         = _period.let_run();

    _state = s_pending;
    _priority = _job->_priority;

    _com_log  = new Com_log( this );
    _com_task = new Com_task( this );

    _job->_task = this;

  //_task_event = CreateEvent( NULL, FALSE, FALSE, NULL );
    _wake_event = CreateEvent( NULL, FALSE, FALSE, NULL );
    _wait_handles.add( _wake_event, "spooler_event", this );


    if( _job->_object_set_descr ) 
    {
        _script_ptr = &_job->_object_set_descr->_class->_script;
    }
    else
        _script_ptr = &_job->_script;

    if( _script_ptr->_reuse == Script::reuse_job )  _script_instance_ptr = &_job->_script_instance;
                                              else  _script_instance_ptr = &_script_instance, _use_task_engine = true;

    _log.msg( "*** Task()");
}

//---------------------------------------------------------------------------------------Task::Task
// Spooler-Thread

Task::~Task()    
{
    _log.msg( "~~~~Task" );

    try{ close(); } catch(const Xc&) {}

    if( _com_task )  
    { 
      //_com_task->_error = _error;     // task.error bleibt nach dem Ende der Task erhalten 
      //_com_task->close(); 
    }   

}

//--------------------------------------------------------------------------------------Task::close

void Task::close()
{
    if( _thread )  TerminateThread( _thread, 999 ), _thread.close();

    _script_instance.close();
    _params = NULL;
    _directory_watcher.close();
    _job->_task = NULL;
    _com_task = NULL;
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set  )  _com_object_set->close();
    if( _com_log )  _com_log->close();

  //FOR_EACH( Task_list, _spooler->_task_list, it )  if( +*it == this )  { _spooler->_task_list.erase( it );  break; }
}

//----------------------------------------------------------------------------------Task::set_state
// Task-Thread

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
// Irgendein Thread

void Task::set_state_cmd( State_cmd cmd )
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

    if( _spooler->_use_threads )  wake();
                            else  _spooler->cmd_wake();
}

//---------------------------------------------------------------------------------------Task::wake
// Irgendein Thread

void Task::wake()
{
    if( _spooler->_use_threads )  SetEvent( _wake_event );
}

//---------------------------------------------------------------------------------Task::state_name
// Irgendein Thread

string Task::state_name( State state )
{
    switch( state )
    {
        case s_stopped:     return "stopped";
        case s_loaded:      return "loaded";
        case s_pending:     return "pending";
        case s_running:     return "running";
        case s_suspended:   return "suspended";
        case s_ending:      return "ending";
        case s_ended:       return "ended";
        default:            return as_string( (int)state );
    }
}

//-----------------------------------------------------------------------------------Task::as_state
// Irgendein Thread

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
// Irgendein Thread

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
// Irgendein Thread

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

//--------------------------------------------------------------------------------------Task::error
// Task-Thread

void Task::error( const Xc& x )
{
    _log.error( x.what() );

    _error = x;
  //stop();
}

//--------------------------------------------------------------------------------------Task::error
// Task-Thread

void Task::error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    error( xc );
}

//-------------------------------------------------------------------------Task::set_next_start_time
// Task-Thread

void Task::set_next_start_time()
{
    Time now = Time::now();

    if( _period.is_single_start() )  _next_start_time = latter_day;
                               else  _next_start_time = _period.next_try( now );

    if( _next_start_time == latter_day ) 
    {
        Time new_time = _period.is_single_start()? now : max( now, _period.end() );

        _period = _job->_run_time.next_period( new_time );

        _next_start_time = _period.begin();
        _let_run         = _period.let_run();
    }
}

//---------------------------------------------------------------------------------------Task::start
// Task-Thread

bool Task::start()
{
    _log.msg( "start" );

    _error = NULL;
    if( !_params )  _params = new Com_variable_set;

    _spooler->_task_count++;
    _running_since = Time::now();

    try 
    {
        //_job->set_current_task( this );

        if( _job->_object_set_descr ) 
        {
            if( !_object_set ) 
            {
                _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
                _com_object_set = new Com_object_set( _object_set );
            }
        }

        if( !_script_instance_ptr->loaded() )
        {
            _script_instance_ptr->init( _script_ptr->_language );
        
            _script_instance_ptr->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"      );
            _script_instance_ptr->add_obj( (IDispatch*)_com_log              , "spooler_log"  );
            _script_instance_ptr->add_obj( (IDispatch*)_job->_com_job        , "spooler_job"  );
            _script_instance_ptr->add_obj( (IDispatch*)_com_task             , "spooler_task" );

            _script_instance_ptr->load( *_script_ptr );
            if( _error )  goto FEHLER;

            _script_instance_ptr->call_if_exists( spooler_init_name );
            if( _error )  goto FEHLER;

            _state = s_loaded;
        }

        if( _job->_object_set_descr )  _object_set->open();
                                 else  _script_instance_ptr->call_if_exists( spooler_open_name );

        _has_spooler_process = _script_instance_ptr->name_exists( spooler_process_name );

        _opened = true;
    }
    catch( const Xc& x        ) { error(x); goto FEHLER; }
    catch( const exception& x ) { error(x); goto FEHLER; }

    _state = s_running;
    //_let_run = _job->_run_time.let_run();
    _spooler->_running_tasks_count++;

    return true;

  FEHLER:
    stop();
    return false;
}

//-------------------------------------------------------------------------Task::on_error_on_success
// Task-Thread

void Task::on_error_on_success()
{
    if( _error )
    {
        try
        {
            _script_instance_ptr->call_if_exists( spooler_on_error_name );
        }
        catch( const Xc& x        ) { _log.error( string(spooler_on_error_name) + ": " + x.what() ); }
        catch( const exception& x ) { _log.error( string(spooler_on_error_name) + ": " + x.what() ); }
    }
    else
    {
        try
        {
            _script_instance_ptr->call_if_exists( spooler_on_success_name );
        }
        catch( const Xc& x        ) { error(x); }
        catch( const exception& x ) { error(x); }
    }
}

//-----------------------------------------------------------------------------------------Task::end
// Task-Thread

void Task::end()
{
    if( _state == s_ending    )  return;   // end() schon einmal gerufen und dabei abgebrochen?
    if( _state == s_ended     )  return;
    if( _state == s_suspended )  _state = s_running;
    if( _state != s_running   )  return;

    _state = s_ending;



    try
    {
        if( _object_set )  _object_set->close();
                     else  _script_instance_ptr->call_if_exists( spooler_close_name );
    }
    catch( const Xc& x        ) { error(x); }
    catch( const exception& x ) { error(x); }


    on_error_on_success();


    if( _use_task_engine )
    {
        try
        {
            _script_instance.close();
            _object_set = NULL;
        }
        catch( const Xc& x        ) { error(x); }
        catch( const exception& x ) { error(x); }
    }


    _opened = false;
    _params = NULL;
    _step_count = 0;

    if( _use_task_engine )  _state = s_ended;                               // ~Task und Job::start()
                      else  _state = s_pending,  set_next_start_time();     // Task weiter verwenden

    _spooler->_running_tasks_count--;
}

//----------------------------------------------------------------------------------------Task::stop
// Task-Thread

void Task::stop()
{
    if( _state != s_stopped ) 
    {
        _log.msg( "stop" );

        if( _opened )  end();

        try 
        {
            if( _script_instance_ptr )   _script_instance_ptr->close();      // Bei use_engine="job" wird die Engine des Jobs geschlossen.
            _script_instance.close();
        }
        catch( const Xc& x        ) { error(x); }
        catch( const exception& x ) { error(x); }

        _directory_watcher.close();
        _state = s_stopped;
    }

    _object_set = NULL;
    _script_instance_ptr = NULL;
    _step_count = 0;
}

//----------------------------------------------------------------------------------------Task::step
// Task-Thread

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
            if( _has_spooler_process )  return check_spooler_process_result( _script_instance_ptr->call( spooler_process_name ) );
                                  else  return false;
            
        }

        _spooler->_step_count++;
        _step_count++;
    }
    catch( const Xc& x        ) { error(x); return false; }
    catch( const exception& x ) { error(x); return false; }

    return result;
}

//---------------------------------------------------------------Task::wait_until_thread_terminated
// Irgendein Thread, aber nicht der Task-Thread

bool Task::wait_until_thread_terminated( double wait_time )
{
    if( !_thread )  return true;

    return wait_for_event( _thread, wait_time );
}

//----------------------------------------------------------------------Task::wait_until_terminated
// Irgendein Thread, aber nicht der Task-Thread

bool Task::wait_until_terminated( double wait_time )
{
    if( !_spooler->_use_threads )  throw_xc( "SPOOLER-123", "wait_until_terminated" );
    if( !_use_task_engine )  throw_xc( "SPOOLER-124", "wait_until_terminated" );

    return wait_until_thread_terminated( wait_time );
/*
    if( _state == s_running || _state == s_suspended )
    {
        Handle event = CreateEvent( NULL, FALSE, FALSE, NULL );
        _state.signal_change_to( event );
        bool result = wait_for_event( event, wait_time );
    }

    return false;
*/
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

        case sc_unstop:     _state = s_pending;     // Sollte nicht durchlaufen werden.
                            break;

        case sc_start:      ok = start();  if(!ok) break;
                            _let_run = true;
                            break;

        case sc_suspend:    _state = s_suspended;
                            _spooler->_running_tasks_count--;
                            break;

        case sc_continue:   _state = s_running;
                            _spooler->_running_tasks_count++;
                            break;
        default: ;
    }


    if( !_error )
    {
        switch( _state )
        {
            case s_stopped:     break;

            case s_pending:     
            {
                if( _directory_watcher._signaled || Time::now() >= _next_start_time )
                {
                    if( _directory_watcher._signaled )  _directory_watcher.watch_again();

                    ok = start();  if(!ok) break;

                    ok = step();
                    if( !ok )  { end(); break; }

                    something_done = true;
                }
                break;
            }

            case s_running:  
            {
                Time now = Time::now();

                if( _let_run  ||  _period.is_in_time( now ) )    
                    ok = true;
                else {
                    set_next_start_time();
                    ok = _let_run || _period.is_in_time( now );
                }

                if( ok ) {
                    ok = step();
                    if( !ok )  { end(); break; }

                    something_done = true;
                }
                else
                    end();

                break;
            }

            case s_suspended: break;

            default: ;
        }
    }

    if( _error )  stop();


    return something_done;
}

//---------------------------------------------------------------------------------------Task::wait
/*
void Task::wait()
{
}
*/
//---------------------------------------------------------------------------------Task::run_thread

int Task::run_thread()
{
    int result = 0;

    try 
    {
        while(1)
        {
            while( _spooler->_state == Spooler::s_paused )  _wait_handles.wait();

            do_something();
            if( _state == s_stopped )  break;
            if( _state == s_ended )  break;

            if( _state != s_running )
            {
                Time wait_time;
                if( _state == s_pending ) {
                    if( _next_start_time < latter_day )  _log.msg( "Nächster Start " + _next_start_time.as_string() );
                                                   else  _log.msg( "Kein Start geplant" );
                    wait_time = _next_start_time - Time::now();
                } else {
                    _log.msg( state_name() );
                    wait_time = latter_day;
                }

                // Nur, wenn ein Ereignis angefordert: SetEvent( _task_event );
                _wait_handles.wait( wait_time );
            }
        }
    }
    catch( const Xc& x ) { error(x); result = 1; }

    _log.msg( "Thread 0x" + as_hex_string( (int)_thread_id ) + " beendet sich" );
    SetEvent( _spooler->_command_arrived_event );
    
    return result;
}

//-------------------------------------------------------------------------------------------thread

static uint __stdcall thread( void* param )
{
    Ole_initialize ole;
    return ((Task*)param)->run_thread();
}

//-------------------------------------------------------------------------------Task::start_thread

void Task::start_thread()
{
   _thread = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
   if( !_thread )  throw_mswin_error( "CreateThread" );

   _log.msg( "Thread 0x" + as_hex_string( (int)_thread_id ) );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
