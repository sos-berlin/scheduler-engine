// $Id: spooler_thread.h,v 1.5 2001/02/12 15:41:39 jz Exp $

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
    void                        load_jobs_from_xml          ( Job_list*, const xml::Element_ptr& );
    
    bool                        empty                       () const                        { return _job_list.empty(); }

    void                        init                        ();
    void                        close                       ();
    void                        start_thread                ();
  //void                        stop_thread                 ();
    void                        wait_until_thread_stopped   ( Time until );
    void                        interrupt_scripts           ();
    Job*                        get_job_or_null             ( const string& job_name );
    void                        signal_object               ( const string& object_set_class_name, const Level& );
    void                        signal                      ( const string& signal_name = "" )  { _event.signal(signal_name); }

    int                         run_thread                  ();
    void                        start                       ();
    void                        stop                        ();
    bool                        step                        ();
    void                        wait                        ();



    Fill_zero                  _zero_;

    string                     _name;
    Spooler*                   _spooler;

    Wait_handles               _wait_handles;
    Event                      _event;
    Job_list                   _job_list;
    Script                     _script;                     // <script>
    Script_instance            _script_instance;
    Prefix_log                 _log;

    Time                       _next_start_time;
    int                        _running_tasks_count;        // Wenn 0, dann warten
  //bool                       _stop;

                                                            // Statistik
    int                        _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    int                        _task_count;                 // Seit Spooler-Start gestartetet Tasks

    Thread_semaphore           _lock;
    Handle                     _thread_handle;
    Thread_id                  _thread_id;
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log
    CComPtr<Com_thread>        _com_thread;                 // COM-Objekt
};

typedef list< Sos_ptr<Thread> >  Thread_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
