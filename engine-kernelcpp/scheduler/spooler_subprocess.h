// $Id: spooler_subprocess.h 13198 2007-12-06 14:13:38Z jz $

#ifndef __SPOOLER_SUBPROCESS_H
#define __SPOOLER_SUBPROCESS_H

#include "../zschimmer/z_process.h"


namespace sos {
namespace scheduler {


struct Process_class;


//---------------------------------------------------------------------------------------Subprocess
// Ein vom einem Job gestarteter Prozess (mit irgendeinem fremden Programm).

struct Subprocess : idispatch_implementation< Subprocess, spooler_com::Isubprocess >
{
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];


                                Subprocess                  ( Subprocess_register*, IDispatch* task );

    Z_GNU_ONLY(                 Subprocess                  (); )
                               ~Subprocess                  ();


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Subprocess"; }


    // interface Isubprocess
    STDMETHODIMP                Close                       ()                                      { Z_COM_IMPLEMENT( close() ); }
    STDMETHODIMP                Start                       ( VARIANT* command_line );              // BSTR oder Array
    STDMETHODIMP            put_Priority                    ( int priority )                        { return _process.put_Priority( priority ); }
    STDMETHODIMP            get_Priority                    ( int* result )                         { return _process.get_Priority( result ); }
    STDMETHODIMP            put_Priority_class              ( BSTR priority )                       { return _process.put_Priority_class( priority ); }
    STDMETHODIMP            get_Priority_class              ( BSTR* result )                        { return _process.get_Priority_class( result ); }

  //STDMETHODIMP            put_With_console_window         ( VARIANT_BOOL b )                      { _process.set_with_console_window( b != 0 );  return S_OK; }

  //STDMETHODIMP                Raise_priority              ( int diff, VARIANT_BOOL* result )      { Z_COM_IMPLEMENT( *result = _process.raise_priority( diff )? VARIANT_TRUE : VARIANT_FALSE ); }
  //STDMETHODIMP                Lower_priority              ( int diff, VARIANT_BOOL* result )      { return Raise_priority( -diff, result ); }

    STDMETHODIMP            get_Pid                         ( int* result )                         { return _process.get_Pid( result ); }
    STDMETHODIMP            get_Terminated                  ( VARIANT_BOOL* result )                { return _process.get_Terminated( result ); }
    STDMETHODIMP            get_Exit_code                   ( int* result )                         { return _process.get_Exit_code( result ); }
    STDMETHODIMP            get_Termination_signal          ( int* result )                         { return _process.get_Termination_signal( result ); }
    STDMETHODIMP            put_Stdout_path                 ( BSTR path )                           { _process.set_stdout_path( string_from_bstr( path ) );  return S_OK; }
    STDMETHODIMP            get_Stdout_path                 ( BSTR* result )                        { return String_to_bstr( _process.stdout_path(), result ); }
    STDMETHODIMP            put_Stderr_path                 ( BSTR path )                           { _process.set_stderr_path( string_from_bstr( path ) );  return S_OK; }
    STDMETHODIMP            get_Stderr_path                 ( BSTR* result )                        { return String_to_bstr( _process.stderr_path(), result ); }
    STDMETHODIMP            put_Environment                 ( BSTR, BSTR );
    STDMETHODIMP            put_Ignore_error                ( VARIANT_BOOL );
    STDMETHODIMP            get_Ignore_error                ( VARIANT_BOOL* );
    STDMETHODIMP            put_Ignore_signal               ( VARIANT_BOOL );
    STDMETHODIMP            get_Ignore_signal               ( VARIANT_BOOL* );
    STDMETHODIMP            put_Timeout                     ( double* );
    STDMETHODIMP            put_Show_window                 ( VARIANT* );
    STDMETHODIMP            put_Own_process_group           ( VARIANT_BOOL b )                      { return _process.put_Own_process_group( b ); }
    STDMETHODIMP            get_Own_process_group           ( VARIANT_BOOL* result )                { return _process.get_Own_process_group( result );}
    STDMETHODIMP                Wait_for_termination        ( VARIANT* seconds, VARIANT_BOOL* );
    STDMETHODIMP                Kill                        ( int signal )                          { return _process.Kill( signal ); }
    STDMETHODIMP            get_Env                         ( spooler_com::Ivariable_set** );

    HRESULT                     Update_register_entry       ();


    void                        close                       ();
    void                        deregister                  ();
    bool                        ignore_error                () const                                { return _ignore_error; }
    bool                        ignore_signal               () const                                { return _ignore_signal; }
    void                        wait_for_termination        ();
    bool                        wait_for_termination        ( double timeout );

  private:
    friend struct               Subprocess_register;

    Fill_zero                  _zero_;
    zschimmer::Process         _process;
    Subprocess_register*       _subprocess_register;
    bool                       _registered;
    ptr<IDispatch>             _task;
    bool                       _ignore_error;
    bool                       _ignore_signal;
    double                     _timeout;
    ptr<Com_variable_set>      _environment;
};

//------------------------------------------------------------------------------Subprocess_register

struct Subprocess_register : Object
{
                                Subprocess_register         ()                                      : _zero_(this+1) {}
                               ~Subprocess_register         ();


    STDMETHODIMP                Create_subprocess           ( VARIANT* program_and_parameters, spooler_com::Isubprocess** result, IDispatch* task );

    void                        wait                        ();                                     // Exception, wenn ein Prozess einen Fehler lieferte
    void                        add                         ( Subprocess* );
    void                        remove                      ( Subprocess* );


    Fill_zero                  _zero_;

    typedef map< int, ptr<Subprocess> >  Subprocess_map;
    Subprocess_map                      _subprocess_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
