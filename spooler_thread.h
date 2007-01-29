// $Id$

#ifndef __SPOOLER_THREAD_H
#define __SPOOLER_THREAD_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------------Task_subsystem

struct Task_subsystem : Subsystem
{
    Fill_zero                  _zero_;

    Z_GNU_ONLY(                 Task_subsystem              (); )                                   // Für gcc 3.2.1
                                Task_subsystem              ( Spooler* );
                               ~Task_subsystem              ();


    virtual void                close                       ()                                      {}

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    
    bool                        has_tasks                   ()                                      { return !_task_list.empty(); }

    bool                        process                     ( const Time& now );                    // Einen Schritt im (Pseudo-)Thread ausführen
    void                        add_task                    ( Task* task )                          { _task_list.push_back( task );  signal( task->obj_name() ); }

    ptr<Task>                   get_task_or_null            ( int task_id );
    Task*                       get_next_task               ();

    void                        increment_running_tasks     ();
    void                        decrement_running_tasks     ();
    void                        count_started_tasks         ();
    void                        count_finished_tasks        ();
    int                         finished_tasks_count        () const                                { return _finished_tasks_count; }

    void                        count_task                  ()                                      { InterlockedIncrement( &_task_count ); }
    void                        count_step                  ()                                      { InterlockedIncrement( &_step_count ); }

    bool                        try_to_free_process         ( Job*, Process_class*, const Time& now );

    // Für andere Threads:
    bool                        is_ready_for_termination    ();
    void                        signal                      ( const string& signal_name )           { if( _event )  _event->signal(signal_name); }

    virtual string             _obj_name                    () const                                { return "Task_subsystem"; }



//private:
    bool                        step                        ( const Time& now );
    bool                        do_something                ( Task*, const Time& now );
    void                        wait                        ();
    Task*                       get_next_task_to_run        ();
    void                        remove_ended_tasks          ();
    void                        build_prioritized_order_job_array();




    Event*                     _event;

    Task_list                  _task_list;
    bool                       _task_closed;

    vector<Job*>               _prioritized_order_job_array;            // Jobs am Ende einer Jobkette priorisieren
    int                        _prioritized_order_job_array_version;    // Wenn verschieden von _spooler->_job_chain_map, _prioritized_order_job_array neu aufbauen!

    long32                     _running_tasks_count;        // Wenn 0, dann warten
                                                            // Statistik
    long32                     _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    long32                     _task_count;                 // Seit Spooler-Start gestartetet Tasks

    int                        _started_tasks_count;
    int                        _finished_tasks_count;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
