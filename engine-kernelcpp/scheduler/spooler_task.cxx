// $Id: spooler_task.cxx 15048 2011-08-26 08:59:42Z ss $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
#include "spooler.h"
#include "file_logger.h"
#include "../kram/sleep.h"
#include "../zschimmer/z_signals.h"

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

static const string             spooler_get_name                = "spooler_get";
static const string             spooler_level_name              = "spooler_level";

//--------------------------------------------------------------------------------------------Calls

namespace job {
    DEFINE_SIMPLE_CALL(Task, Task_starting_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_opening_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_step_completed_call)
    DEFINE_SIMPLE_CALL(Task, Try_next_step_call)
    DEFINE_SIMPLE_CALL(Task, Task_end_completed_call)
    DEFINE_SIMPLE_CALL(Task, Next_spooler_process_call)
    DEFINE_SIMPLE_CALL(Task, Next_order_step_call)
    DEFINE_SIMPLE_CALL(Task, Task_idle_timeout_call)
    DEFINE_SIMPLE_CALL(Task, Task_end_with_period_call)
    DEFINE_SIMPLE_CALL(Task, Task_locks_are_available_call)
    DEFINE_SIMPLE_CALL(Task, Task_check_for_order_call)
    DEFINE_SIMPLE_CALL(Task, Remote_task_running_call)
    DEFINE_SIMPLE_CALL(Task, Task_delayed_spooler_process_call)
    DEFINE_SIMPLE_CALL(Task, Task_on_success_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_on_error_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_exit_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_release_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_wait_for_subprocesses_completed_call)
    DEFINE_SIMPLE_CALL(Task, Task_ended_completed_call)
    DEFINE_SIMPLE_CALL(Task, End_task_call)
    DEFINE_SIMPLE_CALL(Task, Warn_longer_than_call)
    DEFINE_SIMPLE_CALL(Task, Task_timeout_call)
    DEFINE_SIMPLE_CALL(Task, Try_deleting_files_call)
    DEFINE_SIMPLE_CALL(Task, Killing_task_call)
    DEFINE_SIMPLE_CALL(Task, Subprocess_timeout_call)
    DEFINE_SIMPLE_CALL(Task, Kill_timeout_call)
    DEFINE_SIMPLE_CALL(Task, Process_class_available_call)
    DEFINE_SIMPLE_CALL(Task, Task_do_something_call)
}

using namespace job;

//------------------------------------------------------------------------------Task_lock_requestor

struct Task_lock_requestor : lock::Requestor {
    Task_lock_requestor( Task* task ) :
        Requestor( task ),
        _task(task)
    {}

    ~Task_lock_requestor() {}

    bool locks_are_available() const {
        return locks_are_available_for_holder( _task->_lock_holder );
    }

    void on_locks_are_available() {
        _task->on_locks_are_available( this );
    }

  private:
    Task*                      _task;
};

//---------------------------------------------------------------------------------start_cause_name

string start_cause_name( Start_cause cause )
{
    switch( cause ) {
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

//----------------------------------------------------------------------Task_subsystem::dom_element

xml::Element_ptr Task_subsystem::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = Subsystem::dom_element( dom_document, show_what );
    xml::Element_ptr task_subsystem_element = dom_document.createElement( "task_subsystem" );

    if( show_what.is_set( show_statistics ) ) {
        xml::Element_ptr statistics_element = task_subsystem_element.append_new_element( "task_subsystem.statistics" );
        xml::Element_ptr task_statistics_element = statistics_element.append_new_element( "task.statistics" );
        task_statistics_element.appendChild(state_task_statistic_element( dom_document, Task::s_running ));
        task_statistics_element.appendChild(state_task_statistic_element( dom_document, Task::s_starting ));
        task_statistics_element.appendChild(exist_task_statistic_element( dom_document ));
    }

    result.appendChild( task_subsystem_element );
    return result;
}

//-------------------------------------------------------Task_subsystem::state_task_statistic_element

xml::Element_ptr Task_subsystem::state_task_statistic_element( const xml::Document_ptr& dom_document, Task::State state ) const
{
    return task_statistic_element( dom_document, "task_state", Task::state_name( state ), count_tasks_with_state( state ) );
}

//-------------------------------------------------------Task_subsystem::state_task_statistic_element

xml::Element_ptr Task_subsystem::exist_task_statistic_element( const xml::Document_ptr& dom_document ) const
{
    return task_statistic_element( dom_document, "task_state", "exist", count_tasks_exist( ) );
}


//-------------------------------------------------------Task_subsystem::state_task_statistic_element

xml::Element_ptr Task_subsystem::task_statistic_element( const xml::Document_ptr& dom_document,
    const string& attribute_name, const string& attribute_value, int count ) const
{
    xml::Element_ptr result = dom_document.createElement( "task.statistic" );
    result.setAttribute( attribute_name, attribute_value );
    result.setAttribute( "count", count );
    return result;
}

//-------------------------------------------------------------Task_subsystem::count_tasks_with_state

int Task_subsystem::count_tasks_with_state( Task::State state ) const
{
    int result = 0;

    FOR_EACH_TASK_CONST ( it, task )
        if( task->state() == state )  result++;

    return result;
}

//-------------------------------------------------------------------Task_subsystem::count_tasks_exist

int Task_subsystem::count_tasks_exist( ) const
{
    int result = 0;

    // TODO ggf. bestimmte Status nicht zählen
    FOR_EACH_TASK_CONST ( it, task )
        result++;

    return result;
}

//---------------------------------------------------------------------------------------Task::Task

Task::Task(Standard_job* job, Log_level stderr_log_level)
:
    Abstract_scheduler_object( job->_spooler, this, Scheduler_object::type_task ),
    javabridge::has_proxy<Task>(job->_spooler),
    _zero_(this+1),
    _job(job),
    _call_register(this),
    _history(&job->_history,this),
    _timeout(Duration::eternal),
    _lock_requestors( 1+lock_level__max ),
    _warn_if_longer_than( Duration::eternal ),
    _order_state_transition(Order_state_transition::keep),
    _stderr_log_level(stderr_log_level),
    _typed_java_sister(java_sister())    
{
    _log = Z_NEW( Prefix_log( this ) );

    _let_run = _job->_period.let_run();

    _log->set_job_name( job->name() );
    _log->set_task( this );
    _log->inherit_settings( *_job->_log );
    _log->set_mail_defaults();

    _idle_timeout_at = Time::never;

    set_subprocess_timeout();
    _warn_if_shorter_than = _job->get_step_duration_or_percentage( _job->_warn_if_shorter_than_string, Duration(0) );
    _warn_if_longer_than  = _job->get_step_duration_or_percentage( _job->_warn_if_longer_than_string , Duration::eternal );

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
    if( _file_logger )
        _file_logger->set_async_manager( NULL );

    try {
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
    if( !_closed ) {
        if( _file_logger ) {
            _file_logger->set_async_manager( NULL );
            _file_logger->close();
        }

        FOR_EACH( Registered_pids, _registered_pids, p )
            p->second->close();

        if( _operation ) {
            Z_LOG2( "scheduler", *this << ".close(): Operation active: " << _operation->async_state_text() << "\n" );
            try {
                _operation->async_kill();  // do_kill() macht nachher das gleiche
            }
            catch( exception& x )  { Z_LOG2( "scheduler", "Task::close() _operation->async_kill() ==> " << x.what() << "\n" ); }
            close_operation();
        }
        if (_process_class) {
            remove_requisite(Requisite_path(_process_class->subsystem(), _process_class->path()));
            _process_class->remove_requestor(this);
        }
        _order_for_task_end = NULL;

        if( _order ) {
            _order->remove_from_job_chain();
            _order->close();  //detach_order_after_error(); Nicht rufen! Der Auftrag bleibt stehen und der Job startet wieder und wieder.
        }
        if( _module_instance ) {
            try {
                do_kill();
            }
            catch( exception& x )  { Z_LOG2( "scheduler", "Task::close() do_kill() ==> " << x.what() << "\n" ); }
            _module_instance->detach_task();
            _module_instance->close();
        }

/* 2005-09-24 nicht blockieren. Scheduler kann sich beenden (nach timeout), auch wenn Tasks laufen
        try {
            Async_operation* op = do_close__start();
            if( !op->async_finished() )  _log->warn( "Warten auf Abschluss der Task ..." );
            do_close__end();
        }
        catch( const exception& x ) { _log->error( string("close: ") + x.what() ); }
*/

        // Alle, die mit wait_until_terminated() auf diese Task warten, wecken:
        FOR_EACH( vector<Event*>, _terminated_events, it )  (*it)->signal( "task closed" );
        _terminated_events.clear();

        _closed = true;
        if (has_error() || _log->highest_level() >= log_error) {
            try {
                _history.start();
            } catch (exception& x) {
                log()->error(S() << "When starting task history after error: " << x.what());
            }
        }
        _history.end();    // DB-Operation, kann Exception auslösen
    }

    if( _lock_holder )
        _lock_holder->release_locks(),  _lock_holder = NULL;

    set_state_direct( s_closed );
}

//---------------------------------------------------------------------------------------Task::init

void Task::init()
{
    set_state_direct( s_loading );
    _file_logger = Z_NEW( File_logger( _log ) );
    _file_logger->set_object_name( obj_name() );
    _spooler->_task_subsystem->add_task( this );
}

//------------------------------------------------------------------------------------Task::set_dom

void Task::set_dom( const xml::Element_ptr& element )
{
    _force_start = element.bool_getAttribute( "force_start" );

    string web_service_name = element.getAttribute( "web_service" );
    if( web_service_name != "" )  set_web_service( web_service_name );

    DOM_FOR_EACH_ELEMENT( element, e ) {
        if( e.nodeName_is( "environment" ) )
            _environment->set_dom( e, NULL, "variable" );
    }
}

//--------------------------------------------------------------------------------Task::dom_element
// s.a. Spooler_command::execute_show_task() zum Aufbau des XML-Elements <task>

xml::Element_ptr Task::dom_element( const xml::Document_ptr& document, const Show_what& show ) const
{
    xml::Element_ptr task_element = document.createElement( "task" );

    if( !show.is_set( show_for_database_only ) ) {
        if( _job )
        task_element.setAttribute( "job"             , _job->path() );

        task_element.setAttribute( "id"              , _id );
        task_element.setAttribute( "task"            , _id );
        task_element.setAttribute( "state"           , state_name() );
        if (is_waiting_for_remote_scheduler()) {
            task_element.setAttribute_optional("waiting_for_remote_scheduler", "true");
        }

        if( _enqueued_state )
        task_element.setAttribute( "enqueued_state"  , state_name( _enqueued_state ) );

        if( _delayed_after_error_task_id )
        task_element.setAttribute( "delayed_after_error_task", _delayed_after_error_task_id );

        task_element.setAttribute( "name"            , _name );

        if( _running_since.not_zero() )
        task_element.setAttribute( "running_since"   , _running_since.xml_value() );

        if( _enqueue_time.not_zero() )
        task_element.setAttribute( "enqueued"        , _enqueue_time.xml_value() );

        if( _start_at.not_zero() )
        task_element.setAttribute( "start_at"        , _start_at.xml_value() );

        if( _idle_since.not_zero() )
        task_element.setAttribute( "idle_since"      , _idle_since.xml_value() );

        if( _cause )
        task_element.setAttribute( "cause"           , start_cause_name( _cause ) );

        if( _state == s_running  &&  _step_started_at.not_zero() )
        task_element.setAttribute( "in_process_since", _step_started_at.xml_value() );

        task_element.setAttribute( "steps"           , _step_count );

        task_element.setAttribute( "log_file"        , _log->filename() );

        if( _module_instance ) {
            if( _module_instance->_in_call )
                task_element.setAttribute( "calling", _module_instance->_in_call->name() );
            task_element.setAttribute_optional( "process", _module_instance->process_name() );
            if (int pid = _module_instance->pid()) {
                task_element.setAttribute( "pid", pid );       // separate_process="yes", Remote_module_instance_proxy
                try {
                    zschimmer::Process process ( pid );
                    task_element.setAttribute( "priority", process.priority_class() );
                }
                catch( exception& x ) {
                    Z_LOG2( "scheduler", Z_FUNCTION << " priority_class() ==> " << x.what() << "\n" );
                }
            }
        }

        if( _operation )
            task_element.setAttribute( "operation", _operation->async_state_text() );

      //if( _lock_holder )  task_element.appendChild( _lock_holder->dom_element( document, show ) );

        if( Order* order = _order? _order : _order_for_task_end )  dom_append_nl( task_element ),  task_element.appendChild( order->dom_element( document, show ) );
        if( _error )  dom_append_nl( task_element ),  append_error_element( task_element, _error );

        if( !_registered_pids.empty() ) {
            xml::Element_ptr subprocesses_element = document.createElement( "subprocesses" );
            FOR_EACH_CONST( Registered_pids, _registered_pids, it ) {
                if (const Registered_pid* p = it->second) {
                    xml::Element_ptr subprocess_element = document.createElement( "subprocess" );
                    subprocess_element.setAttribute( "pid", p->_pid );
                    try {
                        zschimmer::Process process ( p->_pid );
                        subprocess_element.setAttribute( "priority", process.priority_class() );
                    }
                    catch( exception& x ) {
                        Z_LOG2( "scheduler", Z_FUNCTION << " priority_class() ==> " << x.what() << "\n" );
                    }

                    if( p->_timeout_at != Time::never )
                        subprocess_element.setAttribute( "timeout_at", p->_timeout_at.xml_value() );

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

bool Task::is_waiting_for_remote_scheduler() const {
    if (_state == s_waiting_for_process) {
        if (Remote_module_instance_proxy* o = dynamic_cast<Remote_module_instance_proxy*>(+_module_instance)) {
            if (o->is_waiting_for_remote_scheduler()) {
                return true;
            }
        }
    }
    return false;
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

Standard_job* Task::job()
{
    if( !_job )  assert(0), throw_xc( "TASK-WITHOUT-JOB", obj_name() );
    return _job;
}

//----------------------------------------------------------------------Task::calculated_start_time

Time Task::calculated_start_time( const Time& now )
{
    if( _force_start )
        return _start_at;
    else
    if( _job->file_based_state() == Job::s_active )
        return _job->schedule_use()->next_allowed_start( max( _start_at, now ) );
    else
        return Time(0);
}

//------------------------------------------------------------------------------------Task::cmd_end

void Task::cmd_end(Task_end_mode end_mode, const Duration& timeout)
{
    assert( end_mode != task_end_none );

    //if( end_mode == end_kill_immediately  &&  _end != end_kill_immediately )  _log->warn( message_string( "SCHEDULER-282" ) );    // Kein Fehler, damit ignore_signals="SIGKILL" den Stopp verhindern kann
    if( end_mode == task_end_normal  &&  _state < s_ending  &&  !_end )  _log->info( message_string( "SCHEDULER-914" ) );

    if( _end != task_end_kill_immediately )  _end = end_mode;

    if (end_mode == task_end_kill_immediately) {
        if (!timeout.is_zero()) {
            if (_module_instance) {
                _module_instance->kill(Z_SIGTERM);
                if (!timeout.is_eternal()) {
                    _call_register.call_at<Kill_timeout_call>(Time::now() + timeout);
                }
            }
        } else
        if (!_kill_tried) {
            try_kill();
        }
    }

    if( _state == s_none ) {
        if( _job )  _job->kill_queued_task( _id );
        // this ist hier möglicherweise ungültig!
    }
    else
        _call_register.call<End_task_call>();
}


void Task::on_call(const job::Kill_timeout_call&) {
    if (!_kill_tried) {
        try_kill();
        do_something();
    }
}

//-------------------------------------------------------------------------------Task::cmd_nice_end

void Task::cmd_nice_end(Process_class_requestor* for_requestor)
{
    Scheduler_object* obj = dynamic_cast<Scheduler_object*>(for_requestor);
    if (!obj) z::throw_xc(Z_FUNCTION);
    _log->info(message_string("SCHEDULER-271", obj->obj_name()));   //"Task wird dem Job " + for_job->name() + " zugunsten beendet" );
    cmd_end( task_end_nice );
}

//--------------------------------------------------------------------------Task::set_error_xc_only

void Task::set_error_xc_only( const zschimmer::Xc& x )
{
    if( x.name() == com::object_server::Connection_reset_exception::exception_name  ||  x.code() == "SCHEDULER-202") {
        if( !_is_connection_reset_error ) { // Bisheriger _error ist kein Connection_reset_error?
            _non_connection_reset_error = _error;  // Bisherigen Fehler (evtl. NULL) merken, falls Verbindungsverlust wegen ignore_signals=".." ignoriert wird
            _is_connection_reset_error = true;
        }
    } else {
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
    Xc_copy xx = x;
    if (_killed_immediately_by_command) {
        if (_error && strcmp(_error->code(), "SCHEDULER-728")) return;
        xx = Xc("SCHEDULER-728");
    }
    bool timed_out = _error && strcmp(_error->code(), "SCHEDULER-272") == 0;
    if (!timed_out) {
        const char* scheduler272 = strstr(xx->what(), "SCHEDULER-272 ");
        if (strcmp(xx->code(), "SCHEDULER-140") == 0 && scheduler272) {
            xx->set_code("SCHEDULER-272");  // Remove SCHEDULER-140
            xx->set_what(scheduler272);
        }
        _error = *xx;
        if (_job)
            _job->set_error_xc_only(*xx);
        if( _order )
            _order->set_task_error(*xx);
    }

    if (_exit_code == 0) {
        _exit_code = 1;
    }
}

//-------------------------------------------------------------------------------Task::set_error_xc
// Siehe auch set_error_xc( const Xc& )

void Task::set_error_xc( const zschimmer::Xc& xx )
{
    Xc_copy x = xx;
    if (strcmp(x->code(), "Z-REMOTE-101") == 0) {
        x->set_what(message_string("SCHEDULER-202", state_name(), x->what()));
        x->set_code("SCHEDULER-202"); // Connection to task has been lost
    }
    set_error_xc_only( *x );
    _log->error( x.what() );

    if( _is_connection_reset_error  &&  !_non_connection_reset_error ) {
        S s;

        if( !_job ) {
            s << "(no job)";
        }
        else {
            if( _job->_ignore_every_signal )
                s << "all";
            else {
                Z_FOR_EACH( stdext::hash_set<int>, _job->_ignore_signals_set, it ) {
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
        set_error_xc( *(zschimmer::Xc*)&x );
    else
    if( dynamic_cast< const Xc* >( &x ) )
        set_error_xc( *(Xc*)&x );
    else {
        Xc xc ( "SOS-2000", x.what(), exception_name(x).c_str() );      // Siehe auch Spooler_com::set_error_silently
        set_error_xc( xc );
    }
}

//----------------------------------------------------------------------------------Task::set_error
#ifdef SYSTEM_HAS_COM

void Task::set_error( const _com_error& x )
{
    try {
        throw_mswin_error( x.Error(), x.Description() );
    }
    catch( const Xc& x )
    {
        set_error_xc( x );
    }
}

#endif

//----------------------------------------------------------------------------------Task::set_state

/**
 * \brief Status zum Setzen vormerken
 * \detail
 * JS-380: Job Scheduler freeze after <modify_job ... cmd=suspend/>
 * Merkt bei laufender Operation einen Status zum Setzen vor. Läuft keine Operation (z.B. spooler.process)
 * wird der Status via set_state_direct() sofort gesetzt.
 * \version 2.1.1 - 2010-04-29
 *
 * \param name - description
 * \return type of returnvalue
 */
void Task::set_state( State new_state )
{
    _enqueued_state = new_state;

    if( _operation ) {
        // Verzögern bis Operation fertig ist. Siehe Aufruf von set_enqueued_state()
    }
    else {
        set_enqueued_state();
        assert( !_enqueued_state );
    }
}

//-------------------------------------------------------------------------Task::set_enqueued_state

/**
 * \brief Status zum Setzen vormerken
 * \detail
 * JS-380: Job Scheduler freeze after <modify_job ... cmd=suspend/>
 * Setzt einen bereits vorgemerketer Status für eine Task endgültig. Ist kein Status zum Setzen vorgemerkt, bleibt der
 * Aufruf ohne Wirkung.
 * \version 2.1.1 - 2010-04-29
 *
 * \param name - description
 * \return type of returnvalue
 */
void Task::set_enqueued_state()
{
    if( _enqueued_state ) {
        set_state_direct( _enqueued_state );
        _enqueued_state = s_none;
    }
}

//---------------------------------------------------------------------------Task::set_state_direct

/**
 * \brief Status setzen (ehemals set_state)
 * \detail
 * JS-380: Job Scheduler freeze after <modify_job ... cmd=suspend/>
 * Setzt einen Status unmittelbar. Ein evtl. vorgemerkter Status (_enqueued_state) wird verworfen.
 * \version 2.1.1 - 2010-04-29
 *
 * \param new_state - Status auf den die task gesetzt wird
 */
void Task::set_state_direct( State new_state )
{
    if( _enqueued_state ) {
        Z_LOG2( "scheduler", *this << " _enqueued_state=" + state_name( _enqueued_state ) + " refused" );
        _enqueued_state = s_none;
    }

    if( new_state != s_running_waiting_for_order )
        _idle_since = Time(0);

    if( new_state != _state ) {
        switch( _state ) {
            case s_running:
                if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] )   {
                    _lock_holder->release_locks( lock_requestor );
                    lock_requestor->dequeue_lock_requests();
                }
                break;

            case s_waiting_for_process:
                break;

            default: ;
        }

        switch( new_state ) {
            case s_opening_waiting_for_locks:
            case s_running_process:
                break;

            case s_running_delayed:
                _call_register.call_at<Next_spooler_process_call>(_next_spooler_process);
                break;

            case s_running_waiting_for_locks:
                break;

            case s_running_waiting_for_order: {
                _call_register.call_at<Next_order_step_call>(_job->next_order_time());
                if (_state != s_running_waiting_for_order)
                    _idle_since = Time::now();
                if (_idle_timeout_at != Time::never)
                    _call_register.call_at<Task_idle_timeout_call>(_idle_timeout_at);
                break;
            }

            case s_closed: {
                report_event_code(taskClosedEvent, java_sister());
                _call_register.call(Z_NEW(Task_closed_call(this)));
                break;
            }

            default: ;
        }
    }

    if( new_state != _state ) {
        if( _job  &&  _spooler->_task_subsystem ) {
            if( new_state == s_starting ||  new_state == s_opening  ||  new_state == s_running )  _job->increment_running_tasks(),  _spooler->_task_subsystem->increment_running_tasks();
            if( _state    == s_starting ||  _state    == s_opening  ||  _state    == s_running )  _job->decrement_running_tasks(),  _spooler->_task_subsystem->decrement_running_tasks();
        }

        if( new_state != s_running_delayed )
            _next_spooler_process = Time(0);

        State old_state = _state;
        _state = new_state;

        if( is_idle()  &&  _job ) {
            if (Process_class_requestor* o = process_class()->waiting_requestor_or_null()) {
                cmd_nice_end(o);
            }
        }

        Log_level log_level = new_state == s_starting || new_state == s_closed? log_info : log_debug9;
        if( ( log_level >= log_info || _spooler->_debug )  &&  ( _state != s_closed || old_state != s_none ) ) {
            S details;
            if( new_state == s_starting  &&  _start_at.not_zero() )  details << " (at=" << _start_at.as_string(_job->time_zone_name()) << ")";
            if( new_state == s_starting  &&  _module_instance && _module_instance->process_name() != "" )  details << ", process " << _module_instance->process_name();
            _log->log( log_level, message_string( "SCHEDULER-918", state_name(), details ) );
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
    _history.set_extra_field( name, value );
}

//------------------------------------------------------------------Task::set_delay_spooler_process

void Task::set_delay_spooler_process(const Duration& d)
{
    _log->debug("delay_spooler_process=" + d.as_string() );
    _next_spooler_process = Time::now() + d;
    _call_register.call_at<Task_delayed_spooler_process_call>(_next_spooler_process);
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
        case s_opening_waiting_for_locks:   set_state_direct( s_opening );  break;
        case s_running_waiting_for_locks:   set_state_direct( s_running );  break;
        default:                            z::throw_xc( Z_FUNCTION );
    }

    _call_register.call<Task_locks_are_available_call>();
}

//------------------------------------------------------------------------------Task::set_next_time

//void Task::set_next_time( const Time& t )
//{
//    Time t = min(t, next_time());
//    if (_state == s_closed)
//        _call_register.cancel<Next_time_task_call>();
//    else
//        _call_register.call_at<Next_time_task_call>(t);
//}

//----------------------------------------------------------------------------------Task::next_time

//Time Task::next_time()
//{
//    Time result;
//
//    if( _operation )
//    {
//        if( _operation->async_finished() ) {
//            result = Time(0);   // Falls Operation synchron ist (das ist, wenn Task nicht in einem separaten Prozess läuft)
//        } else {
//            Duration t = _timeout;
//            result = t.is_eternal()? Time::never
//                                   : _last_operation_time + t;     // _timeout sollte nicht zu groß sein
//        }
//    }
//    else
//    if( _state == s_running_process  &&  !_timeout.is_eternal() )
//    {
//        result = Time( _last_operation_time + _timeout );     // _timeout sollte nicht zu groß sein
//    }
//    else
//    {
//        result = _next_time;
//        if( _state == s_running_delayed  &&  result > _next_spooler_process )  result = _next_spooler_process;
//    }
//
//    if( result > _subprocess_timeout )  result = _subprocess_timeout;
//
//    return result;
//}


void Task::notify_a_process_is_available() {
    Z_LOG2("scheduler", Z_FUNCTION << "\n");
    _call_register.call<Process_class_available_call>();
}

bool Task::on_requisite_loaded(File_based* file_based) {
    if (dynamic_cast<Process_class*>(file_based) && _state == s_waiting_for_process) {
        _call_register.call<Task_do_something_call>();
    }
    return true;
}

//------------------------------------------------------------------------------------Task::on_call

void Task::on_call(const Task_starting_completed_call&) {
    do_something();
}

void Task::on_call(const Task_opening_completed_call&) {
    do_something();
}

void Task::on_call(const Task_step_completed_call&) {
    do_something();
}

void Task::on_call(const Try_next_step_call&) {
    do_something();
}

void Task::on_call(const Next_spooler_process_call&) {
    do_something();
}

void Task::on_call(const Next_order_step_call&) {
    do_something();
}

void Task::on_call(const Task_idle_timeout_call&) {
    do_something();
}

void Task::on_call(const Task_locks_are_available_call&) {
    do_something();
}

void Task::on_call(const Task_check_for_order_call&) {
    do_something();
}

void Task::on_call(const Subprocess_timeout_call&) {
    do_something();
}

void Task::on_call(const Task_end_completed_call&) {
    do_something();
}

void Task::on_call(const Remote_task_running_call&) {
    do_something();
}

void Task::on_call(const Task_delayed_spooler_process_call&) {
    do_something();
}

void Task::on_call(const Task_end_with_period_call&) {
    do_something();
}

void Task::on_call(const Task_on_success_completed_call&) {
    do_something();
}

void Task::on_call(const Task_on_error_completed_call&) {
    do_something();
}

void Task::on_call(const Task_exit_completed_call&) {
    do_something();
}

void Task::on_call(const Task_release_completed_call&) {
    do_something();
}

void Task::on_call(const Task_ended_completed_call&) {
    do_something();
}

void Task::on_call(const End_task_call&) {
    do_something();
}

void Task::on_call(const Warn_longer_than_call&) {
    do_something();
}

void Task::on_call(const job::Task_timeout_call&) {
    do_something();
}

void Task::on_call(const Killing_task_call&) {
    do_something();
}

void Task::on_call(const Task_process_ended_call&) {
    do_something();
}

void Task::on_call(const job::Task_wait_for_subprocesses_completed_call&) {
    do_something();
}

void Task::on_call(const job::Try_deleting_files_call&) {
    do_something();
}

void Task::on_call(const job::Process_class_available_call&) {
    do_something();
}

void Task::on_call(const job::Task_do_something_call&) {
    do_something();
}

void Task::on_waiting_for_agent() {
    if (_job->_history.min_steps() == 0) {
        _history.start();
    }
}

//----------------------------------------------------------------------Task::wake_when_longer_than

void Task::wake_when_longer_than()
{
    if (!_warn_if_longer_than.is_eternal() && _call_register.at<Warn_longer_than_call>().is_never())
        _call_register.call_at<Warn_longer_than_call>(Time::now() + _warn_if_longer_than);
}

//------------------------------------------------------------------------------Task::check_timeout

bool Task::check_timeout( const Time& now )
{
    if( !_timeout.is_eternal()  &&  now >= _last_operation_time + _timeout  &&  !_kill_tried ) {
        _log->error( message_string( "SCHEDULER-272", _timeout.seconds() ) );   // "Task wird nach nach Zeitablauf abgebrochen"
        return try_kill();
    }

    return false;
}

//----------------------------------------------------------------------Task::check_if_shorter_than

void Task::check_if_shorter_than( const Time& now )
{
    if( _warn_if_shorter_than.not_zero() )
    {
        Duration step_time = now - _last_operation_time;

        if( step_time < _warn_if_shorter_than )
        {
            string msg = message_string( "SCHEDULER-711", _warn_if_shorter_than.as_string( time::without_ms ), step_time.as_string( time::without_ms ) );
            _log->warn( msg );

            Scheduler_event scheduler_event( evt_task_step_too_short, log_error, this);
            scheduler_event.set_message(msg);
            Mail_defaults mail_defaults  = _log->_mail_defaults;
            mail_defaults.set( "subject", S() << obj_name() << ": " << msg );
            mail_defaults.set( "body"   , S() << obj_name() << ": " << msg << "\n"
                                          "Step time: " << step_time << "\n" <<
                                          "warn_if_shorter_than=" << _warn_if_shorter_than.as_string( time::without_ms ) );
            scheduler_event.send_mail( mail_defaults );
        }
    }
}

//-----------------------------------------------------------------------Task::check_if_longer_than

bool Task::check_if_longer_than( const Time& now )
{
    bool something_done = false;

    if( !_warn_if_longer_than.is_eternal() ) {
        Duration step_time = now - _last_operation_time;   // JS-448

        if( step_time > _warn_if_longer_than ) {
            // JS-448 something_done = true;
            if( _last_warn_if_longer_operation_time != _last_operation_time ) {    // Merker, damit nur einmal gewarnt wird
                something_done = true;  // JS-448
                _last_warn_if_longer_operation_time = _last_operation_time;
                string msg = message_string( "SCHEDULER-712", _warn_if_longer_than.as_string( time::without_ms ) );
                _log->warn( msg );

                Scheduler_event scheduler_event(evt_task_step_too_long, log_error, this);
                scheduler_event.set_message(msg);
                Mail_defaults mail_defaults  = _log->_mail_defaults;
                mail_defaults.set( "subject", S() << obj_name() << ": " << msg );
                mail_defaults.set( "body"   , S() << obj_name() << ": " << msg << "\n"
                                              "Task step time: " << step_time << "\n" <<
                                              "warn_if_longer_than=" << _warn_if_longer_than.as_string( time::without_ms ) );
                scheduler_event.send_mail( mail_defaults );
            }
        }
    }

    return something_done;
}

//------------------------------------------------------------------------------------Task::add_pid

void Task::add_pid( int pid, const Duration& timeout )
{
    if( _module_instance->is_remote_host() ) {
        _log->warn( message_string( "SCHEDULER-849", pid ) );
    } else {
        Time timeout_at = Time::never;

        if(!timeout.is_eternal()) {
            timeout_at = Time::now() + timeout;
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

void Task::add_subprocess( int pid, const Duration& timeout, bool ignore_exitcode, bool ignore_signal, bool is_process_group, const string& title )
{
    Z_LOG2( "scheduler", Z_FUNCTION << " " << pid << "," << timeout << "," << ignore_exitcode << "," << ignore_signal << "," << is_process_group << "\n" );
    Z_LOG2( "scheduler", Z_FUNCTION << "   title=" << title << "\n" );   // Getrennt, falls Parameterübergabe fehlerhaft ist und es zum Abbruch kommt (com_server.cxx)

    if( !_module_instance->is_remote_host() ) {
        Time timeout_at = timeout.is_eternal()? Time::never : Time::now() + timeout;
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
    if( _spooler ) {
        if( _task->_module_instance  &&  !_task->_module_instance->is_remote_host() )  _spooler->unregister_pid( _pid );
        _spooler = NULL;
    }
}

//-------------------------------------------------------------------Task::Registered_pid::try_kill

void Task::Registered_pid::try_kill()
{
    if (!_killed) {
        if( _task->_module_instance->is_remote_host() ) {
            //int REMOTE_SUBPROCESS_KILLEN;
            _task->log()->warn( message_string( "SCHEDULER-849", _pid ) );
            // Asynchron <remote_scheduler.subprocess.kill task="..." pid="..."/>
            // ohne Ende abzuwarten?
            // Operation für solche asynchronen XML-Befehle mit Warteschlange
            // spooler_communication.cxx muss mehrere XML-Dokumente aufeinander empfangen und trennen können.
        } else {
            try {
#               ifdef Z_UNIX
                    if( _is_process_group )  posix::kill_process_group_immediately( _pid, obj_name() );
                  else
#               endif
                    kill_process_immediately( _pid, obj_name() );

                _task->_log->warn( message_string( "SCHEDULER-273", _pid ) );   // "Subprozess " << _pid << " abgebrochen"
            }
            catch( exception& x ) {
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
    _call_register.call_at<Subprocess_timeout_call>(_subprocess_timeout);
}

//-------------------------------------------------------------------Task::check_subprocess_timeout

bool Task::check_subprocess_timeout( const Time& now )
{
    bool something_done = false;
    if( _subprocess_timeout <= now ) {
        FOR_EACH( Registered_pids, _registered_pids, p ) {
            Registered_pid* subprocess = p->second;
            if( subprocess->_timeout_at <= now ) {
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

    try {
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
    bool had_operation      = _operation != NULL;
    bool something_done     = false;
    Time now                = Time::now();

    something_done |= check_subprocess_timeout( now );

    if( _operation &&  !_operation->async_finished() ) {
        // Die Operation tut was und wir wollen prüfen, ob sie es solange darf
        something_done |= check_timeout( now );
        something_done |= check_if_longer_than( now ); // JS-448, ggf. nur im Status running prüfen?
    } else {
        // Periode endet?
        if( !_operation ) {
            something_done |= check_if_longer_than( now ); // JS-448, ggf. nur im Status running prüfen?
            if (!_end &&
             (  _state == s_running
             || _state == s_running_delayed
             || _state == s_running_waiting_for_order))
            {
                bool let_run = _let_run  ||  _job->_period.is_in_time( now )  ||  ( _job->select_period(now), _job->is_in_period(now) );
                if( !let_run ) {
                    _log->info( message_string( "SCHEDULER-278" ) );   // "Laufzeitperiode ist abgelaufen, Task wird beendet"
                    cmd_end();
                }
            }
        }

        int error_count = 0;

        for (bool loop = true; loop;) {
            try {
                try {
                    loop = false;
                    bool ok = true;

                    if( !_operation  &&  _state != s_running_process ) {
                        if( _module_instance && !_module_instance_async_error ) {
                            try {
                                _module_instance->check_connection_error();
                            }
                            catch( exception& x ) {
                                _module_instance_async_error = true;
                                set_error( z::Xc( "SCHEDULER-202", state_name(), x.what() ) );
                                if( _state < s_killing )  set_state_direct( s_killing );
                            }
                        }

                        if( _state < s_ending  &&  _end ) {     // Task beenden?
                            if( _end == task_end_nice  &&  _state <= s_running  &&  _order  &&  _step_count == 0 ) {  // SCHEDULER-226 (s.u.) nach SCHEDULER-271 (Task-Ende wegen Prozessmangels)
                                if( !_scheduler_815_logged ) {
                                    _log->info( message_string( "SCHEDULER-815", _order->obj_name() ) );    // Task einen Schritt machen lassen, um den Auftrag auszuführen
                                    _scheduler_815_logged = true;
                                }
                            }
                            else
                            if( !loaded() )
                                set_state_direct( s_ended );
                            else
                            if( !_begin_called )
                                set_state_direct( s_release );
                            else
                                set_state_direct( s_ending );
                        }
                    }

                    switch( _state ) { // nächster Status
                        case s_loading: {
                            something_done = true;
                            bool ok = load();
                            if (!ok ) {
                                set_state_direct(s_release);
                                loop = true;
                                break;
                            }
                        }

                        case s_waiting_for_process: {
                            if (!_module_instance) z::throw_xc("s_waiting_for_process");
                            bool ok = _module_instance->try_to_get_process();
                            if (!ok) {
                                if (state() != s_waiting_for_process) {
                                    if (!_module_instance->has_process()) {
                                        log()->info(Message_string("SCHEDULER-949", _module_instance->process_class()->path()));
                                        process_class()->enqueue_requestor(this);
                                        task_subsystem()->try_to_free_process(this, _process_class);     // Beendet eine Task in s_running_waiting_for_order
                                    }
                                    set_state_direct( s_waiting_for_process );
                                    if (_order) {
                                        report_event_code(orderWaitingInTask, _order->java_sister());
                                    }
                                }
                            } else {
                                process_class()->remove_requestor(this);
                                log()->info(message_726());
                                set_state_direct(s_starting);
                                something_done = true;
                                // Opportunity for Job to log SCHEDULER-930 with process_class, so do not: loop = true;
                            }
                            if (_module_instance->has_process()) {
                                process_class()->remove_requestor(this);
                            }
                            break;
                        }

                        case s_starting: {
                            _begin_called = true;
                            if( !_operation ) {
                                _process_started_at = now;
                                _operation = begin__start();
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_starting_completed_call>());
                            } else {
                                ok = operation__end();
                                if( _job->_history.min_steps() == 0 )  _history.start();

                                _file_logger->add_file(_module_instance->stdout_path(), log_info         , _stderr_log_level == log_info? "stdout" : "");
                                _file_logger->add_file(_module_instance->stderr_path(), _stderr_log_level, _stderr_log_level == log_info? "stderr" : "");

                                if( _file_logger->has_files() ) {
                                    _file_logger->set_async_manager( _spooler->_connection_manager );
                                    _file_logger->start();
                                }

                                set_state_direct( !ok? s_ending :
                                                _module_instance->_module->kind() == Module::kind_process?
                                                _module_instance->kind() == Module::kind_remote? s_running_remote_process
                                                                                               : s_running_process
                                           : s_opening );
                                report_event_code(taskStartedEvent, java_sister());
                                if (_state == s_running_process) {
                                    if (_order) {
                                        report_event_code(orderStepStartedEvent, _order->java_sister());
                                    }
                                    _last_operation_time = now;
                                    wake_when_longer_than();
                                    if (!_timeout.is_eternal()) {
                                        _call_register.call_at<Task_timeout_call>(_last_operation_time + _timeout);
                                    }
                                }
                                loop = true;
                            }
                            something_done = true;
                            break;
                        }

                        case s_opening: {
                            if( !_operation ) {
                                if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_task_api ] ) {
                                    _lock_holder->hold_locks( lock_requestor );
                                    lock_requestor->dequeue_lock_requests();
                                }
                                _operation = do_call__start( spooler_open_name );
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_opening_completed_call>());
                            } else {
                                bool ok = operation__end();
                                if( _delay_until_locks_available ) {
                                    _delay_until_locks_available = false;
                                    _lock_requestors[ lock_level_task_api ]->enqueue_lock_requests( _lock_holder );
                                    set_state_direct( s_opening_waiting_for_locks );
                                    ok = true;
                                } else {
                                    set_state_direct( ok? s_running : s_ending );
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
                            if (!_module_instance->process_has_signaled()) {
                                check_timeout(now);
                            } else {
                                _call_register.cancel<Warn_longer_than_call>();
                                _file_logger->flush();
                                _log->info( message_string( "SCHEDULER-915" ) );
                                check_if_shorter_than( now );
                                // JS-448 something_done |= check_if_longer_than( now );
                                count_step();
                                set_state_direct( s_ending );
                                loop = true;
                                if (!_running_state_reached) {
                                    _running_state_reached = true;  // Also nicht, wenn der Prozess sich sofort beendet hat (um _min_tasks-Schleife zu vermeiden)
                                    //_next_time = Time::never;       // Nach cmd_end(): Warten bis _module_instance->process_has_signaled()
                                }
                            }
                            break;

                        case s_running_remote_process:
                            assert( _module_instance->_module->kind() == Module::kind_process &&
                                    _module_instance->kind()          == Module::kind_remote     );

                            if( !_operation ) {
                                _operation = do_step__start();
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_step_completed_call>());
                                wake_when_longer_than();
                            } else {
                                _call_register.cancel<Warn_longer_than_call>();
                                string result = remote_process_step__end();
                                check_if_shorter_than( now );
                                // JS-448 something_done |= check_if_longer_than( now );
                                set_state_direct( s_release );
                                loop = true;
                            }
                            break;

                        case s_running: {
                            if( !_operation ) {
                                if( _next_spooler_process.not_zero() ) {
                                    set_state_direct( s_running_delayed );
                                    something_done = true;
                                } else {
                                    if( _job->is_order_controlled() ) {
                                        if( !fetch_and_occupy_order( now, state_name() ) ) {
                                            _idle_timeout_at = now + _job->_idle_timeout;
                                            set_state_direct( s_running_waiting_for_order );
                                            break;
                                        }
                                    }
                                    _running_state_reached = true;
                                    _step_started_at = now;
                                    if( _step_count + 1 == _job->_history.min_steps() )
                                        _history.start();
                                    if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] ) {
                                        _lock_holder->hold_locks( lock_requestor );
                                        lock_requestor->dequeue_lock_requests();
                                    }
                                    _operation = do_step__start();
                                    if (!_operation->async_finished())
                                        _operation->on_async_finished_call(_call_register.new_async_call<Task_step_completed_call>());
                                    wake_when_longer_than();
                                    something_done = true;
                                }
                            } else {
                                _call_register.cancel<Warn_longer_than_call>();
                                ok = step__end();
                                close_operation();

                                if( lock::Requestor* lock_requestor = _lock_requestors[ lock_level_process_api ] ) {
                                    if( ok  &&  !_delay_until_locks_available  &&  !_lock_holder->is_holding_all_of( lock_requestor ) )
                                        log()->warn( message_string( "SCHEDULER-469" ) );    // Try_hold_lock() hat versagt und Call_me_again_when_locks_available() nicht aufgerufen

                                    _lock_holder->release_locks( lock_requestor );  // Task.Try_hold_lock() wieder aufheben

                                    if( _delay_until_locks_available ) {
                                        _delay_until_locks_available = false;
                                        set_state_direct( s_running_waiting_for_locks );
                                        lock_requestor->enqueue_lock_requests( _lock_holder );
                                        ok = true;
                                    } else {
                                        _lock_holder->remove_requestor( lock_requestor );
                                        _lock_requestors[ lock_level_process_api ] = NULL;
                                    }
                                }
                                assert( !_delay_until_locks_available );
                                check_if_shorter_than( now );
                                // JS-448 something_done |= check_if_longer_than( now );
                                if (!ok || has_error() || (_order && _order->moved())) {
                                    set_state_direct( s_ending );
                                    loop = true;
                                }
                                if( _state != s_ending  &&  !_end )  _order_for_task_end = NULL;
                                _call_register.call<Try_next_step_call>();
                                something_done = true;
                            }
                            break;
                        }

                        case s_running_waiting_for_order: {
                            if( _next_spooler_process.not_zero() ) {
                                set_state_direct( s_running_delayed );
                                something_done = true;
                            } else
                            if( fetch_and_occupy_order( now, state_name() ) ) {
                                set_state_direct( s_running );     // Auftrag da? Dann Task weiterlaufen lassen (Ende der Run_time wird noch geprüft)
                                loop = true;                // _order wird in step__end() wieder abgeräumt
                            }
                            else
                            if( now >= _idle_timeout_at ) {
                                if( _job->_force_idle_timeout  ||  _job->above_min_tasks() ) {
                                    _log->debug9( message_string( "SCHEDULER-916" ) );   // "idle_timeout ist abgelaufen, Task beendet sich"
                                    _end = task_end_normal;
                                    loop = true;
                                } else {
                                    _idle_timeout_at = now + _job->_idle_timeout;
                                    set_state_direct( s_running_waiting_for_order );   // _next_time neu setzen
                                    Z_LOG2( "scheduler", obj_name() << ": idle_timeout has been elapsed but force_idle_timeout=\"no\" and not more than min_tasks tasks are running, now=" <<
                                        now.as_string(_job->time_zone_name()) << "\n" );
                                    //_log->debug9( message_string( "SCHEDULER-916" ) );   // "idle_timeout ist abgelaufen, Task beendet sich"
                                    something_done = true;
                                }
                            }
                            else
                                _running_state_reached = true;   // Also nicht bei idle_timeout="0"
                            break;
                        }

                        case s_running_delayed: {
                            if( now >= _next_spooler_process ) {
                                _next_spooler_process = Time(0);
                                set_state_direct( s_running ), loop = true;
                            }
                            _running_state_reached = true;
                            break;
                        }

                        case s_running_waiting_for_locks:
                            break;

                        case s_ending: {
                            if( !_operation ) {
                                if( has_error()  ||  _log->highest_level() >= log_error )
                                    _history.start();
                                if( _begin_called ) {
                                    _operation = do_end__start();
                                    if (!_operation->async_finished())
                                        _operation->on_async_finished_call(_call_register.new_async_call<Task_end_completed_call>());
                                } else {
                                    set_state_direct( s_exit );
                                    loop = true;
                                }
                            } else {
                                operation__end();
                                set_state_direct( loaded()? s_ending_waiting_for_subprocesses
                                                          : s_release );
                                loop = true;
                            }
                            something_done = true;
                            break;
                        }

                        case s_ending_waiting_for_subprocesses: {
                            if( !_operation ) {
                                if( shall_wait_for_registered_pid() ) {
                                    if( Remote_module_instance_proxy* m = dynamic_cast< Remote_module_instance_proxy* >( +_module_instance ) ) {
                                        _operation = m->_remote_instance->call__start( "Wait_for_subprocesses" );
                                        if (!_operation->async_finished())
                                            _operation->on_async_finished_call(_call_register.new_async_call<Task_wait_for_subprocesses_completed_call>());
                                    } else {
                                        try {
                                            _subprocess_register.wait();
                                        }
                                        catch( exception& x ) { set_error( x ); }
                                    }
                                }
                            } else {
                                operation__end();
                            }

                            if( !_operation ) {
                                set_state_direct( has_error()? s_on_error : s_on_success );
                                loop = true;
                            }

                            something_done = true;
                            break;
                        }

                        case s_on_success: {
                            if( !_operation ) {
                                _operation = do_call__start( spooler_on_success_name );
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_on_success_completed_call>());
                            } else {
                                operation__end();
                                set_state_direct( s_exit );
                                loop = true;
                            }

                            something_done = true;
                            break;
                        }

                        case s_on_error: {
                            if( !_operation ) {
                                _operation = do_call__start( spooler_on_error_name );
                               if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_on_error_completed_call>());
                            }
                            else
                                operation__end(), set_state_direct( s_exit ), loop = true;
                            something_done = true;
                            break;
                        }

                        case s_exit: {
                            if( !_operation ) {
                                _operation = do_call__start( spooler_exit_name );
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_exit_completed_call>());
                            } else {
                                operation__end();
                                set_state_direct( s_release );
                                loop = true;
                            }
                            something_done = true;
                            break;
                        }

                        case s_release: {
                            if( !_operation ) {
                                _operation = do_release__start();
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_release_completed_call>());
                            } else  {
                                operation__end();
                                set_state_direct( s_killing );
                                loop = true;
                            }
                            something_done = true;
                            break;
                        }

                        case s_killing: {
                            if( _module_instance  &&  _module_instance->is_kill_thread_running() ) {
                                _call_register.call_at<Killing_task_call>(Time::now() + Duration(0.1));
                            } else {
                                set_state_direct( s_ended );
                                loop = true;
                            }
                            break;
                        }

                        case s_ended: {
                            if( !_operation ) {
                                _operation = do_close__start();
                                if (!_operation->async_finished())
                                    _operation->on_async_finished_call(_call_register.new_async_call<Task_ended_completed_call>());
                            } else {
                                operation__end();
                                if( _module_instance  && _module_instance->_module->kind() == Module::kind_process ) {
                                    if( Process_module_instance* process_module_instance = dynamic_cast<Process_module_instance*>( +_module_instance ) ) {
                                        set_state_texts_from_stdout( process_module_instance->get_first_line_as_state_text() );

                                        if( _order ) {
                                            //Process_module_instance::attach_task() hat temporäre Datei zur Rückgabe der Auftragsparameter geöffnet
                                            process_module_instance->fetch_parameters_from_process( _order->params() );
                                            if( !has_error() ) {
                                                postprocess_order(process_module_instance->order_state_transition());
                                            }
                                            else {}     // detach_order_after_error() wird sich drum kümmern.
                                        }
                                        else
                                           process_module_instance->fetch_parameters_from_process( _params );
                                    }
                                    else
                                    if (_order_state_transition != Order_state_transition::keep) {
                                        bool is_simple_shell_error = _module_instance->_module->kind() == Module::kind_process && _module_instance->_module->_monitors->is_empty() && has_error() && _job->stops_on_task_error();
                                        if (!is_simple_shell_error) {
                                            postprocess_order(_order_state_transition);
                                        } else {
                                            // finish() will handle this case
                                        }
                                    }
                                }

                                report_event(CppEventFactoryJ::newTaskEndedEvent(_id, _job->path(), _exit_code));
                                set_state_direct( s_deleting_files );
                                loop = true;
                            }

                            something_done = true;
                            break;
                        }

                        case s_deleting_files: {
                            bool ok;

                            if( !_module_instance ) {
                                ok = true;
                            } else {
                                ok = false;
                                Time now = Time::now();

                                if (!_trying_deleting_files_until  ||  now >= _call_register.at<Try_deleting_files_call>()) {     // do_something() wird zu oft gerufen, deshalb prüfen, ob_next_time erreicht ist.
                                    // Nicht ins _log, also Task-Log protokollieren, damit mail_on_warning nicht greift. Das soll kein Task-Problem sein!
                                    // Siehe auch ~Process_module_instance
                                    Has_log* my_log = NULL; //_trying_deleting_files_until? NULL : _job->log();

                                    // Folgendes könnte zusammengefasst werden mit Remote_task_close_command_response::async_continue_() als eigene Async_operation

                                    ok = _module_instance->try_delete_files( my_log );
                                    if (ok) {
                                        if( _trying_deleting_files_until.not_zero() )
                                            _log->debug( message_string( "SCHEDULER-877" ) );  // Nur, wenn eine Datei nicht löschbar gewesen ist
                                    } else {
                                        if (!_trying_deleting_files_until) {
                                            string paths = join( ", ", _module_instance->undeleted_files() );
                                            _log->debug( message_string( "SCHEDULER-876", paths ) );  // Nur beim ersten Mal
                                        }

                                        if (_trying_deleting_files_until.not_zero()  &&  now >= _trying_deleting_files_until) {   // Nach Fristablauf geben wir auf
                                            string paths = join( ", ", _module_instance->undeleted_files() );
                                            _log->info( message_string( "SCHEDULER-878", paths ) );
                                            _job->log()->warn( message_string( "SCHEDULER-878", paths ) );
                                            ok = true;
                                        }
                                        else
                                        if (!_trying_deleting_files_until)
                                            _trying_deleting_files_until = now + delete_temporary_files_delay;
                                    }

                                    something_done = true;
                                }
                            }

                            if (!ok) {
                                _call_register.call_at<Try_deleting_files_call>(min(now + delete_temporary_files_retry, _trying_deleting_files_until));
                            } else {
                                Order_state_transition t = error_order_state_transition();   // Falls _order nach Fehler noch da ist
                                if( _module_instance )  _module_instance->detach_process();
                                _module_instance = NULL;
                                finish(t);
                                set_state_direct( s_closed );
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

                    if( _operation ) {
                        loop |= _operation->async_finished();   // Falls Sync_operation
                    } else
                    if (_state != s_running_process) {
                        set_enqueued_state();   // Wegen _operation verzögerten Zustand setzen

                        if( (!ok || has_error() || _killed) && _state < s_ending && _state != s_running_process)
                                set_state_direct( s_ending ), loop = true;
                    }
                }
                catch( _com_error& x )  { throw_com_error( x ); }
            }
            catch( const exception& x ) {
                if( error_count == 0 )  set_error( x );
                else _log->error( x.what() );
                try  {
                    detach_order_after_error(error_order_state_transition());
                }
                catch( exception& x ) { _log->error( "detach_order_after_error: " + string(x.what()) ); }

                if( error_count == 0  &&  _state < s_ending ) {
                    _end = task_end_normal;
                    loop = true;
                }
                else
                if( error_count <= 1 ) {
                    loop = false;
                    try {
                        finish(error_order_state_transition());
                    }
                    catch( exception& x ) { _log->error( "finish(): " + string( x.what() ) ); }
                    try  {
                        if( _job )  _job->stop_after_task_error( x.what() );
                    }
                    catch( exception& x ) { _log->error( "Job->stop_after_task_error(): " + string( x.what() ) ); }
                    set_state_direct( s_closed );
                }
                else
                    throw x;    // Fehlerschleife, Scheduler beenden. Sollte nicht passieren.
                if (const char* p = strstr(x.what(), "LicenseKeyParameterIsMissingException")) {  // JS-1483
                    _job->set_state_text(string(p) + (_process_class? " (" + _process_class->obj_name() + ")" : ""));
                }
                error_count++;
                something_done = true;
            }
        }  // for

        if( _operation && !had_operation && _state != s_running_process) {
            _last_operation_time = now;
            if (!_timeout.is_eternal())
                _call_register.call_at<Task_timeout_call>(_last_operation_time + _timeout);
        }

        if( !something_done )    // Obwohl _next_time erreicht, ist nichts getan?
        {
            // Das kann bei s_running_waiting_for_order passieren, wenn zunächst ein Auftrag da ist (=> _next_time = 0),
            // der dann aber von einer anderen Task genommen wird. Dann ist der Auftrag weg und something_done == false.

            set_state_direct( state() );  // _next_time neu setzen

            //if( _next_time <= now )
            //{
            //    Z_LOG2( "scheduler", obj_name() << ".do_something()  Nothing done. state=" << state_name() << ", _next_time=" << _next_time.as_string(_job->time_zone_name()) << ", delayed\n" );
            //    //set_next_time(Time::now() + Duration(0.1));
            //}
            //else
            {
                Z_LOG2( "scheduler.nothing_done", obj_name() << ".do_something()  Nothing done. state=" << state_name() << "\n" );
            }
        }
    }  // if( _operation &&  !_operation->async_finished() )


    return something_done;
}

//---------------------------------------------------------------------------------------Task::load

bool Task::load()
{
    if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' ) {
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

    Time now = Time::now();
    _running_since = now;
    _last_operation_time = now;
    _timeout = _job->_task_timeout;

    if( _job->is_machine_resumable() )
        _spooler->begin_dont_suspend_machine();

    if (!_let_run)
        _call_register.call_at<Task_end_with_period_call>(_job->_period.end());   // Am Ende der Run_time wecken, damit die Task beendet werden kann

    return do_load();
}

//---------------------------------------------------------------------------------Task::begin_start

Async_operation* Task::begin__start()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    return _module_instance->begin__start();
}

//-----------------------------------------------------------------------------------Task::step__end

bool Task::step__end()
{
    bool continue_task;

    try {
        bool    result;
        Variant spooler_process_result = do_step__end();

        if( spooler_process_result.vt == VT_ERROR  &&  V_ERROR( &spooler_process_result ) == DISP_E_UNKNOWNNAME ) {
            result = true;
            continue_task = false;
        } else {
            continue_task = result = check_result( spooler_process_result );
        }

        if( _job->is_order_controlled() )
            continue_task = true;           // Auftragsgesteuerte Task immer fortsetzen ( _order kann wieder null sein wegen set_state_direct(), §1495 )

        count_step();

        if (_order) {
            postprocess_order(_delay_until_locks_available ? Order_state_transition::keep : Order_state_transition::of_bool(result));
        }

        if( _next_spooler_process.not_zero() )
            continue_task = true;
    }
    catch( const exception& x ) {
        set_error(x);
        if( _order )  detach_order_after_error(error_order_state_transition());
        continue_task = false;
    }

    return continue_task;
}

//---------------------------------------------------------------------------------Task::count_step

void Task::count_step()
{
    if( has_step_count()  ||  _step_count == 0 ) {      // Bei kind_process nur einen Schritt zählen
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

    try {
        result = do_step__end().as_string();

        xml::Document_ptr dom_document = xml::Document_ptr::from_xml_string(result);
        xml::Element_ptr  process_result_element = dom_document.select_element_strict( "/" "process.result" );

        _module_instance->set_spooler_process_result( process_result_element.bool_getAttribute( "spooler_process_result" ) );
        int exit_code = process_result_element.int_getAttribute( "exit_code", 0 );   // JS-563
        if (exit_code) {  // Only exit_code != 0 overrides a previously set exit_code (for example, an error log line)
            _exit_code = exit_code;
            _module_instance->set_exit_code( _exit_code );
        }
        _module_instance->set_termination_signal( process_result_element.int_getAttribute( "signal"   , 0 ) );

        set_state_texts_from_stdout( process_result_element.getAttribute( "state_text" ) );

        if( _order ) {
            if( xml::Element_ptr order_params_element = process_result_element.select_node( "order.params" ) ) {
                ptr<Com_variable_set> p = new Com_variable_set( order_params_element );
                _order->params()->merge( p );
            }
            _order_state_transition = _module_instance->order_state_transition();
            if (_module_instance->_module->_kind != Module::kind_process) {
                postprocess_order(_order_state_transition);
            }
        }

        _spooler->_task_subsystem->count_step();
        _job->count_step();
        _step_count++;
    }
    catch( const exception& x )  {
        set_error(x);
        if( _order ) {
            _order_state_transition = _job->stops_on_task_error()? Order_state_transition::keep : Order_state_transition::standard_error;
            if (_module_instance->_module->_kind != Module::kind_process) {
                detach_order_after_error(error_order_state_transition());
            }
        }
    }

    close_operation();
    return result;
}

//-----------------------------------------------------------------------------Task::operation__end

bool Task::operation__end()
{
    bool result = false;

    try {
        switch( _state ) {
            case s_starting:
                result = do_begin__end();
                break;
            case s_opening:
                result = do_call__end();
                break;
            case s_ending:
                do_end__end();
                break;
            case s_ending_waiting_for_subprocesses:
                if( Remote_module_instance_proxy* m = dynamic_cast< Remote_module_instance_proxy* >( +_module_instance ) )
                    m->_remote_instance->call__end();
                else
                    assert(0), throw_xc( "NO_REMOTE_INSTANCE" );
                break;
            case s_on_error:
                result = do_call__end();
                break;
            case s_on_success:
                result = do_call__end();
                break;
            case s_exit:
                result = do_call__end();
                break;
            case s_release:
                do_release__end();
                break;
            case s_ended:
                do_close__end();
                break;
            default:
                assert(0), throw_xc( "Task::operation__end" );
        }
    }
    catch( const exception& x ) { set_error(x); result = false; }

    close_operation();
    return result;
}
//----------------------------------------------------------------------------Task::close_operation

void Task::close_operation() {
    if (_operation) {
        _operation->async_close();
        _operation = NULL;
    }
}

//---------------------------------------------------------------------Task::fetch_and_occupy_order

Order* Task::fetch_and_occupy_order( const Time& now, const string& cause )
// Wird auch von Job gerufen, wenn ein neuer Auftrag zum Start einer neuen Task führt
{
    assert( !_operation );
    assert( _state == s_none || _state == s_running || _state == s_running_waiting_for_order );

    if( !_order
     && !_end  // Kann beim Aufruf aus Job::do_something() passieren
     && _spooler->state() != Spooler::s_paused) {
        if( Order* order = _job->fetch_and_occupy_order(this, now, cause, _process_class) ) {
            if( order->is_file_order() )
                _trigger_files = order->file_path();
            _order = order;
            _order_for_task_end = order;                // Damit bei Task-Ende im Fehlerfall noch der Auftrag gezeigt wird, s. dom_element()
            _log->set_order_log( _order->_log );

            if (!_job->has_own_process_class() && !_has_remote_scheduler) {
                Process_class* process_class;
                try {
                    process_class = _spooler->process_class_subsystem()->process_class(order->job_chain()->default_process_class_path());
                } catch (exception& x) {
                    order->log()->error(x.what());
                    postprocess_order(Order_state_transition::standard_error);
                    assert(!_order);
                    return NULL;
                }
                if (!_process_class) {
                    // First order
                    assert(_state == s_none);
                    assert(!_module_instance);
                    _process_class = process_class;
                    add_requisite(Requisite_path(_process_class->subsystem(), _process_class->path()));
                }
                if (_process_class != process_class) z::throw_xc(Z_FUNCTION, "Process classes differ", _process_class->obj_name(), process_class->obj_name());
            }
            _log->info(message_string("SCHEDULER-842", _order->obj_name(), _order->state(), _spooler->http_url(), _process_class ? ", Order's " + _process_class->obj_name() : ""));
        } else {
            bool order_announced = _job->request_order(now, cause);
            if (order_announced && !_job->has_own_process_class() && _job->max_tasks_reached()) {
                // An order has been announced which probably has a different process class (because fetch_and_occupy_order did not fetch it),
                // and a further task cannot be started. So we terminate in favour of a new task, running in that different process class.
                cmd_nice_end(_job);
            }
        }
    }

    if (_order) {
        _order->assert_task( Z_FUNCTION );
    }

    return _order;
}

//--------------------------------------------------------------------------Task::postprocess_order

void Task::postprocess_order(const Order_state_transition& state_transition, bool due_to_exception)
{
    if( _order ) {
        _log->info( message_string( "SCHEDULER-843", _order->obj_name(), _order->state(), _spooler->http_url() ) );

        if (!_order->job_chain_path().empty())
            report_event(CppEventFactoryJ::newOrderStepEndedEvent(_order->job_chain_path(), _order->string_id(), state_transition.internal_value()));
        _order->postprocessing(state_transition, _error);

        if( due_to_exception && !_order->setback_called() )
            _log->warn( message_string( "SCHEDULER-846", _order->state().as_string() ) );
        detach_order();
        //_call_register.call<Task_check_for_order_call>();
    }
}

//-------------------------------------------------------------------Task::detach_order_after_error

void Task::detach_order_after_error(const Order_state_transition& transition)
{
    if( _order ) {
        if( _job->stops_on_task_error() ) {
            // Job wird stoppt, deshalb bleibt der Auftrag in der Warteschlange.
            _log->warn( message_string( "SCHEDULER-845" ) );
            _log->info( message_string( "SCHEDULER-843", _order->obj_name(), _order->state(), _spooler->http_url() ) );
            if (!_order->job_chain_path().empty())
                report_event(CppEventFactoryJ::newOrderStepEndedEvent(_order->job_chain_path(), _order->string_id(), Order_state_transition::keep.internal_value()));
            _order->processing_error();
            detach_order();
        } else {
            // Job stoppt nicht, deshalb wechselt der Auftrag in den Fehlerzustand
            postprocess_order(transition, true);
            // _order ist NULL
        }
    }
}

//---------------------------------------------------------------Task::error_order_state_transition

Order_state_transition Task::error_order_state_transition() const {
    if (_module_instance) {
        Order_state_transition t = _module_instance->order_state_transition();
        return t.is_success()? Order_state_transition::standard_error : t;
    } else
        return Order_state_transition::standard_error;
}

//-------------------------------------------------------------------------------Task::detach_order

void Task::detach_order()
{
    if( _order->is_file_order() )
        _trigger_files = "";
    _log->set_order_log( NULL );
    _order = NULL;
}

//-------------------------------------------------------------------------------------Task::finish

void Task::finish(const Order_state_transition& error_order_state_transition)
{
    _job->init_start_when_directory_changed( this );
    process_on_exit_commands();
    if( _order ) {   // Auftrag nicht verarbeitet? spooler_process() nicht ausgeführt, z.B. weil spooler_init() oder spooler_open() false geliefert haben.
        if( !has_error()  &&  _spooler->state() != Spooler::s_stopping )  set_error( Xc( "SCHEDULER-226" ) );
        detach_order_after_error(error_order_state_transition);  // Nur rufen, wenn _move_order_to_error_state, oder der Job stoppt oder verzögert wird! (has_error() == true) Sonst wird der Job wieder und wieder gestartet.
    }

    if( has_error()  &&  _job->repeat().is_zero()  &&  _job->_delay_after_error.empty() ) {
        _job->stop_after_task_error( _error.what() );
    }
    else
    if( _job->_temporary  &&  _job->repeat().is_zero() ) {
        _job->stop( false );   // _temporary && s_stopped ==> spooler_thread.cxx entfernt den Job
    }

    // Bei mehreren aufeinanderfolgenden Fehlern Wiederholung verzögern?

    if( has_error()  &&  _job->_delay_after_error.size() > 0 ) {
        InterlockedIncrement( &_job->_error_steps );
        _is_first_job_delay_after_error = _job->_error_steps == 1;
        if( !_job->repeat() ) {  // spooler_task.repeat hat Vorrang
            Duration delay = _job->_delay_after_error.empty()? Duration::eternal : Duration(0);

            FOR_EACH( Standard_job::Delay_after_error, _job->_delay_after_error, it )
                if( _job->_error_steps >= it->first )  delay = it->second;

            if(delay.is_eternal()) {
                _is_last_job_delay_after_error = true;
                _job->stop( false );
            } else {
                _job->_delay_until = Time::now() + delay;

                if (_cause == cause_queue) {
                    try {
                        ptr<Task> task = _job->start_task( +_params, _name );   // Keine Startzeit: Nach Periode und _delay_until richten
                        task->_delayed_after_error_task_id = id();
                    }
                    catch( exception& x ) {
                        _log->error( x.what() );
                        _job->_delay_until = Time(0);
                        _job->stop_after_task_error( x.what() );
                    }
                }
            }
        }
    } else {
       _job->_error_steps = 0;
    }

    if( _web_service )
        _web_service->forward_task( *this );

    _job->on_task_finished(this, _end);

    {
        // eMail versenden
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
    if( _order )
        _order->set_state_text( state_text );
}

//-------------------------------------------------------------------Task::process_on_exit_commands

void Task::process_on_exit_commands()
{
    if( _job->_commands_document ) {
        xml::Element_ptr commands_element = xml::Element_ptr();

        Standard_job::Exit_code_commands_map::iterator it = _job->_exit_code_commands_map.find( _exit_code );
        if( it != _job->_exit_code_commands_map.end() ) {
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
        if(_exit_code != 0) // JS-550
            commands_element = _job->_commands_document.select_node( "/*/commands [ @on_exit_code='error' ]" );

        if( commands_element ) {
            _log->debug( message_string( "SCHEDULER-328", commands_element.getAttribute( "on_exit_code" ) ) );

            Command_processor cp ( _spooler, Security::seclev_all );
            cp.set_log( _log );
            cp.set_variable_set( "task", _params );

            if (_order_for_task_end)
                cp.set_variable_set("order", _order_for_task_end->params_or_null());

            DOM_FOR_EACH_ELEMENT( commands_element, c ) {
                try {
                    cp.execute_command( c );
                }
                catch( exception& x ) {
                    _log->error( x.what() );
                    _log->error( message_string( "SCHEDULER-327", c.xml_string() ) );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------Task::trigger_event

void Task::trigger_event( Scheduler_event* scheduler_event )
{
    try {
        _log->set_mail_default( "from_name", _spooler->name() + " " + obj_name() );    // "Scheduler host:port -id=xxx Task jobname:id"

        scheduler_event->set_message( _log->highest_msg() );
        if( _error )  scheduler_event->set_error( _error );

        bool is_error = has_error();

        S body;
        body << Time::now().as_string(_job->time_zone_name()) << "\n\nJob " << _job->name() << "  " << _job->title() << "\n";
        body << "Task-Id " << as_string(id()) << ", " << as_string(_step_count) << " steps\n";
        if( _order )  body << _order->obj_name() << "\n";
        body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";

        if( !is_error ) {
            S subject;
            subject << obj_name();
            if( _log->highest_level() == log_error ) {
                subject << " ended with error";
                body << _log->highest_msg() << "\n\n";
            }
            else
            if( _log->highest_level() == log_warn ) {
                subject << " ended with warning";
                body << _log->highest_msg() << "\n\n";
            } else {
                subject << " succeeded";
            }
            _log->set_mail_default( "subject", subject, false );
        } else {
            string errmsg = _error? _error->what() : _log->highest_msg();
            _log->set_mail_default( "subject", string("ERROR ") + errmsg, true );
            body << errmsg << "\n\n";
        }
        _log->set_mail_default( "body", body + "This message contains the job protocol." );   //, is_error );

        bool mail_it = _log->_mail_it;
        if( !mail_it ) {
            bool mail_due_to_error_or_warning = false;

//          JS-425:  if( _log->_mail_on_error | _log->_mail_on_warning  &&  ( has_error() || _log->highest_level() >= log_error ) )  mail_due_to_error_or_warning = true;
            if( _log->_mail_on_error &&  ( has_error() || _log->highest_level() >= log_error ) )  mail_due_to_error_or_warning = true;
            else
//          JS-425: if( _log->_mail_on_warning  &&  _log->has_line_for_level( log_warn ) )  mail_due_to_error_or_warning = true;
            if( _log->_mail_on_warning &&  ( _log->highest_level() == log_warn ) )  mail_due_to_error_or_warning = true;

            if( _job->_delay_after_error.size() > 0 ) {
                switch( _log->_mail_on_delay_after_error ) {
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

        if( !mail_it  &&  _log->_mail_on_success  &&  _step_count >= 0)
            mail_it = true;
        if( !mail_it  &&  _log->_mail_on_process  &&  _step_count >= _log->_mail_on_process )
            mail_it = true;

        if( _log->_mail_it )
            mail_it = true;

        if( mail_it )
            _log->send( scheduler_event );
    }
    catch( const exception& x  ) { _log->warn( x.what() ); }
    catch( const _com_error& x ) { _log->warn( bstr_as_string(x.Description()) ); }
}

//----------------------------------------------------------------------Task::wait_until_terminated

bool Task::wait_until_terminated( double )
{
    z::throw_xc( "SCHEDULER-125" );     // Deadlock
    return false;
}

//----------------------------------------------------------------------------Task::set_web_service

void Task::set_web_service( const string& name )
{
    if( _is_in_database )  z::throw_xc( "SCHEDULER-243", "web_service" );
    set_web_service( name == ""? NULL  : _spooler->_web_services->web_service_by_name( name ) );
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

//----------------------------------------------------------------------------Task::do_close__start

Async_operation* Task::do_close__start()
{
    if( !_module_instance ) {
        _sync_operation = Z_NEW(Sync_operation);
        return _sync_operation;
    } else {
        _module_instance->detach_task();
        return _module_instance->close__start();
    }
}

//------------------------------------------------------------------------------Task::do_close__end

void Task::do_close__end()
{
    if( _module_instance ) {
        _file_logger->flush();      // JS-986 JS-1039
        _module_instance->close__end();
        _file_logger->flush();

        // Siehe auch remote_process_step__end()
        int exit_code = _module_instance->kind() == Module::kind_process? _module_instance->exit_code() : _exit_code;
        if( exit_code ) {
            z::Xc x ( "SCHEDULER-280", exit_code, printf_string( "%X", exit_code ) );
            if (_module_instance->_module->kind() == Module::kind_process)
                set_error( x );
            else
                _log->warn( x.what() );

            _exit_code = exit_code;  // Nach set_error(), weil set_error() _exit_code auf 1 setzt
        }

        if( int termination_signal = _module_instance->termination_signal() ) {
            z::Xc x ( "SCHEDULER-279", termination_signal, signal_name_from_code( termination_signal ) + " " + signal_title_from_code( termination_signal ) );

            if (!_job->_ignore_every_signal
             && _job->_ignore_signals_set.find(termination_signal) == _job->_ignore_signals_set.end())
            {
                set_error( x );
            } else {
                _log->warn( x.what() );
                _log->debug( message_string( "SCHEDULER-973" ) ); //, signal_name_from_code( termination_signal ) ) );

                if( _is_connection_reset_error )
                {
                    _error = _non_connection_reset_error;    // Vorherigen Fehler (vor dem Verbindungsverlust) wiederherstellen
                    _non_connection_reset_error = NULL;
                    _is_connection_reset_error = false;
                }
            }

            _exit_code = -termination_signal;
        }

        _file_logger->finish();
        _file_logger->close();

        //_module_instance = NULL;    // Nach set_error(), weil set_error() _exit_code auf 1 setzt
    }
}

//------------------------------------------------------------------------------------Task::do_kill

bool Task::do_kill()
{
    return _module_instance? _module_instance->kill(Z_SIGKILL) :
           _operation      ? _operation->async_kill()
                           : false;
}

//------------------------------------------------------------------------------------Task::do_load

bool Task::do_load()
{
    string remote_scheduler = read_remote_scheduler_parameter();
    _has_remote_scheduler = !remote_scheduler.empty();

    if (!_process_class) {
        // fetch_and_occupy_order has not been called. This task has no order.
        _process_class = _job->default_process_class();
    }
    if (ptr<Module_instance> module_instance = _job->create_module_instance(_process_class, remote_scheduler, this)) {
        module_instance->set_job_name( _job->name() );      // Nur zum Debuggen (für shell-Kommando ps)

        _module_instance = module_instance;
        _module_instance->attach_task( this, _log );

        if( !module_instance->loaded() ) {
            module_instance->init();
            module_instance->add_objs(this);
            bool ok =  module_instance->load();
            if( !ok ) return false;
            module_instance->start();
        }

        return true;
    } else
        return false;
}

//------------------------------------------------------------Task::read_remote_scheduler_parameter

string Task::read_remote_scheduler_parameter() {
    string remote_scheduler = _order? _order->remote_scheduler() : "";
    if (!remote_scheduler.empty()) {
        if (_spooler->is_cluster())  z::throw_xc("SCHEDULER-483");
        if (_job->module()->kind() != Module::kind_process) {
            log()->warn(Message_string("SCHEDULER-484"));   // Ein API-Prozess kann mehrere Order nacheinander ausführen. Die Tasks könnten dann für jeden Order wechseln.
            remote_scheduler = "";
        }
    }
    return remote_scheduler;
}

//------------------------------------------------------------------------------Task::do_begin__end

bool Task::do_begin__end()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    return _module_instance->begin__end();
}

//------------------------------------------------------------------------------Task::do_end__start

Async_operation* Task::do_end__start()
{
    if (!_module_instance ) {
        _sync_operation = Z_NEW(Sync_operation);
        return _sync_operation;
    } else {
        return _module_instance->end__start( !has_error() );        // Parameter wird nicht benutzt
    }
}

//--------------------------------------------------------------------------------Task::do_end__end

void Task::do_end__end()
{
    if( _module_instance )
        _module_instance->end__end();
}

//-----------------------------------------------------------------------------Task::do_step__start

Async_operation* Task::do_step__start()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    if (_order && _step_count >= 1) {
        _order->log()->info(message_726());
    }
    Async_operation* result = _module_instance->step__start();
    if (_order) report_event_code(orderStepStartedEvent, _order->java_sister());
    return result;
}

string Task::message_726() const {
    string a = remote_scheduler_address();
    string insertion = 
        a.empty() ? 
            "this JobScheduler '" + _spooler->http_url() + "'" + 
                (_module_instance->_module->_kind == Module::kind_java_in_process ? ", in_process" : "")
        : 
            "remote scheduler " + a;
    return Message_string("SCHEDULER-726", insertion);
}

//-------------------------------------------------------------------------------Task::do_step__end

Variant Task::do_step__end()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    _file_logger->flush();      // JS-986 JS-1039
    return _module_instance->step__end();
}

//------------------------------------------------------------------------------Task::do_call__start

Async_operation* Task::do_call__start( const string& method )
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    return _module_instance->call__start( method );
}

//-------------------------------------------------------------------------------Task::do_call__end

bool Task::do_call__end()
{
    if( !_module_instance )  z::throw_xc( "SCHEDULER-199" );
    return check_result( _module_instance->call__end() );
}

//--------------------------------------------------------------------------Task::do_release__start

Async_operation* Task::do_release__start()
{
    if (!_module_instance ) {
        _sync_operation = Z_NEW(Sync_operation);
        return _sync_operation;
    } else {
        return _module_instance->release__start();
    }
}

//----------------------------------------------------------------------------Task::do_release__end

void Task::do_release__end()
{
    if( !_module_instance )  return;  //z::throw_xc( "SCHEDULER-199" );
    _module_instance->release__end();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
