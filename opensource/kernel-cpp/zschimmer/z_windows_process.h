// $Id: z_windows_process.h 13766 2009-03-20 16:10:47Z jz $

#ifndef __ZSCHIMMER_WINDOWS_PROCESS_H
#define __ZSCHIMMER_WINDOWS_PROCESS_H

#include "z_windows.h"
#include "Login.h"

namespace zschimmer {
namespace windows {

//-------------------------------------------------------------------------------------------------
    
bool                            try_kill_process_immediately( Process_handle, const string& debug_string = "" );
void                            kill_process_immediately    ( Process_handle, const string& debug_string = "" );
void                            kill_process_immediately    ( pid_t         , const string& debug_string = "" );
void                            try_kill_process_with_descendants_immediately( pid_t, Has_log* log, const Message_string* msg, const string& debug_string );
int                             priority_class_from_string  ( const string& );

//------------------------------------------------------------------------------------------Process

struct Process : Process_base, Handle
{
                                Process                     ();
                                Process                     ( int pid )                             : _zero_(this+1) { init(); attach_pid( pid ); }

    void                        init                        ();

    STDMETHODIMP            get_Priority_class              ( BSTR* result )                        { Z_COM_IMPLEMENT( hr = com::String_to_bstr( priority_class(), result ) ); } 

    void                        close                       ();
    void                        start                       ( const string& command_line );
    void                        start                       ( const std::vector<string>& program_and_parameters );
  //void                    set_startup_info                ( const STARTUP_INFO si )               { _startup_info = si; );
    void                        attach_handle               ( HANDLE handle, int pid )              { _handle = handle;  _pid = pid; }
    void                        attach_pid                  ( int pid );
    bool                    set_priority                    ( int );          //, Has_log* = NULL );
    int                         priority                    ();
  //virtual void            set_with_console_window         ( bool b )                              { _with_console_window = b; }
    bool                    set_priority_class              ( int );          //, Has_log* = NULL );// NORMAL_PRIORITY_CLASS etc.
    bool                    set_priority_class              ( const string& );//, Has_log* = NULL );
    string                      priority_class              ();               //  Has_log* = NULL );
  //bool                        raise_priority              ( int difference );
    void                        wait                        ();
    bool                        wait                        ( double seconds );                     // Nicht in Unix
    bool                        terminated                  ();
    int                         exit_code                   ();
    int                         termination_signal          ()                                      { return 0; }   // Windows kennt nur exit_code
    void                        kill                        ( int signal = 99 );
    bool                        try_kill                    ( int signal = 99 );
    HANDLE                      take_handle                 ()                                      { HANDLE result = _handle;  _handle = NULL, _pid = 0;  return result; }
    bool                        opened                      () const                                { return _handle != 0; }
    static void                 create_process              (const Login*, const string& application_name, const string& command_line, 
                                                             DWORD creationFlags, BSTR environment, STARTUPINFOW*, PROCESS_INFORMATION*);

    Fill_zero                  _zero_;
    STARTUPINFO                _startup_info;
    
  private:
    string                      make_environment_string     ();

    int                        _priority_class;
  //bool                       _with_console_window;
  //bool                       _raise_priority_set;
  //int                        _raise_priority;
};

//--------------------------------------------------------------------------------------Process_ref
/*
struct Process_ref : Process
{
                                Process_ref                 ( HANDLE handle )                       { attach_handle( handle ); }
                               ~Process_ref                 ()                                      { _handle = NULL; }
};
*/
//-------------------------------------------------------------------------------------------------

} //namespace windows

//-------------------------------------------------------------------------------------------------
} //namespace zschimmer

#endif
