// $Id: spooler_com.h,v 1.23 2002/03/05 18:09:52 jz Exp $

#ifndef __SPOOLER_COM_H
#define __SPOOLER_COM_H
#ifdef SYSTEM_WIN

#include <map>
#include "../kram/olestd.h"
#include "../kram/sysxcept.h"
#include "../kram/sosscrpt.h"
#include "../kram/com.h"
#include "../kram/com_server.h"

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

using spooler_com::Log_level; //enum   Log_level;
using spooler_com::log_debug9;
using spooler_com::log_debug8;
using spooler_com::log_debug7;
using spooler_com::log_debug6;
using spooler_com::log_debug5;
using spooler_com::log_debug4;
using spooler_com::log_debug3;
using spooler_com::log_debug2;
using spooler_com::log_debug1;
using spooler_com::log_debug;
using spooler_com::log_info;
using spooler_com::log_warn;
using spooler_com::log_error;

const Log_level log_debug_spooler = spooler_com::log_debug3;

struct Prefix_log;
struct Log;
struct Object_set;
struct Job;
struct Task;
struct Spooler;

//----------------------------------------------------------------------------------------Com_error

struct Com_error: spooler_com::Ierror, Sos_ole_object
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Error" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_error                   ( const Xc_copy& );

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                get_is_error                ( VARIANT_BOOL* );
    STDMETHODIMP                get_code                    ( BSTR* );
    STDMETHODIMP                get_text                    ( BSTR* );

    void                        close                       ()                                      { _xc = NULL; }

    Xc_copy                    _xc;
};

//----------------------------------------------------------------------------------Com_variable_set

struct Com_variable_set: spooler_com::Ivariable_set, Sos_ole_object
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Variable_set" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_variable_set            ();

    USE_SOS_OLE_OBJECT

    void                        set_xml                     ( const xml::Element_ptr& );
    STDMETHODIMP                set_var                     ( BSTR name, VARIANT* value)            { return put_var( name, value ); }
    STDMETHODIMP                put_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_count                   ( int* );

    std::map<CComBSTR,CComVariant>  _map;
    Thread_semaphore                _lock;
};

//------------------------------------------------------------------------------------------Com_log

struct Com_log : spooler_com::Ilog, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Log" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_log                     ( Prefix_log* = NULL );
                             //~Com_log                     ();

    USE_SOS_OLE_OBJECT

    void                        close                       ()                                      { _log = NULL; }        

    STDMETHODIMP                debug9                      ( BSTR );
    STDMETHODIMP                debug8                      ( BSTR );
    STDMETHODIMP                debug7                      ( BSTR );
    STDMETHODIMP                debug6                      ( BSTR );
    STDMETHODIMP                debug5                      ( BSTR );
    STDMETHODIMP                debug4                      ( BSTR );
    STDMETHODIMP                debug3                      ( BSTR );
    STDMETHODIMP                debug2                      ( BSTR );
    STDMETHODIMP                debug1                      ( BSTR );
    STDMETHODIMP                debug                       ( BSTR );
    STDMETHODIMP                info                        ( BSTR );
    STDMETHODIMP                msg                         ( BSTR );
    STDMETHODIMP                warn                        ( BSTR );
    STDMETHODIMP                error                       ( BSTR );
  //STDMETHODIMP                fatal                       ( BSTR );
    STDMETHODIMP                log                         ( spooler_com::Log_level, BSTR line );

    STDMETHODIMP            get_mail                        ( spooler_com::Imail** );

    STDMETHODIMP            put_mail_on_error               ( VARIANT_BOOL );
    STDMETHODIMP            get_mail_on_error               ( VARIANT_BOOL* );
    
    STDMETHODIMP            put_mail_on_success             ( VARIANT_BOOL );
    STDMETHODIMP            get_mail_on_success             ( VARIANT_BOOL* );

    STDMETHODIMP            put_level                       ( int );
    STDMETHODIMP            get_level                       ( int* );

    STDMETHODIMP            get_filename                    ( BSTR* );

    Fill_zero                  _zero_;
    Prefix_log*                _log;
};

//----------------------------------------------------------------------------------Com_object_set

struct Com_object_set : spooler_com::Iobject_set, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Object_set" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


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
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Job" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_job                     ( Job* );

    USE_SOS_OLE_OBJECT

    void                        close                       ()                                      { _job = NULL; }

    STDMETHODIMP                start_when_directory_changed( BSTR directory_name );
    STDMETHODIMP                clear_when_directory_changed();
    STDMETHODIMP                start                       ( VARIANT*, spooler_com::Itask** );
    STDMETHODIMP                wake                        ();
    STDMETHODIMP                get_thread                  ( spooler_com::Ithread** );
  //STDMETHODIMP                put_include_path            ( BSTR );
    STDMETHODIMP                get_include_path            ( BSTR* );
    STDMETHODIMP                get_name                    ( BSTR* );
    STDMETHODIMP                put_state_text              ( BSTR );

    Job*                       _job;                        // Es gibt nur einen Com_job pro Job
};

//-----------------------------------------------------------------------------------------Com_task

struct Com_task : spooler_com::Itask, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Task" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_task                    ( Task* = NULL );

    USE_SOS_OLE_OBJECT

    void                        set_task                    ( Task* );

    STDMETHODIMP                get_object_set              ( spooler_com::Iobject_set** );
    STDMETHODIMP                put_error                   ( VARIANT* error_text );
    STDMETHODIMP                get_error                   ( spooler_com::Ierror** );
    STDMETHODIMP                get_job                     ( spooler_com::Ijob** );
    STDMETHODIMP                get_params                  ( spooler_com::Ivariable_set** );
    STDMETHODIMP                wait_until_terminated       ( double wait_time, VARIANT_BOOL* ok );
    STDMETHODIMP                end                         ();
    STDMETHODIMP                put_result                  ( VARIANT* value );             //{ return _result.Copy(value); }
    STDMETHODIMP                get_result                  ( VARIANT* value );             //{ VariantInit(value); return VariantCopy(value,&_result); }
    STDMETHODIMP                put_repeat                  ( double seconds );

  private:
    Sos_ptr<Task>              _task;
    Thread_semaphore           _lock;
};

//---------------------------------------------------------------------------------------Com_thread

struct Com_thread : spooler_com::Ithread, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Thread" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_thread                  ( Thread* );

    USE_SOS_OLE_OBJECT

    void                        close                       ()                                      { _thread = NULL; }

    STDMETHODIMP                get_log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_script                  ( IDispatch** );
  //STDMETHODIMP                put_include_path            ( BSTR );
    STDMETHODIMP                get_include_path            ( BSTR* );
    STDMETHODIMP                get_name                    ( BSTR* );

  protected:
    Thread*                    _thread;                     // Es gibt nur einen Com_thread pro Thread
};

//--------------------------------------------------------------------------------------Com_spooler

struct Com_spooler : spooler_com::Ispooler, Sos_ole_object               
{
    void*                       operator new                ( uint size )                           { return sos_alloc( size, "spooler.Spooler" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                                Com_spooler                 ( Spooler* ); 

    USE_SOS_OLE_OBJECT

    void                        close                       ()                                      { _spooler = NULL; }

    STDMETHODIMP                get_log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_param                   ( BSTR* );
    STDMETHODIMP                get_id                      ( BSTR* );
    STDMETHODIMP                get_script                  ( IDispatch** );
    STDMETHODIMP                get_job                     ( BSTR job_name, spooler_com::Ijob** );
    STDMETHODIMP                create_variable_set         ( spooler_com::Ivariable_set** );
  //STDMETHODIMP                put_include_path            ( BSTR );
    STDMETHODIMP                get_include_path            ( BSTR* );
    STDMETHODIMP                get_log_dir                 ( BSTR* );

  protected:
    Spooler*                   _spooler;                    // Es gibt nur einen Com_spooler
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
#endif