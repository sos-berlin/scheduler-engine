// $Id: z_posix_process.h 13678 2008-09-28 04:57:24Z jz $

#ifndef __ZSCHIMMER_WINDOWS_PROCESS_H
#define __ZSCHIMMER_WINDOWS_PROCESS_H


#include "z_posix.h"
#include <signal.h>             // SIGKILL


namespace zschimmer {

//-------------------------------------------------------------------------------------------------
namespace posix {
    
std::vector<string>             argv_from_command_line      ( const string& command_line );
//string                          shell_command_line_from_argv( int argc, char** argv );              // Erstmal nur zur Debug-Ausgabe
string                          shell_command_line_from_argv( const std::vector<string>& );         // Erstmal nur zur Debug-Ausgabe
int                             priority_from_string        ( const string& );
int                             z_clearenv                  ();

void                            kill_process_group_immediately    ( Process_group_handle, const string& debug_string = "" );
void                            kill_process_immediately          ( Process_group_handle, const string& debug_string = "" );
bool                            try_kill_process_group_immediately( Process_group_handle, const string& debug_string = "" );
bool                            try_kill_process_with_descendants_immediately( pid_t, Has_log* log, const Message_string* = NULL, const string& debug_string = "" );

//------------------------------------------------------------------------------------------Process

struct Process : Process_base
{
                                Process                     ();
                                Process                     ( int pid )                             : _zero_(this+1), _stdout_handle(-1) { attach_pid( pid ); }
    virtual                    ~Process                     ();

    void                        close                       ();                                
    void                        start                       ( const string& command );
    void                        start                       ( const std::vector<string>& command );
    void                        attach_pid                  ( int pid )                             { close();  _pid = pid; }
    void                    set_stdout_handle               ( int f )                               { _stdout_handle = f; }
    bool                    set_priority                    ( int ); //, Has_log* = NULL );
    int                         priority                    ();
    bool                    set_priority_class              ( const string& ); //, Has_log* = NULL );
    string                      priority_class              ();
    void                        kill_at_end_with            ( int signal )                          { _kill_at_end_signal = signal; }
  //bool                        raise_priority              ( int difference );
    void                        wait                        ();
    bool                        wait                        ( double seconds );
    bool                        terminated                  ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    void                        kill                        ( int signal = SIGKILL );
    bool                        try_kill                    ( int signal = SIGKILL );
    bool                        opened                      () const                                { return _working_pid != 0; }
  //string                      obj_name                    () const;


  private:
    bool                        call_waitpid                ( bool wait );


    Fill_zero                  _zero_;
    int                        _working_pid;
    int                        _exit_code;
    int                        _termination_signal;
    
    bool                       _priority_set;
    int                        _priority;
  //bool                       _raise_priority_set;
  //int                        _raise_priority;
    int                        _stdout_handle;
    int                        _kill_at_end_signal;
};

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
