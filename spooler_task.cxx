// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
/*
    Hier sind implementiert

    Spooler_object
    Object_set
    Task
*/




#include "spooler.h"
#include "file_logger.h"
#include "../kram/sleep.h"
#include "../zschimmer/z_signals.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif

#define THREAD_LOCK_DUMMY( x )

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

static const string             spooler_get_name                = "spooler_get";
static const string             spooler_level_name              = "spooler_level";

//------------------------------------------------------------------------------Task_lock_requestor

struct Task_lock_requestor : lock::Requestor
{
    Task_lock_requestor( Task* task )
    : 
        Requestor( task ), 
        _task(task) 
    {
    }

    ~Task_lock_requestor()
    {
    }


    // Requestor:

    bool locks_are_available() const
    {
        return locks_are_available_for_holder( _task->_lock_holder );
    }

    void on_locks_are_available()                                      
    { 
        _task->on_locks_are_available( this );
    }

  //void                        on_removing_lock            ( lock::Lock* l )                       { _job->on_removing_lock( l ); }

  private:
    Task*                      _task;
};

//---------------------------------------------------------------------------------start_cause_name

string start_cause_name( Start_cause cause )
{
    switch( cause )
    {
        case cause_none               : return "none";
        case cause_period_once        : return "period_once";
        case cause_period_single      : return "period_single";
        case cause_period_repeat      : return "period_repeat";
      //case cause_job_repeat         : return "job_repeat";
        case cause_queue              : return "queue";
        case cause_queue_at           : return "queue_at";
        case cause_directory          : return "directory";
        case cause_signal             : return "signal";
        case cause_delay_after_error  : return "delay_after_error";
        case cause_order              : return "order";
        case cause_wake               : return "wake";
        case cause_min_tasks          : return "min_tasks";
        default                       : return as_string( (int)cause );
    }
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task( Job* job )
:
    Scheduler_object( job->_spooler, this, Scheduler_object::type_task ),
    _zero_(this+1),
    _job(job),
    _history(&job->_history,this),
    _timeout(job->_task_timeout),
    _lock("Task"),
    _lock_requestors( 1+lock_level__max ),
    _warn_if_longer_than( Time::never )
{
    _log = Z_NEW( Prefix_log( this ) );

    _let_run = _job->_period.let_run();

    _log->set_job_name( job->name() );
    _log->set_task( this );
    _log->inherit_settings( *_job->_log );
    _log->set_mail_defaults();

    _idle_timeout_at = Time::never;

    set_subprocess_timeout();
    _warn_if_shorter_than = _job->get_step_duration_or_percentage( _job->_warn_if_shorter_than_string, Time(0) );
    _warn_if_longer_than  = _job->get_step_duration_or_percentage( _job->_warn_if_longer_than_string , Time::never );

    Z_DEBUG_ONLY( _job_name = job->name(); )

    _params = new Com_variable_set();
    _params->merge( job->_default_params );

    _environment = new Com_variable_set(); 

    _lock_holder = Z_NEW( lock::Holder( this ) );
}

//--------------------------------------------------------------------------------------Task::~Task
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

Task::~Task()
{
    if( _file_logger )  _file_logger->set_async_manager( NULL );

    try
    {
        close();
    }
    catch( const exception& x ) { _log->warn( x.what() ); }

    _log->close();
}

//----------------------------------------------------------------------------------Task::job_close

void Task::job_close()
{
    close();
    _job = NULL;
}

//--------------------------------------------------------------------------------------Task::close
// Kann von anderem Thread gerufen werden, wenn der noch eine COM-Referenz hat

void Task::close()
{
    if( !_closed )
    {
        if( _file_logger )
        {
            _file_logger->set_async_manager( NULL );
            _file_logger->close();
        }

        FOR_EACH( Registered_pids, _registered_pids, p )  p->second->close();

        if( _operation )
        {
            // Was machen wir jetzt?
            // _operation->kill()?
            Z_LOG2( "scheduler", *this << ".close(): Operation aktiv: " << _operation->async_state_text() << "\n" );

            try
            {
                _operation->async_kill();  // do_kill() macht nachher das gleiche
            }
            catch( exception& x )  { Z_LOG2( "scheduler", "Task::close() _operation->async_kill() ==> " << x.what() << "\n" ); }

            _operation = NULL;
        }


        _order_for_task_end = NULL;
        
        if( _order )  
        {
            _order->remove_from_job_chain();
            _order->close();  //remove_order_after_error(); Nicht rufen! Der Auftrag bleibt stehen und der Job startet wieder und wieder.
        }


        if( _module_instance )
        {
            try
            {
                do_kill();
            }
            catch( exception& x )  { Z_LOG2( "scheduler", "Task::close() do_kill() ==> " << x.what() << "\n" ); }

            _module_instance->detach_task();
            _module_instance->close();
        }

/* 2005-09-24 nicht blockieren. Scheduler kann sich beenden (nach timeout), auch wenn Tasks laufen
        try
        {
            //do_close();
            Async_operation* op = do_close__start();
            if( !op->async_finished() )  _log->warn( "Warten auf Abschluss der Task ..." );
            do_close__end();
        }
        catch( const exception& x ) { _log->error( string("close: ") + x.what() ); }
*/

        // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
        THREAD_LOCK_DUMMY( _terminated_events_lock )
        {
            FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "task closed" );
            _terminated_events.clear();
        }

        _closed = true;

        _history.end();    // DB-Operation, kann Exception auslösen
    }

    if( _lock_holder )  _lock_holder->release_locks(),  _lock_holder = NULL;

    set_state( s_closed );
}

//---------------------------------------------------------------------------------------Task::init

void Task::init()
{
    set_state( s_loading );

    _file_logger = Z_NEW( File_logger( _log ) );
    _file_logger->set_object_name( obj_name() );
    _spooler->_task_subsystem->add_task( this );       // Jetzt kann der Thread die Task schon starten!
}

//------------------------------------------------------------------------------------Task::set_dom

void Task::set_dom( const xml::Element_ptr& element )
{
    _force_start = element.bool_getAttribute( "force_start" );

    string web_service_name = element.getAttribute( "web_service" );
    if( web_service_name != "" )  set_web_service( web_service_name );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "environment" ) )
        {
            _environment->set_dom( e, NULL, "variable" );
        }
    }
}

//--------------------------------------------------------------------------------Task::dom_element
// s.a. Spooler_command::execute_show_task() zum Aufbau des XML-Elements <task>

xml::Element_ptr Task::dom_element( const xml::Document_ptr& document, const Show_what& show ) const
{
    xml::Element_ptr task_element = document.createElement( "task" );

    if( !show.is_set( show_for_database_only ) )
    {
        if( _job )
        task_element.setAttribute( "job"             , _job->path() );

        task_element.setAttribute( "id"              , _id );
        task_element.setAttribute( "task"            , _id );
        task_element.setAttribute( "state"           , state_name() );

        if( _delayed_after_error_task_id )
        task_element.setAttribute( "delayed_after_error_task", _delayed_after_error_task_id );

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

        task_element.setAttribute( "log_file"        , _log->filename() );

        if( const Module_task* t = dynamic_cast<const Module_task*>( this ) )
        {
            if( t->_module_instance )
            {
                if( t->_module_instance->_in_call )
                task_element.setAttribute( "calling"         , t->_module_instance->_in_call->name() );

                task_element.setAttribute_optional( "process", t->_module_instance->process_name() );

                int pid = t->_module_instance->pid();
                if( pid )
                {
                    task_element.setAttribute( "pid", pid );       // separate_process="yes", Remote_module_instance_proxy

                    try
                    {
                        zschimmer::Process process ( pid );
                        task_element.setAttribute( "priority", process.priority_class() );
                    }
                    catch( exception& x )
                    {
                        Z_LOG2( "scheduler", Z_FUNCTION << " priority_class() ==> " << x.what() << "\n" );
                    }
                }
            }
        }

      //if( _lock_holder )  task_element.appendChild( _lock_holder->dom_element( document, show ) );

        if( Order* order = _order? _order : _order_for_task_end )  dom_append_nl( task_element ),  task_element.appendChild( order->dom_element( document, show ) );
        if( _error )  dom_append_nl( task_element ),  append_error_element( task_element, _error );

        if( !_registered_pids.empty() )
        {
            xml::Element_ptr subprocesses_element = document.createElement( "subprocesses" );
            FOR_EACH_CONST( Registered_pids, _registered_pids, it )
            {
                const Registered_pid* p = it->second;

                if( p )
                {
                    xml::Element_ptr subprocess_element = document.createElement( "subprocess" );
                    subprocess_element.setAttribute( "pid", p->_pid );

                    try
                    {
                        zschimmer::Process process ( p->_pid );
                        subprocess_element.setAttribute( "priority", process.priority_class() );
                    }
                    catch( exception& x )
                    {
                        Z_LOG2( "scheduler", Z_FUNCTION << " priority_class() ==> " << x.what() << "\n" );
                    }


                    if( p->_timeout_at != Time::never )
                    subprocess_element.setAttribute( "timeout_at", p->_timeout_at.as_string() );

                    if( p->_title != "" )
                    subprocess_element.setAttribute( "title", p->_title );

                    if( p->_killed )
                    subprocess_element.setAttribute( "killed", "yes" );

                    if( p->_ignore_error )
                    subprocess_element.setAttribute( "ignore_error", "yes" );

                    if( p->_ignore_signal )
                    subprocess_element.setAttribute( "ignore_signal", "yes" );

                    if( p->_wait )
                    subprocess_element.setAttribute( "wait", "yes" );

                    subprocesses_element.appendChild( subprocess_element );
                }
            }

            task_element.appendChild( subprocesses_element );
        }

        task_element.appendChild( _log->dom_element( document, show ) );
    }

    task_element.setAttribute( "force_start", _force_start? "yes" : "no" );

    if( _web_service )
    task_element.setAttribute( "web_service"     , _web_service->name() );

    if( _environment  &&  !_environment->is_empty() )
        task_element.appendChild( _environment->dom_element( document, "environment", "variable" ) );

    return task_element;
}

//----------------------------------------------------------------------------------------Task::dom

xml::Document_ptr Task::dom( const Show_what& show ) const
{
    xml::Document_ptr document;

    document.create();
    document.appendChild( dom_element( document, show ) );

    return document;
}

//-------------------------------------------------------------------Task::write_element_attributes

void Task::write_element_attributes( const xml::Element_ptr& element ) const 
{ 
    element.setAttribute( "job" , _job->path() ); 
    element.setAttribute( "task", _id );
}

//----------------------------------------------------------------------------------------Task::job

Job* Task::job()
{
    if( !_job )  assert(0), throw_xc( "TASK-WITHOUT-JOB", obj_name() );
    return _job;
}

//----------------------------------------------------------------------Task::calculated_start_time

Time Task::calculated_start_time( const Time& now )
{
    Time result;

    if( _force_start )
    {
        result = _start_at;
    }
    else
    if( _job->file_based_state() == Job::s_active )
    {
        result = _job->schedule_use()->next_allowed_start( max( _start_at, now ) );
    }

    return result;
}

//------------------------------------------------------------------------------------Task::cmd_end

void Task::cmd_end( End_mode end_mode )
{
    assert( end_mode != end_none );

    THREAD_LOCK_DUMMY( _lock )
    {
        //if( end_mode == end_kill_immediately  &&  _end != end_kill_immediately )  _log->warn( message_string( "SCHEDULER-282" ) );    // Kein Fehler, damit ignore_signals="SIGKILL" den Stopp verhindern kann
        if( end_mode == end_normal  &&  _state < s_ending  &&  !_end )  _log->info( message_string( "SCHEDULER-914" ) );

        if( _end != end_kill_immediately )  _end = end_mode;
        if( !_ending_since )  _ending_since = Time::now();

        if( end_mode == end_kill_immediately  &&  !_kill_tried )
        {
            //_log->warn( message_string( "SCHEDULER-277" ) );   // Kein Fehler, damit ignore_signals="SIGKILL" den Stopp verhindern kann
            try_kill();
        }

        if( end_mode != end_kill_immediately )  signal( "end" );

        if( _state == s_none )
        {
            if( _job )  _job->kill_queued_task( _id );
            // this ist hier möglicherweise ungültig!
        }
    }
}

//-------------------------------------------------------------------------------Task::cmd_nice_end

void Task::cmd_nice_end( Job* for_job )
{
    _log->info( message_string( "SCHEDULER-271", for_job->name() ) );   //"Task wird dem Job " + for_job->name() + " zugunsten beendet" );

    cmd_end( end_nice );
}

//--------------------------------------------------------------------------Task::set_error_xc_only

void Task::set_error_xc_only( const zschimmer::Xc& x )
{
    string code = x.code();
    
    //if( dynamic_cast<const com::object_server::Connection_reset_exception*>( &x )    Funktioniert nicht mit gcc 3.4.3
    if( x.name() == com::object_server::Connection_reset_exception::exception_name 
     || x.code() == "SCHEDULER-202" )
    {
        if( !_is_connection_reset_error )  // Bisheriger _error ist kein Connection_reset_error?
        {
            _non_connection_reset_error = _error;  // Bisherigen Fehler (evtl. NULL) merken, falls Verbindungsverlust wegen ignore_signals=".." ignoriert wird
            _is_connection_reset_error = true;
        }
    }
    else
    {
        _non_connection_reset_error = NULL;
        _is_connection_reset_error = false;
    }

    set_error_xc_only_base( x );
}

//--------------------------------------------------------------------------Task::set_error_xc_only

void Task::set_error_xc_only( const Xc& x )
{
    _non_connection_reset_error = NULL;
    _is_connection_reset_error = false;

    set_error_xc_only_base( x );
}

//---------------------------------------------------------------------Task::set_error_xc_only_base

void Task::set_error_xc_only_base( const Xc& x )
{
    _error = x;
    if( _job )  _job->set_error_xc_only( x );
    if( _order )  _order->set_task_error( x );

    _exit_code = 1;
}

//-------------------------------------------------------------------------------Task::set_error_xc
// Siehe auch set_error_xc( const Xc& )

void Task::set_error_xc( const zschimmer::Xc& x )
{
    set_error_xc_only( x );
    _log->error( x.what() );


    if( _is_connection_reset_error  &&  !_non_connection_reset_error )  
    {
        S s;

        if( !_job )
        {
            s << "(no job)";
        }
        else
        {
            if( _job->_ignore_every_signal )  
                s << "all";
            else
            {
                Z_FOR_EACH( stdext::hash_set<int>, _job->_ignore_signals_set, it )
                {
                    if( s.length() > 0 )  s << " ";
                    s << signal_name_from_code( *it );
                }
            }
        }

        if( s.length() > 0 )  _log->info( message_string( "SCHEDULER-974", s ) );
    }
}

//-------------------------------------------------------------------------------Task::set_error_xc
// Siehe auch set_error_xc( const zschimmer::Xc& )

void Task::set_error_xc( const Xc& x )
{
    set_error_xc_only( x );
    _log->error( x.what() );
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
        Xc xc ( "SOS-2000", x.what(), exception_name(x).c_str() );      // Siehe auch Spooler_com::set_error_silently
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

//------------------------------------------------------------------------------Task::set_exit_code
/*
void Task::set_exit_code( int exit_code )
{
    if( !_module_instance )  z::throw_xc( message_string( "SCHEDULER-323" ) );

    _module_instance->set_exit_code( exit_code );
}

//----------------------------------------------------------------------------------Task::exit_code

int Task::exit_code()
{
    if( !_module_instance )  z::throw_xc( message_string( "SCHEDULER-323" ) );

    return _module_instance->exit_code( exit_code );
}
*/
//----------------------------------------------------------------------------------Task::set_state

void Task::set_state( State new_state )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        if( new_state != s_running_waiting_for_order )  _idle_since = 0;

        if( new_state != _state )
        {
            switch( _state )
            {
                case s_running:
                    if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] )  
                    {
                        _lock_holder->release_locks( lock_requestor );
                        lock_requestor->dequeue_lock_requests();
                    }

                    break;

                default:
                    break;
            }
        }

        switch( new_state )
        {
            case s_waiting_for_process:
                _next_time = Time::never;
                break;

            case s_opening_waiting_for_locks:
                _next_time = Time::never;
                break;

            case s_running_process:
                _next_time = Time::never;
                break;

            case s_running_delayed:
                _next_time = _next_spooler_process;
                break;

            case s_running_waiting_for_locks:
                _next_time = Time::never;
                break;

            case s_running_waiting_for_order:
            {
                _next_time  = _job->combined_job_nodes()->next_time();

                if( _state != s_running_waiting_for_order )  _idle_since = Time::now();

                if( _idle_timeout_at != Time::never )
                {
                    _next_time = min( _next_time, _idle_timeout_at );
                }

                break;
            }

            default:
                _next_time = 0;
        }


        if( _next_time && !_let_run && _job )  _next_time = min( _next_time, _job->_period.end() ); // Am Ende der Run_time wecken, damit die Task beendet werden kann

        if( _end )  _next_time = 0;   // Falls vor set_state() cmd_end() gerufen worden ist. Damit _end ausgeführt wird.

        if( new_state != _state )
        {
            if( _job  &&  _spooler->_task_subsystem )
            {
                if( new_state == s_starting ||  new_state == s_opening  ||  new_state == s_running )  _job->increment_running_tasks(),  _spooler->_task_subsystem->increment_running_tasks();
                if( _state    == s_starting ||  _state    == s_opening  ||  _state    == s_running )  _job->decrement_running_tasks(),  _spooler->_task_subsystem->decrement_running_tasks();
            }

            if( new_state != s_running_delayed )  _next_spooler_process = 0;

            State old_state = _state;
            _state = new_state;

            if( is_idle()  &&  _job )
                if( Process_class* process_class = _job->_module->process_class_or_null() )  process_class->notify_a_process_is_idle();


            Log_level log_level = new_state == s_starting || new_state == s_closed? log_info : log_debug9;
            if( ( log_level >= log_info || _spooler->_debug )  &&  ( _state != s_closed || old_state != s_none ) )
            {
                S details;
                if( _next_time )  details << " (" << _next_time << ")";
                if( new_state == s_starting  &&  _start_at )  details << " (at=" << _start_at << ")";
                if( new_state == s_starting  &&  _module_instance && _module_instance->process_name() != "" )  details << ", process " << _module_instance->process_name();

                _log->log( log_level, message_string( "SCHEDULER-918", state_name(), details ) );
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
        case s_opening:                     return "opening";
        case s_opening_waiting_for_locks:   return "opening_waiting_for_locks";
        case s_running:                     return "running";
        case s_running_delayed:             return "running_delayed";
        case s_running_waiting_for_locks:   return "running_waiting_for_locks";
        case s_running_waiting_for_order:   return "running_waiting_for_order";
        case s_running_process:             return "running_process";
        case s_running_remote_process:      return "running_remote_process";
        case s_suspended:                   return "suspended";
      //case s_end:                         return "end";
        case s_ending:                      return "ending";
        case s_ending_waiting_for_subprocesses: return "ending_waiting_for_subprocesses";
        case s_on_success:                  return "on_success";
        case s_on_error:                    return "on_error";
        case s_exit:                        return "exit";
        case s_release:                     return "release";
        case s_killing:                     return "killing";
        case s_ended:                       return "ended";
        case s_deleting_files:              return "deleting_files";
        case s_closed:                      return "closed";
        default:                            return as_string( (int)state );
    }
}

//-------------------------------------------------------------------------------------Task::signal

void Task::signal( const string& signal_name )
{
    THREAD_LOCK_DUMMY( _lock )
    {
        _signaled = true;
        set_next_time( 0 );

        if( _spooler->_task_subsystem )  _spooler->_task_subsystem->signal( signal_name );
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
  //if( !_job->its_current_task(this) )  z::throw_xc( "SCHEDULER-138" );

    _history.set_extra_field( name, value );
}

//------------------------------------------------------------------Task::set_delay_spooler_process

void Task::set_delay_spooler_process( Time t )
{ 
    _log->debug("delay_spooler_process=" + t.as_string() ); 
    _next_spooler_process = Time::now() + t; 
}

//------------------------------------------------------------------------------Task::try_hold_lock

bool Task::try_hold_lock( const Path& lock_relative_path, lock::Lock::Lock_mode lock_mode )
{
    if( _state != s_opening  &&
        _state != s_running     )  z::throw_xc( "SCHEDULER-468" );

    assert( !_lock_requestors[ lock_level_process_api ]  ||  _state == s_running );


    Absolute_path         lock_path      ( _job->folder_path(), lock_relative_path );
    ptr<lock::Requestor>& lock_requestor = _lock_requestors[ current_lock_level() ];

    if( !lock_requestor )
    {
        lock_requestor = Z_NEW( Task_lock_requestor( this ) );
        _lock_holder->add_requestor( lock_requestor );
    }

    lock::Use* lock_use = lock_requestor->add_lock_use( lock_path, lock_mode );

    return _lock_holder->try_hold( lock_use );
}

//----------------------------------------------------------------Task::delay_until_locks_available

void Task::delay_until_locks_available()
{
    if( _state != s_opening  &&
        _state != s_running     )  z::throw_xc( "SCHEDULER-468" );        // Verhindert auch doppelten Aufruf 

    Lock_level       lock_level     = current_lock_level();
    lock::Requestor* lock_requestor = _lock_requestors[ lock_level ];

    if( !lock_requestor )  z::throw_xc( "SCHEDULER-468", "No try_hold_lock()" );             // try_hold_lock() nicht aufgerufen
    if( _lock_holder->is_holding_all_of( lock_requestor ) )  z::throw_xc( "SCHEDULER-468" );   // Sperren werden bereits gehalten

    // Sicherstellen, dass try_hold_lock() ohne Erfolg aufgerufen worden ist
    // begin__start() erneut aufrufen mit dem Parameter, dass der letzte Aufruf (spooler_init, spooler_open) wiederholt werden soll.
    // Wie stellen wir sicher, dass wir nicht im Konstruktor sind? Wiederholen wir die Konstruktion? In Module_instance::begin__end
    // Oder Exception erst bei der Wiederholung, etwas spät, aber es genügt.

    _delay_until_locks_available = true;
}

//-------------------------------------------------------------------------Task::current_lock_level

Task::Lock_level Task::current_lock_level() 
{
    if( _state == s_opening  ||
        _state == s_opening_waiting_for_locks )  return lock_level_task_api;
    
    if( _state == s_running  ||
        _state == s_running_waiting_for_locks )  return lock_level_process_api;

    z::throw_xc( Z_FUNCTION );
}

//---------------------------------------------------------------------Task::on_locks_are_available

void Task::on_locks_are_available( Task_lock_requestor* lock_requestor )
{    
    assert( _lock_holder->is_known_requestor( lock_requestor ) );
    assert( lock_requestor == _lock_requestors[ current_lock_level() ] );
    assert( lock_requestor->is_enqueued() );
    assert( lock_requestor->locks_are_available_for_holder( _lock_holder ) );
    assert( _state == s_opening_waiting_for_locks ||
            _state == s_running_waiting_for_locks    );

    switch( _state )
    {
        case s_opening_waiting_for_locks:   set_state( s_opening );  break;
        case s_running_waiting_for_locks:   set_state( s_running );  break;
        default:                            z::throw_xc( Z_FUNCTION );
    }
    
    signal( Z_FUNCTION );
}

//------------------------------------------------------------------------------Task::set_next_time

void Task::set_next_time( const Time& next_time )
{
    THREAD_LOCK_DUMMY( _lock )  _next_time = next_time;
}

//----------------------------------------------------------------------------------Task::next_time

Time Task::next_time()
{
    // Besser, ein neues _next_time wird an der Stelle gesetzt, wo das Ereignis auftritt //


    Time result;

    if( _end == end_kill_immediately  &&  !_kill_tried )
    {
        result = Time(0);
    }
    else
    if( _operation )
    {
        if( _operation->async_finished() ) {
            result = 0;   // Falls Operation synchron ist (das ist, wenn Task nicht in einem separaten Prozess läuft)
        } else {
            Time t = _timeout;
            if( _state == s_running ||
                _state == s_running_process ||
                _state == s_running_remote_process )  t = min( t, _warn_if_longer_than );

            result = t.is_never()? Time::never 
                                 : _last_operation_time + t;     // _timeout sollte nicht zu groß sein
        }                 
    }
    else
    if( _state == s_running_process  &&  !_timeout.is_never() )
    {
        result = Time( _last_operation_time + _timeout );     // _timeout sollte nicht zu groß sein
    }
    else
    {
        result = _next_time;
        if( _state == s_running_delayed  &&  result > _next_spooler_process )  result = _next_spooler_process;
    }

    if( result > _subprocess_timeout )  result = _subprocess_timeout;

    return result;
}

//------------------------------------------------------------------------------Task::check_timeout

bool Task::check_timeout( const Time& now )
{
    if( _timeout < Time::never  &&  _last_operation_time  &&  now > _last_operation_time + _timeout  &&  !_kill_tried )
    {
        _log->error( message_string( "SCHEDULER-272", _timeout.as_time_t() ) );   // "Task wird nach nach Zeitablauf abgebrochen"
        return try_kill();
    }

    return false;
}

//----------------------------------------------------------------------Task::check_if_shorter_than

void Task::check_if_shorter_than( const Time& now )
{
    if( _warn_if_shorter_than ) 
    {
        Time step_time = now - _last_operation_time;

        if( step_time < _warn_if_shorter_than ) 
        {
            string msg = message_string( "SCHEDULER-711", _warn_if_shorter_than.as_string( Time::without_ms ), step_time.as_string( Time::without_ms ) );
            _log->warn( msg );

            Scheduler_event scheduler_event ( evt_task_step_too_short, log_error, _spooler );
            Mail_defaults mail_defaults( _spooler );
            mail_defaults.set( "subject", S() << obj_name() << ": " << msg );
            mail_defaults.set( "body"   , S() << obj_name() << ": " << msg << "\n"
                                          "Step time: " << step_time << "\n" <<
                                          "warn_if_shorter_than=" << _warn_if_shorter_than.as_string( Time::without_ms ) );
            scheduler_event.send_mail( mail_defaults );
        }
    }
}

//-----------------------------------------------------------------------Task::check_if_longer_than

bool Task::check_if_longer_than( const Time& now )
{
    bool something_done = false;

    if( !_warn_if_longer_than.is_never() ) 
    {
        double step_time = now - _last_operation_time;

        if( step_time > _warn_if_longer_than )
        {
            something_done = true;
            if( _last_warn_if_longer_operation_time != _last_operation_time ) 
            {
                _last_warn_if_longer_operation_time = _last_operation_time;

                string msg = message_string( "SCHEDULER-712", _warn_if_longer_than.as_string( Time::without_ms ) );
                _log->warn( msg );

                Scheduler_event scheduler_event ( evt_task_step_too_long, log_error, _spooler );
                Mail_defaults mail_defaults( _spooler );
                mail_defaults.set( "subject", S() << obj_name() << ": " << msg );
                mail_defaults.set( "body"   , S() << obj_name() << ": " << msg << "\n"
                                              "Step time: " << step_time << "\n" <<
                                              "warn_if_longer_than=" << _warn_if_longer_than.as_string( Time::without_ms ) );
                scheduler_event.send_mail( mail_defaults );
            }
        }
    }

    return something_done;
}

//------------------------------------------------------------------------------------Task::add_pid

void Task::add_pid( int pid, const Time& timeout_period )
{
    if( _module_instance->is_remote_host() )
    {
        //int REMOTE_SUBPROCESS_KILLEN;
        _log->warn( message_string( "SCHEDULER-849", pid ) );
    }
    else
    {
        Time timeout_at = Time::never;

        if( timeout_period != Time::never )
        {
            timeout_at = Time::now() + timeout_period;
            _log->debug9( message_string( "SCHEDULER-912", pid, timeout_at ) );
        }

        bool is_process_group = false;
        if( pid < 0 )  pid = -pid, is_process_group = true;

        _registered_pids[ pid ] = Z_NEW( Registered_pid( this, pid, timeout_at, false, false, false, is_process_group, "" ) );

        set_subprocess_timeout();
    }
}

//---------------------------------------------------------------------------------Task::remove_pid

void Task::remove_pid( int pid )
{
    _registered_pids.erase( pid );

    set_subprocess_timeout();
}

//-----------------------------------------------------------------------------Task::add_subprocess

void Task::add_subprocess( int pid, double timeout, bool ignore_exitcode, bool ignore_signal, bool is_process_group, const string& title )
{
    Z_LOG2( "scheduler", Z_FUNCTION << " " << pid << "," << timeout << "," << ignore_exitcode << "," << ignore_signal << "," << is_process_group << "\n" );
    Z_LOG2( "scheduler", Z_FUNCTION << "   title=" << title << "\n" );   // Getrennt, falls Parameterübergabe fehlerhaft ist und es zum Abbruch kommt (com_server.cxx)
    
    if( _module_instance->is_remote_host() )
    {
        //int REMOTE_SUBPROCESS_KILLEN;
        //_log->warn( message_string( "SCHEDULER-849", pid ) );
    }
    else
    {
        Time timeout_at = timeout < INT_MAX - 1? Time::now() + timeout
                                               : Time::never;

        _registered_pids[ pid ] = Z_NEW( Registered_pid( this, pid, timeout_at, true, ignore_exitcode, ignore_signal, is_process_group, title ) );

        set_subprocess_timeout();
    }
}

//--------------------------------------------------------------Task::shall_wait_for_registered_pid

bool Task::shall_wait_for_registered_pid()
{
    FOR_EACH( Registered_pids, _registered_pids, p )  if( p->second->_wait )  return true;
    return false;
}

//-------------------------------------------------------------Task::Registered_pid::Registered_pid

Task::Registered_pid::Registered_pid( Task* task, int pid, const Time& timeout_at, bool wait, bool ignore_error, bool ignore_signal, 
                                      bool is_process_group, const string& title )
:
    _spooler(task->_spooler),
    _task(task),
    _pid(pid),
    _timeout_at(timeout_at),
    _wait(wait),
    _ignore_error(ignore_error),
    _ignore_signal(ignore_signal),
    _is_process_group(is_process_group),
    _title(title),
    _killed(false)
{
    if( !_task->_module_instance->is_remote_host() )  _spooler->register_pid( pid, is_process_group );
}

//----------------------------------------------------------------------Task::Registered_pid::close

void Task::Registered_pid::close()
{
    if( _spooler )
    {
        if( _task->_module_instance  &&  !_task->_module_instance->is_remote_host() )  _spooler->unregister_pid( _pid );
        _spooler = NULL;
    }
}

//-------------------------------------------------------------------Task::Registered_pid::try_kill

void Task::Registered_pid::try_kill()
{
    if( !_killed  )
    {
        if( _task->_module_instance->is_remote_host() )
        {
            //int REMOTE_SUBPROCESS_KILLEN;
            _task->log()->warn( message_string( "SCHEDULER-849", _pid ) );
            // Asynchron <remote_scheduler.subprocess.kill task="..." pid="..."/>
            // ohne Ende abzuwarten?
            // Operation für solche asynchronen XML-Befehle mit Warteschlange
            // spooler_communication.cxx muss mehrere XML-Dokumente aufeinander empfangen und trennen können.
        }
        else
        {
            try
            {
#               ifdef Z_UNIX
                    if( _is_process_group )  posix::kill_process_group_immediately( _pid, obj_name() );
                  else
#               endif
                    kill_process_immediately( _pid, obj_name() );

                _task->_log->warn( message_string( "SCHEDULER-273", _pid ) );   // "Subprozess " << _pid << " abgebrochen" 
            }
            catch( exception& x )
            {
                _task->_log->warn( message_string( "SCHEDULER-274", _pid, x ) );   // "Subprozess " << _pid << " lässt sich nicht abbrechen: " << x
            }
        }

        _killed = true;
        _timeout_at = Time::never;

        close();
    }
}

//---------------------------------------------------------------------Task::set_subprocess_timeout

void Task::set_subprocess_timeout()
{
    _subprocess_timeout = Time::never;
    FOR_EACH( Registered_pids, _registered_pids, p )  if( _subprocess_timeout > p->second->_timeout_at )  _subprocess_timeout = p->second->_timeout_at;

    if( _subprocess_timeout != Time::never )  signal( "subprocess_timeout" );
    //if( _subprocess_timeout > _next_time )  set_next_time( _subprocess_timeout );
}

//-------------------------------------------------------------------Task::check_subprocess_timeout

bool Task::check_subprocess_timeout( const Time& now )
{
    bool something_done = false;

    if( _subprocess_timeout <= now )
    {
        FOR_EACH( Registered_pids, _registered_pids, p )
        {
            Registered_pid* subprocess = p->second;

            if( subprocess->_timeout_at <= now )
            {
                _log->warn( message_string( "SCHEDULER-275", subprocess->_pid ) );
                subprocess->try_kill();
                something_done = true;
            }
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

        FOR_EACH( Registered_pids, _registered_pids, p )  p->second->try_kill();

        if( !_killed ) _log->warn( message_string( "SCHEDULER-276" ) );
    }
    catch( const exception& x ) { _log->warn( x.what() ); }

    return _killed;
}

//-------------------------------------------------------------------------------Task::do_something

bool Task::do_something()
{
    //Z_DEBUG_ONLY( _log->debug9( "do_something() state=" + state_name() ); )

    bool had_operation      = _operation != NULL;
    bool something_done     = false;
    Time now                = Time::now();
    Time next_time_at_begin = _next_time;

    _signaled = false;

    //if( _end == end_kill_immediately  &&  !_kill_tried )
    //{
    //    _log->warn( message_string( "SCHEDULER-277" ) );   // Kein Fehler, damit ignore_signals="SIGKILL" den Stopp verhindern kann
    //    return try_kill();
    //}

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
                    _log->info( message_string( "SCHEDULER-278" ) );   // "Laufzeitperiode ist abgelaufen, Task wird beendet"
                    cmd_end();// set_state( s_ending );
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

                    if( !_operation  &&  _state != s_running_process )
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
                                //z::throw_xc( "SCHEDULER-202", x.what() );
                                set_error( z::Xc( "SCHEDULER-202", state_name(), x.what() ) );
                                if( _state < s_killing )  set_state( s_killing );
                            }
                        }

                        if( _state < s_ending  &&  _end )      // Task beenden?
                        {
                            if( _end == end_nice  &&  _state <= s_running  &&  _order  &&  _step_count == 0 )   // SCHEDULER-226 (s.u.) nach SCHEDULER-271 (Task-Ende wegen Prozessmangels)
                            {
                                if( !_scheduler_815_logged )
                                {
                                    _log->info( message_string( "SCHEDULER-815", _order->obj_name() ) );    // Task einen Schritt machen lassen, um den Auftrag auszuführen
                                    _scheduler_815_logged = true;
                                }
                            }
                            else
                            if( !loaded() )         set_state( s_ended );
                            else
                            if( !_begin_called )    set_state( s_release );
                            else
                                                    set_state( s_ending );
                        }
                    }


                    switch( _state )
                    {
                        case s_loading:
                        {
                            bool ok = load();
                            if( ok )  set_state( s_waiting_for_process );
                                else  set_state( s_release );
                            
                            something_done = true;
                            loop = true;
                            break;
                        }


                        case s_waiting_for_process:
                        {
                            bool ok = !_module_instance || _module_instance->try_to_get_process();
                            if( ok )  set_state( s_starting ), something_done = true, loop = true;
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

                                if( _job->_history.min_steps() == 0 )  _history.start();

                                _file_logger->add_file( _module_instance->stdout_path(), "stdout" );
                                _file_logger->add_file( _module_instance->stderr_path(), "stderr" );

                                if( _file_logger->has_files() )
                                {
                                    _file_logger->set_async_manager( _spooler->_connection_manager );
                                    _file_logger->start();
                                }

                                set_state( !ok? s_ending :
                                           _module_instance->_module->kind() == Module::kind_process?
                                                _module_instance->kind() == Module::kind_remote? s_running_remote_process 
                                                                                               : s_running_process
                                           : s_opening );

                                loop = true;
                            }

                            something_done = true;
                            break;
                        }


                        case s_opening:
                        {
                            if( !_operation )
                            {
                                if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_task_api ] )
                                {
                                    _lock_holder->hold_locks( lock_requestor );
                                    lock_requestor->dequeue_lock_requests();
                                }

                                _operation = do_call__start( spooler_open_name );
                            }
                            else
                            {
                                bool ok = operation__end();

                                if( _delay_until_locks_available )
                                {
                                    _delay_until_locks_available = false;
                                    _lock_requestors[ lock_level_task_api ]->enqueue_lock_requests( _lock_holder );
                                    set_state( s_opening_waiting_for_locks );
                                    ok = true;
                                }
                                else
                                {
                                    set_state( ok? s_running : s_ending );
                                }

                                loop = true;
                            }

                            something_done = true;
                            break;
                        }


                        case s_opening_waiting_for_locks: 
                            break;


                        case s_running_process:
                            assert( _module_instance->_module->kind() == Module::kind_process );
                            
                            if( _module_instance->process_has_signaled() )
                            {
                                _file_logger->flush();
                                _log->info( message_string( "SCHEDULER-915" ) );
                                check_if_shorter_than( now );
                                something_done |= check_if_longer_than( now );
                                count_step();
                                set_state( s_ending );
                                loop = true;
                            }
                            else
                            {
                                check_timeout( now );

                                _running_state_reached = true;  // Also nicht, wenn der Prozess sich sofort beendet hat (um _min_tasks-Schleife zu vermeiden)
                                _next_time = Time::never;       // Nach cmd_end(): Warten bis _module_instance->process_has_signaled()
                            }

                            break;


                        case s_running_remote_process:
                            assert( _module_instance->_module->kind() == Module::kind_process &&
                                    _module_instance->kind()          == Module::kind_remote     );

                            if( !_operation )
                            {
                                _operation = do_step__start();
                            }
                            else
                            {
                                string result = remote_process_step__end();
                                check_if_shorter_than( now );
                                something_done |= check_if_longer_than( now );
                                set_state( s_release );
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
                                    something_done = true;
                                }
                                else
                                {
                                    if( _job->is_order_controlled() )
                                    {
                                        if( !fetch_and_occupy_order( now, state_name() ) )
                                        {
                                            _idle_timeout_at = _job->_idle_timeout == Time::never? Time::never : now + _job->_idle_timeout;
                                            set_state( s_running_waiting_for_order );
                                            break;
                                        }
                                    }

                                    _running_state_reached = true;
                                    _last_process_start_time = now;

                                    
                                    if( _step_count + 1 == _job->_history.min_steps() )  _history.start();

                                    if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] )
                                    {
                                        _lock_holder->hold_locks( lock_requestor );
                                        lock_requestor->dequeue_lock_requests();
                                    }

                                    _operation = do_step__start();

                                    something_done = true;
                                }
                            }
                            else
                            {
                                ok = step__end();
                                _operation = NULL;

                                if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] )   
                                {
                                    if( ok  &&  !_delay_until_locks_available  &&  !_lock_holder->is_holding_all_of( lock_requestor ) )  
                                        log()->warn( message_string( "SCHEDULER-469" ) );    // Try_hold_lock() hat versagt und Call_me_again_when_locks_available() nicht aufgerufen

                                    _lock_holder->release_locks( lock_requestor );  // Task.Try_hold_lock() wieder aufheben

                                    if( _delay_until_locks_available )
                                    {
                                        _delay_until_locks_available = false;
                                        set_state( s_running_waiting_for_locks );
                                        lock_requestor->enqueue_lock_requests( _lock_holder );
                                        ok = true;
                                    }
                                    else
                                    {
                                        _lock_holder->remove_requestor( lock_requestor );
                                        _lock_requestors[ lock_level_process_api ] = NULL;
                                    }
                                }
                                assert( !_delay_until_locks_available );

                                check_if_shorter_than( now );
                                something_done |= check_if_longer_than( now );

                                if( !ok || has_error() )  set_state( s_ending ), loop = true;

                                if( _state != s_ending  &&  !_end )  _order_for_task_end = NULL;

                                something_done = true;
                            }

                            break;
                        }


                        case s_running_waiting_for_order:
                        {
                            if( fetch_and_occupy_order( now, state_name() ) )
                            {
                                set_state( s_running );     // Auftrag da? Dann Task weiterlaufen lassen (Ende der Run_time wird noch geprüft)
                                loop = true;                // _order wird in step__end() wieder abgeräumt
                            }
                            else
                            if( now >= _idle_timeout_at )
                            {
                                if( _job->_force_idle_timeout  ||  _job->above_min_tasks() )
                                {
                                    _log->debug9( message_string( "SCHEDULER-916" ) );   // "idle_timeout ist abgelaufen, Task beendet sich" 
                                    _end = end_normal;
                                    loop = true;
                                }
                                else
                                {
                                    _idle_timeout_at = _job->_idle_timeout == Time::never? Time::never : now + _job->_idle_timeout;
                                    set_state( s_running_waiting_for_order );   // _next_time neu setzen
                                    Z_LOG2( "scheduler", obj_name() << ": idle_timeout ist abgelaufen, aber force_idle_timeout=\"no\" und nicht mehr als min_tasks Tasks laufen  now=" << now << ", _next_time=" << _next_time << "\n" );
                                    //_log->debug9( message_string( "SCHEDULER-916" ) );   // "idle_timeout ist abgelaufen, Task beendet sich" 
                                    something_done = true;
                                }
                            }
                            else
                                _running_state_reached = true;   // Also nicht bei idle_timeout="0"

                            break;
                        }


                        case s_running_delayed:
                        {
                            if( now >= _next_spooler_process )
                            {
                                _next_spooler_process = 0;
                                set_state( s_running ), loop = true;
                            }

                            _running_state_reached = true;
                            break;
                        }


                        case s_running_waiting_for_locks: 
                            break;


                        case s_ending:
                        {
                            if( !_operation )
                            {
                                THREAD_LOCK_DUMMY( _lock ) { if( !_ending_since )  _ending_since = now; }    // Wird auch von cmd_end() gesetzt

                                if( has_error()  ||  _log->highest_level() >= log_error )  _history.start();

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

                                set_state( loaded()? s_ending_waiting_for_subprocesses
                                                   : s_release );
                                loop = true;
                            }

                            something_done = true;

                            break;
                        }


                        case s_ending_waiting_for_subprocesses:
                        {
                            if( !_operation )
                            {
                                if( shall_wait_for_registered_pid() )
                                {
                                    if( Remote_module_instance_proxy* m = dynamic_cast< Remote_module_instance_proxy* >( +_module_instance ) )
                                    {
                                        _operation = m->_remote_instance->call__start( "Wait_for_subprocesses" );
                                    }
                                    else
                                    {
                                        try
                                        {
                                            _subprocess_register.wait();
                                        }
                                        catch( exception& x ) { set_error( x ); }
                                    }
                                }
                            }
                            else
                            {
                                operation__end();
                            }

                            if( !_operation )
                            {
                                set_state( has_error()? s_on_error
                                                      : s_on_success );
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
                            if( _job->_module->_reuse == Module::reuse_task )
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
                                         else  operation__end(), set_state( s_killing ), loop = true;

                            something_done = true;
                            break;
                        }


                        case s_killing:
                        {
                            if( _module_instance  &&  _module_instance->is_kill_thread_running() )
                            {
                                _next_time = Time::now() + 0.1;
                            }
                            else
                            {
                                set_state( s_ended );
                                loop = true;
                            }

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

                                if( _module_instance  && _module_instance->_module->kind() == Module::kind_process )
                                {
                                    if( Process_module_instance* process_module_instance = dynamic_cast<Process_module_instance*>( +_module_instance ) )
                                    {
                                        set_state_texts_from_stdout( process_module_instance->get_first_line_as_state_text() );

                                        if( _order )
                                        {
                                            //Process_module_instance::attach_task() hat temporäre Datei zur Rückgabe der Auftragsparameter geöffnet
                                            process_module_instance->fetch_parameters_from_process( _order->params() );
                                        }
                                    }

                                    if( _order )
                                    {
                                        if( !has_error() )
                                        {
                                            postprocess_order( _module_instance->spooler_process_result()? Order::post_success 
                                                                                                         : Order::post_error   );       
                                        }
                                        else {}     // remove_order_after_error() wird sich drum kümmern.
                                    }                                                               
                                }                                                               

                                // Gesammelte eMail senden, wenn collected_max erreicht:
                                //Time log_time = _log->collect_end();
                                //if( log_time > Time::now()  &&  _next_time > log_time )  set_next_time( log_time );

                                set_state( s_deleting_files );
                                //set_state( s_closed );
				
                                loop = true;
                            }

                            something_done = true;
                            break;
                        }


                        case s_deleting_files:
                        {
                            bool ok;

                            if( !_module_instance )
                            {
                                ok = true;
                            }
                            else
                            {
                                ok = false;
                                Time now = Time::now();

                                if( !_trying_deleting_files_until  ||  now >= _next_time )     // do_something() wird zu oft gerufen, deshalb prüfen, ob_next_time erreicht ist.
                                {
                                    // Nicht ins _log, also Task-Log protokollieren, damit mail_on_warning nicht greift. Das soll kein Task-Problem sein!
                                    // Siehe auch ~Process_module_instance
                                    Has_log* my_log = NULL; //_trying_deleting_files_until? NULL : _job->log();

                                    // Folgendes könnte zusammengefasst werden mit Remote_task_close_command_response::async_continue_() als eigene Async_operation

                                    ok = _module_instance->try_delete_files( my_log );
                                    if( ok )
                                    {
                                        if( _trying_deleting_files_until )  
                                        {
                                            _log->debug( message_string( "SCHEDULER-877" ) );  // Nur, wenn eine Datei nicht löschbar gewesen ist
                                        }
                                    }
                                    else
                                    {
                                        if( !_trying_deleting_files_until )  
                                        {
                                            string paths = join( ", ", _module_instance->undeleted_files() );
                                            _log->debug( message_string( "SCHEDULER-876", paths ) );  // Nur beim ersten Mal
                                        }

                                        //if( _end == end_kill_immediately  &&  // Bei kill_immediately nur einmal warten (1/10s, das ist zu kurz!)
                                        if( _trying_deleting_files_until  &&  now >= _trying_deleting_files_until )   // Nach Fristablauf geben wir auf
                                        {
                                            string paths = join( ", ", _module_instance->undeleted_files() );
                                            _log->info( message_string( "SCHEDULER-878", paths ) );
                                            _job->log()->warn( message_string( "SCHEDULER-878", paths ) );
                                            ok = true;
                                        }
                                        else
                                        {
                                            // Funktioniert in lokaler Zeit. Zum Beginn der Winterzeit: +1h, zum Beginn der Sommerzeit: 0s.
                                            if( !_trying_deleting_files_until )  _trying_deleting_files_until = now + delete_temporary_files_delay;
                                            _next_time = min( now + delete_temporary_files_retry, _trying_deleting_files_until );
                                        }
                                    }

                                    something_done = true;
                                }
                            }
                            
                            if( ok )
                            {
                                if( _module_instance )  _module_instance->detach_process();
                                _module_instance = NULL;
                                finish();
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
                            assert(0), throw_xc( "state=none?" );
                    }


                    if( _operation )
                    {
                        loop |= _operation->async_finished();   // Falls Sync_operation
                    }
                    else
                    {
                        if( !ok || has_error() )
                        {
                            if( _state < s_ending )  set_state( s_ending ), loop = true;
                        }

                        if( _killed  &&  _state < s_ending )  set_state( s_ending ), loop = true;
                    }
                }
                catch( _com_error& x )  { throw_com_error( x ); }
            }
            catch( const exception& x )
            {
                if( error_count == 0 )  set_error( x );
                                 else  _log->error( x.what() );

                try
                {
                    remove_order_after_error();
                }
                catch( exception& x ) { _log->error( "remove_order_after_error: " + string(x.what()) ); }

                if( error_count == 0  &&  _state < s_ending )
                {
                    _end = end_normal;
                    loop = true;
                }
                else
                if( error_count <= 1 )
                {
                    loop = false;

                    try
                    {
                        finish();
                    }
                    catch( exception& x ) { _log->error( "finish(): " + string( x.what() ) ); }

                    try
                    {
                        if( _job )  _job->stop_after_task_error( x.what() );
                    }
                    catch( exception& x ) { _log->error( "Job->stop_after_task_error(): " + string( x.what() ) ); }

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

        if( !something_done  &&  _next_time <= now  &&  !_signaled )    // Obwohl _next_time erreicht, ist nichts getan?
        {
            // Das kann bei s_running_waiting_for_order passieren, wenn zunächst ein Auftrag da ist (=> _next_time = 0),
            // der dann aber von einer anderen Task genommen wird. Dann ist der Auftrag weg und something_done == false.

            set_state( state() );  // _next_time neu setzen

            if( _next_time <= now )
            {
                Z_LOG2( "scheduler", obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time=" << _next_time << ", wird verzögert\n" );
                _next_time = Time::now() + 0.1;
            }
            else
            {
                Z_LOG2( "scheduler.nothing_done", obj_name() << ".do_something()  Nichts getan. state=" << state_name() << ", _next_time war " << next_time_at_begin << "\n" );
            }
        }
    }

  //if( _next_time && !_let_run )  set_next_time( min( _next_time, _job->_period.end() ) );                      // Am Ende der Run_time wecken, damit die Task beendet werden kann

/*
    if( _state != s_running
     && _state != s_running_delayed
     && _state != s_running_waiting_for_order
     && _state != s_running_process
     && _state != s_suspended                 )  send_collected_log();
*/


    return something_done;
}

//------------------------------------------------------------------------------------do_something2
/*
bool Task::do_something2()
{
}
*/

//---------------------------------------------------------------------------------------Task::load

bool Task::load()
{
    if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
    {
        bool   remove_after_close = false;
        string filename           = _spooler->log_directory() + "/task." + _job->path().to_filename();

        if( _job->_max_tasks > 1 )  filename += "." + as_string(_id),  remove_after_close = true;
        filename += ".log";

        _log->set_filename( filename );      // Task-Protokoll
        _log->set_remove_after_close( remove_after_close );
        _log->set_title( obj_name() );
        _log->open();                // Jobprotokoll. Nur wirksam, wenn set_filename() gerufen
    }


    _job->count_task();
    _spooler->_task_subsystem->count_task();
    //(nur für altes use_engine="job", löscht Fehlermeldung von Job::do_somethin() init_start_when_directory_changed: reset_error();
    _running_since = Time::now();

    if( _job->is_machine_resumable() )  _spooler->begin_dont_suspend_machine();

    return do_load();
}

//---------------------------------------------------------------------------------Task::begin_start

Async_operation* Task::begin__start()
{
    return do_begin__start();
}

//-----------------------------------------------------------------------------------Task::step__end

bool Task::step__end()
{
    bool continue_task;

    try
    {
        bool    result;
        Variant spooler_process_result = do_step__end();

        if( spooler_process_result.vt == VT_ERROR  &&  V_ERROR( &spooler_process_result ) == DISP_E_UNKNOWNNAME )
        {
            result = true;
            continue_task = false;
        }
        else
        {
            continue_task = result = check_result( spooler_process_result );
        }

        if( _job->is_order_controlled() )  continue_task = true;           // Auftragsgesteuerte Task immer fortsetzen ( _order kann wieder null sein wegen set_state(), §1495 )

        count_step();

        if( _order )  
        {
            postprocess_order( _delay_until_locks_available? Order::post_keep_state :
                               result                      ? Order::post_success 
                                                           : Order::post_error        );
        }

        if( _next_spooler_process )  continue_task = true;
    }
    catch( const exception& x ) 
    { 
        set_error(x); 
        if( _order )  remove_order_after_error();
        continue_task = false; 
    }

    return continue_task;
}

//---------------------------------------------------------------------------------Task::count_step

void Task::count_step()
{
    if( has_step_count()  ||  _step_count == 0 )        // Bei kind_process nur einen Schritt zählen
    {
        _spooler->_task_subsystem->count_step();        // Siehe auch remote_process_step__end()
        _job->count_step();
        _step_count++;
    }
}

//-------------------------------------------------------------------Task::remote_process_step__end

string Task::remote_process_step__end()
{
    // Das könnte mit der Endebehandlung einer Process_module_instance, also ohne kind_remote, zusammengefasst werden.
    // Dazu sollte für beide Verfahren eine gemeinsame Oberklasse gebildet werden, mit einem Proxy für kind_remote als Unterklasse.

    string result;

    try
    {
        result = do_step__end().as_string();

        xml::Document_ptr dom_document           ( result );
        xml::Element_ptr  process_result_element = dom_document.select_element_strict( "/" "process.result" );

        _module_instance->set_spooler_process_result( process_result_element.bool_getAttribute( "spooler_process_result" ) );
        _module_instance->set_exit_code         ( process_result_element.int_getAttribute( "exit_code", 0 ) );
        _module_instance->set_termination_signal( process_result_element.int_getAttribute( "signal"   , 0 ) );

        set_state_texts_from_stdout( process_result_element.getAttribute( "state_text" ) );

        if( _order )
        {
            if( xml::Element_ptr order_params_element = process_result_element.select_node( "order.params" ) )
            {
                ptr<Com_variable_set> p = new Com_variable_set( order_params_element );
                _order->params()->merge( p );
            }
        }

        _spooler->_task_subsystem->count_step();
        _job->count_step();
        _step_count++;
    }
    catch( const exception& x ) 
    { 
        set_error(x); 
    }

    _operation = NULL;

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
            case s_starting:                        result = do_begin__end();    break;
            case s_opening:                         result = do_call__end();     break;
            case s_ending:                                   do_end__end();      break;

            case s_ending_waiting_for_subprocesses: if( Remote_module_instance_proxy* m = dynamic_cast< Remote_module_instance_proxy* >( +_module_instance ) )
                                                        m->_remote_instance->call__end();
                                                    else assert(0), throw_xc( "NO_REMOTE_INSTANCE" );
                                                    break;

            case s_on_error:                        result = do_call__end();     break;
            case s_on_success:                      result = do_call__end();     break;
            case s_exit:                            result = do_call__end();     break;
            case s_release:                                  do_release__end();  break;
            case s_ended:                                    do_close__end();    break;
            default:                                assert(0), throw_xc( "Task::operation__end" );
        }
    }
    catch( const exception& x ) { set_error(x); result = false; }

    _operation = NULL;

    return result;
}

//---------------------------------------------------------------------Task::fetch_and_occupy_order

Order* Task::fetch_and_occupy_order( const Time& now, const string& cause )

// Wird auch von Job gerufen, wenn ein neuer Auftrag zum Start einer neuen Task führt

{
    assert( _job->is_order_controlled() );
    assert( !_operation );
    assert( _state == s_none || _state == s_running || _state == s_running_waiting_for_order );


    if( !_order  
     && !_end   )   // Kann beim Aufruf aus Job::do_something() passieren 
    {
        if( Order* order = _job->combined_job_nodes()->fetch_and_occupy_order( now, cause, this ) )
        {
            if( order->is_file_order() )  _trigger_files = order->file_path();

            _order = order;
            _order_for_task_end = order;                // Damit bei Task-Ende im Fehlerfall noch der Auftrag gezeigt wird, s. dom_element()

            _log->set_order_log( _order->_log );
            _log->info( message_string( "SCHEDULER-842", _order->obj_name(), _order->state(), _spooler->http_url() ) );
        }
        else
        {
            _job->request_order( now, cause );
        }
    }

    if( _order )  _order->assert_task( Z_FUNCTION );

    return _order;
}

//--------------------------------------------------------------------------Task::postprocess_order

void Task::postprocess_order( Order::Postprocessing_mode postprocessing_mode, bool due_to_exception )
{
    if( _order )
    {
        _log->info( message_string( "SCHEDULER-843", _order->obj_name(), _order->state(), _spooler->http_url() ) );
        
        _order->postprocessing( postprocessing_mode );
        
        if( due_to_exception )
        {
            if( !_order->setback_called() )  _log->warn( message_string( "SCHEDULER-846", _order->state().as_string() ) );
        }

        remove_order();
    }
}

//-------------------------------------------------------------------Task::remove_order_after_error

void Task::remove_order_after_error()
{
    if( _order )
    {
        if( _job->stops_on_task_error() )  
        {
            // Job wird stoppt, deshalb bleibt der Auftrag in der Warteschlange.

            _log->warn( message_string( "SCHEDULER-845" ) );
            _log->info( message_string( "SCHEDULER-843", _order->obj_name(), _order->state(), _spooler->http_url() ) );
            _order->processing_error();
            remove_order();
        }
        else
        {
            // Job stoppt nicht, deshalb wechselt der Auftrag in den Fehlerzustand

            postprocess_order( Order::post_error, true );
            // _order ist NULL
        }
    }
}

//-------------------------------------------------------------------------------Task::remove_order

void Task::remove_order()
{
    if( _order->is_file_order() )  _trigger_files = "";
    _log->set_order_log( NULL );
    _order = NULL;
}

//-------------------------------------------------------------------------------------Task::finish

void Task::finish()
{
    _job->init_start_when_directory_changed( this );

    process_on_exit_commands();

    if( _order )    // Auftrag nicht verarbeitet? spooler_process() nicht ausgeführt, z.B. weil spooler_init() oder spooler_open() false geliefert haben.
    {
        if( !has_error()  &&  _spooler->state() != Spooler::s_stopping )  set_error( Xc( "SCHEDULER-226" ) );
        remove_order_after_error();  // Nur rufen, wenn _move_order_to_error_state, oder der Job stoppt oder verzögert wird! (has_error() == true) Sonst wird der Job wieder und wieder gestartet.
    }

    if( has_error()  &&  _job->repeat() == 0  &&  _job->_delay_after_error.empty() )
    {
        _job->stop_after_task_error( _error.what() );
    }
    else
    if( _job->_temporary  &&  _job->repeat() == 0 )
    {
        _job->stop( false );   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
    }

    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzögern?

    if( has_error()  &&  _job->_delay_after_error.size() > 0 )
    {
        InterlockedIncrement( &_job->_error_steps );

        _is_first_job_delay_after_error = _job->_error_steps == 1;

        if( !_job->repeat() )   // spooler_task.repeat hat Vorrang
        {
            Time delay = _job->_delay_after_error.empty()? Time::never : Time(0);

            FOR_EACH( Job::Delay_after_error, _job->_delay_after_error, it )
                if( _job->_error_steps >= it->first )  delay = it->second;

            if( delay == Time::never )
            {
                _is_last_job_delay_after_error = true;
                _job->stop( false );
            }
            else
            {
                _job->_delay_until = Time::now() + delay;

                if( !_job->is_order_controlled() )
                {
                    try
                    {
                        ptr<Task> task = _job->start( +_params, _name );   // Keine Startzeit: Nach Periode und _delay_until richten
                        task->_delayed_after_error_task_id = id();
                    }
                    catch( exception& x )
                    {
                        _log->error( x.what() );
                        _job->_delay_until = 0;
                        _job->stop_after_task_error( x.what() );
                    }
                }
            }
        }
    }
    else
    {
       _job->_error_steps = 0;
    }


    if( _web_service )
    {
        _web_service->forward_task( *this );
    }


    _job->on_task_finished( this );


    // eMail versenden
    {
        // Vor Task::close(), damit _order_for_task_end genutzt werden kann, s. Task::dom_element()
        Scheduler_event event ( evt_task_ended, _log->highest_level(), this );
        trigger_event( &event );
    }


    close();

    task_subsystem()->count_finished_tasks();

    if( _job->is_machine_resumable() )  _spooler->end_dont_suspend_machine();
}

//----------------------------------------------------------------Task::set_state_texts_from_stdout

void Task::set_state_texts_from_stdout( const string& state_text )
{
    _job->set_state_text( state_text );
    if( _order )  _order->set_state_text( state_text );
}

//-------------------------------------------------------------------Task::process_on_exit_commands

void Task::process_on_exit_commands()
{
    if( _job->_commands_document )
    {
        xml::Element_ptr commands_element = NULL;

        Job::Exit_code_commands_map::iterator it = _job->_exit_code_commands_map.find( _exit_code );
        if( it != _job->_exit_code_commands_map.end() )
        {
            commands_element = it->second;
        }
#       ifdef Z_UNIX
            else
            if( _exit_code < 0  &&  _job->_exit_code_commands_on_signal )  // Signal?
            {
                commands_element = _job->_exit_code_commands_on_signal;
            }
#       endif
        else
        {
            commands_element = _job->_commands_document.select_node( "/*/commands [ @on_exit_code='error' ]" );
        }

        if( commands_element )
        {
            _log->debug( message_string( "SCHEDULER-328", commands_element.getAttribute( "on_exit_code" ) ) );

            Command_processor cp ( _spooler, Security::seclev_all );
            cp.set_log( _log );
            cp.set_variable_set( "task", _params );
            
            if( _job->is_order_controlled() )
            {
                ptr<Com_variable_set> order_params = _order_for_task_end? _order_for_task_end->params_or_null() : NULL;
                cp.set_variable_set( "order", order_params );
            }

            DOM_FOR_EACH_ELEMENT( commands_element, c )
            {
                try
                {
                    cp.execute_command( c );
                }
                catch( exception& x )
                {
                    _log->error( x.what() );
                    _log->error( message_string( "SCHEDULER-327", c.xml() ) );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------Task::trigger_event

void Task::trigger_event( Scheduler_event* scheduler_event )
{
    try
    {
        _log->set_mail_default( "from_name", _spooler->name() + " " + obj_name() );    // "Scheduler host:port -id=xxx Task jobname:id"

        scheduler_event->set_message( _log->highest_msg() );
        if( _error )  scheduler_event->set_error( _error );

        bool is_error = has_error();

        S body;
        body << Sos_optional_date_time::now().as_string() << "\n\nJob " << _job->name() << "  " << _job->title() << "\n";
        body << "Task-Id " << as_string(id()) << ", " << as_string(_step_count) << " steps\n";
        if( _order )  body << _order->obj_name() << "\n";
        body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";

        if( !is_error )
        {
            S subject;
            subject << obj_name();

            if( _log->highest_level() == log_error )
            {
                subject << " ended with error";
                body << _log->highest_msg() << "\n\n";
            }
            else
            if( _log->highest_level() == log_warn )
            {
                subject << " ended with warning";
                body << _log->highest_msg() << "\n\n";
            }
            else
            {
                subject << " succeeded";
            }

            _log->set_mail_default( "subject", subject, false );
        }
        else
        {
            string errmsg = _error? _error->what() : _log->highest_msg();
            _log->set_mail_default( "subject", string("ERROR ") + errmsg, true );

            body << errmsg << "\n\n";
        }

        _log->set_mail_default( "body", body + "This message contains the job protocol." );   //, is_error );



        bool mail_it = _log->_mail_it;

        if( !mail_it )
        {
            bool mail_due_to_error_or_warning = false;

            if( _log->_mail_on_error | _log->_mail_on_warning  &&  ( has_error() || _log->highest_level() >= log_error ) )  mail_due_to_error_or_warning = true;
            else
            if( _log->_mail_on_warning  &&  _log->has_line_for_level( log_warn ) )  mail_due_to_error_or_warning = true;

            if( _job->_delay_after_error.size() > 0 )
            {
                switch( _log->_mail_on_delay_after_error )
                {
                    case fl_all:                    break;
                    case fl_first_only:             if( !_is_first_job_delay_after_error )  mail_due_to_error_or_warning = false;  break;
                    case fl_last_only:              if( !_is_last_job_delay_after_error  )  mail_due_to_error_or_warning = false;  break;
                    case fl_first_and_last_only:    if( !_is_first_job_delay_after_error  
                                                    &&  !_is_last_job_delay_after_error  )  mail_due_to_error_or_warning = false;  break;
                    default: assert(0), z::throw_xc( Z_FUNCTION );
                }
            }

            mail_it = mail_due_to_error_or_warning;
        }

        if( !mail_it  &&  _log->_mail_on_success  &&  _step_count >= 0                      )  mail_it = true;
        if( !mail_it  &&  _log->_mail_on_process  &&  _step_count >= _log->_mail_on_process )  mail_it = true;


        if( _log->_mail_it )  mail_it = true;

        if( mail_it )
        {
            _log->send( scheduler_event );
        }
    }
    catch( const exception& x  ) { _log->warn( x.what() ); }
    catch( const _com_error& x ) { _log->warn( bstr_as_string(x.Description()) ); }
}

//----------------------------------------------------------------------Task::wait_until_terminated
// Anderer Thread

bool Task::wait_until_terminated( double )
{
    z::throw_xc( "SCHEDULER-125" );     // Deadlock
    return false;
}

//-------------------------------------------------------------------------Task::send_collected_log

//void Task::send_collected_log()
//{
//    try
//    {
//        Scheduler_event scheduler_event ( evt_task_ended, _log->highest_level(), this );
//        _log->send( -2, &scheduler_event );
//    }
//    catch( const exception&  x ) { _spooler->log()->error( x.what() ); }
//    catch( const _com_error& x ) { _spooler->log()->error( bstr_as_string(x.Description()) ); }
//}

//--------------------------------------------------------------------------Task::set_mail_defaults
/*
void Task::set_mail_defaults()
{
    bool is_error = has_error();

    _log->set_mail_from_name( _job->profile_section() );

    string body = Sos_optional_date_time::now().as_string() + "\n\nJob " + _job->name() + "  " + _job->title() + "\n";
    body += "Task-Id " + as_string(id()) + ", " + as_string(_step_count) + " Schritte\n";
    body += "Scheduler -id=" + _spooler->id() + "  host=" + _spooler->_complete_hostname + "\n\n";

    if( !is_error )
    {
        string subject = obj_name();

        if( _log->highest_level() == log_warn )
        {
            subject += " mit Warnung beendet";
            body += _log->highest_msg() + "\n\n";
        }
        else
        {
            subject += " gelungen";
        }

        _log->set_mail_subject( subject );
    }
    else
    {
        string errmsg = _error? _error->what() : _log->highest_msg();
        _log->set_mail_subject( string("FEHLER ") + errmsg );   //, is_error );

        body += errmsg + "\n\n";
    }

    _log->set_mail_body( body + "Das Jobprotokoll liegt dieser Nachricht bei." );   //, is_error );
}
*/
//---------------------------------------------------------------------------------Task::clear_mail
/*
void Task::clear_mail()
{
    _log->set_mail_from_name( "", true );
    _log->set_mail_subject  ( "", true );
    _log->set_mail_body     ( "", true );
}
*/

//----------------------------------------------------------------------------Task::set_web_service

void Task::set_web_service( const string& name )
{ 
    if( _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    set_web_service( name == ""? NULL 
                               : _spooler->_web_services->web_service_by_name( name ) );
}

//----------------------------------------------------------------------------Task::set_web_service

void Task::set_web_service( Web_service* web_service )                
{ 
    if( _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );

    _web_service = web_service; 
}

//--------------------------------------------------------------------------------Task::web_service

Web_service* Task::web_service() const
{
    Web_service* result = web_service_or_null();
    if( !result )  z::throw_xc( "SCHEDULER-241" );
    return result;
}

//---------------------------------------------------------------------Module_task::do_close__start

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
        _file_logger->flush();

        int exit_code = _module_instance->exit_code();
        if( exit_code )
        {
            z::Xc x ( "SCHEDULER-280", exit_code, printf_string( "%X", exit_code ) );
            if( _module_instance->_module->kind() == Module::kind_process  &&  !_module_instance->_module->_process_ignore_error )  
                set_error( x );
            else  
                _log->warn( x.what() );

            if( _module_instance->_module->kind() == Module::kind_process )
                _exit_code = exit_code;  // Nach set_error(), weil set_error() _exit_code auf 1 setzt
        }
                                          
        if( int termination_signal = _module_instance->termination_signal() )
        {
            z::Xc x ( "SCHEDULER-279", termination_signal, signal_name_from_code( termination_signal ) + " " + signal_title_from_code( termination_signal ) );

            if( !_job->_ignore_every_signal 
             && _job->_ignore_signals_set.find( termination_signal ) == _job->_ignore_signals_set.end()
             && ( _module_instance->_module->kind() != Module::kind_process  ||  !_module_instance->_module->_process_ignore_signal ) )
            {
                set_error( x );
            }
            else  
            {
                _log->warn( x.what() );
                _log->debug( message_string( "SCHEDULER-973" ) ); //, signal_name_from_code( termination_signal ) ) );
                
                if( _is_connection_reset_error )
                {
                    _error = _non_connection_reset_error;    // Vorherigen Fehler (vor dem Verbindungsverlust) wiederherstellen
                    _non_connection_reset_error = NULL;
                    _is_connection_reset_error = false;
                }
            }

            //if( _module_instance->_module->kind() == Module::kind_process )  
            _exit_code = -termination_signal;
        }
 

        //if( !_file_logger->has_files() )     // add_file() nicht aufgerufen? (Vielleicht wegen Fehler)  Dann selbst loggen:
        //{
        //    _log->log_file( _module_instance->stdout_path(), "stdout:" );
        //    _log->log_file( _module_instance->stderr_path(), "stderr:" );
        //}

        _file_logger->finish();
        _file_logger->close();

        //_module_instance = NULL;    // Nach set_error(), weil set_error() _exit_code auf 1 setzt
    }
}

//--------------------------------------------------------------------------Job_module_task::do_kill

bool Job_module_task::do_kill()
{
    return _module_instance? _module_instance->kill() :
           _operation      ? _operation->async_kill()
                           : false;
}

//-------------------------------------------------------------------------Job_module_task::do_load

bool Job_module_task::do_load()
{
    ptr<Module_instance> module_instance;
    bool                 is_new = false;


    if( _job->_module->_reuse == Module::reuse_job )
    {
        //module_instance = _job->get_free_module_instance( this );
        module_instance = _job->create_module_instance();
        if( !module_instance )  return false;
    }
    else
    {
        module_instance = _job->create_module_instance();
        if( !module_instance )  return false;

        is_new = true;
        module_instance->set_close_instance_at_end( true );
        module_instance->set_job_name( _job->name() );      // Nur zum Debuggen (für shell-Kommando ps)
    }

  //module_instance->set_title( obj_name() );
  //module_instance->_com_task->set_task( this );
  //module_instance->_com_log->set_log( &_log );


    _module_instance = module_instance;
    _module_instance->attach_task( this, _log );

    //_module_instance->set_params( _params );

    if( !module_instance->loaded() )
    {
        module_instance->init();

        module_instance->add_obj( (IDispatch*)_spooler->_com_spooler    , "spooler"        );
        module_instance->add_obj( (IDispatch*)_job->_com_job            , "spooler_job"    );
        module_instance->add_obj( (IDispatch*)module_instance->_com_task, "spooler_task"   );
        module_instance->add_obj( (IDispatch*)module_instance->_com_log , "spooler_log"    );

        bool ok =  module_instance->load();
        if( !ok ) return false;

        module_instance->start();
    }

    return true;
}

//------------------------------------------------------------------Job_module_task::do_begin_start

Async_operation* Job_module_task::do_begin__start()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

    return _module_instance->begin__start();
}

//-------------------------------------------------------------------Job_module_task::do_begin__end

bool Job_module_task::do_begin__end()
{
    bool ok;

    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

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
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

    return _module_instance->step__start();
}

//--------------------------------------------------------------------Job_module_task::do_step__end

Variant Job_module_task::do_step__end()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

    return _module_instance->step__end();
}

//------------------------------------------------------------------Job_module_task::do_call__start

Async_operation* Job_module_task::do_call__start( const string& method )
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

    return _module_instance->call__start( method );
}

//--------------------------------------------------------------------Job_module_task::do_call__end

bool Job_module_task::do_call__end()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );

    return check_result( _module_instance->call__end() );
}

//---------------------------------------------------------------Job_module_task::do_release__start

Async_operation* Job_module_task::do_release__start()
{
    if( !_module_instance )  return &dummy_sync_operation; //z::throw_xc( "SCHEDULER-199" );

    return _module_instance->release__start();
}

//-----------------------------------------------------------------Job_module_task::do_release__end

void Job_module_task::do_release__end()
{
    if( !_module_instance )  return;  //z::throw_xc( "SCHEDULER-199" );

    _module_instance->release__end();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
