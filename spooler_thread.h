// $Id: spooler_thread.h,v 1.55 2004/03/29 22:03:14 jz Exp $

#ifndef __SPOOLER_THREAD_H
#define __SPOOLER_THREAD_H

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------Spooler_thread

struct Spooler_thread : zschimmer::Thread
{
    Fill_zero                  _zero_;

    Z_GNU_ONLY(                 Spooler_thread              (); )                                   // Für gcc 3.2.1
                                Spooler_thread              ( Spooler* );
                               ~Spooler_thread              ();

  //void                        set_dom                     ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );
    
    const string&               name                        () const                                { return _name; }
    void                    set_name                        ( const string& name )                  { _name = name; set_thread_name( _name ); _log.set_prefix( _obj_name() ); }
  //Job*                        current_job                 () const                                { return _current_job; }
  //string                      include_path                () const                                { return _include_path; }
    bool                        has_tasks                   ()                                      { return !_task_list.empty(); }

    void                        init                        ();
    void                        close1                      ();                                     // Wird vom Thread beim Beenden selbst gerufen
    void                        close                       ();                                     // Wird vom Spooler gerufen
    void                        start_thread                ();
    bool                        terminated                  ()                                      { return _terminated; }

    virtual int                 thread_main                 ();
  //bool                        running                     ()                                      { DWORD rc; return GetExitCodeThread(_thread_handle,&rc)? rc == STILL_ACTIVE : false; }
    bool                        process                     ();                                     // Einen Schritt im (Pseudo-)Thread ausführen
    void                        start                       ( Event* destination );
  //void                        stop_jobs                   ();

    void                        cmd_shutdown                ();
    void                        add_task                    ( Task* task )                          { _task_list.push_back( task );  signal( task->obj_name() ); }
  //void                        remove_task                 ( Task* this_task )                     { FOR_EACH_TASK( t, task )  if( task == this_task )  { _task_list.erase(t);  break; } }
    int                         task_count                  ( Job* = NULL );

    Sos_ptr<Task>               get_task_or_null            ( int task_id );
    Task*                       get_next_task               ();

    void                        increment_running_tasks     ()                                      { InterlockedIncrement( &_running_tasks_count ); }
    void                        decrement_running_tasks     ()                                      { InterlockedDecrement( &_running_tasks_count ); }

    void                        count_task                  ()                                      { InterlockedIncrement( &_task_count ); }
    void                        count_step                  ()                                      { InterlockedIncrement( &_step_count ); }

  //Time                        next_time                   ()                                      { THREAD_LOCK_RETURN( _lock, Time, _next_time ); }

 //?bool                        finished                    ();
    bool                        try_to_free_process         ( Job*, Process_class*, const Time& now );

    // Für andere Threads:
    bool                        is_ready_for_termination    ();
    void                        signal_object               ( const string& object_set_class_name, const Level& );
  //void                        signal                      ( const string& signal_name )           { THREAD_LOCK( _lock )  if(_event) _event->signal(signal_name), _next_time = 0; }
    void                        signal                      ( const string& signal_name )           { THREAD_LOCK( _lock )  if(_event) _event->signal(signal_name); }
  //Job*                        get_job_or_null             ( const string& job_name );
  //void                        interrupt_scripts           ();
    void                        nichts_getan                ();

    virtual string             _obj_name                    () const                                { return "Thread " + _name; }



  //Wait_handles               _wait_handles;
    Task*                      _current_task;               // Task, die gerade einen Schritt tut
    bool                       _free_threading;

  private:
    bool                        step                        ();
    bool                        do_something                ( Task* );
    void                        wait                        ();
    Task*                       get_next_task_to_run        ();
    void                        remove_ended_tasks          ();
    void                        build_prioritized_order_job_array();




    string                     _name;
    Spooler*                   _spooler;
    Thread_semaphore           _lock;
    Prefix_log                 _log;

  //string                     _include_path;

    int                        _thread_priority;

    Event*                     _event;
  //Event                      _my_event;                   // Für _free_threading

  //ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log
  //ptr<Com_thread>            _com_thread;                 // COM-Objekt

  //Module                     _module;                     // <script>
  //ptr<Module_instance>       _module_instance;

    ptr<object_server::Session> _object_server_session;

  //Job_list                   _job_list;
    Task_list                  _task_list;
    bool                       _task_closed;

    vector<Job*>               _prioritized_order_job_array;        // Jobs am Ende einer Jobkette priorisieren
    Time                       _prioritized_order_job_array_time;

    typedef list<object_server::Session*>   Session_list;
    Session_list               _session_list;

 //?Task*                      _next_task;
    Time                       _next_time;

    long                       _running_tasks_count;        // Wenn 0, dann warten
                                                            // Statistik
    long                       _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    long                       _task_count;                 // Seit Spooler-Start gestartetet Tasks

  //int                        _nothing_done_count;
  //int                        _nothing_done_max;

    bool                       _terminated;
};

typedef list< ptr<Spooler_thread> >  Thread_list;

/*
    Threads und Thread::_job_list:
        Jobs werden nur vom eigenen Thread gelöscht: in remove_temporary_jobs(), close() und stop().
        Jobs werden auch von anderen Threads hinzugefügt: in do_add_jobs().
        Beim Lesen der _job_list muss darauf geachtet werden, dass jederzeit Jobs hinzugefügt werden können.
        Wenn ein anderer Thread die _job_list liest, muss über die Schleife eine Semaphore gelegt werden, weil
        Jobs vom besitzenden Thread gelöscht werden können.
*/

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
