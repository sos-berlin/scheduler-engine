// $Id: spooler_thread.h,v 1.2 2001/02/06 12:08:44 jz Exp $

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
    void                        load_jobs_from_xml          ( Job_list*, const xml::Element_ptr& );

    void                        init                        ();
    void                        close                       ();
    void                        start_thread                ();
    void                        stop_thread                 ();
    Job*                        get_job_or_null             ( const string& job_name );
    void                        signal                      ()                              { _event.signal(); }

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
    bool                       _stop;

                                                            // Statistik
    int                        _step_count;                 // Seit Spooler-Start ausgeführte Schritte
    int                        _task_count;                 // Seit Spooler-Start gestartetet Tasks

    Handle                     _thread;
    Thread_id                  _thread_id;
    CComPtr<Com_log>           _com_log;                    // COM-Objekt spooler.log
};

typedef list< Sos_ptr<Thread> >  Thread_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
