// $Id: spooler_thread.h,v 1.34 2002/12/01 08:57:46 jz Exp $

#ifndef __SPOOLER_THREAD_H
#define __SPOOLER_THREAD_H

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------Spooler_thread

struct Spooler_thread : zschimmer::Thread
{
                                Spooler_thread              ( Spooler* );
                               ~Spooler_thread              ();

    void                        set_dom                     ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );
    void                        load_jobs_from_xml          ( const xml::Element_ptr&, const Time& xml_mod_time, bool init = false );
    void                        add_job                     ( const Sos_ptr<Job>& );
    void                        build_prioritized_order_job_array();
    
    bool                        empty                       () const                        { return _job_list.empty(); }
    const string&               name                        () const                        { return _name; }
    Job*                        current_job                 () const                        { return _current_job; }
    string                      include_path                () const                        { return _include_path; }
    bool                        any_tasks_there             ();
    bool                        has_java                    ();

    void                        init                        ();
    void                        close1                      ();                             // Wird vom Thread beim Beenden selbst gerufen
    void                        close                       ();                             // Wird vom Spooler gerufen
    void                        start_thread                ();

    virtual int                 main                        ()                              { return run_thread(); }
    int                         run_thread                  ();
  //bool                        running                     ()                              { DWORD rc; return GetExitCodeThread(_thread_handle,&rc)? rc == STILL_ACTIVE : false; }
    bool                        process                     ();                             // Einen Schritt im (Pseudo-)Thread ausführen
    void                        start                       ();
    void                        stop_jobs                   ();
    bool                        step                        ();
    bool                        do_something                ( Job* );
    void                        wait                        ();

  //Time                        next_start_time             ();
    Job*                        get_next_job_to_start       ();

    bool                        finished                    ();

    void                        do_add_jobs                 ();
    void                        remove_temporary_jobs       ();

    // Für andere Threads:
    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        signal                      ( const string& signal_name = "" )  { THREAD_LOCK( _lock )  _event.signal(signal_name), _next_start_time = 0, _next_job = NULL; }
    Job*                        get_job_or_null             ( const string& job_name );
  //void                        wait_until_thread_stopped   ( Time until );
  //void                        interrupt_scripts           ();
    void                        cmd_add_jobs                ( const xml::Element_ptr& );
    void                        nichts_getan                ( double wait_time );

    virtual string             _obj_name                    () const                        { return "Thread" + _name; }


    Fill_zero                  _zero_;

    string                     _name;
    Spooler*                   _spooler;
    Thread_semaphore           _lock;
    Prefix_log                 _log;

    string                     _include_path;
    bool                       _free_threading;             // Dieser Thread soll frei, ohne _serialize_lock laufen.

  //Handle                     _thread_handle;
  //Thread_id                  _thread_id;
    int                        _thread_priority;

    Event                      _event;
    Wait_handles               _wait_handles;

    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log
    ptr<Com_thread>            _com_thread;                 // COM-Objekt

    Module                     _module;                     // <script>
    ptr<Module_instance>       _module_instance;

    Job_list                   _job_list;
    vector<Job*>               _prioritized_order_job_array;
    Time                       _prioritized_order_job_array_time;
    Job*                       _current_job;                // Job, der gerade einen Schritt tut

    Time                       _next_start_time;
    Job*                       _next_job;
    int                        _running_tasks_count;        // Wenn 0, dann warten
                                                            // Statistik
    int                        _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    int                        _task_count;                 // Seit Spooler-Start gestartetet Tasks

    int                        _nothing_done_count;
    int                        _nothing_done_max;

    bool                       _terminated;

  private:
                                Spooler_thread              ( const Spooler_thread& );      // Nicht implementiert
    Spooler_thread&             operator =                  ( const Spooler_thread& );      // Nicht implementiert

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
