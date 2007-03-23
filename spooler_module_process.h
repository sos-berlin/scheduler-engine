// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_MODULE_PROCESS_H
#define __SPOOLER_MODULE_PROCESS_H

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------Process_module_instance

struct Process_module_instance : Module_instance
{
#   ifndef Z_WINDOWS
        struct Process_event : Event
        {
                                        Process_event               ( const string& name )                  : Event(name) {}

            virtual void                close                       ();
            bool                        signaled                    ();
            virtual bool                wait                        ( double seconds );
                                        operator bool               ()                                      { return _pid != 0; }

            int                        _pid;
            int                        _process_signaled;
            int                        _process_exit_code;
        };
#   endif


                                Process_module_instance     ( Module* );
                               ~Process_module_instance     ();

    virtual void                attach_task                 ( Task*, Prefix_log* );

    void                        init                        ();
    bool                        load                        ();
    void                        start                       ();
    void                        close__end                  ();
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
    int                         exit_code                   ()                                      { return _exit_code; }
    int                         termination_signal          ();
    File_path                   stdout_path                 ()                                      { return _stdout_file.path(); }
    File_path                   stderr_path                 ()                                      { return _stderr_file.path(); }

  private:
    void                        close_handle                ();
    string                      program_path                ();


    Fill_zero                  _zero_;
    File                       _stdout_file;
    File                       _stderr_file;
  //bool                       _stdout_logged;
    bool                       _is_killed;
    File                       _shell_file;
    string                     _process_param;
    ptr<Com_variable_set>      _process_environment;

#   ifdef Z_WINDOWS
        //Process_id             _pid;
        Event                  _process_handle;
#    else
        Process_event          _process_handle;
        int                    _pid_to_unregister;
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
