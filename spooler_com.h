// $Id: spooler_com.h,v 1.10 2001/02/08 11:21:15 jz Exp $

#ifndef __SPOOLER_COM_H
#define __SPOOLER_COM_H
#ifdef SYSTEM_WIN

#include <map>
#include "../kram/olestd.h"
#include "../kram/sysxcept.h"

#include "../kram/sosscrpt.h"

#include <atlbase.h>
//    extern CComModule& _Module;
//#   include <atlcom.h>

#if defined _DEBUG
#    import "debug/spooler.tlb"   rename_namespace("spooler_com") raw_interfaces_only named_guids
# else
#    import "release/spooler.tlb" rename_namespace("spooler_com") raw_interfaces_only named_guids
#endif


namespace sos {
namespace spooler {

enum   Log_kind;
struct Prefix_log;
struct Log;
struct Object_set;
struct Job;
struct Task;
struct Spooler;

//----------------------------------------------------------------------------------------Com_error

struct Com_error: spooler_com::Ierror, Sos_ole_object
{
                                Com_error                   ( const Xc_copy& );

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                get_is_error                ( VARIANT_BOOL* );
    STDMETHODIMP                get_code                    ( BSTR* );
    STDMETHODIMP                get_text                    ( BSTR* );

    void                        close                       ()                              { _xc = NULL; }

    Xc_copy                    _xc;
};

//----------------------------------------------------------------------------------Com_variable_set

struct Com_variable_set: spooler_com::Ivariable_set, Sos_ole_object
{
                                Com_variable_set            ();

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                set_var                     ( BSTR name, VARIANT* value)    { return put_var( name, value ); }
    STDMETHODIMP                put_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_count                   ( int* );

    std::map<CComBSTR,CComVariant>  _map;
    Thread_semaphore                _lock;
};

//------------------------------------------------------------------------------------------Com_log

struct Com_log : spooler_com::Ilog, Sos_ole_object               
{
                                Com_log                     ( Prefix_log* = NULL );
                             //~Com_log                     ();

    USE_SOS_OLE_OBJECT

    void                        close                       ()                              { _log = NULL; }        

    STDMETHODIMP                msg                         ( BSTR );
    STDMETHODIMP                warn                        ( BSTR );
    STDMETHODIMP                error                       ( BSTR );
    STDMETHODIMP                log                         ( Log_kind kind, BSTR line );


    Fill_zero                  _zero_;
    Prefix_log*                _log;
};

//----------------------------------------------------------------------------------Com_object_set

struct Com_object_set : spooler_com::Iobject_set, Sos_ole_object               
{
                                Com_object_set              ( Object_set* );

    USE_SOS_OLE_OBJECT

    void                        close                       ()                              { _object_set = NULL; }

    STDMETHODIMP                get_low_level               ( int* );
    STDMETHODIMP                get_high_level              ( int* );

    Object_set*                _object_set;
};

//------------------------------------------------------------------------------------------Com_job

struct Com_job : spooler_com::Ijob, Sos_ole_object               
{
                                Com_job                     ( Job* );

    USE_SOS_OLE_OBJECT

    void                        close                       ()                              { _job = NULL; }

    STDMETHODIMP                start_when_directory_changed( BSTR directory_name );
    STDMETHODIMP                start                       ( VARIANT*, spooler_com::Itask** );
    STDMETHODIMP                get_thread                  ( spooler_com::Ithread** );

    Job*                       _job;                        // Es gibt nur einen Com_job pro Job
};

//-----------------------------------------------------------------------------------------Com_task

struct Com_task : spooler_com::Itask, Sos_ole_object               
{
                                Com_task                    ( Task* = NULL );

    USE_SOS_OLE_OBJECT

    void                        set_task                    ( Task* );

    STDMETHODIMP                get_object_set              ( spooler_com::Iobject_set** );
  //STDMETHODIMP                wake_when_directory_changed ( BSTR directory_name );
    STDMETHODIMP                put_error                   ( VARIANT* error_text );
    STDMETHODIMP                get_error                   ( spooler_com::Ierror** );
    STDMETHODIMP                get_job                     ( spooler_com::Ijob** );
    STDMETHODIMP                get_params                  ( spooler_com::Ivariable_set** );
    STDMETHODIMP                wait_until_terminated       ( double wait_time, VARIANT_BOOL* ok );
    STDMETHODIMP                put_result                  ( VARIANT* value );             //{ return _result.Copy(value); }
    STDMETHODIMP                get_result                  ( VARIANT* value );             //{ VariantInit(value); return VariantCopy(value,&_result); }
    STDMETHODIMP                put_repeat                  ( double seconds );

  private:
    Sos_ptr<Task>              _task;
  //Xc_copy                    _error;                      // Nur gültig, wenn _task == NULL
  //CComVariant                _result;                     // Das Ergebnis ist noch nach ~Task zugreifbar
    Thread_semaphore           _lock;
};

//---------------------------------------------------------------------------------------Com_thread

struct Com_thread : spooler_com::Ithread, Sos_ole_object               
{
                                Com_thread                  ( Thread* );

    USE_SOS_OLE_OBJECT

    void                        close                       ()                              { _thread = NULL; }

    STDMETHODIMP                get_log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_script                  ( IDispatch** );

  protected:
    Thread*                    _thread;                     // Es gibt nur einen Com_thread pro Thread
};

//--------------------------------------------------------------------------------------Com_spooler

struct Com_spooler : spooler_com::Ispooler, Sos_ole_object               
{
                                Com_spooler                 ( Spooler* ); 

    USE_SOS_OLE_OBJECT

    void                        close                       ()                              { _spooler = NULL; }

    STDMETHODIMP                get_log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_param                   ( BSTR* );
    STDMETHODIMP                get_id                      ( BSTR* );
  //STDMETHODIMP                get_script                  ( IDispatch** );
    STDMETHODIMP                get_job                     ( BSTR job_name, spooler_com::Ijob** );
    STDMETHODIMP                create_variable_set         ( spooler_com::Ivariable_set** );

  protected:
    Spooler*                   _spooler;                    // Es gibt nur einen Com_spooler
};



} //namespace spooler
} //namespace sos

#endif
#endif