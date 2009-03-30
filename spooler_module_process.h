// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_MODULE_PROCESS_H
#define __SPOOLER_MODULE_PROCESS_H

namespace sos {
namespace scheduler {

struct Process_module_instance_operation;

//--------------------------------------------------------------------------Process_module_instance

struct Process_module_instance : Module_instance
{
#   ifndef Z_WINDOWS
        struct Process_event : Event
        {
                                        Process_event               ( const string& name )                  : Event(name) {}

            virtual void                close                       ();
            bool                        signaled                    ();
            virtual bool                wait                        ( double seconds = INT_MAX );
                                        operator bool               ()                                      { return _pid != 0; }

            int                        _pid;
            int                        _process_signaled;
            int                        _process_exit_code;
        };
#   endif


                                Process_module_instance     ( Module* );
                               ~Process_module_instance     ();

  //virtual void                attach_task                 ( Task*, Prefix_log* );

    void                        init                        ();
    bool                        load                        ();
    void                        start                       ();
    void                        close__end                  ();
    void                        close_process               ();
    Async_operation*            begin__start                ();
    bool                        begin__end                  ();
    void                        end__end                    ();
    Variant                     step__end                   ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, const Variant& param, const Variant& );
    bool                        name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _idispatch != NULL; }
    bool                        callable                    ()                                      { return _idispatch != NULL; }
    bool                        kill                        ();
    bool                        process_has_signaled        ();
    string                      get_first_line_as_state_text();
    void                        fill_process_environment_with_params();
    string                      step_result                 ();
    void                        transfer_back_order_params  ();
    void                        fetch_parameters_from_process( Com_variable_set* );
    File_path                   stdout_path                 ()                                      { return _stdout_file.path(); }
    File_path                   stderr_path                 ()                                      { return _stderr_file.path(); }
    bool                        try_delete_files            ( Has_log* );
    std::list<File_path>        undeleted_files             ();
    bool                        is_kill_thread_running      ()                                      { return _kill_thread  &&  _kill_thread->thread_is_running(); }

  private:
    friend struct               Process_module_instance_operation;

    void                        close_handle                ();
    string                      program_path                ();


    Fill_zero                  _zero_;
    File                       _stdout_file;
    File                       _stderr_file;
  //bool                       _stdout_logged;
    Mapped_file                _order_params_file;

    bool                       _is_killed;
    File                       _shell_file;
    string                     _process_param;

#   ifdef Z_WINDOWS
        //Process_id             _pid;
        Event                  _process_handle;
#    else
        Process_event          _process_handle;
        int                    _pid_to_unregister;
#   endif

    ptr<Thread>                _kill_thread;
    bool                       _spooler_process_before_called;
    ptr<Process_module_instance_operation> _operation;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
