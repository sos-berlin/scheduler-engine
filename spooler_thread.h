// $Id: spooler_thread.h,v 1.12 2001/07/16 16:39:36 jz Exp $

#ifndef __SPOOLER_THREAD_H
#define __SPOOLER_THREAD_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------Thread

struct Thread : Sos_self_deleting
{
                                Thread                      ( Spooler* );
                               ~Thread                      ();

    void                        set_xml                     ( const xml::Element_ptr& );
    xml::Element_ptr            xml                         ( xml::Document_ptr );
    void                        load_jobs_from_xml          ( const xml::Element_ptr&, bool init = false );
    void                        add_job                     ( const Sos_ptr<Job>& );
    
    bool                        empty                       () const                        { return _job_list.empty(); }
    const string&               name                        () const                        { return _name; }
    Job*                        current_job                 () const                        { return _current_job; }
    string                      include_path                () const                        { return _include_path; }

    void                        init                        ();
    void                        close                       ();
    void                        start_thread                ();

    int                         run_thread                  ();
    void                        start                       ();
    void                        stop_jobs                   ();
    bool                        step                        ();
    bool                        do_something                ( Job* );
    void                        wait                        ();

    void                        do_add_jobs                 ();
    void                        remove_temporary_jobs       ();

    // Für andere Threads:
    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        signal                      ( const string& signal_name = "" )  { _event.signal(signal_name); }
    Job*                        get_job_or_null             ( const string& job_name );
  //void                        wait_until_thread_stopped   ( Time until );
  //void                        interrupt_scripts           ();
    void                        cmd_add_jobs                ( const xml::Element_ptr& );

    virtual string             _obj_name                    () const                        { return "Thread" + _name; }


    Fill_zero                  _zero_;

    string                     _name;
    Spooler*                   _spooler;

    Wait_handles               _wait_handles;
    Event                      _event;
    Job_list                   _job_list;
    Job*                       _current_job;                // Job, der gerade einen Schritt tut
    Script                     _script;                     // <script>
    Script_instance            _script_instance;
    Prefix_log                 _log;
    string                     _include_path;

    Time                       _next_start_time;
    int                        _running_tasks_count;        // Wenn 0, dann warten
                                                            // Statistik
    int                        _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    int                        _task_count;                 // Seit Spooler-Start gestartetet Tasks

    Thread_semaphore           _lock;
    bool                       _free_threading;             // Dieser Thread soll frei, ohne _serialize_lock laufen.
    Handle                     _thread_handle;
    Thread_id                  _thread_id;
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log
    CComPtr<Com_thread>        _com_thread;                 // COM-Objekt

  private:
                                Thread                      ( const Thread& );      // Nicht implementiert
    Thread&                     operator =                  ( const Thread& );      // Nicht implementiert
};

typedef list< Sos_ptr<Thread> >  Thread_list;

/*
    Threads und Thread::_job_list:
        Jobs werden nur vom eigenen Thread gelöscht: in remove_temporary_jobs(), close() und stop().
        Jobs werden auch von anderern Threads hinzugefügt: in do_add_jobs().
        Beim Lesen der _job_list muss darauf geachtet werden, dass jederzeit Jobs hinzugefügt werden können.
        Wenn ein anderer Thread die _job_list liest, muss über die Schleife eine Semaphore gelegt werden, weil
        Jobs vom besitzenden Thread gelöscht werden können.
*/

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
