// $Id: spooler_task.cxx,v 1.30 2001/02/20 10:52:48 jz Exp $
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

bool Object_set::open()
{
    bool        ok;
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

    if( com_name_exists( _idispatch, spooler_open_name ) )  ok = check_result( com_call( _idispatch, spooler_open_name ) );
                                                      else  ok = true;

    return ok && !_task->_job->has_error();
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

        if( _task->_job->has_error() )  return false;       // spooler_task.error() gerufen?

        object.process( result_level );
        return true;
    }
    else
    {
        return check_result( _task->_job->_script_instance.call( spooler_process_name, result_level ) );
    }
}

//-------------------------------------------------------------------------------Object_set::thread

Thread* Object_set::thread() const
{ 
    return _task->_job->_thread; 
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Thread* thread )
: 
    _zero_(this+1),
    _thread(thread),
    _spooler(thread->_spooler),
    _log( &thread->_spooler->_log ),
    _script_instance(&_log)
{
    _params = new Com_variable_set;
}

//----------------------------------------------------------------------------------------Job::~Job

Job::~Job()
{
    close();
}

//---------------------------------------------------------------------------------------Job::close

void Job::close()
{
    close_task();

    _directory_watcher.close();

    close_engine();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job  )  _com_job->close(),         _com_job  = NULL;
    if( _com_task )  _com_task->set_task(NULL), _com_task = NULL;
    if( _com_log  )  _com_log->close(),         _com_log  = NULL;
}

//--------------------------------------------------------------------------------Job::close_engine

void Job::close_engine()
{
    _com_task = new Com_task();

    try 
    {
        _script_instance.close();
    }
    catch( const Xc& x        ) { set_error(x); }
    catch( const exception& x ) { set_error(x); }
}

//----------------------------------------------------------------------------------Job::close_task

void Job::close_task()
{
    THREAD_LOCK( _lock )
    {
        if( _task )  _task->close();
        _task = NULL; 
        _params = new Com_variable_set; 
        if( _state != s_stopped )  set_state( s_ended );
    }

    if( _load_error 
      || _script_ptr  &&  _script_ptr->_reuse == Script::reuse_task )  close_engine();
}

//----------------------------------------------------------------------------------------Job::init
// Bei <add_jobs> von einem anderen Thread gerufen.

void Job::init()
{
    set_state( s_none );

    _log.set_prefix( "Job " + _name );

    _script_ptr = _object_set_descr? &_object_set_descr->_class->_script
                                   : &_script;

    _period          = _run_time.next_period( Time::now() );
    _next_start_time = _period.begin();

    _com_job  = new Com_job( this );
    _com_log  = new Com_log( &_log );
    _com_task = new Com_task();

    set_state( s_pending );
}

//---------------------------------------------------------------------------------Job::create_task

void Job::create_task()
{
    if( _task )  throw_xc( "SPOOLER-118", _name, state_name() );

    _error = NULL;
    _load_error = false;
    _repeat = 0;

    if( !_process_filename.empty() )   _task = SOS_NEW( Process_task( _spooler, this ) );
    else
    if( _object_set_descr          )   _task = SOS_NEW( Object_set_task( _spooler, this ) );
    else                             
                                       _task = SOS_NEW( Job_script_task( _spooler, this ) );

    _com_task->set_task( _task );

    set_state( s_task_created );
}

//---------------------------------------------------------------------------------------Job::start

void Job::start( const CComPtr<spooler_com::Ivariable_set>& params )
{
    THREAD_LOCK( _lock )  start_without_lock( params );
}

//---------------------------------------------------------------------------------------Job::start

void Job::start_without_lock( const CComPtr<spooler_com::Ivariable_set>& params )
{
    _params = params? params : new Com_variable_set;
    create_task();

    _task->_let_run = true;
    _thread->signal( "start job" );
}

//----------------------------------------------------------------Job::start_when_directory_changed

void Job::start_when_directory_changed( const string& directory_name )
{
    _log.msg( "start_when_directory_changed " + directory_name + "\"" );

#   ifdef SYSTEM_WIN

        _directory_watcher.watch_directory( directory_name );
        _directory_watcher.set_name( "job(\"" + _name + "\").start_when_directory_changed(\"" + directory_name + "\")" );


        _directory_watcher.add_to( &_thread->_wait_handles );

#    else

        throw_xc( "SPOOLER-112", "Job::start_when_directory_changed" );

#   endif
}

//----------------------------------------------------------------------------------------Job::load

bool Job::load()
{
    _script_instance.init( _script_ptr->_language );

    _script_instance.add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"        );
    _script_instance.add_obj( (IDispatch*)_thread->_com_thread  , "spooler_thread" );
    _script_instance.add_obj( (IDispatch*)_com_log              , "spooler_log"    );
    _script_instance.add_obj( (IDispatch*)_com_job              , "spooler_job"    );
    _script_instance.add_obj( (IDispatch*)_com_task             , "spooler_task"   );

    _script_instance.load( *_script_ptr );
    if( has_error() )  { _load_error = true; return false; }

    bool ok = check_result( _script_instance.call_if_exists( spooler_init_name ) );
    if( !ok || has_error() )  { _load_error = true; return false; }

    _has_spooler_process = _script_instance.name_exists( spooler_process_name );

    set_state( s_loaded );
    return true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    if( !_state )  return;

    if( _state == s_suspended )  set_state( s_running );
    if( _state & ( s_running | s_running_process ) )  if( _task )  _task->end();
    
    close_task();
    set_state( s_ended );
}

//----------------------------------------------------------------------------------------Job::stop

void Job::stop()
{
    _log.msg( "stop" );

    end();
    close_engine();

    // Kein Signal mehr soll kommen, wenn Job gestoppt:
    _directory_watcher.close();

    set_state( s_stopped );
}

//----------------------------------------------------------------------------Job::interrupt_script

void Job::interrupt_script()
{
    if( _script_instance )  _script_instance.interrupt();
}

//-------------------------------------------------------------------------Job::set_next_start_time

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
    if( !_state )  return false;

    bool something_done = false;
    bool ok = true;

    if( _state_cmd )
    {
        switch( _state_cmd )
        {
            case sc_stop:       if( _state != s_stopped )  stop(), something_done = true;
                                break;

            case sc_unstop:     if( _state == s_stopped )  set_state( s_pending ), something_done = true;
                                break;

            case sc_end:        if( _state == s_running )  end(), something_done = true;
                                break;

            case sc_suspend:    if( _state == s_running )  set_state( s_suspended ), something_done = true;
                                break;

            case sc_continue:   if( _state == s_suspended )  set_state( s_running ), something_done = true;
                                break;

            default: ;
        }

        _state_cmd = sc_none;
    }


    if( _state == s_stopped   )  return something_done;
    if( _state == s_suspended )  return something_done;

    if( _state == s_pending )
    {
        if( !_directory_watcher.signaled()  &&  Time::now() < _next_start_time )  return false;

        if( _directory_watcher.signaled() )  _directory_watcher.watch_again();
        THREAD_LOCK( _lock )  create_task();
        something_done = true;
    }

    bool do_a_step = false;

    if( _state == s_task_created )
    {
        ok = _task->start();
        if( ok )  do_a_step = true;
        something_done = true;
    }

    if( ok && !has_error() )
    {
        if( _state == s_running || _state == s_running_process )
        {
            Time now;
            bool call_step = do_a_step | _task->_let_run;
            if( !call_step )  now = Time::now(), ok = _period.is_in_time( now );
            if( !call_step )   // Period abgelaufen?
            {
                set_next_start_time(now);  
                call_step = _task->_let_run || _period.is_in_time(now);  // Gilt schon die nächste Periode?
            }

            if( call_step )   ok = _task->step(), something_done = true;
        }
    }

    if( !ok || has_error() )
    {
        if( _state == s_running || _state == s_running_process )  end(), something_done = true;
        if( _state != s_stopped  &&  has_error()  &&  _repeat == 0 )  stop(), something_done = true;
    }


    if( _state == s_ended ) 
    {
        close_task();

        if( _temporary && _repeat == 0 )  
        {
            stop();   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
        }
        else
        {
            set_next_start_time();
            if( _next_start_time == latter_day )  stop();
                                            else  set_state( s_pending );
        }

        something_done = true;
    }

    return something_done;
}

//-----------------------------------------------------------------------------------Job::set_error

void Job::set_error( const Xc& x )
{
    _log.error( x.what() );

    THREAD_LOCK( _lock )
    {
        _error = x;
        if( _task )  THREAD_LOCK( _task->_lock )  _task->_error = _error;
    }

    _repeat = 0;
}

//-----------------------------------------------------------------------------------Job::set_error

void Job::set_error( const exception& x )
{
    Xc xc ( "SOS-2000", x.what(), exception_name(x) );
    set_error( xc );
}

//-----------------------------------------------------------------------------------Job::set_state

void Job::set_state( State new_state )
{ 
    if( new_state == s_running )  _thread->_running_tasks_count++;
    if( _state    == s_running )  _thread->_running_tasks_count--;

    _state = new_state;
}

//-----------------------------------------------------------------------------------Job::set_state
/*
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
*/
//-------------------------------------------------------------------------------Job::set_state_cmd

void Job::set_state_cmd( State_cmd cmd )
{ 
    bool ok = false;

    switch( cmd )
    {
        case sc_stop:       ok = true;                  break;

        case sc_unstop:     ok = _state == s_stopped;   if(!ok) break;
                            break;

        case sc_start:      start(NULL);
                            break;

        case sc_end:        ok = _state == s_running;   break;

        case sc_suspend:    ok = _state == s_running;   break;

        case sc_continue:   ok = _state == s_suspended; if(!ok) break;
                            break;


        default:            ok = false;
    }

    if( !ok )  return;    //throw_xc( "SPOOLER-109" );
    
    _state_cmd = cmd;

    _thread->signal( state_cmd_name(cmd) );
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
        case s_running_process:return "running_process";
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

//-----------------------------------------------------------------------------------------Job::xml
// Anderer Thread

xml::Element_ptr Job::xml( xml::Document_ptr document )
{
    xml::Element_ptr job_element = document->createElement( "task" );

    THREAD_LOCK( _lock )
    {
        job_element->setAttribute( "job"  , as_dom_string( _name ) );
        job_element->setAttribute( "state", as_dom_string( state_name() ) );

        if( _state_cmd )  job_element->setAttribute( "cmd", as_dom_string( state_cmd_name() ) );

        if( _state == s_pending  &&  _next_start_time != latter_day )
            job_element->setAttribute( "next_start_time", as_dom_string( _next_start_time.as_string() ) );

        if( _task )
        THREAD_LOCK( _task->_lock )
        {
            job_element->setAttribute( "running_since", as_dom_string( _task->_running_since.as_string() ) );
            job_element->setAttribute( "steps"        , as_dom_string( as_string( _task->_step_count ) ) );
        }

        if( _error )  append_error_element( job_element, _error );
    }

    return job_element;
}

//-------------------------------------------------------------------------------Job::signal_object
// Anderer Thread

void Job::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK( _lock )
    {
        if( _state == Job::s_pending
         && _object_set_descr
         && _object_set_descr->_class->_name == object_set_class_name 
         && _object_set_descr->_level_interval.is_in_interval( level ) )
        {
            start_without_lock();
        }
    }
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& job )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(job)
{
    _let_run = _job->_period.let_run();
    _params  = _job->_params;
}

//---------------------------------------------------------------------------------------Task::Task

Task::~Task()    
{
    try{ close(); } catch(const Xc&) {}
}

//--------------------------------------------------------------------------------------Task::close

void Task::close()
{
    if( _job->_com_task )  _job->_com_task->set_task( NULL );

    do_close();

    // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
    THREAD_LOCK( _terminated_events_lock )  FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "close task" );
}

//--------------------------------------------------------------------------------------Task::start

bool Task::start()
{
    _job->_log.msg( "start" );

    _error = _job->_error = NULL;
    _job->_thread->_task_count++;
    _running_since = Time::now();

    try 
    {
        bool ok = do_start();
        if( !ok || _job->has_error() )  return false;
        _opened = true;
    }
    catch( const Xc& x        ) { _job->set_error(x); }
    catch( const exception& x ) { _job->set_error(x); }

    return !_job->has_error();
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    if( _opened )  
    {
        _job->set_state( Job::s_ending );

        try
        {
            do_end();
        }
        catch( const Xc& x        ) { _job->set_error(x); }
        catch( const exception& x ) { _job->set_error(x); }

        _opened = false;
    }

    on_error_on_success();
    close();
}

//-------------------------------------------------------------------------Task::on_error_on_success

void Task::on_error_on_success()
{
    if( _job->has_error() )
    {
        try
        {
            do_on_error();
        }
        catch( const Xc& x        ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
        catch( const exception& x ) { _job->_log.error( string(spooler_on_error_name) + ": " + x.what() ); }
    }
    else
    {
        try
        {
            do_on_success();
        }
        catch( const Xc& x        ) { _job->set_error(x); }
        catch( const exception& x ) { _job->set_error(x); }
    }
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    bool result;

    try 
    {
        result = do_step();

        _job->_thread->_step_count++;
        _step_count++;
    }
    catch( const Xc& x        ) { _job->set_error(x); return false; }
    catch( const exception& x ) { _job->set_error(x); return false; }

    return result;
}

//----------------------------------------------------------------------Task::wait_until_terminated

bool Task::wait_until_terminated( double wait_time )
{
    if( GetCurrentThreadId() == _job->_thread->_thread_id )  throw_xc( "SPOOLER-125" );     // Deadlock

    Event event ( "Task " + _job->_name + " wait_until_terminated" );
    int   i = 0;
    
    THREAD_LOCK( _terminated_events_lock ) 
    {
        i = _terminated_events.size();
        _terminated_events.push_back( &event );
    }

    bool result = event.wait( wait_time );

    { THREAD_LOCK( _terminated_events_lock )  _terminated_events.erase( &_terminated_events[i] ); }

    return result;
}

//-----------------------------------------------------------------------Script_task::do_on_success

void Script_task::do_on_success()
{
    _job->_script_instance.call_if_exists( spooler_on_success_name );
}

//-----------------------------------------------------------------Script_task::on_error_on_success

void Script_task::do_on_error()
{
    _job->_script_instance.call_if_exists( spooler_on_error_name );
}

//------------------------------------------------------------------------Object_set_task::do_close

void Object_set_task::do_close()
{
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set )  _com_object_set->close();
}

//------------------------------------------------------------------------Object_set_task::do_start

bool Object_set_task::do_start()
{
    bool ok;

    if( !_object_set ) 
    {
        _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
        _com_object_set = new Com_object_set( _object_set );
    }

    if( !_job->_script_instance.loaded() )
    {
        ok = _job->load();
        if( !ok || _job->has_error() )  return false;
    }

    ok = _object_set->open();
    if( !ok )  return false;

    _job->set_state( Job::s_running );
    return true;
}

//--------------------------------------------------------------------------Object_set_task::do_end

void Object_set_task::do_end()
{
    _object_set->close();
    _object_set = NULL;
}

//-------------------------------------------------------------------------Object_set_task::do_step

bool Object_set_task::do_step()
{
    return _object_set->step( _job->_output_level );
}

//------------------------------------------------------------------------Job_script_task::do_start

bool Job_script_task::do_start()
{
    bool ok;

    if( !_job->_script_instance.loaded() )
    {
        ok = _job->load();
        if( !ok || _job->has_error() )  return false;
    }

    ok = check_result( _job->_script_instance.call_if_exists( spooler_open_name ) );
    if( !ok )  return false;

    _job->set_state( Job::s_running );
    return true;
}

//--------------------------------------------------------------------------Job_script_task::do_end

void Job_script_task::do_end()
{
    _job->_script_instance.call_if_exists( spooler_close_name );
}

//-------------------------------------------------------------------------Job_script_task::do_step

bool Job_script_task::do_step()
{
    if( _job->_has_spooler_process )  
        return check_result( _job->_script_instance.call( spooler_process_name ) );
    else
        return false;
}

//----------------------------------------------------------------------------Process_task::do_start

bool Process_task::do_start()
{
    PROCESS_INFORMATION process_info; 
    STARTUPINFO         startup_info; 
    BOOL                ok;

    memset( &process_info, 0, sizeof process_info );

    memset( &startup_info, 0, sizeof startup_info );
    startup_info.cb = sizeof startup_info; 

    string command_line = _job->_process_filename;
    if( !_job->_process_param.empty() )  command_line += " " + _job->_process_param;

    for( int i = 1;; i++ )
    {
        string      nr_str  = as_string( i );
        CComBSTR    nr_bstr = SysAllocString_string( nr_str );
        CComVariant vt;
        HRESULT     hr;

        hr = _params->get_var( nr_bstr, &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr_str.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        hr = vt.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", nr_str.c_str() );

        string param = bstr_as_string( vt.bstrVal );
        if( param.find_first_of(' ') != string::npos )  param = quoted_string( param, '"', '"' );  // Ist Verdoppeln richtig?

        command_line += " " + param;
    }


    ok = CreateProcess( _job->_process_filename.c_str(),  // application name
                        (char*)command_line.c_str(),      // command line 
                        NULL,                       // process security attributes 
                        NULL,                       // primary thread security attributes 
                        FALSE,                      // handles are inherited?
                        0,                          // creation flags 
                        NULL,                       // use parent's environment 
                        NULL,                       // use parent's current directory 
                        &startup_info,              // STARTUPINFO pointer 
                        &process_info );            // receives PROCESS_INFORMATION 
    if( !ok )  throw_mswin_error( "CreateProcess", _job->_process_filename.c_str() );

    CloseHandle( process_info.hThread );

    _process_id = process_info.dwProcessId;
    _process_handle = process_info.hProcess;
    _process_handle.set_name( "Process " + _job->_process_filename );
    _process_handle.add_to( &_job->_thread->_wait_handles );

    _job->set_state( Job::s_running_process );
    return true;
}

//-----------------------------------------------------------------------------Process_task::do_end

void Process_task::do_end()
{
    DWORD exit_code;

    BOOL ok = GetExitCodeProcess( _process_handle, &exit_code );
    if( !ok )  throw_mswin_error( "GetExitCodeProcess" );

    _process_handle.close();
    _result = (int)exit_code;
    if( exit_code )  throw_xc( "SPOOLER-126", exit_code );
}

//----------------------------------------------------------------------------Process_task::do_step

bool Process_task::do_step()
{
    return !_process_handle.signaled();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
