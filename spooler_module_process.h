// $Id: spooler_module_com.h 3978 2005-10-26 21:03:35Z jz $

#ifndef __SPOOLER_MODULE_PROCESS_H
#define __SPOOLER_MODULE_PROCESS_H

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------Process_module_instance

struct Process_module_instance : Module_instance
{
                                Process_module_instance     ( Module* );
                               ~Process_module_instance     ();

    void                        init                        ();
    bool                        load                        ();
    void                        start                       ();
    void                        close__end                  ();
    bool                        begin__end                  ();
    void                        end__end                    ();
    Variant                     step__end                   ();
    Variant                     call                        ( const string& name );
    Variant                     call                        ( const string& name, bool param );
    bool                        name_exists                 ( const string& name );
    bool                        loaded                      ()                                      { return _idispatch != NULL; }
    bool                        callable                    ()                                      { return _idispatch != NULL; }
    bool                        kill                        ();
    bool                        process_has_signaled        ();
    int                         exit_code                   ()                                      { return _exit_code; }
    int                         termination_signal          ();
    string                      stdout_filename             ()                                      { return _stdout_file.filename(); }
    string                      stderr_filename             ()                                      { return _stderr_file.filename(); }

  private:
    void                        close_handle                ();
    string                      program_path                ();
    ptr<Com_variable_set>       variable_set_from_environment();


    Fill_zero                  _zero_;
    File                       _stdout_file;
    File                       _stderr_file;
  //bool                       _stdout_logged;
    int                        _exit_code;
    bool                       _is_killed;
    File                       _shell_file;

#   ifdef Z_WINDOWS
        //Process_id             _pid;
        Event                  _process_handle;
#    else
        Process_event          _process_handle;
        int                    _pid_to_unregister;
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
