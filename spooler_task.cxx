// $Id: spooler_task.cxx,v 1.3 2001/01/20 23:39:16 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/



#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/log.h"


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

static const char spooler_init_name[]    = "spooler_init";
static const char spooler_open_name[]    = "spooler_open";
static const char spooler_close_name[]   = "spooler_close";
static const char spooler_get_name[]     = "spooler_get";
static const char spooler_process_name[] = "spooler_process";
static const char spooler_level_name[]   = "spooler_level";

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
    _class(descr->_class),
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

    if( _class->_object_interface )
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

    if( com_name_exists( _dispatch, spooler_open_name ) )  com_call( _dispatch, spooler_open_name );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _dispatch, spooler_close_name ) )  com_call( _dispatch, spooler_close_name );
    _dispatch = NULL;
}

//----------------------------------------------------------------------------------Object_set::eof
/*
bool Object_set::eof()
{
    CComVariant eof;

    eof = com_call( _dispatch, "spooler_eof" );

    eof.ChangeType( VT_BOOL );

    return V_BOOL( &eof ) != 0;
}
*/
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

        object.process( result_level );
        return true;
    }
    else
    {
        return check_spooler_process_result( _script_instance.call( spooler_process_name, result_level ) );
    }
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler)
{
    _com_job = new Com_job( this );
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job )  _com_job->close();
}

//----------------------------------------------------------------Task::wake_when_directory_changed 

void Job::start_when_directory_changed( const string& directory_name )
{
    Task* task = NULL;
    FOR_EACH( Task_list, _spooler->_task_list, it )  if( +(*it)->_job == this )  { task = *it; break; }
    if( !task )  throw_xc( "Job::start_when_directory_changed" );

#   ifdef SYSTEM_WIN

        task->_directory_watcher.watch_directory( directory_name );
        if( task->_directory_watcher._handle )  _spooler->_wait_handles.remove( task->_directory_watcher._handle );
        _spooler->_wait_handles.add( task->_directory_watcher._handle, task );

#    else

        throw_xc( "SPOOLER-112", "wake_when_directory_changed" );

#   endif
}

//---------------------------------------------------------------------------------------Job::start

void Job::start()
{
    Task* task = NULL;
    FOR_EACH( Task_list, _spooler->_task_list, it )  if( +(*it)->_job == this )  { task = *it; break; }
    if( !task )  throw_xc( "Job::start" );

    if( !( task->_state & (Task::s_pending|Task::s_stopped) ) )  throw_xc( "SPOOLER-118", _name, task->state_name() );

    task->start();
}

//---------------------------------------------------------------------------------------Task::Task

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

//---------------------------------------------------------------------------------------Task::Task

Task::~Task()    
{
    if( _script_instance )  _script_instance->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set  )  _com_object_set->close();
    if( _com_task        )  _com_task->close();
    if( _com_log         )  _com_log->close();

    if( _directory_watcher )  _spooler->_wait_handles.remove( _directory_watcher._handle );
}

//-------------------------------------------------------------------------Task::set_new_start_time

void Task::set_new_start_time()
{
    _next_start_time = _job->_run_time.next();
}

//--------------------------------------------------------------------------------------Task::error

void Task::error( const Xc& x )
{
    _log.error( x.what() );

    _error = x;
  //stop();
}

//--------------------------------------------------------------------------------------Task::error

void Task::error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    error( xc );
}

//--------------------------------------------------------------------------------Task::start_error

void Task::start_error( const Xc& x )
{
    error( x );
}

//----------------------------------------------------------------------------------Task::end_error

void Task::end_error( const Xc& x )
{
    error( x );
}

//---------------------------------------------------------------------------------Task::step_error

void Task::step_error( const Xc& x )
{
    error( x );
}

//-----------------------------------------------------------------------------Task::prepare_script

void Task::prepare_script()
{
    if( _script_instance )  return;


    Script_instance* s = NULL;

    if( _object_set            )  s = &_object_set->_script_instance;
    if( !_job->_script.empty() )  s = &_job_script_instance;
    if( !s )  throw_xc( "SPOOLER-111" );
    
  //if( s->_script->_reuse == Script::reuse_task )  _script_instance = NULL;

  //if( !_script_instance )
    {
        _script_instance = s;
        _script_instance->init();

        _script_instance->add_obj( (IDispatch*)_com_log              , "spooler_log" );
        _script_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"     );

        _script_instance->add_obj( (IDispatch*)_job->_com_job        , "spooler_job" );

        _script_instance->load();

        // Bei reuse_job soll spooler_task erst nach spooler_init() bekannt sein,
        // denn spooler_task kann mit jeder Task wechseln (nicht in dieser Version).
        _script_instance->add_obj( (IDispatch*)_com_task, "spooler_task" );

        if( _script_instance->name_exists( spooler_init_name ) )  _script_instance->call( spooler_init_name );
    }
}

//---------------------------------------------------------------------------------------Task::start

bool Task::start()
{
  //_log.msg( "start" );

    _error = NULL;

    _spooler->_task_count++;
    _running_since = Time::now();

    try 
    {
        if( _job->_object_set_descr ) 
        {
          //if( !_object_set  ||  _script_instance->_script->_reuse == Script::reuse_task ) 
            if( !_object_set ) 
            {
                _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
                _com_object_set = new Com_object_set( _object_set );
            }
        }

        prepare_script();

        if( _job->_object_set_descr )  _object_set->open();
        else
        if( _script_instance->name_exists( spooler_open_name ) )  _script_instance->call( spooler_open_name );

        _opened = true;

        _step_count = 0;
        //_next_start_time = max( _next_start_time + _job->_run_time._retry_period, now() );
        //if( now() >= _job->_run_time._next_end_time )  set_new_start_time();
    }
    catch( const Xc& x        ) { start_error(x); return false; }
    catch( const exception& x ) { error(x); return false; }

    _state = s_running;
    _let_run = _job->_run_time._let_run;
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

  //_log.msg( "end" );

    try
    {
        if( _object_set )  _object_set->close();

        if( _script_instance  &&  _script_instance->_script->_reuse == Script::reuse_task )
        {
            if( _script_instance->name_exists( spooler_close_name ) )  _script_instance->call( spooler_close_name );
            _script_instance->close();
            _script_instance = NULL;
            _object_set = NULL;
        }
    }
    catch( const Xc& x        ) { end_error(x); }
    catch( const exception& x ) { error(x); }

    _opened = false;

    _next_start_time = Time::now() + _job->_run_time._retry_period;
    if( Time::now() >= _job->_run_time._next_end_time )  set_new_start_time();

    _state = s_pending;
    _spooler->_running_jobs_count--;
}

//----------------------------------------------------------------------------------------Task::stop

void Task::stop()
{
    _log.msg( "stop" );

    if( _opened )  end();

    try 
    {
        if( _script_instance )  _script_instance->close();
    }
    catch( const Xc& x        ) { end_error(x); }
    catch( const exception& x ) { error(x); }

    if( _directory_watcher )  _spooler->_wait_handles.remove( _directory_watcher._handle );
    _object_set = NULL;
    _script_instance = NULL;
    _state = s_stopped;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;

  //_log.msg( "step" );

    try 
    {
        if( !_job->_script.empty() ) 
        {
            if( !_job_script_instance.name_exists( spooler_process_name ) )  return false;
            return check_spooler_process_result( _job_script_instance.call( spooler_process_name ) );
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
    catch( const Xc& x        ) { step_error(x); return false; }
    catch( const exception& x ) { error(x); return false; }

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
                            _let_run = true;
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

        case s_pending:     if( _directory_watcher._signaled || Time::now() >= _next_start_time )
                            {
                                ok = start();  if(!ok) break;

                                ok = step();
                                if( !ok )  { end(); break; }

                                something_done = true;
                            }
                            break;

        case s_running:     if( _let_run 
                            //| _directory_watcher._signaled 
                              | _job->_run_time.should_run_now() )
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

    if( _error && _state != s_stopped )  stop();

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

        case sc_start:      ok = ( _state & (s_pending|s_stopped) ) != 0;   if(!ok) break;
                            _next_start_time = Time::now();  
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

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
