// $Id: spooler_task.cxx,v 1.250 2004/04/06 10:23:42 jz Exp $
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/




#include "spooler.h"
#include "../kram/sleep.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

static const string spooler_get_name        = "spooler_get";
static const string spooler_level_name      = "spooler_level";

//----------------------------------------------------------------------------Spooler_object::level
/*
Level Spooler_object::level()
{
    Variant level = com_property_get( _idispatch, spooler_level_name );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_call( _idispatch, spooler_process_name, output_level );
}

//---------------------------------------------------------------------------Object_set::Object_set

Object_set::Object_set( Spooler* spooler, Module_task* task, const Sos_ptr<Object_set_descr>& descr ) 
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
    bool    ok;
    Variant object_set_vt;

    if( _class->_object_interface )
    {
        Module_instance::In_call( _task->_module_instance, "spooler_make_object_set" );
        object_set_vt = _task->_module_instance->call( "spooler_make_object_set" );

        if( object_set_vt.vt != VT_DISPATCH 
         || object_set_vt.pdispVal == NULL  )  throw_xc( "SCHEDULER-103", _object_set_descr->_class_name );

        _idispatch = object_set_vt.pdispVal;
    }
    else
    {
        _idispatch = _task->_module_instance->dispatch();
    }

    if( com_name_exists( _idispatch, spooler_open_name ) ) 
    {
        Module_instance::In_call in_call ( _task->_module_instance, spooler_open_name );
        ok = check_result( com_call( _idispatch, spooler_open_name ) );
        in_call.set_result( ok );
    }
    else  
        ok = true;

    return ok && !_task->has_error();
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    if( com_name_exists( _idispatch, spooler_close_name ) )  
    {
        Module_instance::In_call in_call ( _task->_module_instance, spooler_close_name );
        com_call( _idispatch, spooler_close_name );
    }

    _idispatch = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        Module_instance::In_call in_call ( _task->_module_instance, spooler_get_name );
        Variant obj = com_call( _idispatch, spooler_get_name );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH
         || obj.pdispVal == NULL  )  throw_xc( "SCHEDULER-102", _object_set_descr->_class_name );
    
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;

        //_log.info( "Objekt-Level " + as_string( object.level() ) + " ist nicht im Intervall" );
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

        if( _task->has_error() )  return false;       // spooler_task.error() gerufen?

        Module_instance::In_call in_call ( _task->_module_instance, spooler_process_name );
        object.process( result_level );

        return true;
    }
    else
    {
        Module_instance::In_call in_call ( _task->_module_instance, spooler_process_name );
        bool result = check_result( _task->_module_instance->call( spooler_process_name, result_level ) );
        in_call.set_result( result );
        return result;
    }
}
*/
//-------------------------------------------------------------------------------Object_set::thread
/*
Spooler_thread* Object_set::thread() const
{ 
    return _task->_job->_thread; 
}
*/
//---------------------------------------------------------------------------------start_cause_name

string start_cause_name( Start_cause cause )
{
    switch( cause )
    {
        case cause_none               : return "none";
        case cause_period_once        : return "period_once";
        case cause_period_single      : return "period_single";
        case cause_period_repeat      : return "period_repeat";
        case cause_job_repeat         : return "job_repeat";
        case cause_queue              : return "queue";
        case cause_queue_at           : return "queue_at";
        case cause_directory          : return "directory";
        case cause_signal             : return "signal";
        case cause_delay_after_error  : return "delay_after_error";
        case cause_order              : return "order";
        case cause_wake               : return "wake";
        default                       : return as_string( (int)cause );
    }
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Job* job )    
: 
    _zero_(this+1), 
    _spooler(job->_spooler), 
    _job(job),
    _log(job->_spooler),
    _history(&job->_history,this),
    _timeout(job->_task_timeout),
    _lock("Task")
  //_success(true)
{
    _let_run = _job->_period.let_run();

    _log.set_job( _job );
    _log.set_task( this );
    _log.inherit_settings( _job->_log );

    set_subprocess_timeout();

    Z_DEBUG_ONLY( _job_name = job->name(); )
}

//--------------------------------------------------------------------------------------Task::~Task
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

Task::~Task()    
{
    try
    { 
        close(); 
    } 
    catch( const exception& x ) { _log.warn( x.what() ); }
}

//--------------------------------------------------------------------------------------Task::close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

void Task::close()
{
    if( !_closed ) 
    {
        FOR_EACH( Subprocesses, _subprocesses, p )  p->second->close();

        if( _operation )
        {
            // Was machen wir jetzt?
            // _operation->kill()?
            LOG( *this << " _operation ist nicht NULL\n" );
            _operation = NULL;
        }

        if( _order )  _order->close();//remove_order_after_error();

        try
        {
            //do_close();
            Async_operation* op = do_close__start();
            if( !op->async_finished() )  _log.warn( "Warten auf Abschluss der Task ..." );
            do_close__end();
        }
        catch( const exception& x ) { _log.error( string("close: ") + x.what() ); }


        // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
        THREAD_LOCK( _terminated_events_lock )  
        {
            FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "task closed" );
            _terminated_events.clear();
        }

        _closed = true;

        _history.end();    // DB-Operation, kann Exception auslösen
    }

    set_state( s_closed );
}

//----------------------------------------------------------------------------------------Task::dom
// s.a. Spooler_command::execute_show_task() zum Aufbau des XML-Elements <task>

xml::Element_ptr Task::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr task_element = document.createElement( "task" );

    THREAD_LOCK( _lock )
    {
        task_element.setAttribute( "id"              , _id );
        task_element.setAttribute( "state"           , state_name() );

        if( _thread )
        task_element.setAttribute( "thread"          , _thread->name() );

        task_element.setAttribute( "name"            , _name );

        if( _running_since )
        task_element.setAttribute( "running_since"   , _running_since.as_string() );

        if( _enqueue_time )
        task_element.setAttribute( "enqueued"        , _enqueue_time.as_string() );

        if( _start_at )
        task_element.setAttribute( "start_at"        , _start_at.as_string() );

        if( _idle_since )
        task_element.setAttribute( "idle_since"      , _idle_since.as_string() );

        if( _cause )
        task_element.setAttribute( "cause"           , start_cause_name( _cause ) );

        if( _state == s_running  &&  _last_process_start_time )
        task_element.setAttribute( "in_process_since", _last_process_start_time.as_string() );

        task_element.setAttribute( "steps"           , _step_count );

        task_element.setAttribute( "log_file"        , _log.filename() );

        if( Module_task* t = dynamic_cast<Module_task*>( this ) )
        {
            if( t->_module_instance )  
            {
                if( t->_module_instance->_in_call )  
                task_element.setAttribute( "calling"         , t->_module_instance->_in_call->name() );

                int pid = t->_module_instance->pid();
                if( pid )  task_element.setAttribute( "pid", pid );       // separate_process="yes", Remote_module_instance_proxy
            }
        }

        if( _order )  dom_append_nl( task_element ),  task_element.appendChild( _order->dom( document, show ) );
        if( _error )  dom_append_nl( task_element ),  append_error_element( task_element, _error );
        
        if( !_subprocesses.empty() )
        {
            xml::Element_ptr subprocesses_element = document.createElement( "subprocesses" );
            FOR_EACH_CONST( Subprocesses, _subprocesses, it )  
            {
                const Subprocess* p = it->second;

                if( p )
                {
                    xml::Element_ptr subprocess_element = document.createElement( "subprocess" );
                    subprocess_element.setAttribute( "pid", p->_pid );

                    if( p->_timeout != latter_day )
                    subprocess_element.setAttribute( "timeout_at", p->_timeout.as_string() );

                    if( p->_killed )
                    subprocess_element.setAttribute( "killed", "yes" );

                    subprocesses_element.appendChild( subprocess_element );
                }
            }
            
            task_element.appendChild( subprocesses_element );
        }

        if( show & show_log )  dom_append_text_element( task_element, "log", _log.as_string() );
    }

    return task_element;
}

//-------------------------------------------------------------------------------Task::enter_thread
// Anderer Thread!

void Task::enter_thread( Spooler_thread* thread )
{ 
    THREAD_LOCK( _lock )
    {
        _thread = thread;  

      //if( dynamic_cast<Remote_module_instance_proxy*>( +_module_instance ) )
        set_state( s_loading );

        thread->add_task( this );       // Jetzt kann der Thread die Task schon starten!
    }
}

//------------------------------------------------------------------------------------Task::cmd_end

void Task::cmd_end( bool kill_immediately )
{ 
    THREAD_LOCK( _lock ) 
    { 
        _end = true; 
        _kill_immediately = kill_immediately;
        if( !_ending_since )  _ending_since = Time::now(); 
        signal( "end" ); 

        if( _state == s_none )
        {
            _job->kill_queued_task( _id );
            // this ist hier möglicherweise ungültig!
        }
    } 
}

//-------------------------------------------------------------------------------Task::cmd_nice_end

void Task::cmd_nice_end( Job* for_job )
{
    //if( for_job )  
        _log.debug( "Task wird dem Job " + for_job->name() + " zugunsten beendet" );
    //         else  _log.debug( "Task wird einem anderen Job zugunsten beendet" );

    cmd_end();
}

//-------------------------------------------------------------------------------Task::leave_thread

void Task::leave_thread()
{ 
    // Thread entfernt die Task, wenn er die Schleife über die _task_list beendet hat. _thread->remove_task( this );  
    _thread = NULL; 
}

//-------------------------------------------------------------------------Task::attach_to_a_thread
// Anderer Thread!

void Task::attach_to_a_thread()
{
    assert( current_thread_id() == _spooler->thread_id() );

    enter_thread( _spooler->select_thread_for_task( this ) );       // Es ist immer derselbe Thread, denn es gibt nur einen
}

//--------------------------------------------------------------------------Task::set_error_xc_only

void Task::set_error_xc_only( const Xc& x )
{
    THREAD_LOCK( _lock )  _error = x;
    _job->set_error_xc_only( x );
}

//-------------------------------------------------------------------------------Task::set_error_xc

void Task::set_error_xc( const Xc& x )
{
    string msg; 

    //Module_task* t = dynamic_cast<Module_task*>( this );
    //Ist nie in _in_call: if( t  &&  t->_module_instance  &&  t->_module_instance->_in_call )  msg = "In " + t->_module_instance->_in_call->name() + "(): ";
    
    _log.error( msg + x.what() );

    set_error_xc_only( x );
}

//----------------------------------------------------------------------------------Task::set_error

void Task::set_error( const exception& x )
{
    if( dynamic_cast< const zschimmer::Xc* >( &x ) ) 
    {
        set_error_xc( *(zschimmer::Xc*)&x );
    }
    else
    if( dynamic_cast< const Xc* >( &x ) ) 
    {
        set_error_xc( *(Xc*)&x );
    }
    else
    {
        Xc xc ( "SOS-2000", x.what(), exception_name(x) );
        set_error_xc( xc );
    }
}

//----------------------------------------------------------------------------------Task::set_error
#ifdef SYSTEM_HAS_COM

void Task::set_error( const _com_error& x )
{
    try
    {
        throw_mswin_error( x.Error(), x.Description() );
    }
    catch( const Xc& x )
    {
        set_error_xc( x );
    }
}

#endif
//----------------------------------------------------------------------------------Task::set_state

void Task::set_state( State new_state )
{ 
    THREAD_LOCK( _lock )  
    {
        _idle_since = 0;

        switch( new_state )
        {
            case s_waiting_for_process:         
                _next_time = latter_day;                        
                break;

            case s_running_process:             
                _next_time = latter_day;                        
                break;

            case s_running_delayed:             
                _next_time = _next_spooler_process;             
                break;

            case s_running_waiting_for_order:
            {
                _next_time  = _job->order_queue()->next_time();

                if( _state != s_running_waiting_for_order )  _idle_since = Time::now();

                if( _job->_idle_timeout != latter_day )
                {
                    _next_time = min( _next_time, _idle_since + _job->_idle_timeout );
                }

                break;
            }

            default:                            
                _next_time = 0;
        }


        if( _next_time && !_let_run )  _next_time = min( _next_time, _job->_period.end() ); // Am Ende der Run_time wecken, damit die Task beendet werden kann

        if( _end )  _next_time = 0;   // Falls vor set_state() cmd_end() gerufen worden ist. Damit _end ausgeführt wird.

        if( new_state != _state ) 
        {
            if( new_state == s_running  ||  new_state == s_starting )  _job->increment_running_tasks(),  _thread->increment_running_tasks();
            if( _state    == s_running  ||  _state    == s_starting )  _job->decrement_running_tasks(),  _thread->decrement_running_tasks();

            if( new_state != s_running_delayed )  _next_spooler_process = 0;

            _state = new_state;


            if( is_idle()  &&  _job->_module._process_class )  _job->_module._process_class->notify_a_process_is_idle();
        //if( is_idle()  &&  _job->_module._process_class  &&  _job->_module._process_class->need_process() )  cmd_nice_end();


            Log_level log_level = new_state == s_starting || new_state == s_closed? log_info : log_debug9;
            if( log_level >= log_info || _spooler->_debug )
            {
                string msg = "state=" + state_name();
                if( _next_time )  msg += " (" + _next_time.as_string() + ")";
                if( new_state == s_starting  &&  _start_at )  msg += " (at=" + _start_at.as_string() + ")";
            //if( new_state == s_starting  &&  _thread->_free_threading )  msg += ", dem Thread " + _thread->name() + " zugeordnet";
                if( new_state == s_starting  &&  _module_instance && _module_instance->pid() )  msg += ", pid=" + as_string( _module_instance->pid() );

                _log.log( log_level, msg );
            }
        }
    }
}

//---------------------------------------------------------------------------------Task::state_name

string Task::state_name( State state )
{
    switch( state )
    {
        case s_none:                        return "none";
        case s_loading:                     return "loading";
        case s_waiting_for_process:         return "waiting_for_process";
      //case s_start_task:                  return "start_task";
        case s_starting:                    return "starting";
        case s_running:                     return "running";
        case s_running_delayed:             return "running_delayed";
        case s_running_waiting_for_order:   return "running_waiting_for_order";
        case s_running_process:             return "running_process";
        case s_suspended:                   return "suspended";
      //case s_end:                         return "end";
        case s_ending:                      return "ending";
        case s_on_success:                  return "on_success";
        case s_on_error:                    return "on_error";
        case s_exit:                        return "exit";
        case s_release:                     return "release";
        case s_ended:                       return "ended";
        case s_closed:                      return "closed";
        default:                            return as_string( (int)state );
    }
}

//-------------------------------------------------------------------------------------Task::signal

void Task::signal( const string& signal_name )
{ 
    THREAD_LOCK( _lock )
    {
        _signaled = true;
        set_next_time( 0 );

        if( _thread )  _thread->signal( signal_name ); 
               //else  Task ist noch nicht richtig gestartet. Passiert, wenn end() von anderer Task gerufen wird.
    }
}

//----------------------------------------------------------------------------------Task::set_cause

void Task::set_cause( Start_cause cause )
{
    if( !_cause )  _cause = cause;
}

//-----------------------------------------------------------------------------Task::has_parameters

bool Task::has_parameters()
{
    int n = 0;
    _params->get_Count( &n );
    return n != 0;
}

//--------------------------------------------------------------------------Task::set_history_field

void Task::set_history_field( const string& name, const Variant& value )
{
  //if( !_job->its_current_task(this) )  throw_xc( "SCHEDULER-138" );

    _history.set_extra_field( name, value );
}

//------------------------------------------------------------------------------Task::set_next_time

void Task::set_next_time( const Time& next_time )
{
    THREAD_LOCK( _lock )  _next_time = next_time;
}

//----------------------------------------------------------------------------------Task::next_time

Time Task::next_time()
{ 
    Time result;
    
    if( _operation )
    {
        result = _operation->async_finished()? Time(0) :                                    // Falls Operation synchron ist (das ist, wenn Task nicht in einem separaten Prozess läuft)
                 _timeout == latter_day      ? latter_day
                                             : Time( _last_operation_time + _timeout );     // _timeout sollte nicht zu groß sein
    }
    else
    {
        result = _next_time;
    }

    if( result > _subprocess_timeout )  result = _subprocess_timeout;

    return result;
}

//------------------------------------------------------------------------------Task::check_timeout

bool Task::check_timeout( const Time& now )
{
    if( _timeout < latter_day  &&  now > _last_operation_time + _timeout  &&  !_kill_tried )
    {
        _log.error( "Task wird nach nach Zeitablauf abgebrochen" );
        return try_kill();
    }

    return false;
}

//------------------------------------------------------------------------------------Task::add_pid

void Task::add_pid( int pid, const Time& timeout_period )
{ 
    Time timeout = latter_day;
    
    if( timeout_period != latter_day )
    {
        timeout = Time::now() + timeout_period;
        _log.debug9( S() << "add_pid(" << pid << ")  Frist endet " << timeout );
    }

    _subprocesses[ pid ] = Z_NEW( Subprocess( this, pid, timeout ) );  

    set_subprocess_timeout();
}

//---------------------------------------------------------------------------------Task::remove_pid

void Task::remove_pid( int pid )
{ 
    _subprocesses.erase( pid );  

    set_subprocess_timeout();
}

//---------------------------------------------------------------------Task::Subprocess::Subprocess

Task::Subprocess::Subprocess( Task* task, int pid, const Time& timeout )
: 
    _spooler(task->_spooler),
    _task(task),
    _pid(pid), 
    _timeout(timeout), 
    _killed(false) 
{
    _spooler->register_pid( pid );
}

//--------------------------------------------------------------------------Task::Subprocess::close

void Task::Subprocess::close()
{
    if( _spooler )  
    {
        _spooler->unregister_pid( _pid );
        _spooler = NULL;
    }
}

//-----------------------------------------------------------------------Task::Subprocess::try_kill
                                
void Task::Subprocess::try_kill()
{ 
    if( !_killed )
    {
        bool ok = try_kill_process_immediately( _pid );

        _task->_log.warn( S() << "Subprozess " << _pid << ( ok? " abgebrochen" : " lässt sich nicht abbrechen" ) );

        _killed = true; 
        _timeout = latter_day; 

        close();
    }
}

//---------------------------------------------------------------------Task::set_subprocess_timeout

void Task::set_subprocess_timeout()
{
    _subprocess_timeout = latter_day;
    FOR_EACH( Subprocesses, _subprocesses, p )  if( _subprocess_timeout > p->second->_timeout )  _subprocess_timeout = p->second->_timeout;

    //if( _subprocess_timeout > _next_time )  set_next_time( _subprocess_timeout );
}

//-------------------------------------------------------------------Task::check_subprocess_timeout

bool Task::check_subprocess_timeout( const Time& now )
{
    bool something_done = false; 

    if( _subprocess_timeout < now )
    {
        FOR_EACH( Subprocesses, _subprocesses, p )  
        {
            if( p->second->_timeout < now )  p->second->try_kill(),  something_done = true;
        }

        set_subprocess_timeout();
    }

    return something_done;
}

//-----------------------------------------------------------------------------------Task::try_kill

bool Task::try_kill()
{
    _kill_tried = true;

    try
    {
        _killed = do_kill();

        FOR_EACH( Subprocesses, _subprocesses, p )  p->second->try_kill();

        if( !_killed ) _log.warn( "Task konnte nicht abgebrochen werden" );
    }
    catch( const exception& x ) { _log.warn( x.what() ); }

    return _killed;
}

//-------------------------------------------------------------------------------Task::do_something

bool Task::do_something()
{
    //Z_DEBUG_ONLY( _log.debug9( "do_something() state=" + state_name() ); )

    bool had_operation      = _operation != NULL;
    bool something_done     = false;
    Time now                = Time::now();
    Time next_time_at_begin = _next_time;

    _signaled = false;

    if( _kill_immediately  &&  !_kill_tried ) 
    {
        _log.error( "Task wird nach Anforderung abgebrochen" );
        return try_kill();
    }

    something_done |= check_subprocess_timeout( now );

    if( _operation &&  !_operation->async_finished() )  
    {
        something_done |= check_timeout( now );
    }
    else
    {
        // Periode endet?
        if( !_operation )
        {
            if( _state == s_running 
             || _state == s_running_process 
             || _state == s_running_delayed  
             || _state == s_running_waiting_for_order )      
            {
                bool let_run = _let_run  ||  _job->_period.is_in_time( now )  ||  ( _job->select_period(now), _job->is_in_period(now) );

                if( !let_run ) 
                {
                    _log( "Laufzeitperiode ist abgelaufen, Task wird beendet" );
                    set_state( s_ending );
                }
            }
        }


        int error_count = 0;

        bool loop = true;
        while( loop )
        {
            try
            {
                try
                {
                    loop = false;
                    bool ok = true;

                    if( !_operation )
                    {
                        if( _module_instance && !_module_instance_async_error ) 
                        {
                            try 
                            { 
                                _module_instance->check_connection_error(); 
                            }  
                            catch( exception& x )
                            { 
                                _module_instance_async_error = true;  
                                throw_xc( "SCHEDULER-202", x.what() );
                            }
                        }

                        if( _state < s_ending  &&  _end )      // Task beenden?
                        {
                            if( !loaded() )         set_state( s_ended );
                            else
                            if( !_begin_called )    set_state( s_release );
                            else
                                                    set_state( s_ending );
                        }

                        // Historie beginnen?
                        if( _state == s_starting 
                         || _state == s_running 
                         || _state == s_running_delayed 
                         || _state == s_running_waiting_for_order 
                         || _state == s_running_process           )
                        {
                            if( _step_count == _job->_history.min_steps() )  _history.start();
                        }
                    }


                    switch( _state )
                    {
                        case s_loading:
                        {
                            load();

                        case s_waiting_for_process:
                            bool ok = !_module_instance || _module_instance->try_to_get_process();
                            if( ok )  something_done = true, set_state( s_starting ), loop = true;
                                else  set_state( s_waiting_for_process );
                            break;
                        }


                        case s_starting:
                        {
                            _begin_called = true;

                            if( !_operation )
                            {
                                _operation = begin__start();
                            }
                            else
                            {
                                ok = operation__end(); 
                                if( _state != s_running_process )  set_state( ok? s_running : s_ending );
                                loop = true;
                            }

                            something_done = true;

                            break;
                        }


                        case s_running_process:
                            if( ((Process_task*)this)->signaled() )
                            {
                                _log( "signaled!" );
                                set_state( s_ending );
                                loop = true;
                            }
                            break;


                        case s_running:
                        {
                            if( !_operation )
                            {
                                if( _next_spooler_process )
                                {
                                    set_state( s_running_delayed );
                                }
                                else
                                {
                                    if( _job->order_queue() )
                                    {
                                        if( !take_order( now ) )
                                        {
                                            set_state( s_running_waiting_for_order );
                                            break;
                                        }

                                        _log.set_order_log( &_order->_log );
                                    }

                                    _last_process_start_time = now;

                                    _operation = do_step__start();

                                    something_done = true;
                                }
                            }
                            else
                            {
                                ok = step__end();
                                _operation = NULL;

                                if( !ok || has_error() )  set_state( s_ending ), loop = true;
                                something_done = true;
                            }

                            break;
                        }


                        case s_running_waiting_for_order:
                        {
                            if( take_order( now ) )
                            {
                                set_state( s_running );     // Auftrag da? Dann Task weiterlaufen lassen (Ende der Run_time wird noch geprüft)
                                loop = true;                // _order wird in step__end() wieder abgeräumt
                            }
                            else
                            if( _job->_idle_timeout != latter_day )
                            {
                                if( now >= _idle_since + _job->_idle_timeout )  
                                {
                                    _log.debug9( "idle_timeout ist abgelaufen, Task beendet sich" );
                                    _end = true;
                                    loop = true;
                                }
                            }
                            break;
                        }


                        case s_running_delayed:  
                        {
                            if( now >= _next_spooler_process )
                            {
                                _next_spooler_process = 0;
                                set_state( s_running ), loop = true;
                            }
                         
                            break;
                        }


                        case s_ending:
                        {
                            if( !_operation )
                            {
                                THREAD_LOCK( _lock ) { if( !_ending_since )  _ending_since = now; }    // Wird auch von cmd_end() gesetzt

                                if( has_error() )  _history.start();

                                if( _begin_called )
                                {
                                    _operation = do_end__start();
                                }
                                else
                                {
                                    set_state( s_exit );
                                    loop = true;
                                }
                            }
                            else
                            {
                                operation__end();

                                set_state( loaded()? has_error()? s_on_error 
                                                                : s_on_success 
                                                : s_release );

                                set_mail_defaults();

                                loop = true;
                            }

                            something_done = true;

                            break;
                        }


                        case s_on_success:
                        {
                            if( !_operation )  _operation = do_call__start( spooler_on_success_name );
                                        else  operation__end(), set_state( s_exit ), loop = true;

                            something_done = true;
                            break;
                        }


                        case s_on_error:
                        {
                            if( !_operation )  _operation = do_call__start( spooler_on_error_name );
                                        else  operation__end(), set_state( s_exit ), loop = true;

                            something_done = true;
                            break;
                        }


                        case s_exit:
                        {
                            if( _job->_module._reuse == Module::reuse_task )
                            {
                                if( !_operation )  _operation = do_call__start( spooler_exit_name );
                                            else  operation__end(), set_state( s_release ), loop = true;
                            }
                            else
                            {
                                set_state( s_release );
                                loop = true;
                            }

                            something_done = true;
                            break;
                        }


                        case s_release:
                        {
                            if( !_operation )  _operation = do_release__start();
                                        else  operation__end(), set_state( s_ended ), loop = true;

                            something_done = true;
                            break;
                        }


                        case s_ended:
                        {
                            if( !_operation )  
                            {
                                _operation = do_close__start();
                            }
                            else
                            {
                                operation__end();
                                finish();

                                // Gesammelte eMail senden, wenn collected_max erreicht:
                                //Time log_time = _log.collect_end();
                                //if( log_time > Time::now()  &&  _next_time > log_time )  set_next_time( log_time );

                                set_state( s_closed );
						        something_done = true;
                                loop = true;
                            }

                            break;
                        }


                        case s_suspended:
                            break;

                        case s_closed:
                            break;

                        case s_none:
                        case s__max:
                            throw_xc( "state=none?" );
                    }


                    if( !_operation )
                    {
                        if( !ok || has_error() )  
                        {
                            if( _state < s_ending )  set_state( s_ending ), loop = true;
                        }

                    
                        if( _killed  &&  _state < s_ended )  set_state( s_ended ), loop = true;
                    }
                }
                catch( _com_error& x )  { throw_com_error( x ); }
            }
        //catch( Stop_scheduler_exception& ) { throw; }
            catch( const exception& x )
            {
                if( error_count == 0 )  set_error( x );
                                else  _log.error( x.what() );

                if( error_count == 0  &&  _state < s_ending )
                {
                    _end = true;
                    loop = true;
                }
                else
                if( error_count <= 1 )
                {
                    loop = false;
                    finish();
                    set_state( s_closed );
                }
                else
                    throw x;    // Fehlerschleife, Scheduler beenden. Sollte nicht passieren.

                error_count++;

                something_done = true;

                //sos_sleep( 5 );  // Bremsen, falls sich der Fehler wiederholt
            }
        }

        if( _operation && !had_operation )  _last_operation_time = now;    // Für _timeout
    }

  //if( _next_time && !_let_run )  set_next_time( min( _next_time, _job->_period.end() ) );                      // Am Ende der Run_time wecken, damit die Task beendet werden kann

/*
    if( _state != s_running  
     && _state != s_running_delayed
     && _state != s_running_waiting_for_order
     && _state != s_running_process  
     && _state != s_suspended                 )  send_collected_log();
*/

    if( !something_done  &&  _next_time <= now  &&  !_signaled )    // Obwohl _next_time erreicht, ist nichts getan?
    {
        // Das kann bei s_running_waiting_for_order passieren, wenn zunächst ein Auftrag da ist (=> _next_time = 0),
        // der dann aber von einer anderen Task genommen wird. Dann ist der Auftrag weg und something_done == false.

        set_state( state() );  // _next_time neu setzen

        if( _next_time <= now )
        {
            LOG( obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time=" << _next_time << ", wird verzögert\n" );
            _next_time = Time::now() + 0.1;
        }
        else
        {
            LOG2( "scheduler.nothing_done", obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time war " << next_time_at_begin << "\n" );
        }
    }


    return something_done;
}

//------------------------------------------------------------------------------------do_something2
/*
bool Task::do_something2()
{
}
*/
//---------------------------------------------------------------------------------------Task::load

void Task::load()
{
    if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
    {
        bool   remove_after_close = false;
        string filename           = _spooler->log_directory() + "/task." + _job->jobname_as_filename();

        if( _job->_max_tasks > 1 )  filename += "." + as_string(_id),  remove_after_close = true;
        filename += ".log";

        _log.set_filename( filename );      // Task-Protokoll
        _log.set_remove_after_close( remove_after_close );
        _log.open();                // Jobprotokoll. Nur wirksam, wenn set_filename() gerufen
    }


    THREAD_LOCK( _lock )
    {
        _thread->count_task();
        reset_error();
        _running_since = Time::now();
    }

    do_load();
    //if( has_error() )  return false;
}

//---------------------------------------------------------------------------------Task::begin_start

Async_operation* Task::begin__start()
{
    return do_begin__start();
}

//-----------------------------------------------------------------------------------Task::step__end

bool Task::step__end()
{
    bool result;

    try 
    {
        result = do_step__end();

        if( has_step_count()  ||  _step_count == 0 )        // Bei Process_task nur einen Schritt zählen
        {
            _thread->count_step();
            _job->count_step();
            _step_count++;
        }


        if( _order )
        {
            _order->postprocessing( result );
            _log.set_order_log( NULL );
            THREAD_LOCK( _lock )  _order = NULL;
            result = true;
        }

        if( _next_spooler_process )  result = true;
    }
    catch( const exception& x ) { set_error(x); result = false; }


    if( _order )  remove_order_after_error();

    return result;
}

//-----------------------------------------------------------------------------Task::operation__end

bool Task::operation__end()
{
    bool result = false;

    try 
    {
        switch( _state )
        {
            case s_starting:    result = do_begin__end();    break;
            case s_ending:               do_end__end();      break;
            case s_on_error:    result = do_call__end();     break;
            case s_on_success:  result = do_call__end();     break;
            case s_exit:        result = do_call__end();     break;
            case s_release:              do_release__end();  break;
            case s_ended:                do_close__end();    break;
            default:            throw_xc( "Task::operation__end" );
        }
    }
    catch( const exception& x ) { set_error(x); result = false; }

    _operation = NULL;

    return result;
}

//---------------------------------------------------------------------------------Task::set_order

void Task::set_order( Order* order )
{                                  
    // Wird von Job gerufen, wenn Task wegen neuen Auftrags startet

    _order = order;
    if( _order )  _order->attach_task( this );  // Auftrag war schon bereitgestellt
}

//---------------------------------------------------------------------------------Task::take_order

Order* Task::take_order( const Time& now )
{
    if( !_order )  THREAD_LOCK( _lock )  set_order( _job->order_queue()->get_order_for_processing( now ) );

    return _order;
}

//-------------------------------------------------------------------Task::remove_order_after_error

void Task::remove_order_after_error()
{
    if( _order )
    {
        _order->processing_error();
        _log.set_order_log( NULL );
        _order = NULL;
    }
}

//-------------------------------------------------------------------------------------Task::finish

void Task::finish()
{
    if( has_error()  &&  _job->repeat() == 0  &&  _job->_delay_after_error.empty() )
    {
        _job->stop( false );
    }
    else
    if( _job->_temporary  &&  _job->repeat() == 0 )  
    {
        _job->stop( false );   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
    }


    close();


    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzögern?

    if( has_error() )
    {
        InterlockedIncrement( &_job->_error_steps );
    
        if( !_job->repeat() )   // spooler_task.repeat hat Vorrang
        {
            Time delay = _job->_delay_after_error.empty()? latter_day : Time(0);

            FOR_EACH( Job::Delay_after_error, _job->_delay_after_error, it )  
                if( _job->_error_steps >= it->first )  delay = it->second;

            if( delay == latter_day )
            {
                _job->stop( false );
            }
            else
            {
                _job->_delay_until = Time::now() + delay;
            }
        }
    }
    else
    {
       _job->_error_steps = 0;
    }

    leave_thread();


    // eMail versenden

    try
    {
        if( !_spooler->_manual )
        {
            set_mail_defaults();
            _log.send( has_error()? -1 : _step_count );
        }

        clear_mail();
    }
    catch( const exception& x  ) { _log.warn( x.what() ); }
    catch( const _com_error& x ) { _log.warn( bstr_as_string(x.Description()) ); }  
}

//----------------------------------------------------------------------Task::wait_until_terminated
// Anderer Thread

bool Task::wait_until_terminated( double wait_time )
{
    Thread_id my_thread_id = current_thread_id();
    if( my_thread_id == _thread->thread_id() )  throw_xc( "SCHEDULER-125" );     // Deadlock

  //Spooler_thread* calling_thread = _spooler->thread_by_thread_id( my_thread_id );
  //if( calling_thread &&  !calling_thread->_free_threading )  throw_xc( "SCHEDULER-131" );

    Event event ( obj_name() + " wait_until_terminated" );

    THREAD_LOCK( _terminated_events_lock )  _terminated_events.push_back( &event );

    bool result = event.wait( wait_time );

    THREAD_LOCK( _terminated_events_lock )  _terminated_events.pop_back();

    return result;
}

//-------------------------------------------------------------------------Task::send_collected_log

void Task::send_collected_log()
{
    try
    {
        _log.send( -2 );
    }
    catch( const exception&  x ) { _spooler->_log.error( x.what() ); }
    catch( const _com_error& x ) { _spooler->_log.error( bstr_as_string(x.Description()) ); }
}

//--------------------------------------------------------------------------Task::set_mail_defaults

void Task::set_mail_defaults()
{
    bool is_error = has_error();

    _log.set_mail_from_name( _job->profile_section() );

    string body = Sos_optional_date_time::now().as_string() + "\n\nJob " + _job->name() + "  " + _job->title() + "\n";
    body += "Task-Id " + as_string(id()) + ", " + as_string(_step_count) + " Schritte\n";
    body += "Scheduler -id=" + _spooler->id() + "  host=" + _spooler->_hostname + "\n\n";

    if( !is_error )
    {
        _log.set_mail_subject( obj_name() + " gelungen" );
    }
    else
    {
        string errmsg = _error? _error->what() : _log.highest_msg();
        _log.set_mail_subject( string("FEHLER ") + errmsg, is_error );
    
        body += errmsg + "\n\n";
    }

    _log.set_mail_body( body + "Das Jobprotokoll liegt dieser Nachricht bei.", is_error );
}

//---------------------------------------------------------------------------------Task::clear_mail

void Task::clear_mail()
{
    _log.set_mail_from_name( "", true );
    _log.set_mail_subject  ( "", true );
    _log.set_mail_body     ( "", true );
}

//-----------------------------------------------------------------------Module_task::do_close__end

Async_operation* Module_task::do_close__start()
{
    if( !_module_instance )  return &dummy_sync_operation;

    _module_instance->detach_task();

    return _module_instance->close__start();
}

//-----------------------------------------------------------------------Module_task::do_close__end

void Module_task::do_close__end()
{
    if( _module_instance )  
    { 
        _module_instance->close__end();

        int exit_code = _module_instance->exit_code();
        if( exit_code != 0 )  _log.warn( "Prozess hat mit Exit code " + as_string(exit_code) + " (0x" + printf_string( "%X", exit_code ) + ") geendet" );

        int termination_signal = _module_instance->termination_signal();
        if( termination_signal != 0 )  _log.warn( "Prozess hat mit Signal " + as_string( termination_signal ) + " geendet" );

        _log.log_file( _module_instance->stdout_filename(), "stdout:" );
        _log.log_file( _module_instance->stderr_filename(), "stderr:" );

      //if( _job->_module_ptr->_reuse == Module::reuse_job )  _job->release_module_instance( _module_instance );

        _module_instance = NULL;
    }
}

//------------------------------------------------------------------------Module_task::close_engine
/*
void Module_task::close_engine()
{
    try 
    {
      //_log.debug3( "close scripting engine" );
        _module_instance->close();
        _module_instance->_com_task = new Com_task();
      //_module_instance = NULL;
    }
    catch( const exception& x ) { set_error(x); }
}
*/
//------------------------------------------------------------------------Object_set_task::do_close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat
/*
void Object_set_task::do_close()
{
    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_object_set )  _com_object_set->clear();
}
*/
//----------------------------------------------------------------------------Object_set_task::load
/*
bool Object_set_task::do_load()
{
    bool ok = true;

    if( !_object_set ) 
    {
        _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
        _com_object_set = new Com_object_set( _object_set );
    }

    if( !_module_instance->loaded() )
    {
        //ok = _module_instance->load();
        //if( !ok || _job->has_error() )  return false;
        throw_xc( "Object_set_task::do_load" );  //ok = do_load();
        if( !ok || has_error() )  return false;
    }

    return ok;
}
*/
//-------------------------------------------------------------------Object_set_task::do_begin__end
/*
bool Object_set_task::do_begin__end()
{
    if( !loaded() )
    {
        bool ok = true;

        if( !_object_set ) 
        {
            _object_set = SOS_NEW( Object_set( _spooler, this, _job->_object_set_descr ) );
            _com_object_set = new Com_object_set( _object_set );
        }

        if( !_module_instance->loaded() )
        {
            //ok = _module_instance->load();
            //if( !ok || _job->has_error() )  return false;
            throw_xc( "Object_set_task::do_start" );  //ok = do_load();
            if( !ok || has_error() )  return false;
        }

        if( !ok )  return false;
    }


    set_state( s_running );

    return _object_set->open();
}

//---------------------------------------------------------------------Object_set_task::do_end__end

void Object_set_task::do_end__end()
{
    _object_set->close();
    _object_set = NULL;
}

//--------------------------------------------------------------------Object_set_task::do_step__end

bool Object_set_task::do_step__end()
{
    return _object_set->step( _job->_output_level );
}
*/
//--------------------------------------------------------------------------Job_module_task::do_kill

bool Job_module_task::do_kill()
{
    return _module_instance? _module_instance->kill() :
           _operation      ? _operation->async_kill() 
                           : false;
}

//-------------------------------------------------------------------------Job_module_task::do_load

void Job_module_task::do_load()
{
    ptr<Module_instance> module_instance;
    bool                 is_new = false;


    if( _job->_module_ptr->_reuse == Module::reuse_job )
    {
        //module_instance = _job->get_free_module_instance( this );
        module_instance = _job->create_module_instance();
    }
    else
    {
        module_instance = _job->create_module_instance();
        is_new = true;
        module_instance->set_close_instance_at_end( true );
        module_instance->set_job_name( _job->name() );      // Nur zum Debuggen (für shell-Kommando ps)
    }

  //module_instance->set_title( obj_name() );
  //module_instance->_com_task->set_task( this );
  //module_instance->_com_log->set_log( &_log );

    if( !module_instance->loaded() )
    {
        module_instance->init();

        module_instance->add_obj( (IDispatch*)_spooler->_com_spooler    , "spooler"        );
      //module_instance->add_obj( (IDispatch*)_job->_thread->_com_thread, "spooler_thread" );
        module_instance->add_obj( (IDispatch*)_job->_com_job            , "spooler_job"    );
        module_instance->add_obj( (IDispatch*)module_instance->_com_task, "spooler_task"   );
        module_instance->add_obj( (IDispatch*)module_instance->_com_log , "spooler_log"    );

        module_instance->load();
        module_instance->start();
    }

    _module_instance = module_instance;

    _module_instance->attach_task( this, &_log );
}

//------------------------------------------------------------------Job_module_task::do_begin_start

Async_operation* Job_module_task::do_begin__start()
{
    //ok = load_module_instance();
    //if( !ok || has_error() )  return false;

    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    return _module_instance->begin__start();
}

//-------------------------------------------------------------------Job_module_task::do_begin__end

bool Job_module_task::do_begin__end()
{
    bool ok;

    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    ok = _module_instance->begin__end();

    return ok;
}

//-------------------------------------------------------------------Job_module_task::do_end__start

Async_operation* Job_module_task::do_end__start()
{
    if( !_module_instance )  return &dummy_sync_operation;

    return _module_instance->end__start( !has_error() );        // Parameter wird nicht benutzt
}

//---------------------------------------------------------------------Job_module_task::do_end__end

void Job_module_task::do_end__end()
{
    if( !_module_instance )  return;

    _module_instance->end__end();
}

//------------------------------------------------------------------Job_module_task::do_step__start

Async_operation* Job_module_task::do_step__start()
{
    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    return _module_instance->step__start();
}

//--------------------------------------------------------------------Job_module_task::do_step__end

bool Job_module_task::do_step__end()
{
    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    return _module_instance->step__end();
}

//------------------------------------------------------------------Job_module_task::do_call__start

Async_operation* Job_module_task::do_call__start( const string& method )
{
    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    return _module_instance->call__start( method );
}

//--------------------------------------------------------------------Job_module_task::do_call__end

bool Job_module_task::do_call__end()
{
    if( !_module_instance )  throw_xc( "SCHEDULER-199" );

    return _module_instance->call__end();
}

//---------------------------------------------------------------Job_module_task::do_release__start

Async_operation* Job_module_task::do_release__start()
{
    if( !_module_instance )  return &dummy_sync_operation; //throw_xc( "SCHEDULER-199" );

    return _module_instance->release__start();
}

//-----------------------------------------------------------------Job_module_task::do_release__end

void Job_module_task::do_release__end()
{
    if( !_module_instance )  return;  //throw_xc( "SCHEDULER-199" );

    _module_instance->release__end();
}

//-----------------------------------------------------------------------Process_task::Process_task

Process_task::Process_task( Job* job ) 
: 
    Task(job),
    _zero_(this+1)

#   ifdef Z_WINDOWS
        ,_process_handle( "process_handle" ) 
#   endif
{
}

//----------------------------------------------------------------------Process_task::~Process_task

Process_task::~Process_task()
{ 
}

//----------------------------------------------------------------------Process_task::do_close__end

void Process_task::do_close__end()
{
#ifdef Z_WINDOWS
    if( _process_handle )
#endif
        do_kill();

    if( _job )
    {
#       ifdef Z_WINDOWS
            _job->_spooler->unregister_process_handle( _process_handle ); 
#        else
            _job->_spooler->unregister_process_handle( _process_handle._pid ); 
#       endif
    }

    _process_handle.close();
}

//----------------------------------------------------------------------Process_task::do_begin__end
#ifdef Z_WINDOWS

bool Process_task::do_begin__end()
{
    if( _spooler->_process_count == max_processes )  throw_xc( "SCHEDULER-210" );

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
        string nr = as_string(i);
        Variant vt;
        HRESULT hr;

        hr = _params->get_Var( Bstr(nr), &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        hr = vt.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_ole( hr, "VariantChangeType", nr.c_str() );

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
    _process_handle.set_handle( process_info.hProcess );
    _process_handle.set_name( "Process " + _job->_process_filename );
  //_process_handle.add_to( &_thread->_wait_handles );
    _process_handle.add_to( &_spooler->_wait_handles );

    _job->_spooler->register_process_handle( _process_handle ); 

    set_state( s_running_process );

    return true;
}

//----------------------------------------------------------------------------Process_task::do_kill

bool Process_task::do_kill()
{
    if( !_process_handle )  return false;

    _log.warn( "Prozess wird abgebrochen" );

    LOG( "TerminateProcess(" <<  _process_handle << ",255)\n" );
    BOOL ok = TerminateProcess( _process_handle, 255 );
    if( !ok )  throw_mswin_error( "TerminateProcess" );

    return true;
}

//------------------------------------------------------------------------Process_task::do_end__end
/*
void Process_task::do_end__end()
{
    DWORD exit_code;

    BOOL ok = GetExitCodeProcess( _process_handle, &exit_code );
    if( !ok )  throw_mswin_error( "GetExitCodeProcess" );

    if( exit_code == STILL_ACTIVE )  throw_xc( "STILL_ACTIVE", obj_name() );

    _process_handle.close();
    _result = (int)exit_code;

    _log.log_file( _job->_process_log_filename ); 

    if( exit_code )
    {
        try
        {
            throw_xc( "SCHEDULER-126", exit_code );
        }
        catch( const exception& x )
        {
            if( !_job->_process_ingore_error  )  throw;
            _log.warn( x.what() );
        }
    }
}
*/
//---------------------------------------------------------------------------Process_task::signaled

bool Process_task::signaled()
{
    bool signaled = _process_handle.signaled();
    //_log.debug3( "signaled=" + as_string( signaled ) );
    return signaled;
}

#endif
//--------------------------------------------------------------------------Process_task::end_start

bool Process_task::do_step__end()
{
    return !signaled();
}

//-----------------------------------------------------------------------------Process_event::close
#ifndef Z_WINDOWS

void Process_event::close()
{
    // waitpid() rufen, falls noch nicht geschehen (um Zombie zu schließen)

    if( _pid )
    {
        int status = 0;

        if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."

        if( log_ptr )  if( ret == -1 )  *log_ptr << "ERRNO-" << errno << "  " << strerror(errno) << endl;
                                  else  *log_ptr << endl;

        _pid = 0;
    }
}

//------------------------------------------------------------------------------Process_event::wait

bool Process_event::wait( double seconds )
{
    if( !_pid )  return true;

    while(1)
    {
        int status = 0;

      //if( log_ptr )  *log_ptr << "waitpid(" << _pid << ")  ";

        int ret = waitpid( _pid, &status, WNOHANG | WUNTRACED );    // WUNTRACED: "which means to also return for children which are stopped, and whose status has not been reported."
        if( ret == -1 )  
        { 
            LOG( "waitpid(" << _pid << ") ==> ERRNO-" << errno << "  " << strerror(errno) << "\n" );
            //int pid = _pid; 
            _pid = 0; 
            //throw_errno( errno, "waitpid", as_string(pid).c_str() ); 
            set_signaled();
            return true;
        }

        if( ret == _pid )
        {
            if( WIFEXITED( status ) )
            {
                _process_exit_code = WEXITSTATUS( status );
              //LOG( "exit_code=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signaled();
                return true;
            }

            if( WIFSIGNALED( status ) )
            {
                _process_signaled = WTERMSIG( status );
              //LOG( "signal=" << _process_exit_code << "\n" );
                _pid = 0;
                set_signaled();
                return true;
            }
        }

      //LOG( "Prozess läuft noch\n" );

        if( seconds < 0.0001 )  break;

        double w = min( 0.1, seconds );
        seconds -= w;

        struct timespec t;
        t.tv_sec = (time_t)floor( w );
        t.tv_nsec = (time_t)max( 0.0, ( w - floor( w ) ) * 1e9 );

        int err = nanosleep( &t, NULL );
        if( err )
        {
            if( errno == EINTR )  return true;
            throw_errno( errno, "nanosleep" );
        }
    }

    return false;
}

#endif
//------------------------------------------------------------------Process_task::do_begin__end
#ifndef Z_WINDOWS

bool Process_task::do_begin__end()
{
    if( _spooler->_process_count == max_processes )  throw_xc( "SCHEDULER-210" );

    vector<string> string_args;

    string_args.push_back( _job->_process_filename );   // argv[0]
    if( _job->_process_param != "" )  string_args.push_back( _job->_process_param );

    for( int i = 1;; i++ )
    {
        string nr = as_string(i);
        Variant vt;
        HRESULT hr;

        hr = _params->get_Var( Bstr(nr), &vt );
        if( FAILED(hr) )  throw_ole( hr, "Variable_set.var", nr.c_str() );

        if( vt.vt == VT_EMPTY )  break;

        string_args.push_back( string_from_variant( vt ) );
    }


#   ifndef SYSTEM_WINDOWS
        LOG( "signal(SIGCHLD,SIG_DFL)\n" );
        ::signal( SIGCHLD, SIG_DFL );                 // Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.
#   endif

    if( log_ptr )  *log_ptr << "fork()  ";
    int pid = fork();

    switch( pid )
    {
        case -1:    
            throw_errno( errno, "fork" );
        
        case 0:
        {
            char** args = new char* [ string_args.size() + 1 ];
            int    i;

            for( i = 0; i < string_args.size(); i++ )  args[i] = (char*)string_args[i].c_str();
            args[i] = NULL;

            execvp( _job->_process_filename.c_str(), args );

            fprintf( stderr, "ERRNO-%d  %s, bei execlp(\"%s\")\n", errno, strerror(errno), _job->_process_filename.c_str() );
            _exit( errno? errno : 250 );  // Wie melden wir den Fehler an den rufenden Prozess?
        }

        default:
            LOG( "pid=" << pid << "\n" );
            _process_handle._pid = pid;
            break;
    }


    set_state( s_running_process );

    _process_handle.set_name( "Process " + _job->_process_filename );
    _process_handle.add_to( &_spooler->_wait_handles );

    _job->_spooler->register_process_handle( _process_handle._pid ); 

    _operation = &dummy_sync_operation;

    return true;
}

//----------------------------------------------------------------------------Process_task::do_kill

bool Process_task::do_kill()
{
    if( _process_handle._pid )
    {
        _log.warn( "Prozess wird abgebrochen" );

        LOG( "kill(" << _process_handle._pid << ",SIGTERM)\n" );
        int err = kill( _process_handle._pid, SIGTERM );
        if( err )  throw_errno( errno, "killpid" );

        //? _process_handle._pid = 0;
        return true;
    }
    
    return false;
}

#endif
//------------------------------------------------------------------------Process_task::do_end__end

void Process_task::do_end__end()
{
#   ifdef Z_WINDOWS

        DWORD exit_code;

        BOOL ok = GetExitCodeProcess( _process_handle, &exit_code );
        if( !ok )  throw_mswin_error( "GetExitCodeProcess" );

        if( exit_code == STILL_ACTIVE )  throw_xc( "STILL_ACTIVE", obj_name() );

        _process_handle.close();

#    else

        if( _process_handle._pid )
        {
            do_step__end();      // waitpid() sollte schon gerufen sein. 
            if( _process_handle._pid )   throw_xc( "SCHEDULER-179", _process_handle._pid );       // Sollte nicht passieren (ein Zombie wird stehen bleiben)
        }

        _process_handle.close();

        if( _process_handle._process_signaled )  throw_xc( "SCHEDULER-181", _process_handle._process_signaled );

        int exit_code = _process_handle._process_exit_code;

#   endif

    _result = (int)exit_code;

    _log.log_file( _job->_process_log_filename ); 

    if( exit_code )
    {
        try
        {
            throw_xc( "SCHEDULER-126", exit_code );
        }
        catch( const exception& x )
        {
            if( !_job->_process_ingore_error )  throw;
            _log.warn( x.what() );
        }
    }
}

//---------------------------------------------------------------------------Process_task::signaled
#ifndef Z_WINDOWS

bool Process_task::signaled()
{
    _process_handle.wait( 0 );
    return _process_handle.signaled();
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
