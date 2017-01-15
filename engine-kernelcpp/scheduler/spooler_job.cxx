// $Id: spooler_job.cxx 15019 2011-08-24 16:47:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "spooler_job_java.h"
#include "Timed_call.h"
#include "../zschimmer/z_signals.h"
#include "../zschimmer/z_sql.h"
#include "../kram/sleep.h"
#include "../javaproxy/com__sos__scheduler__engine__data__job__JobPersistentState.h"

typedef ::javaproxy::com::sos::scheduler::engine::data::job::JobPersistentState JobPersistentStateJ;

#ifndef Z_WINDOWS
#   include <signal.h>
#   include <sys/signal.h>
#   include <sys/wait.h>
#endif


namespace sos {
namespace scheduler {

using namespace zschimmer::sql;
using job_chain::Job_node;

//--------------------------------------------------------------------------------------------const

const Duration max_task_time_out           = Duration(365*24*3600);
const Duration directory_watcher_intervall = Duration(10.0);          // Nur für Unix (Windows gibt ein asynchrones Signal)
const bool   Job::force_start_default      = true;

//--------------------------------------------------------------------------------------------Calls

namespace job {
    struct State_cmd_call : object_call<Standard_job, State_cmd_call> {
        Job::State_cmd const _cmd;
        State_cmd_call(Standard_job* job, Job::State_cmd cmd) : object_call<Standard_job, State_cmd_call>(job), _cmd(cmd) {}
    };

    DEFINE_SIMPLE_CALL(Standard_job, Period_begin_call)
    DEFINE_SIMPLE_CALL(Standard_job, Period_end_call)
    DEFINE_SIMPLE_CALL(Standard_job, Start_queued_task_call)
    DEFINE_SIMPLE_CALL(Standard_job, Calculated_next_time_do_something_call)
    DEFINE_SIMPLE_CALL(Standard_job, Start_when_directory_changed_call)
    DEFINE_SIMPLE_CALL(Standard_job, Order_timed_call)
    DEFINE_SIMPLE_CALL(Standard_job, Order_possibly_available_call)
    DEFINE_SIMPLE_CALL(Standard_job, Process_available_call)
    DEFINE_SIMPLE_CALL(Standard_job, Below_min_tasks_call)
    DEFINE_SIMPLE_CALL(Standard_job, Below_max_tasks_call)
    DEFINE_SIMPLE_CALL(Standard_job, Locks_available_call)
    DEFINE_SIMPLE_CALL(Standard_job, Remove_temporary_job_call)

    Task_closed_call::Task_closed_call(Task* task) : object_call<Standard_job, Task_closed_call>(task->job()), _task(task) {}
}

using namespace job;

//-------------------------------------------------------------------------------Job_subsystem_impl

struct Job_subsystem_impl : Job_subsystem
{
                                Job_subsystem_impl          ( Scheduler* );

    // Subsystem:
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();


    // Job_subsystem

    ptr<Job_folder>             new_job_folder              ( Folder* folder )                      { return Z_NEW( Job_folder( folder ) ); }
    bool                        has_any_order               ();
    bool                        is_any_task_queued          ();
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );
    Schedule*                   default_schedule            ()                                      { return _default_schedule; }
    void                        do_something                ();


    // File_based_subsystem:

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;
    string                      xml_element_name            () const                                { return "job"; }
    string                      xml_elements_name           () const                                { return "jobs"; }
    void                        assert_xml_elements_name    ( const xml::Element_ptr& ) const;
    string                      object_type_name            () const                                { return "Job"; }
    string                      filename_extension          () const                                { return ".job.xml"; }
    string                      normalized_name             ( const string& name ) const            { return lcase( name ); }
    ptr<Job>                    new_file_based              (const string& source);


private:
    xml::Element_ptr            state_job_statistic_element    ( const xml::Document_ptr&, Job::State ) const;
    xml::Element_ptr            job_statistic_element          ( const xml::Document_ptr&, const string& attribute_name, const string& attribute_value, int count ) const;
    xml::Element_ptr            waiting_for_process_job_statistic_element( const xml::Document_ptr& dom_document ) const;
    int                         count_jobs_with_state          ( Job::State ) const;
    int                         count_jobs_waiting_for_process () const;


    ptr<Schedule>              _default_schedule;
};

//---------------------------------------------------------------------------------Job_schedule_use

struct Job_schedule_use : Schedule_use
{
    Job_schedule_use( Standard_job* job )
    : 
        Schedule_use(job), 
        _job(job) 
    {
        _job->add_accompanying_dependant( this );
    }

    ~Job_schedule_use()
    {
        _job->remove_accompanying_dependant( this );
    }


    void                        on_schedule_loaded          ()                                      { _job->on_schedule_loaded(); }
    void                        on_schedule_modified        ()                                      { _job->on_schedule_modified(); }
    bool                        on_schedule_to_be_removed   ()                                      { return _job->on_schedule_to_be_removed(); }
  //void                        on_schedule_removed         ()                                      { _job->on_schedule_removed(); }
    string                      name_for_function           () const                                { return _job->name(); }

  private:
    Standard_job*              _job;
};

//-------------------------------------------------------------------------------Job_lock_requestor

struct Job_lock_requestor : lock::Requestor
{
    Job_lock_requestor( Standard_job* job )
    : 
        Requestor( job ), 
        _job(job) 
    {
        //_job->add_accompanying_dependant( this );
    }

    ~Job_lock_requestor()
    {
        //_job->remove_accompanying_dependant( this );
    }


    // Requestor:
    void                        on_locks_are_available      ()                                      { _job->on_locks_available(); }
  //void                        on_removing_lock            ( lock::Lock* l )                       { _job->on_removing_lock( l ); }

  private:
    Standard_job*              _job;
};

//------------------------------------------------------------------------------Combined_job_nodes

struct Combined_job_nodes : Object
{
                                Combined_job_nodes          ( Standard_job* );
                               ~Combined_job_nodes          ();

    void                        close                       ();
    bool                        is_empty                    () const                                { return _job_node_set.empty(); }
    Order_queue*                any_order_queue             () const;
    bool                        request_order               ( const Time& now, const string& cause );
    void                        withdraw_order_requests     ();
    Time                        next_time                   ();
    Order*                      fetch_and_occupy_order      (Task* occupying_task, const Time& now, const string& cause, const Process_class*);
    xml::Element_ptr            why_dom_element             (const xml::Document_ptr&, const Time&);
    void                        connect_with_order_queues   ();
    void                        connect_job_node            ( job_chain::Job_node* );
    void                        disconnect_job_node         ( job_chain::Job_node* );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what&, Job_chain* );
    Spooler*                    spooler                     () const                                { return _spooler; }

  private:
    Fill_zero                  _zero_;
    Standard_job*              _job;
    Spooler*                   _spooler;
    typedef stdext::hash_set< job_chain::Job_node* >  Job_node_set;
    Job_node_set               _job_node_set;
};

//--------------------------------------------------------------------------------new_job_subsystem

ptr<Job_subsystem> new_job_subsystem( Scheduler* scheduler )
{
    ptr<Job_subsystem_impl> job_subsystem = Z_NEW( Job_subsystem_impl( scheduler ) );
    return +job_subsystem;
}

//---------------------------------------------------------------------Job_subsystem::Job_subsystem

Job_subsystem::Job_subsystem( Scheduler* scheduler, Type_code t )   
: 
    file_based_subsystem<Job>( scheduler, this, t )
{
}

//-----------------------------------------------------------Job_subsystem_impl::Job_subsystem_impl

Job_subsystem_impl::Job_subsystem_impl( Scheduler* scheduler )
: 
    Job_subsystem( scheduler, type_job_subsystem )
{
    _default_schedule = _spooler->schedule_subsystem()->new_schedule();
    _default_schedule->set_xml_string( (File_based*)NULL, "<run_time/>" );
}

//------------------------------------------------------------------------Job_subsystem_impl::close
    
void Job_subsystem_impl::close()
{
    _subsystem_state = subsys_stopped;

    file_based_subsystem<Job>::close();
}

//---------------------------------------------------------Job_subsystem_impl::subsystem_initialize

bool Job_subsystem_impl::subsystem_initialize()
{
    _subsystem_state = subsys_initialized;
    
    file_based_subsystem<Job>::subsystem_initialize();

    return true;
}

//---------------------------------------------------------------Job_subsystem_impl::subsystem_load

bool Job_subsystem_impl::subsystem_load()
{
    _subsystem_state = subsys_loaded;           // Schon jetzt für Job::load()
    file_based_subsystem<Job>::subsystem_load();
    return true;
}

//-----------------------------------------------------------Job_subsystem_impl::subsystem_activate

bool Job_subsystem_impl::subsystem_activate()
{
    _subsystem_state = subsys_active;           // Schon jetzt für Job::activate()
    file_based_subsystem<Job>::subsystem_activate();

    return true;
}

//---------------------------------------------------------------Job_subsystem_impl::new_file_based

ptr<Job> Job_subsystem_impl::new_file_based(const string& source)
{
    if (source.find("<new_job") != string::npos) {
        return new_java_job(_spooler);
    } else {
        ptr<Standard_job> result = Z_NEW( Standard_job( _spooler ) );
        return +result;
    }
}

//-------------------------------------------------Job_subsystem_impl::append_calendar_dom_elements

void Job_subsystem_impl::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    FOR_EACH_JOB( job )
    {
        if( options->_count >= options->_limit )  break;

        job->append_calendar_dom_elements( element, options );
    }
}

//-----------------------------------------------------------------Job_subsystem_impl::do_something

void Job_subsystem_impl::do_something() 
{
    FOR_EACH_JOB(job) {
        if (Standard_job* j = dynamic_cast<Standard_job*>(job))
            j->try_start_tasks();
    }
}

//-----------------------------------------------------------Job_subsystem_impl::is_any_task_queued

bool Job_subsystem_impl::is_any_task_queued()
{
    FOR_EACH_JOB( job )
    {
        if( job->queue_filled() )  return true;
    }

    return false;
}

//-----------------------------------------------------Job_subsystem_impl::assert_xml_elements_name

void Job_subsystem_impl::assert_xml_elements_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( "add_jobs" ) )  File_based_subsystem::assert_xml_elements_name( e );
}

//------------------------------------------------------------------Job_subsystem_impl::dom_element

xml::Element_ptr Job_subsystem_impl::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what ) const
{
    xml::Element_ptr result = file_based_subsystem<Job>::dom_element( dom_document, show_what );
    xml::Element_ptr job_subsystem_element = dom_document.createElement( "job_subsystem" );
    
    if( show_what.is_set( show_statistics ) ) {
        xml::Element_ptr statistics_element = job_subsystem_element.append_new_element( "job_subsystem.statistics" );
        xml::Element_ptr job_statistics_element = statistics_element.append_new_element( "job.statistics" );

        job_statistics_element.appendChild( state_job_statistic_element( dom_document, Standard_job::s_pending ) );
        job_statistics_element.appendChild( state_job_statistic_element( dom_document, Standard_job::s_running ) );
        job_statistics_element.appendChild( state_job_statistic_element( dom_document, Standard_job::s_stopped ) );
        job_statistics_element.appendChild( waiting_for_process_job_statistic_element( dom_document ) );
    }

    result.appendChild( job_subsystem_element );
    return result;
}

//------------------------------------Job_subsystem_impl::waiting_for_process_job_statistic_element

xml::Element_ptr Job_subsystem_impl::waiting_for_process_job_statistic_element( const xml::Document_ptr& dom_document ) const
{
    return job_statistic_element( dom_document, "need_process", "true", count_jobs_waiting_for_process() );
}

//--------------------------------------------------Job_subsystem_impl::state_job_statistic_element

xml::Element_ptr Job_subsystem_impl::state_job_statistic_element( const xml::Document_ptr& dom_document, Job::State state ) const
{
    return job_statistic_element( dom_document, "job_state", Job::state_name( state ), count_jobs_with_state( state ) );
}

//--------------------------------------------------Job_subsystem_impl::state_job_statistic_element

xml::Element_ptr Job_subsystem_impl::job_statistic_element( const xml::Document_ptr& dom_document, 
    const string& attribute_name, const string& attribute_value, int count ) const
{
    xml::Element_ptr result = dom_document.createElement( "job.statistic" );
    result.setAttribute( attribute_name, attribute_value );
    result.setAttribute( "count", count );
    return result;
}

//--------------------------------------------------------Job_subsystem_impl::count_jobs_with_state

int Job_subsystem_impl::count_jobs_with_state( Job::State state ) const
{
    int result = 0;

    FOR_EACH_JOB( job )
        if( job->state() == state )  result++;

    return result;
}

//--------------------------------------------------------Job_subsystem_impl::count_jobs_with_state

int Job_subsystem_impl::count_jobs_waiting_for_process() const
{
    int result = 0;

    FOR_EACH_JOB( job )
        if (job->waiting_for_process())  result++;

    return result;
}

//---------------------------------------------------------------------------Job_folder::Job_folder

Job_folder::Job_folder( Folder* folder )
:
    typed_folder<Job>( folder->spooler()->job_subsystem(), folder, Scheduler_object::type_job_folder )
{
}

//-----------------------------------------------------------Combined_job_nodes::Combined_job_nodes

Combined_job_nodes::Combined_job_nodes( Standard_job* job )
: 
    _zero_(this+1),
    _job(job),
    _spooler(job->_spooler)
{
}

//----------------------------------------------------------Combined_job_nodes::~Combined_job_nodes
    
Combined_job_nodes::~Combined_job_nodes()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//------------------------------------------------------------------------Combined_job_nodes::close

void Combined_job_nodes::close()
{
    withdraw_order_requests();

    while (!_job_node_set.empty()) {
        Job_node* job_node = *_job_node_set.begin();
        job_node->disconnect_job();     // Ruft disconnect_job_node() und der löscht den Eintrag
    }
}

//-------------------------------------------------------------Combined_job_nodes::connect_job_node

void Combined_job_nodes::connect_job_node( Job_node* job_node )
{
    _job_node_set.insert( job_node );
}

//----------------------------------------------------------Combined_job_nodes::disconnect_job_node

void Combined_job_nodes::disconnect_job_node( Job_node* job_node )
{
    //_job->log()->debug( S() << Z_FUNCTION << "  " << job_node->obj_name() );

    _job_node_set.erase( job_node );
}

//--------------------------------------------------------------Combined_job_nodes::any_order_queue

Order_queue* Combined_job_nodes::any_order_queue() const
{
    return _job_node_set.empty()? NULL
                                : (*_job_node_set.begin())->order_queue();
}

//----------------------------------------------------------------Combined_job_nodes::request_order

bool Combined_job_nodes::request_order( const Time& now, const string& cause )
{
    bool result = false;

    Z_FOR_EACH( Job_node_set, _job_node_set, it )
    {
        result |= (*it)->request_order( now, cause );
        if( result )  break;
    }

    return result;
}

//------------------------------------------------------Combined_job_nodes::withdraw_order_requests

void Combined_job_nodes::withdraw_order_requests()

{
    //Z_LOGI2( "zschimmer", obj_name() << " " << Z_FUNCTION << "\n" );

    // Jetzt prüfen wir die verteilten Aufträge.
    // Die können auch von anderen Schedulern verarbeitet werden, und sind deshalb nachrangig.

    Z_FOR_EACH( Job_node_set, _job_node_set, it )
    {
        (*it)->withdraw_order_request();
    }
}

//----------------------------------------------------Combined_job_nodes::fetch_and_occupy_order

Order* Combined_job_nodes::fetch_and_occupy_order(Task* occupying_task, const Time& now, const string& cause, const Process_class* required_process_class)
{
    Order* result = NULL;

    Z_FOR_EACH( Job_node_set, _job_node_set, it )
    {
        Job_node* job_node = *it;
        result = job_node->fetch_and_occupy_order(occupying_task, now, cause, required_process_class);
        if( result )  break;
    }

    return result;
}

//--------------------------------------------------------------Combined_job_nodes::why_dom_element

xml::Element_ptr Combined_job_nodes::why_dom_element(const xml::Document_ptr& doc, const Time& now)
{
    xml::Element_ptr result = doc.createElement("job_chain_nodes.why");
    Z_FOR_EACH(Job_node_set, _job_node_set, it) {
        Job_node* job_node = *it;
        result.appendChild(job_node->why_dom_element(doc, now));
    }
    return result;
}

//-----------------------------------------------------------------Combined_job_nodes::next_time

Time Combined_job_nodes::next_time()
{
    Time result = Time::never;

    Z_FOR_EACH( Job_node_set, _job_node_set, it )
    {
        if( result.is_zero() )  break;

        Order_queue* order_queue = (*it)->order_queue();
        result = min( result, order_queue->next_time() );
    }

    return result;
}

//---------------------------------------------------------------Combined_job_nodes::dom_element

xml::Element_ptr Combined_job_nodes::dom_element( const xml::Document_ptr& document, const Show_what& show_what, Job_chain* which_job_chain )
{
    xml::Element_ptr element = document.createElement( "order_queue" );

    int       count        = 0;
    Show_what my_show_what = show_what;

    Z_FOR_EACH( Job_node_set, _job_node_set, it )
    {
        Order_queue* order_queue = (*it)->order_queue();
        
        if( !which_job_chain  ||  order_queue->job_chain() == which_job_chain )
        {
            count += order_queue->order_count( (Read_transaction*)NULL );

            if( my_show_what.is_set( show_job_orders )  &&  my_show_what._max_orders > 0 )
            {
                xml::Element_ptr order_queue_element = order_queue->dom_element( document, my_show_what );

                // Alle <order> in unser kombiniertes <order_queue> übernehmen:

                DOM_FOR_EACH_ELEMENT( order_queue_element, e )
                {
                    if( my_show_what._max_orders > 0 )  --my_show_what._max_orders;
                    element.importAndAppendChild( e );
                }
            }
        }
    }

    element.setAttribute( "length", count );

    return element;
}

//--------------------------------------------------------------------------------Standard_job::Standard_job

Job::Job( Scheduler* scheduler)
: 
    file_based<Job,Job_folder,Job_subsystem>( scheduler->job_subsystem(), this, Scheduler_object::type_job ),
    javabridge::has_proxy<Job>(scheduler),
    _zero_(this+1),
    _typed_java_sister(java_sister())
{}

//--------------------------------------------------------------------------------Standard_job::Standard_job

Standard_job::Standard_job( Scheduler* scheduler, const string& name, const ptr<Module>& module )
: 
    Job(scheduler),
    _zero_(this+1),
    _call_register(this),
    _task_queue( Z_NEW( Task_queue( this ) ) ),
    _history(this),
    _stop_on_error(true),
    _db_next_start_time( Time::never ),
    _enabled(true),      // JS-551
    _stderr_log_level(log_info)
{
    if( name != "" )  set_name( name );

    _log = Z_NEW( Prefix_log( this ) );
    set_log();

    _module = module ? module : Z_NEW(Module(_spooler, this, _spooler->include_path(), (Has_log*)NULL, false));
    _module->set_log( _log );
    _module->set_injectorJ(_spooler->java_subsystem()->injectorJ());

    _com_job  = new Com_job( this );

    _schedule_use = Z_NEW( Job_schedule_use( this ) );

    _default_params = new Com_variable_set;
    _task_timeout   = Duration::eternal;
    _idle_timeout   = Duration(5);
    _max_tasks      = 1;

    _combined_job_nodes = Z_NEW( Combined_job_nodes( this ) );
}

//-------------------------------------------------------------------------------Standard_job::~Standard_job

Standard_job::~Standard_job()
{
    try
    {
        close();
    }
    catch( exception& x ) { _log->warn( x.what() ); }     

    _schedule_use = NULL;
}

//------------------------------------------------------------------------------Standard_job::close

void Standard_job::close()
{
    _combined_job_nodes->close();


    try
    {
        clear_when_directory_changed();
    }
    catch( const exception& x ) { _log->warn( S() << "clear_when_directory_changed() ==> " << x.what() ); }


    Z_FOR_EACH( Task_set, _running_tasks, t )
    {
        Task* task = *t;
        try
        {
            task->try_kill();
        }
        catch( const exception& x ) { Z_LOG2( "scheduler", *task << ".kill() => " << x.what() << "\n" ); }
    }

    while (!_running_tasks.empty()) {
        Task_set::iterator t = _running_tasks.begin();
        Task* task = *t;
        task->job_close();
        _running_tasks.erase(t);
    }

    Z_FOR_EACH( Task_list, *_task_queue, t )  (*t)->job_close();
    _task_queue->clear();


    Z_FOR_EACH( Module_instance_vector, _module_instances, m )
    {
        if( *m ) 
        {
            (*m)->close();
            *m = NULL;
        }
    }

    _log->finish_log();
    _log->close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_job  )  _com_job->close(), _com_job  = NULL;

    if( _schedule_use )  _schedule_use->close(), _schedule_use = NULL;
    _lock_requestor = NULL;
    
    //remove_requisite( spooler()->schedule_subsystem(), _schedule_path );

    File_based::close();
}

//----------------------------------------------------------------------Standard_job::on_initialize

/*!
 * \change 2.1.2 - JS-559: new licence type scheduler-agent
 */
bool Standard_job::on_initialize()
{
    bool result = true;

    if( _state < s_initialized )
    {
        Z_LOGI2( "scheduler", obj_name() << ".initialize()\n" );

        _spooler->settings()->require_role(Settings::role_scheduler, obj_name());
        if( !_module )  z::throw_xc( "SCHEDULER-440", obj_name() );

        add_requisite( Requisite_path( spooler()->process_class_subsystem(), _default_process_class_path ) );

        //_module->set_folder_path( folder_path() );
        _module->init();
        if (_module->kind() == Module::kind_internal) {
            if (!Process_class_subsystem::is_empty_default_path(_default_process_class_path))
                if( Process_class* process_class = default_process_class_or_null() )
                    if (process_class->is_remote_host())  
                        z::throw_xc("SCHEDULER-REMOTE-INTERNAL?");
        } 

        if( !_module->set() )  z::throw_xc( "SCHEDULER-146" );
        if( _module->kind() == Module::kind_none )  z::throw_xc( "SCHEDULER-440", obj_name() );

        if( _max_tasks < _min_tasks )  z::throw_xc( "SCHEDULER-322", _min_tasks, _max_tasks );

        prepare_on_exit_commands();
        
        if( !_schedule_use->is_defined()  &&  _schedule_use->schedule_path() == "" )            // Standard_job ohne <run_time>?
        {
            _schedule_use->set_dom( (File_based*)NULL, xml::Document_ptr::from_xml_string("<run_time/>").documentElement() );     // Dann ist das der Default
        }

        set_next_start_time( Time::never );

        if( _lock_requestor )  
        {
            _lock_requestor->initialize();
        }

        _state = s_initialized;

        result = true;
    }

    return result;
}

//----------------------------------------------------------------------------Standard_job::on_load

bool Standard_job::on_load() // Transaction* ta )
{
    _spooler->settings()->require_role(Settings::role_scheduler, obj_name());
    // Nach Fehler nicht wiederholbar.

    if (_module->kind() == Module::kind_process) {
        Z_LOG2("scheduler", "Suppressing min_tasks for " << obj_name() << "\n");
        _min_tasks = 0;
    }

    bool result = false;

    if( _state < s_loaded )
    {
        Z_LOGI2( "scheduler", obj_name() << ".load()\n" );

        set_log();  // Wir haben einen eigenen Präfix mit extra Blank "Job  xxx", damit's in einer Spalte mit "Task xxx" ist.

        if( !_spooler->log_directory().empty()  &&  _spooler->log_directory()[0] != '*' )
        {
            _log->set_append( _log_append );
            _log->set_filename( _spooler->log_directory() + "/job." + path().to_filename() + ".log" );      // Jobprotokoll
        }

        _log->open();

        if( _lock_requestor )  _lock_requestor->load();       // Verbindet mit bekannten Sperren


        try {
            if (_spooler->settings()->_use_java_persistence) {
                if (JobPersistentStateJ persistentState = typed_java_sister().tryFetchPersistentState()) {
                    _is_permanently_stopped = persistentState.isPermanentlyStopped();
                    //_db_next_start_time = persistentState.nextStartTimeDouble();
                }
                typed_java_sister().loadPersistentTasks();
                _history.open(NULL);
            } else {
                for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try {
                    database_record_load( &ta );
                    _history.open( &ta );
                    load_tasks_from_db( &ta );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_jobs_table.name(), x ), Z_FUNCTION ); }
            }
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-330", obj_name(), x ) );
            throw;
        }

        set_state( s_loaded );
        result = true;
    }

    return result;
}

//------------------------------------------------------------------------Standard_job::on_activate

bool Standard_job::on_activate()
{
    bool result = false;

    if( _state < s_pending )
    {
        try
        {
            bool ok = _schedule_use->try_load();  
            if (ok) {  // Nach _schedule_use->set_default_schedule() immer true
                ok = _module->_monitors->try_load();
            }
            if( !ok )    
            {
                set_file_based_state( s_incomplete );
            }
            else
            {
                set_state( _is_permanently_stopped? s_stopped : s_pending );
                
                _delay_until = Time(0);
                reset_scheduling();
                init_start_when_directory_changed();
                check_min_tasks( Z_FUNCTION );

                if( _tasks_count == 0 )
                {
                    for( Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
                    {
                        if( (*it)->filename_pattern() != "" )  
                        {
                            _start_once_for_directory = true;
                            break;
                        }
                    }
                }

                set_next_start_time( Time::now() );

                //TODO
                // Man könnte hier warnen, wenn die Schedule keine Periode hat und in der Warteschlange eine Task ohne Startzeit ist.
                // Die würde nie gestartet werden.

                result = true;
            }
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-330", obj_name(), x ) );
            throw;
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------Job::on_replace_now

//Job* Job::on_replace_now() {
//    Job* job = static_cast<Job*>(My_file_based::on_replace_now());
////    _enabled ? signal( state_cmd_name(Job::sc_enable) ) : signal( state_cmd_name(Job::sc_disable) );
//    _enabled ? _state_cmd = Job::sc_enable : _state_cmd = Job::sc_disable ;    
//    return job;
//}

//----------------------------------------------------------------------------Standard_job::set_dom

void Standard_job::set_dom( const xml::Element_ptr& element )
{
    assert_is_not_initialized();

    assert( element );
    if( !element )  return;
    if( !element.nodeName_is( "job" ) )  z::throw_xc( "SCHEDULER-409", "job", element.nodeName() );

    _module->set_folder_path( folder_path() );

    {
        bool order;

        set_name    ( element.     getAttribute( "name"         , name()      ) );
        
        if( element.hasAttribute( "visible" ) )
            _visible = element.getAttribute( "visible" ) == "never"? visible_never :
                       element.bool_getAttribute( "visible" )      ? visible_yes 
                                                                   : visible_no;
    
        _temporary  = element.bool_getAttribute( "temporary"    , _temporary  );
        _module->set_priority( element.getAttribute( "priority"     , _module->_priority   ) );
        _title      = element.     getAttribute( "title"        , _title      );
        _log_append = element.bool_getAttribute( "log_append"   , _log_append );
        order       = element.bool_getAttribute( "order"        );
        _default_process_class_path = Absolute_path(folder_path(), element.getAttribute("process_class", _default_process_class_path));
        _module->_java_options += " " + subst_env( 
                      element.     getAttribute( "java_options" ) );
        _min_tasks  = element.uint_getAttribute( "min_tasks"    , _min_tasks );
        _max_tasks  = element.uint_getAttribute( "tasks"        , _max_tasks );
        string t    = element.     getAttribute( "timeout"      );
        if( t != "" )  
        {
            _task_timeout = Duration::of(t);
        }

        t           = element.     getAttribute( "idle_timeout"    );
        if( t != "" )  
        {
            set_idle_timeout(Duration::of(t));
        }

        {
            string s = element.getAttribute( "ignore_signals" );
            if( !s.empty() )
            {
                if( s == "all" )
                {
                    _ignore_every_signal = true;
                    _ignore_signals_set.clear();
                }
                else
                {
                    _ignore_every_signal = false;
                    vector<string> signals = vector_split( " +", s );
                    for( int i = 0; i < signals.size(); i++ )
                    {
                        bool unknown_in_this_os_only = false;
                        int signal = signal_code_from_name( signals[ i ], &unknown_in_this_os_only );
                        if( unknown_in_this_os_only )  _log->warn( message_string( "SCHEDULER-337", signals[ i ] ) );
                                                 else  _ignore_signals_set.insert( signal );
                    }
                }
            }
        }

        _stop_on_error = element.bool_getAttribute( "stop_on_error", _stop_on_error );

        _force_idle_timeout = element.bool_getAttribute( "force_idle_timeout", _force_idle_timeout );

        set_mail_xslt_stylesheet_path( element.getAttribute( "mail_xslt_stylesheet" ) );

        _warn_if_shorter_than_string = element.getAttribute( "warn_if_shorter_than", _warn_if_shorter_than_string );
        _warn_if_longer_than_string  = element.getAttribute( "warn_if_longer_than" , _warn_if_longer_than_string  );
        _enabled                     = element.bool_getAttribute( "enabled" , _enabled  );  // JS-551
        {
            string s = element.getAttribute("stderr_log_level");
            if (s != "") {
                _stderr_log_level = make_log_level(s);
            }
        }
        _call_register.call(Z_NEW(State_cmd_call(this, _enabled? sc_enable : sc_disable)));

        if( order )  set_order_controlled();

        DOM_FOR_EACH_ELEMENT( element, e )
        {
            if( e.nodeName_is( "description" ) )
            {
                try 
                { 
                    _description = Text_with_includes( _spooler, this, _spooler->include_path(), e ).read_plain_or_xml_string();
                }
                catch( const exception& x  ) { _log->warn( x.what() );  _description = x.what(); }
                catch( const _com_error& x ) { string d = bstr_as_string(x.Description()); _log->warn(d);  _description = d; }
            }
            else
            if( e.nodeName_is( "lock.use" ) )  
            {
                if( !_lock_requestor ) 
                {
                    _lock_requestor = Z_NEW( Job_lock_requestor( this ) );
                    _lock_requestor->set_folder_path( folder_path() );
                }

                _lock_requestor->set_dom( e );
            }
            else
            if( e.nodeName_is( "environment" ) )
            {
                _module->_process_environment->set_dom( e, (Variable_set_map*)NULL, "variable" );
            }
            else
            if( e.nodeName_is( "params"     ) )  _default_params->register_include_and_set_dom( _spooler, this, e, &_spooler->_variable_set_map, "param" );    // Kann <include> registrieren
            else
            if (e.nodeName_is("login")) {
                _module->_login = Z_NEW(Login(e.getAttribute("user"), e.first_child_element().getAttribute("password")));  // <password.plain password=".."/>
            }
            else
            if( e.nodeName_is( "script"     ) )  
            {
                _module->set_dom( e );
            }
            else
            if (e.nodeName_is("monitor.use") || e.nodeName_is("monitor"))
            {
                _module->_monitors->set_dom( e );
            }
            else
            if( e.nodeName_is( "commands" ) )
            {
                add_on_exit_commands_element( e );
            }
            else
            if( e.nodeName_is( "start_when_directory_changed" ) )
            {
                _start_when_directory_changed_list.push_back( pair<string,string>( subst_env( e.getAttribute( "directory" ) ), e.getAttribute( "regex" ) ) );
            }
            else
            if( e.nodeName_is( "delay_after_error" ) )
            {
                set_delay_after_error( e.int_getAttribute( "error_count" ), e.getAttribute( "delay" ) );
            }
            else
            if( e.nodeName_is( "delay_order_after_setback" ) )
            {
                if( e.bool_getAttribute( "is_maximum", false ) )
                {
                    set_max_order_setbacks( e.int_getAttribute( "setback_count" ) );
                }
                else
                    if( !e.hasAttribute( "delay" ) )  z::throw_xc( "SCHEDULER-231", "delay_order_after_setback", "delay" );

                if( e.hasAttribute( "delay" ) )
                {
                    set_delay_order_after_setback( e.int_getAttribute( "setback_count" ), e.getAttribute( "delay" ) );
                }
            }
            else
            if( e.nodeName_is( "run_time" ) &&  !_spooler->_manual )  _schedule_use->set_dom( this, e );
        }
    }

    if( xml::Element_ptr settings_element = element.select_node( "settings" ) ) {
        _history.set_dom_settings( settings_element );
        _log->set_dom_settings( settings_element );
    }
}

list<Requisite_path> Standard_job::missing_requisites() {
    list<Requisite_path> result;
    list<Requisite_path> missings = Dependant::missing_requisites();
    Z_FOR_EACH_CONST(list<Requisite_path>, missings, i) 
        result.push_back(*i);
    if (_lock_requestor) {
        Z_FOR_EACH_CONST(lock::Requestor::Use_list, _lock_requestor->_use_list, i) {
            list<Requisite_path> missings = (*i)->missing_requisites();
            Z_FOR_EACH_CONST(list<Requisite_path>, missings, i) 
                result.push_back(*i);
        }
    }
    if (_module && _module->_monitors) {
        list<Requisite_path> missings = _module->_monitors->missing_requisites();
        Z_FOR_EACH_CONST(list<Requisite_path>, missings, i) 
            result.push_back(*i);
    }
    if (_schedule_use) {
        list<Requisite_path> missings = _schedule_use->missing_requisites();
        Z_FOR_EACH_CONST(list<Requisite_path>, missings, i) 
            result.push_back(*i);
    }
    return result;
}

//----------------------------------------------------Standard_job::get_step_duration_or_percentage

Duration Standard_job::get_step_duration_or_percentage( const string& value, const Duration& deflt )
{
    Duration result = deflt;

    if( value != "" )
    {
        if( value.find( ':' ) != string::npos ) 
        {
            Sos_optional_date_time dt;
            dt.set_time( value );
            result = Duration(dt.time_as_double());
        }
        else
        if( string_ends_with( value, "%" ) ) 
        {
            int percentage = as_int( value.substr( 0, value.length() - 1 ) );
            Duration avg = average_step_duration( deflt );
            result = avg.is_eternal()? Duration::eternal 
                : Duration( percentage/100.0 * avg.as_double() );
        }
        else
        {
            result = Duration(as_double(value));
        }
    }

    return result.rounded_to_next_second();
}

//--------------------------------------------------------------Standard_job::average_step_duration

Duration Standard_job::average_step_duration( const Duration& deflt )
{
    if (_spooler->settings()->_use_java_persistence) {
        ::javaproxy::scala::Option duration_option = typed_java_sister().tryFetchAverageStepDuration();
        return duration_option.isDefined()? Duration(javaproxy::org::joda::time::Duration(duration_option.get()).getMillis() / 1000.0) : deflt;
    } else {
        return db_average_step_duration(deflt);
    }
}
//-----------------------------------------------------------Standard_job::db_average_step_duration

Duration Standard_job::db_average_step_duration( const Duration& deflt )
{
    Duration result = deflt;

    Record record;
    S select_sql;
    select_sql << "select round (sum( %secondsdiff( `end_time`, `start_time` ) ) / sum( `steps` ),2 )" // JS-448
                    "  from " << db()->_job_history_tablename
                << "  where `steps` > 0 "
                    " and `spooler_id`=" << sql::quoted( _spooler->id_for_db() )
                <<  " and `job_name`=" << sql::quoted( path().without_slash() );

    for( Retry_transaction ta ( db() ); ta.enter_loop(); ta++ ) try
    {
        record = ta.read_single_record( select_sql, Z_FUNCTION );
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_job_history_tablename, x ), Z_FUNCTION ); }

    if( !record.null(0) && record.as_string(0) != "" ) {
        result = Duration(floor( record.as_double( 0 ) ));
    }

    return result;
}

//--------------------------------------------------------------------Standard_job::is_in_job_chain

bool Standard_job::is_in_job_chain() const
{ 
    return _combined_job_nodes && !_combined_job_nodes->is_empty(); 
}

//--------------------------------------------------------------------Standard_job::next_order_time

Time Standard_job::next_order_time() const
{ 
    return _combined_job_nodes->next_time(); 
}

//-------------------------------------------------------------Standard_job::fetch_and_occupy_order

Order* Standard_job::fetch_and_occupy_order(Task* occupying_task, const Time& now, const string& cause, const Process_class* required_process_class) {
    return _combined_job_nodes->fetch_and_occupy_order(occupying_task, now, cause, required_process_class);
}

//---------------------------------------------------------------Standard_job::set_order_controlled

void Standard_job::set_order_controlled()
{
    if( _temporary )  z::throw_xc( "SCHEDULER-155" );
    _is_order_controlled = true;
}

//-------------------------------------------------------------------Standard_job::set_idle_timeout

void Standard_job::set_idle_timeout( const Duration& d )
{ 
    _idle_timeout = d; 
    if( _idle_timeout > max_task_time_out )  _idle_timeout = max_task_time_out;   // Begrenzen, damit's beim Addieren mit now() keinen Überlauf gibt
}

//-------------------------------------------------------Standard_job::add_on_exit_commands_element

void Standard_job::add_on_exit_commands_element( const xml::Element_ptr& commands_element )
{
    if( !_commands_document )
    {
        _commands_document.create();
        _commands_document.create_root_element( "all_commands" );       // Name ist egal
    }

    _commands_document.documentElement().importAndAppendChild(commands_element);
}

//-----------------------------------------------------------Standard_job::prepare_on_exit_commands
/* commands-Exit Codes in OS Exit Codes umsetzen, damit Auswertung des OS ExitCodes am Task-Ende möglich ist
success => 0
error   => Wird nicht umgesetzt, da mehrere Werte umfassen kann, alle Integers ungleich 0 (lt. XSD-Schema). Erst bei Task-Ende ausgewertet
signal  => Nur für Unix, wenn Exit-Code < 0
numerisch => Unverändert übernommen
Unix-Signalname (lt. XSD) => Signal-Tabellenwert * (-1)
*/
void Standard_job::prepare_on_exit_commands()
{
    if( _commands_document )
    {
        //xml::Element_ptr error_commands_element = NULL;
        bool passed_error_commands = false;

        for( xml::Element_ptr commands_element = _commands_document.documentElement().firstChild(); 
             commands_element; 
             commands_element = commands_element.nextSibling() )
        {
            string on_exit_code = commands_element.getAttribute( "on_exit_code" );
            if( on_exit_code == "" )  z::throw_xc( "SCHEDULER-324", on_exit_code );

            if( on_exit_code == "error" )
            {
                if( passed_error_commands )  z::throw_xc( "SCHEDULER-326", on_exit_code, on_exit_code );
                passed_error_commands = true;
            }
            else
            {
                vector<int> exit_codes;

                if( on_exit_code == "success" )
                {
                    exit_codes.push_back( 0 );      // on_exit_code="success" ist dasselbe wie on_exit_code="0"
                }
                else
                {
                    vector<string> values = vector_split( " +", on_exit_code );
                    exit_codes.reserve( values.size() );

                    for( int i = 0; i < values.size(); i++ )
                    {
                        try
                        {
                            string v = values[ i ];
                            if( v.empty() )  z::throw_xc( "SCHEDULER-324", on_exit_code, "(missing value)" );
                            
                            if( v == "signal" )                                        _exit_code_commands_on_signal = commands_element;
                            else
                            if( isdigit( (unsigned char)v[ 0 ] )  ||  v[ 0 ] == '-' )  exit_codes.push_back( as_int( v ) );
                            else  
                            {
                                bool unknown_in_this_os_only = false;
                                int signal = signal_code_from_name( v, &unknown_in_this_os_only );
                                if( unknown_in_this_os_only )  _log->warn( message_string( "SCHEDULER-337", v ) );
                                                         else  exit_codes.push_back( -signal );
                            }
                        }
                        catch( exception& x ) { z::throw_xc( "SCHEDULER-324", on_exit_code, x.what() ); }
                    }
                }

                for( int i = 0; i < exit_codes.size(); i++ )
                {
                    int exit_code = exit_codes[ i ];
                    if( _exit_code_commands_map.find( exit_code ) != _exit_code_commands_map.end() )  throw_xc( "SCHEDULER-326", on_exit_code, exit_code );
                    _exit_code_commands_map[ exit_code ] = commands_element;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------Standard_job::set_log

void Standard_job::set_log()
{
    _log->set_job_name( name() );
    _log->set_prefix( "Job  " + path().without_slash() );       // Zwei Blanks, damit die Länge mit "Task " übereinstimmt
    _log->set_profile_section( profile_section() );
    _log->set_title( obj_name() );
    _log->set_mail_defaults();
}

//--------------------------------------------------Standard_job::init_start_when_directory_changed

void Standard_job::init_start_when_directory_changed( Task* task )
{
    for( Start_when_directory_changed_list::iterator it = _start_when_directory_changed_list.begin(); 
         it != _start_when_directory_changed_list.end();
         it++ ) //it = _start_when_directory_changed_list.erase( it ) )
    {
        try
        {
            start_when_directory_changed( it->first, it->second );
        }
        catch( exception& x )
        {
            set_error( x );
            if( task )  task->set_error_xc_only( x );
            ( task? task->log() : +_log )->log( log_error, string( "<start_when_directory_changed>  " ) + x.what() );
            //if( error_state )  set_state( error_state );
        }
    }
}

//-----------------------------------------------------------------Standard_job::on_schedule_loaded

void Standard_job::on_schedule_loaded()
{
    if( file_based_state() == s_incomplete )  
    {
        bool ok = activate();
        if( ok )  set_state( _is_permanently_stopped? s_stopped : s_pending );
    }

    reset_scheduling();
}

//---------------------------------------------------------------Standard_job::on_schedule_modified

void Standard_job::on_schedule_modified()
{
    reset_scheduling();
}

//----------------------------------------------------------Standard_job::on_schedule_to_be_removed

bool Standard_job::on_schedule_to_be_removed()
{
    string schedule_name = _schedule_use->schedule()->obj_name();

    _schedule_use->disconnect();            // Schaltet auf default_schedule um, falls gesetzt

    if( !_schedule_use->is_defined() )
    {
        set_file_based_state( File_based::s_incomplete );
        end_tasks( message_string( "SCHEDULER-885", schedule_name ) );
        stop_simply( false );  //2008-10-14 ? Nicht stoppen, sondern set_file_based_state( s_incomplete )? Task-Start verhindern!
    }

    reset_scheduling();

    return true;
}

//-----------------------------------------------------------------Standard_job::on_monitors_loaded

bool Standard_job::on_monitors_loaded() {
    if (file_based_state() == s_incomplete) {
        bool ok = activate();
        if (ok) set_state(_is_permanently_stopped ? s_stopped : s_pending);
    } 
    return file_based_state() == s_active;
}

//-----------------------------------------------------------Standard_job::on_monitor_to_be_removed

bool Standard_job::on_monitor_to_be_removed(Monitor* monitor) {
    if (!_module->_monitors->is_loaded()) {
        set_file_based_state(File_based::s_incomplete);
        end_tasks(message_string("SCHEDULER-885", monitor->obj_name()));
        bool end_all_tasks = true;
        stop_simply(end_all_tasks);
    }
    return true;
}

//---------------------------------------------------------------Standard_job::on_prepare_to_remove

void Standard_job::on_prepare_to_remove()
{ 
    end_tasks( "" );
    stop_simply( true );   //2008-10-14: Nicht stoppen, sondern neuer Zustand s_closed?

    My_file_based::on_prepare_to_remove();
}

//-----------------------------------------------------------------Standard_job::can_be_removed_now

bool Standard_job::can_be_removed_now()
{ 
    if( job_folder()  &&  ( is_to_be_removed()  ||  _temporary ) )
    {
        if( _temporary  &&  !is_to_be_removed() )  return false;

        if (!_running_tasks.empty())  //2007-09-26 ||  _task_queue->size() > 0 )
            return false;

        if( _state == s_not_initialized )  return true;
        if( _state == s_initialized     )  return true;
        if( _state == s_stopped         )  return true;
      //if( _state == s_error           )  return true;  Diesen Zustand sollte es nicht geben
        if( _state == s_pending         )  return true;
    }

    return false;
}

//----------------------------------------------------------------------Standard_job::on_remove_now

bool Standard_job::on_remove_now()
{
    if( remove_flag() != rm_temporary )  database_record_remove();
    return true;
}

//-----------------------------------------------------------------------Standard_job::remove_error

zschimmer::Xc Standard_job::remove_error()
{
    return zschimmer::Xc( "SCHEDULER-258" );
}

//-----------------------------------------------------------------------------Job::profile_section

string Job::profile_section() 
{
    return "Job " + path().without_slash();
}

//------------------------------------------------------------------Standard_job::set_error_xc_only

void Standard_job::set_error_xc_only( const Xc& x )
{
    _error = x;
    _repeat = Duration(0);
}

//-----------------------------------------------------------------------Standard_job::set_error_xc

void Standard_job::set_error_xc( const Xc& x )
{
    _log->error( x.what() );

    set_error_xc_only( x );
}

//--------------------------------------------------------------------------Standard_job::set_error

void Standard_job::set_error( const exception& x )
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
        Xc xc ( "SOS-2000", x.what(), exception_name(x).c_str() );
        set_error_xc( xc );
    }
}

//------------------------------------------------------------------------Standard_job::create_task

ptr<Task> Standard_job::create_task(Com_variable_set* params, const string& task_name, bool force, const Time& start_at, int id )
{
    assert_is_initialized();
    if( is_to_be_removed() )  z::throw_xc( "SCHEDULER-230", obj_name() );

    switch( _state )
    {
        case s_error:       z::throw_xc( "SCHEDULER-204", name(), _error.what() );
        case s_stopped:     if( force  &&  _spooler->state() != Spooler::s_stopping )  set_state( s_pending );  break;
        default:            if( _state < s_initialized )  z::throw_xc( "SCHEDULER-396", Job::state_name( s_initialized ), Z_FUNCTION, state_name() );
    }

    ptr<Task> task = Z_NEW(Task(this, _stderr_log_level));

    task->_id          = id;
    task->_obj_name    = S() << "Task " << path().without_slash() << ":" << task->_id;
    task->_name        = task_name;
    task->_force_start = start_at.not_zero()? force : false;
    task->_start_at    = start_at;     // 0: Bei nächster Periode starten

    if( const Com_variable_set* p = dynamic_cast<const Com_variable_set*>( +params ) )  task->merge_params( p );

    return +task;
}

//------------------------------------------------------------------------Standard_job::create_task

ptr<Task> Standard_job::create_task(Com_variable_set* params, const string& name, bool force, const Time& start_at )
{
    return create_task( params, name, force, start_at, _spooler->db()->get_task_id(obj_name()) );
}

//-------------------------------------------------------------------------Standard_job::load_tasks

void Standard_job::load_tasks(Read_transaction* ta)
{
    if (_spooler->settings()->_use_java_persistence) 
        load_tasks_with_java();
    else
        load_tasks_from_db(ta);
}

//---------------------------------------------------------------Standard_job::load_tasks_with_java

void Standard_job::load_tasks_with_java()
{
    typed_java_sister().loadPersistentTasks();
}

//-----------------------------------------------------------------Standard_job::load_tasks_from_db

void Standard_job::load_tasks_from_db( Read_transaction* ta )
{
    Time now = Time::now();

    S select_sql;
    select_sql << "select `task_id`, `enqueue_time`, `start_at_time`"
               << "  from " << db()->_tasks_tablename
               << "  where `spooler_id`="        << sql::quoted( _spooler->id_for_db() )
               <<    " and `cluster_member_id` " << sql::null_string_equation( _spooler->distributed_member_id() )
               <<    " and `job_name`="          << sql::quoted( path().without_slash() ) 
               << "  order by `task_id`";

    Any_file sel = ta->open_result_set( select_sql, Z_FUNCTION );
    
    while( !sel.eof() )
    {
        Record record  = sel.get_record();
        int    task_id = record.as_int( "task_id" );
        try
        {
            Time                    start_at;
            ptr<Com_variable_set>   parameters = new Com_variable_set;
            xml::Document_ptr       task_dom;
            bool                    force_start = force_start_default;

            start_at = Time::of_utc_date_time( record.as_string( "start_at_time" ) );
            _log->info( message_string( "SCHEDULER-917", task_id, start_at.not_zero()? start_at.as_string(time_zone_name()) : "period" ) );

            string parameters_xml = file_as_string( "-binary " + _spooler->db()->db_name() + " -table=" + db()->_tasks_tablename + " -clob='parameters'"
                                                                                       " where \"TASK_ID\"=" + as_string( task_id ), 
                                                    "" );
            if( !parameters_xml.empty() )  parameters->set_xml_string( parameters_xml );


            string xml = file_as_string( "-binary " + _spooler->db()->db_name() + " -table=" + db()->_tasks_tablename + " -clob='task_xml'"
                                                                                 " where \"TASK_ID\"=" + as_string( task_id ),
                                         "" );

            if( !xml.empty() )
            {
                task_dom = xml::Document_ptr::from_xml_string( xml );
                force_start = task_dom.documentElement().bool_getAttribute( "force_start", force_start );
            }

            ptr<Task> task = create_task( +parameters, "", force_start, start_at, task_id );
            
            if( task_dom )  task->set_dom( task_dom.documentElement() );

            task->_is_in_database = true;
            task->_let_run        = true;
            task->_enqueue_time = Time::of_utc_date_time( record.as_string( "enqueue_time" ) );

            if( !start_at  &&  !_schedule_use->period_follows( now ) ) 
            {
                try{ z::throw_xc( "SCHEDULER-143" ); } catch( const exception& x ) { _log->warn( x.what() ); }
            }

            _task_queue->enqueue_task( task );
        }
        catch( exception& x )
        {
            _log->error( message_string( "SCHEDULER-283", task_id, x ) );
        }
    }
}

//-----------------------------------------------------------Standard_job::Task_queue::enqueue_task

void Standard_job::Task_queue::enqueue_task( const ptr<Task>& task )
{
    _job->set_visible();
    if( !task->_enqueue_time )  task->_enqueue_time = Time::now();

    if( !task->_is_in_database) {
        xml::Document_ptr task_document = task->dom( show_for_database_only );
        xml::Element_ptr  task_element  = task_document.documentElement();
        bool has_xml = task_element.hasAttributes()  ||  task_element.firstChild();

        if (_spooler->settings()->_use_java_persistence)
            _job->typed_java_sister().persistEnqueuedTask(task->_id, task->_enqueue_time.millis(), task->_start_at.millis(), 
                task->has_parameters()? xml_as_string(task->parameters_as_dom()) : "", 
                has_xml? xml_as_string(task_document) : "");
        else {
            while(1) try {
                Transaction ta ( _spooler->db() );

                Insert_stmt insert ( ta.database_descriptor() );
                insert.set_table_name( _spooler->db()->_tasks_tablename );

                insert             [ "TASK_ID"       ] = task->_id;
                insert             [ "JOB_NAME"      ] = task->_job->path().without_slash();
                insert             [ "SPOOLER_ID"    ] = _spooler->id_for_db();

                if( _spooler->distributed_member_id() != "" ) //if( _spooler->is_cluster() )
                insert             [ "cluster_member_id" ] = _spooler->distributed_member_id();

                insert.set_datetime( "ENQUEUE_TIME"  ,   task->_enqueue_time.db_string( time::without_ms ) );

                if( task->_start_at.not_zero() )
                insert.set_datetime( "START_AT_TIME" ,   task->_start_at.db_string( time::without_ms ) );

                ta.execute( insert, Z_FUNCTION );

                if( task->has_parameters() )
                {
                    Any_file clob;
                    clob = ta.open_file( "-out " + _spooler->db()->db_name(), " -table=" + _spooler->db()->_tasks_tablename + " -clob='parameters'"
                            "  where \"TASK_ID\"=" + as_string( task->_id ) );
                    clob.put( xml_as_string( task->parameters_as_dom() ) );
                    clob.close();
                }

                if (has_xml)
                    ta.update_clob( _spooler->db()->_tasks_tablename, "task_xml", "task_id", task->id(), task_document.xml_string() );

                ta.commit( Z_FUNCTION );

                task->_is_in_database = true;
                break;
            }
            catch( exception& x )
            {
                _spooler->db()->try_reopen_after_error( x, Z_FUNCTION );
            }
        }
    }


    Queue::iterator it = _queue.begin();  // _queue nach _start_at geordnet halten
    while( it != _queue.end()  &&  (*it)->_start_at <= task->_start_at )  it++;
    _queue.insert( it, task );

    _job->_log->info( message_string( "SCHEDULER-919", task->id() ) );
}

//----------------------------------------------------Standard_job::Task_queue::remove_task_from_db

void Standard_job::Task_queue::remove_task_from_db( int task_id )
{
    if (_spooler->settings()->_use_java_persistence)
        _job->typed_java_sister().deletePersistedTask(task_id);
    else
    while(1)
    {
        if (_spooler->db()->opened()) {
            try {
                Transaction ta ( _spooler->db() );
                ta.execute( "DELETE from " + _spooler->db()->_tasks_tablename +
                            "  where \"TASK_ID\"=" + as_string( task_id ),
                            Z_FUNCTION );
                ta.commit( Z_FUNCTION);
                break;
            }
            catch (exception& x) {
                _spooler->db()->try_reopen_after_error( x, Z_FUNCTION );
            }
        }
    }
}

//------------------------------------------------------------Standard_job::Task_queue::remove_task

bool Standard_job::Task_queue::remove_task( int task_id, Why_remove )
{
    bool result = false;

    for( Queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
    {
        Task* task = *it;
        if( task->_id == task_id )
        {
            bool remove_from_db = task->_is_in_database;
            _queue.erase( it );
            task = NULL;

            if( remove_from_db )  remove_task_from_db( task_id );

            result = true;
            break;
        }
    }

    return result;
}

//------------------------------------------------Standard_job::Task_queue::move_to_job_replacement

void Standard_job::Task_queue::move_to_new_job( Standard_job* new_job )
{
    _job = new_job;

    for( Task_queue::iterator it = _queue.begin(); it != _queue.end(); it++ )
    {
        Task* task = *it;
        task->move_to_new_job( new_job );
    }
}

//--------------------------------------------------------Standard_job::Task_queue::next_start_time

Time Standard_job::Task_queue::next_start_time()
{
    Time next_force     = Time::never;
    Time next_in_period = Time::never;

    Z_FOR_EACH_CONST( Queue, _queue, q )
    {
        Task* task = *q;

        if( task->_force_start )  
        {
            next_force = task->_start_at;
            break;
        }
    }

    Z_FOR_EACH_CONST( Queue, _queue, q )
    {
        Task* task = *q;

        if( !task->_force_start )  
        {
            next_in_period = task->_start_at;
            break;
        }
    }

    return min( next_force, next_in_period );
}

//-------------------------------------------Standard_job::Task_queue::append_calendar_dom_elements

void Standard_job::Task_queue::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    Z_FOR_EACH( Queue, _queue, it )
    {
        Task* task = *it;

        if( options->_count >  options->_limit )  break;
        if( task->_start_at >= options->_before ) break;
        
        if( task->_start_at >= options->_from )
        {
            Time             start_at = task->calculated_start_time( options->_from );
            xml::Element_ptr e        = new_calendar_dom_element( element.ownerDocument(), start_at );

            element.appendChild( e );
            e.setAttribute( "job", _job->path().without_slash() );
            e.setAttribute( "task", task->id() );

            options->_count++;
        }
    }
}

//--------------------------------------------------------Standard_job::Task_queue::why_dom_element

xml::Element_ptr Standard_job::Task_queue::why_dom_element(const xml::Document_ptr& doc, const Time& now, bool in_period) {
    // Wie Standard_job::get_task_from_queue()

    xml::Element_ptr result = doc.createElement("task_queue.why");
    result.setAttribute("length", (int)_queue.size());
    Z_FOR_EACH_CONST(Queue, _queue, it) {
        Task* task = *it;
        xml::Element_ptr task_element = result.append_new_element("task_queue_element.why");
        task_element.setAttribute("id", as_string(task->id()));
        bool at_reached = task->at() <= now; 
        if (at_reached) {
            task_element.setAttribute("at", task->at().xml_value());
            if (!in_period)  append_obstacle_element(task_element, "in_period", as_bool_string(in_period));
            if (!task->force())  append_obstacle_element(task_element, "force", as_bool_string(task->force()));
        } else {
            append_obstacle_element(task_element, "at", task->at().xml_value());
        }
        //if (at_reached && (in_period || task->force()))  break;
    }
    return result;
}

jlong Standard_job::next_possible_start_millis() const { 
    Time result = _period.begin();
    Task_queue::iterator i = _task_queue->begin();
    if (i != _task_queue->end()) {
        Task* task = *i;
        if (task->force()) {
            result = min(result, task->at());
        }
    }
    return result.millis();
}

//----------------------------------------------------------------Standard_job::get_task_from_queue

ptr<Task> Standard_job::get_task_from_queue( const Time& now )
{
    ptr<Task> task;

    if( _state == s_error      )  return NULL;

    if( _task_queue->empty() )     return NULL;

    bool                 in_period = is_in_period(now);
    Task_queue::iterator it        = _task_queue->begin();
    
    for( ; it != _task_queue->end(); it++ )
    {
        task = *it;

        if( task->_force_start )    // Start auch außerhalb einer Periode
        {
            if( task->_start_at <= now )  break;        // Task mit Startzeitpunkt
        }
        else
        {
            if( task->_start_at <= now  &&  in_period )  break;        // Task-Start in einer Periode
        }
    }

    if( it == _task_queue->end() )  return NULL;

    return task;
}

//----------------------------------------------------------------Standard_job::remove_running_task

void Standard_job::remove_running_task( Task* task )
{
    _running_tasks.erase(task);

    if (_running_tasks.empty()) {
        switch (_state) {
            case s_stopping:
                set_state(s_stopped);
                break;
            case s_running:
                set_state(s_pending);
                set_next_start_time(Time::now(), true);
            default: ;
        }
    }

    if (_state == s_pending  &&  _running_tasks.size() == _max_tasks - 1)
        _call_register.call<Below_max_tasks_call>();
}

ArrayListJ Standard_job::java_tasks() const {
    ArrayListJ result = ArrayListJ::new_instance(_running_tasks.size() + _task_queue->_queue.size());
    Z_FOR_EACH_CONST(Task_set, _running_tasks, i) {
        result.add((*i)->java_sister());
    }
    Z_FOR_EACH_CONST(Task_queue::Queue, _task_queue->_queue, i) {
        result.add((*i)->java_sister());
    }
    return result;
}

//---------------------------------------------------------------------------------Job::start_task

ptr<Task> Job::start_task(Com_variable_set* params, const string& task_name, const Time& at)
{
    if( is_to_be_removed() )  z::throw_xc( "SCHEDULER-230", obj_name() );
    return start_task(params, (Com_variable_set*)NULL, at, force_start_default, task_name, "");
}

//---------------------------------------------------------------------------------Job::start_task

void Job::start_task(Com_variable_set* params, Com_variable_set* environment)
{
    start_task(params, environment, Time(0), force_start_default, "", "");
}

//------------------------------------------------------------------------Standard_job::start_task_

ptr<Task> Standard_job::start_task_(Com_variable_set* params, Com_variable_set* environment, const Time& at, bool force, const string& task_name, const string& web_service_name)
{
    ptr<Task> task = create_task(params, task_name, force, at);
    task->set_web_service( web_service_name );
    if (environment)  task->merge_environment(environment);
    enqueue_task(task);
    return task;
}

//--------------------------------------------------------Standard_job::enqueue_taskPersistentState

void Standard_job::enqueue_taskPersistentState(const TaskPersistentStateJ& taskPersistentStateJ) {
    int task_id = taskPersistentStateJ.taskIdNumber();
    Time start_at = Time::of_millis(taskPersistentStateJ.startTimeMillis());
    _log->info( message_string( "SCHEDULER-917", task_id, start_at.not_zero()? start_at.as_string(time_zone_name()) : "period" ) );

    ptr<Com_variable_set> parameters = new Com_variable_set;
    string parameters_xml = taskPersistentStateJ.parametersXml();
    if( !parameters_xml.empty() )  parameters->set_xml_string(parameters_xml);

    xml::Document_ptr task_dom;
    bool force_start = force_start_default;
    string xml = taskPersistentStateJ.xml();
    if (!xml.empty()) {
        task_dom = xml::Document_ptr::from_xml_string(xml);
        force_start = task_dom.documentElement().bool_getAttribute("force_start", force_start);
    }

    ptr<Task> task = create_task( +parameters, "", force_start, start_at, task_id );            
    if( task_dom )  task->set_dom( task_dom.documentElement() );
    task->_is_in_database = true;
    task->_let_run        = true;
    task->_enqueue_time = Time::of_millis(taskPersistentStateJ.enqueueTimeMillis());
    
    if (!start_at && !_schedule_use->period_follows(Time::now())) {
        try { z::throw_xc( "SCHEDULER-143" ); } 
        catch( const exception& x ) { _log->warn( x.what() ); }
    }

    _task_queue->enqueue_task( task );
}

//-----------------------------------------------------------------------Standard_job::enqueue_task

void Standard_job::enqueue_task( Task* task )
{
    Time now = Time::now();

    if( _spooler->_debug )  _log->debug( "start(at=" + task->_start_at.as_string(time_zone_name()) + ( task->_name == ""? "" : ",name=\"" + task->_name + '"' ) + ")" );

    if( _state > s_loaded )
    {
        if( !task->_force_start  &&  !_schedule_use->period_follows( now ) )   z::throw_xc( "SCHEDULER-143" );
    }
    else
    {
        // _schedule_use ist noch nicht gesetzt (<schedule next_start_function=""> kann erst nach dem Laden des Scheduler-Skripts ausgeführt werden)
        // Kann nur beim Laden des Scheduler-Skripts passieren
    }

    task->_let_run = true;

    _task_queue->enqueue_task( task );
    _call_register.call_at<Start_queued_task_call>(_task_queue->next_start_time());
}

//--------------------------------------------------------------Standard_job::stop_after_task_error

void Standard_job::stop_after_task_error( const string& error_message )
{
    if( stops_on_task_error() )
    {
        _log->debug3( message_string( "SCHEDULER-978", error_message ) );
        bool end_all_tasks = false;
        stop( end_all_tasks );
    }
    else  
        _log->debug3( message_string( "SCHEDULER-977", error_message ) );
}

//-------------------------------------------------------------------------------Standard_job::stop

void Standard_job::stop( bool end_all_tasks )
{
    _is_permanently_stopped = true;
    stop_simply( end_all_tasks );
}

//------------------------------------------------------------------------Standard_job::stop_simply

void Standard_job::stop_simply( bool end_all_tasks )
{
    // _is_permanenty_stopped wird nicht gesetzt. Muss verbessert werden!

    set_state(_running_tasks.empty()? s_stopped : s_stopping);
    if( end_all_tasks )  end_tasks( "" );
    clear_when_directory_changed();
    _start_min_tasks = false;
}

//--------------------------------------------------------------------------Standard_job::end_tasks

void Standard_job::end_tasks( const string& task_warning )
{
    Z_FOR_EACH( Task_set, _running_tasks, t )
    {
        Task* task = *t;

        if( !task->ending() )
        {
            if( task_warning != "" )  task->log()->warn( task_warning );
            task->cmd_end( task_end_normal );
        }
    }
}

//-------------------------------------------------------------Standard_job::on_call State_cmd_call

void Standard_job::on_call(const State_cmd_call& call) {
    set_state_cmd(call._cmd);
}

//----------------------------------------------------------Standard_job::on_call Period_begin_call

void Standard_job::on_call(const Period_begin_call&) {
    try_start_tasks();
}

//------------------------------------------------------------Standard_job::on_call Period_end_call

void Standard_job::on_call(const Period_end_call&) {
    Time now = Time::now();
    select_period(now);
    if( !_period.is_in_time(_next_start_time)  &&
        _next_single_start.is_never() ) // Wenn aus absolute_repeat errechnet, stimmt _period vielleicht nicht, dann nicht set_next_start_time() rufen
    {
        set_next_start_time(now);
    }
}

//-----------------------------------------------------Standard_job::on_call Start_queued_task_call

void Standard_job::on_call(const Start_queued_task_call&) 
{
    Time t = _task_queue->next_start_time();
    if (t <= Time::now()) {
        try_start_tasks();
        Time next = _task_queue->next_start_time();
        if (next != t) {
            _call_register.call_at<Start_queued_task_call>(next);
        }
    } else {
        _call_register.call_at<Start_queued_task_call>(t);
    }
}

//-------------------------------------Standard_job::on_call Calculated_next_time_do_something_call

void Standard_job::on_call(const Calculated_next_time_do_something_call&) {
    try_start_tasks();
}

//-----------------------------------------------------------Standard_job::on_call Order_timed_call

void Standard_job::on_call(const Order_timed_call&) {
    _call_register.cancel<Order_timed_call>();
    Time t = next_order_time();
    if (t <= Time::now()) {
        process_orders();
        Time next = next_order_time();
        if (next != t) {
            Z_LOG2("scheduler.signal", "Order_timed_call next=" << t.as_string(time_zone_name()) << "\n");
            _call_register.call_at<Order_timed_call>(next);
        }
    } else {
        _call_register.call_at<Order_timed_call>(t);
    }
}

//----------------------------------------------Standard_job::on_call Order_possibly_available_call

void Standard_job::on_call(const Order_possibly_available_call&) {
    process_orders();
}

//---------------------------------------------------------------------Standard_job::process_orders

void Standard_job::process_orders() {
    continue_tasks_waiting_for_order();
    try_start_tasks();
}


void Standard_job::continue_tasks_waiting_for_order() {
    Time now = Time::now();
    Z_FOR_EACH( Task_set, _running_tasks, t ) {
        Task* task = *t;
        if( task->state() == Task::s_running_waiting_for_order  &&  !task->order() ) {
            if( task->fetch_and_occupy_order( now, Z_FUNCTION ) ) {
                task->do_something();
            }
        }
    }
}

//-----------------------------------------------------Standard_job::on_call Process_available_call

void Standard_job::on_call(const Process_available_call&) {
    try_start_tasks();
}

//-------------------------------------------------------Standard_job::on_call Below_min_tasks_call

void Standard_job::on_call(const Below_min_tasks_call&) {
    try_start_tasks();
}

//-------------------------------------------------------Standard_job::on_call Below_max_tasks_call

void Standard_job::on_call(const Below_max_tasks_call&) {
    try_start_tasks();
}

//-------------------------------------------------------Standard_job::on_call Locks_available_call

void Standard_job::on_call(const Locks_available_call&) {
    try_start_tasks();
}

//-----------------------------------------------------------Standard_job::on_call Task_closed_call

void Standard_job::on_call(const Task_closed_call& call) {
    assert(call._task->state() == Task::s_closed);
    _spooler->_task_subsystem->remove_task(call._task);
    if (_temporary) 
        _call_register.call<Remove_temporary_job_call>();
    else
        try_start_tasks();
}

//------------------------------------------Standard_job::on_call Start_when_directory_changed_call

void Standard_job::on_call(const Start_when_directory_changed_call&) {
    bool ok = check_for_changed_directory(Time::now());         // Hier prüfen, damit Signal zurückgesetzt wird
    log()->debug(S() << "Start_when_directory_changed_call ok=" << ok);
    if (ok) try_start_one_task();
}

//--------------------------------------------------Standard_job::on_call Remote_temporary_job_call

void Standard_job::on_call(const job::Remove_temporary_job_call&) {
    remove();
}

//----------------------------------------------------------------------Standard_job::set_state_cmd

void Standard_job::set_state_cmd(State_cmd state_cmd)
{
    switch( state_cmd ) {
        case sc_disable: // JS-551
        case sc_stop:
            if (_state != s_stopping && _state != s_stopped) {
                stop( true );
                if(state_cmd == sc_disable) // JS-551
                    _enabled = false;       // JS-551
            }
            break;

        case sc_enable: // JS-551
        case sc_unstop:    
            if( _state == s_stopping
             || _state == s_stopped
             || _state == s_error)
            {
                if( is_to_be_removed() ) {
                    _log->error( message_string( "SCHEDULER-284", "unstop" ) );
                } else {
                    if (state_cmd == sc_unstop) set_state( s_pending );  // JS-671
                    if (state_cmd == sc_enable) _enabled = true;         // JS-551
                    check_min_tasks( "job has been unstopped" );
                    set_next_start_time( Time::now() );
                    init_start_when_directory_changed();
                    try_start_tasks();
                }
            }
            break;

        case sc_end:        
            Z_FOR_EACH( Task_set, _running_tasks, t )  (*t)->cmd_end();
            break;

        case sc_suspend: {
            if( _state == s_running ) {
                Z_FOR_EACH( Task_set, _running_tasks, t ) {
                    Task* task = *t;
                    if( task->_state == Task::s_running 
                     || task->_state == Task::s_running_delayed
                     || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_suspended );
                }
            }
            break;
        }

        case sc_continue: {
            Z_FOR_EACH( Task_set, _running_tasks, t ) {
                Task* task = *t;
                if( task->_state == Task::s_suspended 
                 || task->_state == Task::s_running_delayed
                 || task->_state == Task::s_running_waiting_for_order )  task->set_state( Task::s_running );
            }
                    
            set_state(_running_tasks.empty()? s_pending : s_running);
            check_min_tasks( "job has been unstopped with cmd=\"continue\"" );
            try_start_tasks();
            break;
        }

        case sc_wake: {
            if( _state == s_pending
             || _state == s_stopped )
            {
                if( is_to_be_removed() ) {
                    _log->error( message_string( "SCHEDULER-284", "wake" ) );
                } else {
                    ptr<Task> task = create_task( NULL, "", 0 ); 
                            
                    set_state( s_pending );
                    check_min_tasks( "job has been unstopped with cmd=\"wake\"" );

                    task->_cause = cause_wake;
                    task->_let_run = true;
                    task->init();
                    try_start_tasks();
                }
            }
            break;
        }

        case sc_wake_when_in_period: {
            if (is_in_period(Time::now())) {
                _wake_when_in_period = true;
                if (_state == s_pending || _state == s_running)
                    try_start_tasks();
            }
            return;
        }

        case sc_start:
            start_task( NULL, "", Time::now() );
            try_start_one_task();
            break;

        case sc_remove:     
            remove( File_based::rm_base_file_too );
            break;

        default: ;
    }
}

//-------------------------------------------------------Standard_job::start_when_directory_changed

void Standard_job::start_when_directory_changed( const string& directory_name, const string& filename_pattern )
{
    _log->debug( "start_when_directory_changed \"" + directory_name + "\", \"" + filename_pattern + "\"" );

    Directory_watcher_list::iterator it; 

    for( it = _directory_watcher_list.begin(); it != _directory_watcher_list.end(); it++ )
    {
        Directory_watcher* old_directory_watcher = *it;

        if( old_directory_watcher->directory()        == directory_name 
         && old_directory_watcher->filename_pattern() == filename_pattern )  
        {
#           ifdef Z_WINDOWS
                //return;

                // Windows: Überwachung erneuern
                // Wenn das Verzeichnis bereits überwacht war, aber inzwischen gelöscht, und das noch nicht bemerkt worden ist
                // (weil Task_subsystem::wait vor lauter Jobaktivität nicht gerufen wurde), dann ist es besser, die Überwachung 
                // hier zu erneuern. Besonders, wenn das Verzeichnis wieder angelegt ist.
                // Das ist bei lokalen Verzeichnissen nicht möglich, weil mkdir auf einen Fehler läuft, solange die Überwachung noch aktiv ist.
                // Aber bei Netzwerkverzeichnissen gibt es keinen Fehler, und die Überwachung schweigt.

                break;
#            else
                (*it)->renew();
                return;   // Unix: Alles in Ordnung
#           endif
        }
    }


    ptr<Directory_watcher> new_dw = Z_NEW( Directory_watcher( _log ) );

    new_dw->watch_directory( directory_name, filename_pattern );
    new_dw->set_name( "job(\"" + name() + "\").start_when_directory_changed(\"" + directory_name + "\",\"" + filename_pattern + "\")" );
    new_dw->on_signaled_call(Z_NEW(Start_when_directory_changed_call(this)));
    new_dw->add_to( &_spooler->_wait_handles );

    if( it == _directory_watcher_list.end() )  // neu?
    {
        _directory_watcher_list.push_back( new_dw );
    }
    else
    {
        Directory_watcher* old_directory_watcher = *it;

        try
        {
            old_directory_watcher->wait( 0 );

            if( old_directory_watcher->signaled() ) 
            {
                new_dw->_signaled = true;  // Ist gerade etwas passiert? Dann in die neue Überwachung hinüberretten
                Z_LOG2( "scheduler",  Z_FUNCTION << " old directory watchers signal has been transfered to new watcher.\n" );
            }
        }
        catch( exception& x ) { log()->warn( string(x.what()) + ", in old_directory_watcher->wait(0)" ); }      // Vorsichtshalber

        // Nicht aus der Liste löschen, das bringt init_start_when_directory_changed() durcheinander! _directory_watcher_list.erase( it );
        *it = new_dw;       // Alte durch neue Überwachung ersetzen
    }

    _call_register.call_at<Start_when_directory_changed_call>(Time(0));
}

//-------------------------------------------------------Standard_job::clear_when_directory_changed

void Standard_job::clear_when_directory_changed()
{
    if( !_directory_watcher_list.empty() )  _log->debug( "clear_when_directory_changed" );
    _directory_watcher_list.clear();
    _call_register.cancel<Start_when_directory_changed_call>();
}

//---------------------------------------------------------Standard_job::update_changed_directories

void Standard_job::update_changed_directories( Directory_watcher* directory_watcher )
{
    _directory_changed = true;

    if( directory_watcher->directory().find( ';' ) != string::npos )  _log->warn( message_string( "SCHEDULER-976", directory_watcher->directory() ) );
    else
    {
        if( ( ";" + _changed_directories + ";" ).find( ";" + directory_watcher->directory() + ";" ) == string::npos )  // Noch nicht drin?
        {
            if( !_changed_directories.empty() )  _changed_directories += ";";
            _changed_directories += directory_watcher->directory();
        }
    }
}

//--------------------------------------------------------Standard_job::check_for_changed_directory

bool Standard_job::check_for_changed_directory( const Time& now )
{
    bool something_done = false;

    _call_register.call_at<Start_when_directory_changed_call>(_directory_watcher_list.empty()? Time::never : now + directory_watcher_intervall);

    for (Directory_watcher_list::iterator it = _directory_watcher_list.begin(); it != _directory_watcher_list.end();) {
        Directory_watcher* directory_watcher = *it;

        directory_watcher->has_changed();                        // has_changed() für Unix (und seit 22.3.04 für Windows, siehe dort).
        
        if (directory_watcher->signaled_then_reset()) {
            Z_LOG2( "zschimmer", Z_FUNCTION << " something_done=true\n" );
            something_done = true;
            update_changed_directories( directory_watcher );
            if (!directory_watcher->valid()) {
                it = _directory_watcher_list.erase( it );  // Folge eines Fehlers, s. Directory_watcher::set_signal
                continue;
            }
        }            
        it++;
    }

    //Z_LOG2( "zschimmer", obj_name() << " " << Z_FUNCTION << " something_done=" << something_done << "  _changed_directories="  << _changed_directories << "\n" ); 
    return something_done;
}

//----------------------------------------------------------------------Standard_job::trigger_files

string Standard_job::trigger_files( Task* task )
{
    S result;

    Z_FOR_EACH( Directory_watcher_list, _directory_watcher_list, it )
    {
        Directory_watcher* directory_watcher = *it;

        if( directory_watcher->filename_pattern() != "" )   // Nur mit regex= überwachte Verzeichnisse sollen berücksichtigt werden
        {
            try
            {
                Directory_watcher::Directory_reader dir ( directory_watcher );
                while(1)
                {
                    ptr<zschimmer::file::File_info> file_info = dir.get();
                    if( !file_info )  break;
                    
                    string path = file_info->path().path();

                    if( path.find( ';' ) != string::npos )  _log->warn( message_string( "SCHEDULER-975", path ) );
                    else
                    {
                        if( result.length() > 0 )  result << ";";
                        result << path;
                    }
                }
            }
            catch( exception& x )
            {
                set_error( x );
                if( task )  task->log()->warn( x.what() );   // Kein Fehler, sonst endet die Task bevor sie startet
            }
        }
    }


    return result;
}

//--------------------------------------------------------------Standard_job::database_record_store

void Standard_job::database_record_store()
{
    if( file_based_state() >= File_based::s_loaded ) {    // Vorher ist database_record_load() nicht aufgerufen worden
        Time next_start_time = this->next_start_time();
        if( next_start_time != _db_next_start_time  || _is_permanently_stopped != _db_stopped) {
            if (_spooler->settings()->_use_java_persistence) {
                typed_java_sister().persistState();
            }
            else {
                for( Retry_transaction ta ( _spooler->db() ); ta.enter_loop(); ta++ ) try
                {
                    sql::Update_stmt update ( &db()->_jobs_table );
                    
                    update[ "spooler_id"        ] = _spooler->id_for_db();
                    update[ "cluster_member_id" ] = _spooler->db_distributed_member_id();
                    update[ "path"              ] = path().without_slash();

                    if( next_start_time != _db_next_start_time )  update[ "next_start_time" ] = next_start_time.is_never()? sql::Value() : next_start_time.db_string();
                    update[ "stopped" ] = _is_permanently_stopped;      // Bei insert _immer_ stopped schreiben, ist not null

                    ta.store( update, Z_FUNCTION );
                    ta.commit( Z_FUNCTION );
                }
                catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_jobs_table.name(), x ), Z_FUNCTION ); }
            }
        }
        _db_next_start_time = next_start_time;
        _db_stopped         = _is_permanently_stopped;
    }
}

//-------------------------------------------------------------Standard_job::database_record_remove

void Standard_job::database_record_remove()
{
    if (_spooler->settings()->_use_java_persistence) {
        typed_java_sister().deletePersistentState();
    }
    else {
        for( Retry_transaction ta ( _spooler->db() ); ta.enter_loop(); ta++ ) try
        {
            sql::Delete_stmt delete_statement ( &db()->_jobs_table );
            
            delete_statement.and_where_condition( "spooler_id"       , _spooler->id_for_db()            );
            delete_statement.and_where_condition( "cluster_member_id", _spooler->db_distributed_member_id() );
            delete_statement.and_where_condition( "path"              , path().without_slash()          );

            ta.execute( delete_statement, Z_FUNCTION );
            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_jobs_table.name(), x ), Z_FUNCTION ); }
    }
}

//---------------------------------------------------------------Standard_job::database_record_load

void Standard_job::database_record_load( Read_transaction* ta )
{
    assert( file_based_state() == File_based::s_initialized );

    // Lesen aus Tabelle scheduler_jobs
    Any_file result_set = ta->open_result_set
    ( 
        S() << "select `stopped`, `next_start_time`"
            << "  from " << db()->_jobs_table.sql_name()
            << "  where `spooler_id`="        << sql::quoted( _spooler->id_for_db() )
            <<    " and `cluster_member_id`=" << sql::quoted( _spooler->db_distributed_member_id() )
            <<    " and `path`="              << sql::quoted( path().without_slash() ), 
        Z_FUNCTION 
    );
    
    if( !result_set.eof() )  
    {
        Record record  = result_set.get_record();
        _is_permanently_stopped = _db_stopped = record.as_int( "stopped" ) != 0;
        _db_next_start_time = record.null( "next_start_time" )? Time::never :  Time::of_utc_date_time( record.as_string( "next_start_time" ) );
    }
}

//-------------------------------------------------------------------Standard_job::set_schedule_dom

void Standard_job::set_schedule_dom(const xml::Element_ptr& element) {
    schedule_use()->set_dom((File_based*)NULL, element);
}

//-----------------------------------------------------------------------Standard_job::schedule_use

Schedule_use* Standard_job::schedule_use() const                                
{ 
    return +_schedule_use; 
}

//-------------------------------------------------------------------Standard_job::reset_scheduling

void Standard_job::reset_scheduling()
{
    if( file_based_state() >= s_active )
    {
        Time now = Time::now();

        Period period            = _schedule_use->next_period      ( now );  
        Time   next_single_start = _schedule_use->next_single_start( now );

        if( period            != _period            ||
            next_single_start != _next_single_start )
        {
            set_period(period);
            set_next_start_time( now );
        }
    }
}

//----------------------------------------------------------------------Standard_job::select_period

void Standard_job::select_period( const Time& now )
{
    if( now.is_never()  ||  !_schedule_use->is_defined() )
    {
        _period = Period();
    }
    else
    {
        if( now >= _period.end()  ||                                       // Periode abgelaufen?
            _period.begin().is_never() && _period.end().is_never() )       // oder noch nicht gesetzt?
        {
            _schedule_use->log_changed_active_schedule( now );

            Period next_period = _schedule_use->next_period(now);
            if (_period.end() < next_period.begin())    // Folgende Periode schließt sich nicht nahtlos an?
                _wake_when_in_period = false;

            set_period(next_period);  

            if( !_period.begin().is_never() )
            {
                _log->debug( message_string( "SCHEDULER-921", _period.to_xml(), _period.schedule_path().name() == ""? Absolute_path() : _period.schedule_path() ) );
            }
            else 
                _log->debug( message_string( "SCHEDULER-922" ) );
        }

    }

    _start_once = _tasks_count == 0  &&  _schedule_use->is_defined()  &&  _schedule_use->schedule()->active_schedule_at( now )->once();
}

//-------------------------------------------------------------------------Standard_job::set_period

void Standard_job::set_period(const Period& p) 
{
    _period = p;
    _call_register.call_at<Period_begin_call>(_period.begin());
    _call_register.call_at<Period_end_call>(_period.end());       //? + Duration::epsilon);
}

//-----------------------------------------------------------------------Standard_job::is_in_period

bool Standard_job::is_in_period( const Time& now )
{
    return now >= _delay_until  &&  now >= _period.begin()  &&  now < _period.end();
}

//----------------------------------------------------------------Standard_job::set_next_start_time

void Standard_job::set_next_start_time( const Time& now, bool repeat )
{
    select_period( now );
    _next_single_start = Time::never;

    if (!now.is_never()  &&  _state >= s_pending  &&  _schedule_use->is_defined())
        set_next_start_time2(now, repeat);
    else 
        _next_start_time = Time::never;

    calculate_next_time( now );
    database_record_store();
}

//---------------------------------------------------------------Standard_job::set_next_start_time2

void Standard_job::set_next_start_time2(const Time& now, bool repeat) {
    Time next_start_time = Time::never;
    string msg;

    if( _delay_until.not_zero() ) {
        next_start_time = _period.next_try( _delay_until );
        if( _spooler->_debug )  msg = message_string( "SCHEDULER-923", next_start_time );   // "Wiederholung wegen delay_after_error"
    }
    else
    if( _state == s_pending && _max_tasks > 0 ) {
        if( !_period.is_in_time( _next_start_time ) ) {
            if( !_repeat )  _next_single_start = _schedule_use->next_single_start( now );
            if( _start_once || _start_min_tasks || !repeat && _period.has_repeat_or_once() ) {
                if( _period.begin() > now ) {
                    next_start_time = _period.begin();
                    if( _spooler->_debug )  msg = message_string( "SCHEDULER-924", next_start_time );   // "Erster Start zu Beginn der Periode "
                } else {
                    next_start_time = now;
                }
            }
            else
            if( repeat ) {
                if( !_repeat.is_zero() ) {      // spooler_task.repeat
                    next_start_time = _period.next_try( now + _repeat );
                    if( _spooler->_debug )  msg = message_string( "SCHEDULER-925", _repeat, next_start_time );   // "Wiederholung wegen spooler_job.repeat="
                    _repeat = Duration(0);
                }
                else
                if( !_period.repeat().is_eternal() ) {
                    next_start_time = _period.next_repeated( now );
                    if( _spooler->_debug && !next_start_time.is_never())  msg = message_string( "SCHEDULER-926", _period.repeat(), next_start_time );   // "Nächste Wiederholung wegen <period repeat=\""
                    if( next_start_time >= _period.end() ) {
                        Period next_period = _schedule_use->next_period( _period.end() );
                        if( _period.end()    == next_period.begin()  &&  
                            _period.repeat() == next_period.repeat()  &&  
                            _period.absolute_repeat().is_eternal() )
                        {
                            if( _spooler->_debug )  msg += " (in the following period)";
                        } else {
                            next_start_time = Time::never;
                            if( _spooler->_debug )  msg = message_string( "SCHEDULER-927" );    // "Nächste Startzeit wird bestimmt zu Beginn der nächsten Periode "
                                              else  msg = "";
                        }
                    }
                }
            }
        }
    }
    else
    if( _state == s_running ) {
        if( _start_min_tasks )  next_start_time = min( now, _period.begin() );
    } else
        next_start_time = Time::never;

    if( _spooler->_debug ) {
        if( _next_single_start < next_start_time )  msg = message_string( "SCHEDULER-928", _next_single_start );
        if( !msg.empty() )  _log->debug( msg );
    }
    _next_start_time = next_start_time;
}

//--------------------------------------------------------------------Standard_job::next_start_time

Time Standard_job::next_start_time() const
{
    if( _state == s_pending  ||  _state == s_running ) {
        Time t = min( _next_start_time, _next_single_start );
        return !is_in_job_chain() || t.is_zero()? t : min(t, max(next_order_time(), _period.begin()));
    } else 
        return Time::never;
}

//----------------------------------------------------------------Standard_job::calculate_next_time
// Für Task_subsystem
// Wird auch gerufen von Directory_file_order_source::start()

void Standard_job::calculate_next_time( const Time& now )
{
    // Algorithmus ist mit task_to_start() abgestimmt.
    // (Schön wäre ja, wenn man beide zusammenfassen könnte und wenn ein String geliefert würde, warum der Job noch nicht startet, worauf er wartet.)

    Time next_time = Time::never;

    if( _state == s_running || _state == s_pending ) {
        if( _lock_requestor  &&  
            ( _lock_requestor->is_enqueued()  ||  !_lock_requestor->locks_are_known() ) )
        {
            if( _lock_requestor->locks_are_available() )  next_time = Time(0);    // task_to_start() ruft _lock_requestor->dequeue_lock_requests
        }
        else
        if( _waiting_for_process )
        {
            if( _waiting_for_process_try_again )  next_time = Time(0);            // task_to_start() ruft remove_waiting_job_from_process_list()
        }
        else
        {
            if( _state == s_pending   &&  _max_tasks > 0
             || _state == s_running   &&  _running_tasks_count < _max_tasks )
            {
                bool in_period = is_in_period(now);

                if( in_period  &&  ( _start_once || _start_once_for_directory ) )
                    next_time = Time(0);
                else {
                    next_time = min(next_time, _next_start_time);
                    next_time = min(next_time, _next_single_start);
                }

                if (next_time > now && is_in_job_chain() && in_period) {
                    bool has_order = request_order( now, Z_FUNCTION );
                    next_time = has_order? Time(0) : min(next_time, next_order_time() );
                }
            }
        }
    }

    _call_register.call_at<Calculated_next_time_do_something_call>(next_time);
}

//------------------------------------------------------------------------Job::signal_earlier_order

void Job::signal_earlier_order( Order* order )
{
    signal_earlier_order( order->next_time(), order->obj_name(), Z_FUNCTION );
}

//---------------------------------------------------------------Standard_job::signal_earlier_order

void Standard_job::signal_earlier_order( const Time& t, const string& order_name, const string& function )
{
    if (!t.is_never()) {
        Z_LOG2( "scheduler.signal", Z_FUNCTION << "  " << function << " " << order_name << " " << t.as_string(time_zone_name()) << "\n" );
        if (_call_register.at<Order_timed_call>() >= t)
            _call_register.call_at<Order_timed_call>(t);
    }
}

//-------------------------------------------------------------------Standard_job::connect_job_node

bool Standard_job::connect_job_node( Job_node* job_node )
{
    bool result = false;

    if( !is_order_controlled() && _module->kind() != Module::kind_process)  z::throw_xc( "SCHEDULER-147", obj_name() );

    if( _state >= s_initialized ) {
        _combined_job_nodes->connect_job_node( job_node );
        on_order_possibly_available();  // Ruft request_order()
        result = true;
    }

    return result;
}

//----------------------------------------------------------------Standard_job::disconnect_job_node

void Standard_job::disconnect_job_node( Job_node* job_node )
{
    _combined_job_nodes->disconnect_job_node( job_node );
}

//--------------------------------------------------------------------Standard_job::any_order_queue

Order_queue* Standard_job::any_order_queue() const
{
    return _combined_job_nodes->any_order_queue();
}

//----------------------------------------------------------------------Standard_job::request_order

bool Standard_job::request_order( const Time& now, const string& cause )
{
    return _combined_job_nodes->request_order( now, cause ); 
}

//-------------------------------------------------------------Standard_job::withdraw_order_request

void Standard_job::withdraw_order_request()
{
    Z_LOGI2( "zschimmer", obj_name() << " " << Z_FUNCTION << "\n" );

    _combined_job_nodes->withdraw_order_requests();
}

//-----------------------------------------------------------Standard_job::notify_a_process_is_idle

void Standard_job::notify_a_process_is_available()
{
    _waiting_for_process_try_again = true;
    _call_register.call<Process_available_call>();
}

//-----------------------------------------------Standard_job::remove_waiting_job_from_process_list

void Standard_job::remove_waiting_job_from_process_list()
{
    if( _waiting_for_process )
    {
        _waiting_for_process = false;
        if (Process_class* process_class = default_process_class_or_null()) {
            process_class->remove_requestor(this);
        }
    }

    _waiting_for_process_try_again = false;
}

//-----------------------------------------------------------------Standard_job::on_requisite_loaded

bool Standard_job::on_requisite_loaded( File_based* file_based )
{
    assert( file_based->subsystem() == spooler()->process_class_subsystem() );

    if (!_spooler->_ignore_process_classes) {
        assert(file_based == default_process_class());
        assert( dynamic_cast<Process_class*>( file_based ) );
        if (_waiting_for_process) {
            notify_a_process_is_available();
        }
    }

    return true;
}

//---------------------------------------------------------Standard_job::on_requisite_to_be_removed

bool Standard_job::on_requisite_to_be_removed( File_based* file_based )
{
    end_tasks( message_string( "SCHEDULER-885", file_based->obj_name() ) );
    return true;
}

//-----------------------------------------------------------------Standard_job::on_locks_available

void Standard_job::on_locks_available()
{
    _call_register.call<Locks_available_call>();
}

//--------------------------------------------------------Standard_job::on_order_possibly_available

void Standard_job::on_order_possibly_available()
{
    Z_LOG2("scheduler.order", Z_FUNCTION << "\n");
    _call_register.call<Order_possibly_available_call>();
}

//----------------------------------------------------------------------Standard_job::task_to_start

ptr<Task> Standard_job::task_to_start()
{
    Time            now       = Time::now();
    Start_cause     cause     = cause_none;
    ptr<Task>       task      = NULL;
    bool            has_order = false;
    string          log_lines;

    
    task = get_task_from_queue( now );
    if( task )  cause = task->_start_at.not_zero()? cause_queue_at : cause_queue;
        
    if( _state == s_pending  &&  _max_tasks > 0  &&  now >= _next_single_start )  
    {
        cause = cause_period_single, log_lines += "Task starts due to <period single_start=\"...\">\n";
        has_order = request_order(now, obj_name());
    }
    else
    if( is_in_period(now) )
    {
        if( _state == s_pending  &&  _max_tasks > 0 )
        {
            if( _start_once )              cause = cause_period_once,                           log_lines += "Task starts due to <run_time once=\"yes\">\n";
            else
            if( now >= _next_start_time )  
                if( _delay_until.not_zero() && now >= _delay_until )
                                           cause = cause_delay_after_error,                     log_lines += "Task starts due to delay_after_error\n";
                                      else cause = cause_period_repeat,                         log_lines += "Task starts, because start time is reached: " + _next_start_time.as_string(time_zone_name()) + "\n";

            if( _start_once_for_directory )
            {
                _start_once_for_directory = false;
                if( !_directory_changed  &&  trigger_files() != "" )  _directory_changed = true;   // Einmal starten, wenn bereits Dateien vorhanden sind 2006-09-11
            }
                
            if( _directory_changed )       cause = cause_directory,                             log_lines += "Task starts due to an event for watched directory " + _changed_directories + "\n";
        }

        if (_wake_when_in_period && (_state == s_pending || _state == s_running) && _running_tasks.size() < _max_tasks)
            cause = cause_wake, log_lines += "Task starts due to wake_when_in_period\n";

        if( _start_min_tasks )
        {
            assert( not_ending_tasks_count() < _min_tasks );
            cause = cause_min_tasks, log_lines = "Task starts due to min_tasks\n";
        }

        has_order = request_order( now, obj_name() );
    }
    else
    {
        assert( !is_in_period(now) );
        withdraw_order_request();
    }


    if( cause || has_order )     // Es soll also eine Task gestartet werden.
    {
        if( _lock_requestor )
        {
            if( _lock_requestor->locks_are_available() )
            {
                Z_LOG2( "scheduler", obj_name() << ": Locks are available\n" );
            }
            else
            {
                // Wir können die Task nicht starten, denn die Sperre ist nicht verfügbar
                task = NULL, cause = cause_none, has_order = false;      

                if( !_lock_requestor->is_enqueued() )  _lock_requestor->enqueue_lock_requests( (lock::Holder*)NULL );
            }
        }
    }


    if( cause || has_order )
    {
        if (!_spooler->_ignore_process_classes) {
            // Ist ein Prozess verfügbar?
            Process_class* process_class = default_process_class_or_null();
            if( !process_class  ||  !process_class->process_available( this ) )
            {
                if( process_class )
                {
                    if( cause != cause_min_tasks  &&  
                        ( !_waiting_for_process  ||  _waiting_for_process_try_again ) )
                    {
                        if( !_waiting_for_process  )
                        {
                            Message_string m ( "SCHEDULER-949", _default_process_class_path.to_string() );   // " ist für einen verfügbaren Prozess vorgemerkt" );
                            if( task )  m.insert( 2, task->obj_name() );
                            log()->info( m );
                            process_class->enqueue_requestor(this);
                            _waiting_for_process = true;
                        }

                        _waiting_for_process_try_again = false;
                        _spooler->task_subsystem()->try_to_free_process(this, process_class);     // Beendet eine Task in s_running_waiting_for_order
                    }
                }
                else
                    _waiting_for_process = true;

                task = NULL, cause = cause_none, has_order = false;      // Wir können die Task nicht starten, denn kein Prozess ist verfügbar
            }
            else
            {
                remove_waiting_job_from_process_list();
            }
        }

        if( cause || has_order )
        {
            if( task )
            {
                assert( cause );
                _task_queue->remove_task( task->id(), Task_queue::w_task_started );
                task->_trigger_files = trigger_files( task );     // Ebenso im else-Zweig
            }
            else
            {
                task = create_task( NULL, "", 0 ); 
                task->_let_run |= ( cause == cause_period_single );
                task->_trigger_files = trigger_files( task );   // Vor occupy_order()!

                if( has_order ) 
                {
                    Order* order = task->fetch_and_occupy_order( now, Z_FUNCTION );   // Versuchen, den Auftrag für die neue Task zu belegen
                    
                    if( !order  &&  !cause )    // Fehlgeschlagen? Dann die Task vergessen 
                    {
                        // Z_LOG2( "scheduler", obj_name() << ": fetch_and_occupy_order() failed, Task will be rejected\n" );  // müllt das Log zu
                        task->close(); 
                        task = NULL;
                    }
                    else 
                    {
                        log_lines += "Task starts for " + order->obj_name() + "\n";
                        if( !cause )  cause = cause_order;
                    }
                }
            }

            if( task )
            {
                if( !log_lines.empty() )  _log->debug(log_lines);

                task->_cause = cause;
                task->_changed_directories = _changed_directories;  
                _changed_directories = "";
                _directory_changed = false;
            }

            if( now >= _next_single_start )  _next_single_start = Time::never;  // Vorsichtshalber, 26.9.03
        }
    }

    if( task  &&  _lock_requestor )
    {
        task->_lock_holder->add_requestor( _lock_requestor );
        task->_lock_holder->hold_locks( _lock_requestor );
        if( _lock_requestor->is_enqueued() )  _lock_requestor->dequeue_lock_requests( log_none );
    }

    if( _waiting_for_process )
    {
        if( Process_class* process_class = default_process_class_or_null() )
            if( process_class->process_available( this ) )
                remove_waiting_job_from_process_list();
    }

    if( task )  _start_once = false;

    return task;
}

//---------------------------------------------------------------------Standard_job::try_start_task

void Standard_job::try_start_tasks()
{
    while (true) {
        bool started = try_start_one_task();
        if (!started) break;
    }
}

//-----------------------------------------------------------------Standard_job::try_start_one_task

bool Standard_job::try_start_one_task()
{
    bool task_started = false;

    if ((_state == s_pending && _max_tasks > 0  || _state == s_running && _running_tasks.size() < _max_tasks)  && 
        (!_waiting_for_process || _waiting_for_process_try_again)  && 
        _spooler->state() != Spooler::s_paused &&
        _spooler->state() != Spooler::s_stopping_let_run) {
        try {
            if (ptr<Task> task = task_to_start()) {
                _log->open();           // Jobprotokoll, nur wirksam, wenn set_filename() gerufen, s. Standard_job::init().
                reset_error();
                _repeat = Duration(0);
                _delay_until = Time(0);
                _running_tasks.insert(task);
                set_state( s_running );

                _next_start_time = Time::never;

                task->init();

                string c = task->cause() == cause_order && task->order()? task->order()->obj_name()
                                                                        : start_cause_name( task->cause() );
                if( _min_tasks <= not_ending_tasks_count() )  
                    _start_min_tasks = false;

                task->do_something();           // Damit die Task den Prozess startet und die Prozessklasse davon weiß
                _log->info(message_string("SCHEDULER-930", task->id(), c, 
                    task->process_class()? task->process_class()->path() : string("(process class not yet known)")));   // Task::do_load() has set process_class_path

                task_started = true;
                _wake_when_in_period = false;
            }
        }
        catch( const exception& x ) { set_error( x );  set_job_error( x );  sos_sleep(1); }     // Bremsen, falls sich der Fehler sofort wiederholt
    }

    if( !task_started  &&  _lock_requestor  &&  _lock_requestor->is_enqueued()  &&  _lock_requestor->locks_are_available() ) 
        _lock_requestor->dequeue_lock_requests();
    return task_started;
}

//-------------------------------------------------------------------Standard_job::on_task_finished

void Standard_job::on_task_finished(Task* task, Task_end_mode end_mode)
{
    if( !_start_min_tasks  &&  ( _state == s_pending  ||  _state == s_running ) )
    {
        if( task->running_state_reached() )
        {
            check_min_tasks( task->obj_name() + " has finished" );
        }
        else
        if( should_start_task_because_of_min_tasks() )
        {
            _log->log(end_mode == task_end_nice ? log_info : log_warn, 
                message_string("SCHEDULER-970", task->obj_name(), _min_tasks));   // Task hat sich zu schnell beendet, wir starten keine neue
        }
    }
}

//--------------------------------------------------------------------Standard_job::check_min_tasks

void Standard_job::check_min_tasks( const string& cause )
{
    if( !_start_min_tasks  &&  should_start_task_because_of_min_tasks() )
    {
        _log->debug( message_string( "SCHEDULER-969", _min_tasks, cause ) );
        _start_min_tasks = true;
        _call_register.call<Below_min_tasks_call>();
    }
    else
    {
        _start_min_tasks = false;
    }
}

//---------------------------------------------Standard_job::should_start_task_because_of_min_tasks

bool Standard_job::should_start_task_because_of_min_tasks()
{
    return ( _state == s_pending || _state == s_running )  
       &&  below_min_tasks();
     //&&  is_in_period( Time::now() );
}

//--------------------------------------------------------------------Standard_job::above_min_tasks

bool Standard_job::above_min_tasks() const
{
    return not_ending_tasks_count() > _min_tasks;       // Nur Tasks zählen, die nicht beendet werden
}

//--------------------------------------------------------------------Standard_job::below_min_tasks

bool Standard_job::below_min_tasks() const
{
    return _min_tasks > 0  &&  not_ending_tasks_count() < _min_tasks;       // Nur Tasks zählen, die nicht beendet werden
}

//-------------------------------------------------------------Standard_job::not_ending_tasks_count

int Standard_job::not_ending_tasks_count() const
{
    int result = 0;

    Z_FOR_EACH_CONST( Task_set, _running_tasks, t )
    {
        if( !(*t)->ending() )  result++;
    }

    return result;
}

//----------------------------------------------------------------------Standard_job::set_job_error

void Standard_job::set_job_error( const exception& x )
{
    set_state( s_error );

    S body;
    body << "Scheduler: Standard_job " << name() << " is in now error state after the error\n" <<
            x.what() << "\n"
            "No more task will be started.";

    Scheduler_event scheduler_event ( evt_job_error, log_error, this );
    scheduler_event.set_error( x );

    Mail_defaults mail_defaults ( _spooler );
    mail_defaults.set( "subject", x.what() );
    mail_defaults.set( "body", body );

    scheduler_event.send_mail( mail_defaults);
}

//--------------------------------------------------------------------------Standard_job::set_state

void Standard_job::set_state( State new_state )
{ 
    if( new_state == _state )  return;

    if( new_state == s_pending  &&  !_delay_until )  reset_error();      // Bei delay_after_error Fehler stehen lassen

    State old_state = _state;
    _state = new_state;

    if( _state == s_stopped 
     || _state == s_error      )  _next_start_time = Time::never;

    if( old_state > s_initialized  ||  new_state != s_stopped )  // Übergang von none zu stopped interessiert uns nicht
    {
        if( new_state == s_stopping
         || new_state == s_stopped  && is_visible()
         || new_state == s_error      )  _log->info  ( message_string( "SCHEDULER-931", state_name() ) ); 
                                   else  _log->debug9( message_string( "SCHEDULER-931", state_name() ) );
    }

    if( _state != s_pending  ||  _state != s_running )
    {
        if( _waiting_for_process )
        {
            remove_waiting_job_from_process_list();
        }

        if( _lock_requestor  &&  _lock_requestor->is_enqueued() )  // &&  _lock_requestor->locks_are_available() )
        {
            _lock_requestor->dequeue_lock_requests();
        }
    }

    if( _state == s_stopped )  check_for_replacing_or_removing();

    if( new_state == s_pending  ||  new_state == s_running )  _is_permanently_stopped = false;

    database_record_store();
    report_event_code(jobStateChanged, java_sister());
}

//----------------------------------------------------------------------Standard_job::set_state_cmd

void Standard_job::set_state_cmd(const string& cmd)
{ 
    set_state_cmd(as_state_cmd(cmd));
}

//--------------------------------------------------------------------------Standard_job::job_state

string Standard_job::job_state()
{
    return "state=" + state_name();
}

//----------------------------------------------------------------------------------Job::state_name

string Job::state_name( State state )
{
    switch( state )
    {
        case s_not_initialized: return "not_initialized";
        case s_initialized:     return "initialized";
        case s_loaded:          return "loaded";
        case s_stopping:        return "stopping";
        case s_stopped:         return "stopped";
        case s_error:           return "error";
        case s_pending:         return "pending";
        case s_running:         return "running";
        default:                return as_string( (int)state );
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

    if( !name.empty() )  z::throw_xc( "SCHEDULER-110", name );
    return s_not_initialized;
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

    if( !name.empty() )  z::throw_xc( "SCHEDULER-106", name );
    return sc_none;
}

//------------------------------------------------------------------------------Job::state_cmd_name

string Job::state_cmd_name( Standard_job::State_cmd cmd )
{
    switch( cmd )
    {
        case Job::sc_stop:     return "stop";
        case Job::sc_unstop:   return "unstop";
        case Job::sc_start:    return "start";
        case Job::sc_wake:     return "wake";
        case Job::sc_wake_when_in_period: return "wake_when_in_period";
        case Job::sc_end:      return "end";
        case Job::sc_suspend:  return "suspend";
        case Job::sc_continue: return "continue";
        case Job::sc_reread:   return "reread";
        case Job::sc_remove:   return "remove";
        case Job::sc_enable:   return "enable";     // JS-551
        case Job::sc_disable:  return "disable";    // JS-551
        default:               return as_string( (int)cmd );
    }
}

//--------------------------------------------------------------Standard_job::set_delay_after_error

void Standard_job::set_delay_after_error( int error_steps, const string& delay )
{ 
    if( lcase( delay ) == "stop" )  set_stop_after_error( error_steps );
                              else  set_delay_after_error( error_steps, Duration::of( delay ) );
}

//------------------------------------------------------Standard_job::set_delay_order_after_setback

void Standard_job::set_delay_order_after_setback( int setback_count, const string& delay )
{
    set_delay_order_after_setback( setback_count, Duration::of( delay ) );
}

//------------------------------------------------------Standard_job::get_delay_order_after_setback

Duration Standard_job::get_delay_order_after_setback( int setback_count )
{
    Duration delay = Duration(0);

    FOR_EACH( Delay_order_after_setback, _delay_order_after_setback, it )  
    {
        if( setback_count >= it->first )  delay = it->second;
    }

    return delay;
}

//--------------------------------------------------------------------Job::is_visible_in_xml_folder

bool Job::is_visible_in_xml_folder( const Show_what& show_what ) const
{
    return is_visible()  &&  ( show_what._job_name == ""  ||  show_what._job_name == path().without_slash() );
}

//------------------------------------------------------------------------Standard_job::dom_element

xml::Element_ptr Standard_job::dom_element( const xml::Document_ptr& document, const Show_what& show_what, Job_chain* which_job_chain )
{
    Time             now    = Time::now();
    xml::Element_ptr result = document.createElement( "job" );

    fill_file_based_dom_element( result, show_what );

    result.setAttribute( "job"       , name()                   );
    result.setAttribute( "state"     , state_name()            );

    if( !_title.empty() )
    result.setAttribute( "title"     , _title                  );

    if( !is_visible() ) result.setAttribute( "visible", _visible == visible_never? "never" : "no" );

    result.setAttribute_optional("process_class", _default_process_class_path);

    if( _state != s_not_initialized )
    {
        if( _waiting_for_process )
        result.setAttribute( "waiting_for_process", _waiting_for_process? "yes" : "no" );

        result.setAttribute( "all_steps" , _step_count             );

        result.setAttribute( "all_tasks" , _tasks_count            );

        if( !_state_text.empty() )
        result.setAttribute( "state_text", _state_text             );

        result.setAttribute( "log_file"  , _log->filename()         );
        result.setAttribute( "order"     , is_order_controlled()? "yes" : "no" );
        result.setAttribute( "tasks"     , _max_tasks              );

        if( _min_tasks )
        result.setAttribute( "min_tasks" , _min_tasks              );


        if( _description != "" )  result.setAttribute( "has_description", "yes" );

        Time next = next_start_time();
        if( !next.is_never() )
        result.setAttribute( "next_start_time", next.xml_value() );

        if( _delay_until.not_zero() )
        result.setAttribute( "delay_until", _delay_until.xml_value() );

        result.setAttribute( "in_period", is_in_period( now )? "yes" : "no" );

        if( is_to_be_removed() )
        result.setAttribute( "remove", "yes" );

        if( _temporary )
        result.setAttribute( "temporary", "yes" );

        if( is_in_job_chain() )
        result.setAttribute( "job_chain_priority", job_chain_priority() );

        if( _warn_if_shorter_than_string != "" )
            result.setAttribute( "warn_if_shorter_than", _warn_if_shorter_than_string );

        if( _warn_if_longer_than_string != "" )
            result.setAttribute( "warn_if_longer_than", _warn_if_longer_than_string );

        result.setAttribute( "enabled", _enabled ? "yes" : "no" );      // JS-551

        if( show_what.is_set( show_job_params )  &&  _default_params )  result.appendChild( _default_params->dom_element( document, "params", "param" ) );

        if( show_what.is_set( show_schedule ) )  result.appendChild( _schedule_use->dom_element( document, show_what ) ),
                                                 dom_append_nl( result );

        if( _schedule_use->is_defined() )   // Wie in Order::dom_element(), besser nach Schedule_use::dom_element()  <schedule.use covering_schedule="..."/>
            if( Schedule* covering_schedule = _schedule_use->schedule()->active_schedule_at( now ) )  
                if( covering_schedule->is_in_folder() )
                    result.setAttribute( "active_schedule", covering_schedule->path() );


        if( _lock_requestor )  result.appendChild( _lock_requestor->dom_element( document, show_what ) );

        if( show_what.is_set( show_tasks ) )
        {
            xml::Element_ptr tasks_element = document.createElement( "tasks" );
            int task_count = 0;        
            Z_FOR_EACH( Task_set, _running_tasks, t )
            {
                Task* task = *t;
                if( !which_job_chain  ||  !task->_order  ||  task->_order->job_chain_for_api() == which_job_chain )
                {
                    if( !show_what._task_id  ||  show_what._task_id == task->id() )
                    {
                        tasks_element.appendChild( task->dom_element( document, show_what ) ), dom_append_nl( tasks_element );
                    }

                    task_count++;
                }
            }
            tasks_element.setAttribute( "count", task_count );
            result.appendChild( tasks_element );
        }
        else
            result.append_new_comment( "<tasks> suppressed. Use what=\"tasks\"." );



        if( show_what.is_set( show_description ) )  result.append_new_text_element( "description", _description );

        if( show_what.is_set( show_job_commands ) && _commands_document )  
        {
            for( xml::Node_ptr n = _commands_document.documentElement().firstChild(); n; n = n.nextSibling() )
            {
                if( n.nodeType() == xml::ELEMENT_NODE ) 
                    result.importAndAppendChild(n);
            }
        }

        xml::Element_ptr queue_element = document.createElement( "queued_tasks" );
        queue_element.setAttribute( "length", as_string( _task_queue->size() ) );
        dom_append_nl( queue_element );
        result.appendChild( queue_element );

        if( show_what.is_set( show_task_queue )  &&  !_task_queue->empty() )
        {
            FOR_EACH( Task_queue, *_task_queue, it )
            {
                Task*            task                = *it;
                xml::Element_ptr queued_task_element = document.createElement( "queued_task" );
                
                queued_task_element.setAttribute( "task"       , task->id() );
                queued_task_element.setAttribute( "id"         , task->id() );                         // veraltet
                queued_task_element.setAttribute( "enqueued"   , task->_enqueue_time.xml_value() );
                queued_task_element.setAttribute( "name"       , task->_name );
                queued_task_element.setAttribute( "force_start", task->_force_start? "yes" : "no" );

                if( task->_start_at.not_zero() )
                    queued_task_element.setAttribute( "start_at", task->_start_at.xml_value() );
                
                if( task->has_parameters() )  queued_task_element.appendChild( task->_params->dom_element( document, "params", "param" ) );

                queue_element.appendChild( queued_task_element );
                dom_append_nl( queue_element );
            }
        }

        if( show_what.is_set( show_task_history ) )
        {
            result.appendChild( _history.read_tail( document, -1, -show_what._max_task_history, show_what, true ) );
        }

        if (is_in_job_chain()) {
            Show_what modified_show = show_what;
            if( modified_show.is_set( show_job_orders ) )  modified_show |= show_orders;
            result.appendChild( _combined_job_nodes->dom_element( document, modified_show, which_job_chain ) );
        }

        if( _error )  append_error_element( result, _error );

        result.appendChild( _log->dom_element( document, show_what ) );
    }

    return result;
}

//--------------------------------------------------------------------Standard_job::why_dom_element

xml::Element_ptr Standard_job::why_dom_element(const xml::Document_ptr& doc) {
    xml::Element_ptr result = doc.createElement("job.why");
    result.setAttribute("job",this->name());
    Time now = Time::now();
    bool in_period = is_in_period(now);
    int not_ending_tasks_count = this->not_ending_tasks_count();


    // do_something():

    //if (_state <= s_loaded || _state == s_error) 
    if (_state != s_pending && _state != s_running)
        append_obstacle_element(result, "state", state_name());

    result.appendChild(_combined_job_nodes->why_dom_element(doc, now));

    //boolean has_order = request_order( now, obj_name() );
    if (!is_in_job_chain()) {
        if (is_order_controlled())
            append_obstacle_element(result, "order_controlled", as_bool_string(is_order_controlled()));
    } else {
        xml::Element_ptr e = result.append_new_element("if_order_is_ready.why");
        //e.appendChild(order->dom_element(doc, Show_what()));
        if (!is_order_controlled())
            append_obstacle_element(result, "order_controlled", as_bool_string(is_order_controlled()));
        if (!_running_tasks.empty()) {
            xml::Element_ptr tasks = e.append_new_element("tasks.why");
            Z_FOR_EACH(Task_set, _running_tasks, it) {
                Task* task = *it;
                xml::Element_ptr t = tasks.append_new_element("task.why");
                if (task->state() != Task::s_running_waiting_for_order)  append_obstacle_element(t, "state", task->state_name());
                if (Order* o = task->order())  append_obstacle_element(t, o->dom_element(doc, Show_what()));
            }
        }
    }

    if (!(_state == s_pending  &&  _max_tasks > 0  ||  _state == s_running  &&  _running_tasks.size() < _max_tasks)) {
        xml::Element_ptr o = result.append_new_element(obstacle_element_name);
        if (_running_tasks.size() >= _max_tasks) {
            o.setAttribute("max_tasks", as_string(_max_tasks));
            o.setAttribute("running_tasks", (int)_running_tasks.size());
        }
        else
            o.setAttribute("state", state_name());
    }

    if (_waiting_for_process) {
        xml::Element_ptr o = append_obstacle_element(result, "waiting_for_process", as_string(_waiting_for_process));
        if (_waiting_for_process_try_again)
            o.setAttribute("waiting_for_process_try_again", as_bool_string(_waiting_for_process_try_again));
    }


    // task_to_start():

    if( _spooler->state() == Spooler::s_stopping || _spooler->state() == Spooler::s_stopping_let_run || _spooler->state() == Spooler::s_paused)
        append_obstacle_element(result, "scheduler_state", _spooler->state_name());

    if (!_task_queue->_queue.empty()) {     // task_queue_at oder cause_queue
        result.appendChild(_task_queue->why_dom_element(doc, now, in_period));
        if (ptr<Task> task = get_task_from_queue(now)) {
            xml::Element_ptr e = result.append_new_element(reason_start_element_name);
            e.appendChild(task->dom_element(doc, Show_what()));
        }
    }

    if (now >= _next_single_start) {    // cause_period_single
        xml::Element_ptr e = result.append_new_element(reason_start_element_name);
        e.setAttribute("next_single_start", _next_single_start.xml_value());
        if (_state != s_pending)  append_obstacle_element(e, "state", state_name());
        if (_max_tasks <= 0)  append_obstacle_element(e, "max_tasks", as_string(_max_tasks));
    }

    if (_start_once) {  // cause_period_once
        xml::Element_ptr e = result.append_new_element(reason_start_element_name);
        e.setAttribute("once", as_bool_string(_start_once));
        if (!in_period)  append_obstacle_element(e, "in_period", as_bool_string(in_period));
        if (_max_tasks == 0)  append_obstacle_element(e, "max_tasks", as_string(_max_tasks));
        if (_state != s_pending)  append_obstacle_element(e, "state", state_name());
    }

    if (_delay_until.not_zero()) { // cause_delay_after_error
        xml::Element_ptr e = result.append_new_element(reason_start_element_name);
        e.setAttribute("delay_until", _delay_until.xml_value());
        if (now >= _delay_until)
            e.setAttribute("now", now.xml_value());
        else
            append_obstacle_element(e, "now", now.xml_value());
    }

    if (!_next_start_time.is_never()) { // cause_period_repeat
        xml::Element_ptr e = result.append_new_element(reason_start_element_name);
        e.setAttribute("next_start_time", _next_start_time.xml_value());
        if (now < _next_start_time)  append_obstacle_element(e, "now", now.xml_value());
        if (!in_period) append_obstacle_element(e, "in_period", as_bool_string(in_period));
        if (_max_tasks == 0) append_obstacle_element(e, "max_tasks", as_string(_max_tasks));
        if (_state != s_pending) append_obstacle_element(e, "state", state_name());
    }

    if (!_start_when_directory_changed_list.empty()) {  // cause_directory
        xml::Element_ptr e = result.append_new_element("start_when_directory_changed.why");
        if (!_directory_changed && !_start_once_for_directory) {
            xml::Element_ptr o = append_obstacle_element(e, "directory_changed", as_bool_string(_directory_changed));
            o.setAttribute("start_once_for_directory", as_bool_string(_start_once_for_directory));
        }  
    }

    if (_min_tasks) {   // cause_min_task
        xml::Element_ptr e = result.append_new_element(reason_start_element_name);
        e.setAttribute("min_tasks", _min_tasks);
        if (not_ending_tasks_count < _min_tasks) 
            append_obstacle_element(e, "not_ending_tasks", as_string(not_ending_tasks_count));
        if (!in_period) append_obstacle_element(e, "in_period", as_bool_string(in_period));
    }

    if (_lock_requestor && !_lock_requestor->locks_are_available())
        append_obstacle_element(result, "locks_available", as_bool_string(false));

    if (!_spooler->_ignore_process_classes) {
        Process_class* process_class = default_process_class_or_null();
        if (!process_class) append_obstacle_element(result, "process_class_is_unknown", as_bool_string(true));
        if (!process_class->process_available(this)) {
            xml::Element_ptr obstacle = result.append_new_element(obstacle_element_name);
            obstacle.setAttribute("process_class", process_class->path());
            obstacle.setAttribute("process_of_process_class_available", as_bool_string(false));
        }
    }

    return result;
}

vector<string> Standard_job::unavailable_lock_path_strings() const {
    if (_lock_requestor && _lock_requestor->is_enqueued()) 
        return _lock_requestor->unavailable_lock_path_strings((lock::Holder*)NULL);
    else
        return vector<string>();
}

//------------------------------------------------------Standard_job::append_calendar_dom_elements

void Standard_job::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    if( _state == s_pending  ||  _state == s_running )
    {
        xml::Node_ptr node_before = element.lastChild();

        _schedule_use->append_calendar_dom_elements( element, options );

        for( xml::Simple_node_ptr node = node_before? node_before.nextSibling() : element.firstChild();
             node;
             node = node.nextSibling() )
        {
            if( xml::Element_ptr e = xml::Element_ptr( node, xml::Element_ptr::no_xc ) )
            {
                e.setAttribute( "job", path() );
            }
        }

        _task_queue->append_calendar_dom_elements( element, options );
    }
}

//---------------------------------------------------------------------Standard_job::time_zone_name

string Standard_job::time_zone_name() const
{ 
    return _schedule_use->time_zone_name(); 
}

//--------------------------------------------------------------------Standard_job::try_to_end_task

bool Standard_job::try_to_end_task(Process_class_requestor* for_requestor, Process_class* process_class) 
{
    Z_FOR_EACH(Task_set, _running_tasks, i) {
        Task* task = *i;
        if (task->is_idle() && task->process_class_or_null() == process_class) {
            task->cmd_nice_end(for_requestor);
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------kill_queued_task

void Standard_job::kill_queued_task( int task_id )
{
    bool ok = _task_queue->remove_task( task_id, Task_queue::w_task_killed );
    if (ok) 
        _call_register.call_at<Start_queued_task_call>(_task_queue->next_start_time());
}

//--------------------------------------------------------------------------Standard_job::kill_task

void Standard_job::kill_task(int id, bool immediately, const Duration& timeout)
{
    Z_FOR_EACH( Task_set, _running_tasks, t ) {
        if( (*t)->_id == id ) { 
            if (immediately) (*t)->set_killed_immediately_by_command();
            (*t)->cmd_end(immediately? task_end_kill_immediately : task_end_normal, timeout);       // Ruft kill_queued_task()
            return;
        }
    }

    kill_queued_task( id );
}

//-------------------------------------------------------------Standard_job::create_module_instance

ptr<Module_instance> Standard_job::create_module_instance(Process_class* process_class, const string& remote_scheduler, Task* task)
{
    ptr<Module_instance>  result;

    {
        if( _state == s_error      )  z::throw_xc( "SCHEDULER-204", name(), _error.what() );

        result = _module->create_instance(process_class, remote_scheduler, task);

        if( result )
        {
            result->set_job_name( name() ); 
            result->set_log( _log );
        }
    }

    return result;
}


Process_class* Standard_job::default_process_class_or_null() const { 
    return _spooler->_ignore_process_classes ? NULL
        :_spooler->process_class_subsystem()->process_class_or_null(_default_process_class_path);
}

Process_class* Standard_job::default_process_class() const {
    return _spooler->process_class_subsystem()->process_class(_default_process_class_path);
}

bool Standard_job::is_task_ready_for_order(Process_class* process_class) {
    if (!process_class) {
        process_class == default_process_class_or_null();
        if (!process_class) return false;
    }
    Z_FOR_EACH_CONST(Task_set, _running_tasks, i) {
        Task* task = *i;
        if (task->is_ready_for_order(process_class)) return true;
    }
    return false;
}

//------------------------------------------------------------------------nternal_job::Internal_job

Internal_job::Internal_job( Scheduler* scheduler, const string& name, const ptr<Module>& module )
:
    Standard_job( scheduler, name, module )
{
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
