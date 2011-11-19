// $Id: z_process.h 13677 2008-09-27 20:17:00Z jz $

#ifndef __ZSCHIMMER_PROCESS_H
#define __ZSCHIMMER_PROCESS_H

#include "z_com_server.h"


namespace zschimmer
{

//-------------------------------------------------------------------------------------------------

bool                            try_kill_process_immediately( pid_t pid, const string& debug_string = "" );

// Mehr siehe unten.

//-------------------------------------------------------------------------------------Process_base
    
struct Process_base : Object
{
    typedef stdext::hash_map<string,string> Environment;

                                Process_base                ()                                      : _zero_(this+1), _inherit_environment(true) {}
    virtual                    ~Process_base                ();

    virtual int                 pid                         ()                                      { return _pid; }
    virtual string              command_line                () const                                { return _command_line; }
    string                      obj_name                    () const;

    bool                        started                     () const                                { return _pid != 0; }
    void                        assert_not_started          ()                                      { if( started()    )  throw_xc( "Z-PROCESS-002" ); }
    void                        assert_terminated           ()                                      { if( !_terminated )  throw_xc( "Z-PROCESS-001", _pid ); }
    virtual bool            set_priority                    ( int )                                 = 0;
    virtual int                 priority                    ()                                      = 0;
    virtual bool            set_priority_class              ( const string& )                       = 0;
    virtual string              priority_class              ()                                      = 0;
    virtual bool                terminated                  ()                                      = 0;
    virtual int                 exit_code                   ()                                      = 0;
    virtual int                 termination_signal          ()                                      { assert_terminated(); return 0; }
    virtual void                kill                        ( int )                                 = 0;
  //virtual void            set_with_console_window         ( bool )                                {}

    void                    set_stdout_path                 ( const string& path )                  { assert_not_started();  _stdout_path = path; }
    string                      stdout_path                 ()                                      { return _stdout_path; }
    void                    set_stderr_path                 ( const string& path )                  { assert_not_started();  _stderr_path = path; }
    string                      stderr_path                 ()                                      { return _stderr_path; }
    void                    set_environment_entry           ( const string& name, const string& value ) { _environment[ name ] = value; }
  //virtual Environment&        environment                 ()                                      { return _environment; }
    void                    set_inherit_environment         ( bool b )                              { _inherit_environment = b; }
    void                    set_own_process_group           ( bool );
    bool                        own_process_group           () const                                { return _own_process_group; }


  //STDMETHODIMP                Raise_priority              ( int diff, VARIANT_BOOL* result )      { Z_COM_IMPLEMENT( *result = _process.raise_priority( diff )? VARIANT_TRUE : VARIANT_FALSE ); }
  //STDMETHODIMP                Lower_priority              ( int diff, VARIANT_BOOL* result )      { return Raise_priority( -diff, result ); }

    virtual STDMETHODIMP    get_Pid                         ( int* result )                         { Z_COM_IMPLEMENT( *result = pid() ) }
    virtual STDMETHODIMP    get_Terminated                  ( VARIANT_BOOL* result )                { Z_COM_IMPLEMENT( *result = terminated()? VARIANT_TRUE: VARIANT_FALSE ) }
    virtual STDMETHODIMP    get_Exit_code                   ( int* result )                         { Z_COM_IMPLEMENT( *result = exit_code() ) }
    virtual STDMETHODIMP    get_Termination_signal          ( int* result )                         { Z_COM_IMPLEMENT( *result = termination_signal() ) }
    virtual STDMETHODIMP    put_Priority                    ( int priority )                        { Z_COM_IMPLEMENT( set_priority( priority ) ) } 
    virtual STDMETHODIMP    get_Priority                    ( int* result )                         { Z_COM_IMPLEMENT( *result = priority() ) } 
    virtual STDMETHODIMP    put_Priority_class              ( BSTR priority )                       { Z_COM_IMPLEMENT( set_priority_class( com::string_from_bstr( priority ) ) ) } 
    virtual STDMETHODIMP    get_Priority_class              ( BSTR* result )                        { Z_COM_IMPLEMENT( return com::String_to_bstr( priority_class(), result ) ) } 
    virtual STDMETHODIMP    put_Own_process_group           ( VARIANT_BOOL b )                      { set_own_process_group( b != VARIANT_FALSE ); return S_OK; } 
    virtual STDMETHODIMP    get_Own_process_group           ( VARIANT_BOOL* result )                { *result = own_process_group()? VARIANT_TRUE : VARIANT_FALSE; return S_OK; } 
    virtual STDMETHODIMP        Kill                        ( int kill_signal )                     { Z_COM_IMPLEMENT( kill( kill_signal ) ); }

  protected:
    Fill_zero                  _zero_;
    int                        _pid;
    bool                       _terminated;
    string                     _command_line;               // Fürs Log
    string                     _stdout_path;
    string                     _stderr_path;
    Environment                _environment;
    bool                       _inherit_environment;
    bool                       _own_process_group;
};

//-------------------------------------------------------------------------------------------------

} // namespace zschimmer

#ifdef Z_WINDOWS
#   include "z_windows_process.h"
#else
#   include "z_posix_process.h"
#endif

namespace zschimmer
{

//-------------------------------------------------------------------------------------------------

using system_interface::Process;
using system_interface::kill_process_immediately;               // Variante mit pid

#ifdef Z_WINDOWS
    using zschimmer::windows::kill_process_immediately;         // Variante mit HANDLE
    using zschimmer::windows::try_kill_process_immediately;     // Variante mit HANDLE
#endif

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
