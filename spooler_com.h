// $Id: spooler_com.h,v 1.56 2002/11/20 11:03:10 jz Exp $

#ifndef __SPOOLER_COM_H
#define __SPOOLER_COM_H

#include <map>
//#include "../kram/olestd.h"
#include "../zschimmer/com.h"
#include "../kram/com_simple_standards.h"
#include "../kram/sysxcept.h"
#include "../kram/sosscrpt.h"
#include "../kram/com.h"
#include "../kram/com_server.h"

#ifdef Z_WINDOWS
#   if defined _DEBUG
#       import "debug/spooler.tlb"   rename_namespace("spooler_com") raw_interfaces_only named_guids
#    else
#       import "release/spooler.tlb" rename_namespace("spooler_com") raw_interfaces_only named_guids
#   endif
#else
#   include "spooler_idl.h"     // namespace spooler_com
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

struct Com_error: spooler_com::Ierror, 
                  spooler_com::Ihas_java_class_name, 
                  Sos_ole_object
{
                                Com_error                   ( const Xc_copy& );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                            { return string_to_bstr( "sos.spooler.Error", result ); }

    STDMETHODIMP                get_is_error                ( VARIANT_BOOL* );
    STDMETHODIMP                get_code                    ( BSTR* );
    STDMETHODIMP                get_text                    ( BSTR* );

    void                        close                       ();

  private:
                                Com_error                   ( const Com_error& );
    void                        operator =                  ( const Com_error& );

    Thread_semaphore           _lock;
    Xc_copy                    _xc;
};

//-------------------------------------------------------------------------------------Com_variable

struct Com_variable: spooler_com::Ivariable, 
                     spooler_com::Ihas_java_class_name, 
                     Sos_ole_object
{
                                Com_variable                ( const BSTR name, const VARIANT& );
                                Com_variable                ( const Com_variable& );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Variable", result ); }

    STDMETHODIMP                put_value                   ( VARIANT* v )                          { HRESULT hr = NOERROR; THREAD_LOCK(_lock) _value = *v; return hr; }
    STDMETHODIMP                get_value                   ( VARIANT* result )                     { HRESULT hr = NOERROR; THREAD_LOCK(_lock) hr = VariantCopy( result, &_value ); return hr; }
    STDMETHODIMP                get_name                    ( BSTR* result )                        { return _name.CopyTo(result); }
    STDMETHODIMP                Clone                       ( spooler_com::Ivariable** );

  private:

    Thread_semaphore           _lock;
    Bstr                       _name;
    Variant                    _value;
};

//----------------------------------------------------------------------------------Com_variable_set

struct Com_variable_set: spooler_com::Ivariable_set, 
                         spooler_com::Ihas_java_class_name, 
                         Sos_ole_object
{
                                Com_variable_set            ();
                                Com_variable_set            ( const Com_variable_set& );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Variable_set", result ); }

#   ifdef Z_WINDOWS
        STDMETHODIMP            get_dom                     ( msxml::IXMLDOMDocument** );
#   endif

    void                        set_dom                     ( const xml::Element_ptr& );

    xml::Document_ptr           dom                         ();

    STDMETHODIMP                set_var                     ( BSTR name, VARIANT* value )           { return put_var( name, value ); }
    STDMETHODIMP                put_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_var                     ( BSTR, VARIANT* );
    STDMETHODIMP                get_count                   ( int* );
    STDMETHODIMP                Clone                       ( spooler_com::Ivariable_set** );
    STDMETHODIMP                merge                       ( spooler_com::Ivariable_set* );
    STDMETHODIMP                get__NewEnum                ( IUnknown** );    


  private:
    friend struct               Com_variable_set_enumerator;

    typedef std::map< Bstr, ptr<Com_variable> >  Map;

    void                        operator =                  ( const Com_variable_set& );

    Thread_semaphore           _lock;
    Map                        _map;
};

//----------------------------------------------------------------------Com_variable_set_enumerator

struct Com_variable_set_enumerator : spooler_com::Ivariable_set_enumerator, Sos_ole_object
{
    STDMETHODIMP                QueryInterface          ( REFIID, void** );
    
    USE_SOS_OLE_OBJECT_ADDREF_RELEASE
    USE_SOS_OLE_OBJECT_GETTYPEINFO
    USE_SOS_OLE_OBJECT_INVOKE           

                                Com_variable_set_enumerator();

    STDMETHODIMP                Next                    ( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched );
    STDMETHODIMP                Skip                    ( unsigned long celt );
    STDMETHODIMP                Reset                   ();
    STDMETHODIMP                Clone                   ( IEnumVARIANT** ppenum );

    void                        initialize              ( Com_variable_set* );

    ptr<Com_variable_set>           _variable_set;
    Com_variable_set::Map::iterator _iterator;
};

//------------------------------------------------------------------------------------------Com_log

struct Com_log : spooler_com::Ilog, 
                 spooler_com::Ihas_java_class_name, 
                 Sos_ole_object               
{
                                Com_log                     ( Prefix_log* = NULL );
                             //~Com_log                     ();

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );
   
    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Log", result ); }

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _log = NULL; }        

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

    STDMETHODIMP            put_mail_on_process             ( int );
    STDMETHODIMP            get_mail_on_process             ( int* );

    STDMETHODIMP            put_level                       ( int );
    STDMETHODIMP            get_level                       ( int* );

    STDMETHODIMP            get_filename                    ( BSTR* );

    STDMETHODIMP            put_new_filename                ( BSTR );
    STDMETHODIMP            get_new_filename                ( BSTR* );

    STDMETHODIMP            put_collect_within              ( VARIANT* );
    STDMETHODIMP            get_collect_within              ( double* );

    STDMETHODIMP            put_collect_max                 ( VARIANT* );
    STDMETHODIMP            get_collect_max                 ( double* );

  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Prefix_log*                _log;
};

//----------------------------------------------------------------------------------Com_object_set

struct Com_object_set : spooler_com::Iobject_set, 
                      //spooler_com::Ihas_java_class_name, 
                        Sos_ole_object               
{
                                Com_object_set              ( Object_set* );

    USE_SOS_OLE_OBJECT

  //STDMETHODIMP            get_java_class_name         ( BSTR* result );

    void                        clear                       ()                                      { THREAD_LOCK(_lock)  _object_set = NULL; }

    STDMETHODIMP                get_low_level               ( int* );
    STDMETHODIMP                get_high_level              ( int* );


  private:
    Thread_semaphore           _lock;
    Object_set*                _object_set;
};

//------------------------------------------------------------------------------------------Com_job

struct Com_job : spooler_com::Ijob, 
                 spooler_com::Ihas_java_class_name, 
                 Sos_ole_object               
{
                                Com_job                     ( Job* );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Job", result ); }

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _job = NULL; }

    STDMETHODIMP                start_when_directory_changed( BSTR directory_name, BSTR filename_pattern );
    STDMETHODIMP                clear_when_directory_changed();
    STDMETHODIMP                start                       ( VARIANT*, spooler_com::Itask** );
    STDMETHODIMP                wake                        ();
    STDMETHODIMP                get_thread                  ( spooler_com::Ithread** );
  //STDMETHODIMP                put_include_path            ( BSTR );
    STDMETHODIMP                get_include_path            ( BSTR* );
    STDMETHODIMP                get_name                    ( BSTR* );
    STDMETHODIMP                put_state_text              ( BSTR );
    STDMETHODIMP                get_title                   ( BSTR* );
    STDMETHODIMP                put_delay_after_error       ( int error_steps, VARIANT* time );
    STDMETHODIMP                get_order_queue             ( spooler_com::Iorder_queue** );

  private:
    Thread_semaphore           _lock;
    Job*                       _job;                        // Es gibt nur einen Com_job pro Job
};

//-----------------------------------------------------------------------------------------Com_task

struct Com_task : spooler_com::Itask, 
                  spooler_com::Ihas_java_class_name, 
                  Sos_ole_object               
{
                                Com_task                    ( Task* = NULL );
                               ~Com_task                    ()                                      {}

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Task", result ); }

    void                        set_task                    ( Task* );
    Task*                       task                        ()                                      { return _task; }

    STDMETHODIMP                get_object_set              ( spooler_com::Iobject_set** );
    STDMETHODIMP                put_error                   ( VARIANT* error_text );
    STDMETHODIMP                get_error                   ( spooler_com::Ierror** );
    STDMETHODIMP                get_job                     ( spooler_com::Ijob** );
    STDMETHODIMP                get_params                  ( spooler_com::Ivariable_set** );
    STDMETHODIMP                wait_until_terminated       ( double wait_time, VARIANT_BOOL* ok );
    STDMETHODIMP                end                         ();
    STDMETHODIMP                put_result                  ( VARIANT* value );
    STDMETHODIMP                get_result                  ( VARIANT* value );
    STDMETHODIMP                put_repeat                  ( double seconds );
    STDMETHODIMP                put_history_field           ( BSTR name, VARIANT* value );
    STDMETHODIMP                get_id                      ( int* value );
    STDMETHODIMP                put_delay_spooler_process   ( VARIANT* time );
    STDMETHODIMP                put_close_engine            ( VARIANT_BOOL );
    STDMETHODIMP                get_order                   ( spooler_com::Iorder** );

  private:
    Thread_semaphore           _lock;
    Sos_ptr<Task>              _task;
};

//---------------------------------------------------------------------------------------Com_thread

struct Com_thread : spooler_com::Ithread, 
                    spooler_com::Ihas_java_class_name, 
                    Sos_ole_object               
{
                                Com_thread                  ( Thread* );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Thread", result ); }

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _thread = NULL; }

    STDMETHODIMP                get_log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_script                  ( IDispatch** );
  //STDMETHODIMP                put_include_path            ( BSTR );
    STDMETHODIMP                get_include_path            ( BSTR* );
    STDMETHODIMP                get_name                    ( BSTR* );

  protected:
    Thread_semaphore           _lock;
    Thread*                    _thread;                     // Es gibt nur einen Com_thread pro Thread
};

//--------------------------------------------------------------------------------------Com_spooler

struct Com_spooler : spooler_com::Ispooler, 
                     spooler_com::Ihas_java_class_name, 
                     Sos_ole_object               
{
                                Com_spooler                 ( Spooler* ); 

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return string_to_bstr( "sos.spooler.Spooler", result ); }

    STDMETHODIMP                QueryInterface              ( REFIID, void** );

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _spooler = NULL; }

    STDMETHODIMP            get_log                         ( spooler_com::Ilog** );
    STDMETHODIMP            get_param                       ( BSTR* );
    STDMETHODIMP            get_id                          ( BSTR* );
    STDMETHODIMP            get_script                      ( IDispatch** );
    STDMETHODIMP            get_job                         ( BSTR job_name, spooler_com::Ijob** );
    STDMETHODIMP                create_variable_set         ( spooler_com::Ivariable_set** );
  //STDMETHODIMP            put_include_path                ( BSTR );
    STDMETHODIMP            get_include_path                ( BSTR* );
    STDMETHODIMP            get_log_dir                     ( BSTR* );
    STDMETHODIMP                let_run_terminate_and_restart();
    STDMETHODIMP            get_variables                   ( spooler_com::Ivariable_set** );
    STDMETHODIMP            put_var                         ( BSTR name, VARIANT* value );
    STDMETHODIMP            get_var                         ( BSTR name, VARIANT* value );
    STDMETHODIMP            get_db_name                     ( BSTR* );
    STDMETHODIMP                create_job_chain            ( spooler_com::Ijob_chain** );
    STDMETHODIMP                add_job_chain               ( spooler_com::Ijob_chain* );
    STDMETHODIMP            get_job_chain                   ( BSTR, spooler_com::Ijob_chain** );
    STDMETHODIMP                create_order                ( spooler_com::Iorder** );
    STDMETHODIMP            get_is_service                  ( VARIANT_BOOL* );

  protected:
    Thread_semaphore           _lock;
    Spooler*                   _spooler;                    // Es gibt nur einen Com_spooler
};

//--------------------------------------------------------------------------------------Com_context

struct Com_context : spooler_com::Icontext, Sos_ole_object               
{
                                Com_context                 ();


    void                        close                       ()                      { THREAD_LOCK(_lock)  _log     = NULL, 
                                                                                                          _spooler = NULL, 
                                                                                                          _thread  = NULL, 
                                                                                                          _job     = NULL, 
                                                                                                          _task    = NULL; }

    USE_SOS_OLE_OBJECT

    STDMETHODIMP                get_log                     ( spooler_com::Ilog**     o )        { return _log    .CopyTo(o); }
    STDMETHODIMP                get_spooler                 ( spooler_com::Ispooler** o )        { return _spooler.CopyTo(o); }
    STDMETHODIMP                get_thread                  ( spooler_com::Ithread**  o )        { return _thread .CopyTo(o); }
    STDMETHODIMP                get_job                     ( spooler_com::Ijob**     o )        { return _job    .CopyTo(o); }
    STDMETHODIMP                get_Task                    ( spooler_com::Itask**    o )        { return _task   .CopyTo(o); }


    Thread_semaphore           _lock;

    ptr<spooler_com::Ilog>     _log;
    ptr<spooler_com::Ispooler> _spooler;
    ptr<spooler_com::Ithread>  _thread;
    ptr<spooler_com::Ijob>     _job;
    ptr<spooler_com::Itask>    _task;
};

//------------------------------------------------------------------------------------Com_job_chain

struct Com_job_chain : spooler_com::Ijob_chain, 
                       spooler_com::Ihas_java_class_name, 
                       Sos_ole_object               
{
                                Com_job_chain           ( Job_chain* );

    void                        close                   ()                                          { THREAD_LOCK( _lock )  _job_chain = NULL; }

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface          ( REFIID, void** );

    STDMETHODIMP            get_java_class_name         ( BSTR* result )                            { return string_to_bstr( "sos.spooler.Job_chain", result ); }

    STDMETHODIMP            put_name                    ( BSTR );
    STDMETHODIMP            get_name                    ( BSTR* );

    STDMETHODIMP            get_order_count             ( int* );
    
    STDMETHODIMP            get_order_queue             ( VARIANT* state, spooler_com::Iorder_queue** );
    
    STDMETHODIMP            get_node                    ( VARIANT* state, spooler_com::Ijob_chain_node** );

    STDMETHODIMP                add_job                 ( VARIANT*, VARIANT*, VARIANT*, VARIANT* );
    STDMETHODIMP                add_end_state           ( VARIANT* );

    STDMETHODIMP                add_order               ( VARIANT* order_or_payload, spooler_com::Iorder** );


    Thread_semaphore           _lock;
    Job_chain*                 _job_chain;
};

//-------------------------------------------------------------------------------Com_job_chain_node

struct Com_job_chain_node : spooler_com::Ijob_chain_node, 
                            spooler_com::Ihas_java_class_name, 
                            Sos_ole_object               
{
                                Com_job_chain_node      ();

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface          ( REFIID, void** );

    STDMETHODIMP            get_java_class_name         ( BSTR* result )                            { return string_to_bstr( "sos.spooler.Job_chain_node", result ); }

    STDMETHODIMP            get_state                   ( VARIANT* );
    STDMETHODIMP            get_next_node               ( spooler_com::Ijob_chain_node** );
    STDMETHODIMP            get_error_node              ( spooler_com::Ijob_chain_node** );
    STDMETHODIMP            get_job                     ( spooler_com::Ijob** );
    STDMETHODIMP            get_next_state              ( VARIANT* );
    STDMETHODIMP            get_error_state             ( VARIANT* );
};

//----------------------------------------------------------------------------------------Com_order

struct Com_order : spooler_com::Iorder, 
                   spooler_com::Ihas_java_class_name, 
                   Sos_ole_object
{
                                Com_order               ( Order* );
                              //Com_order               ();

    void                        close                   ()                                          { THREAD_LOCK( _lock )  _order = NULL; }


    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface          ( REFIID, void** );

    STDMETHODIMP            get_java_class_name         ( BSTR* result )                            { *result = SysAllocString( L"sos.spooler.Order" ); return S_OK; }

    STDMETHODIMP            put_id                      ( VARIANT* );
    STDMETHODIMP            get_id                      ( VARIANT* );

    STDMETHODIMP            put_title                   ( BSTR );
    STDMETHODIMP            get_title                   ( BSTR* );
    
    STDMETHODIMP            put_priority                ( int );
    STDMETHODIMP            get_priority                ( int* );
    
    STDMETHODIMP            get_job_chain               ( spooler_com::Ijob_chain** );

    STDMETHODIMP            get_job_chain_node          ( spooler_com::Ijob_chain_node** );
    
    STDMETHODIMP            put_job                     ( VARIANT* );
    STDMETHODIMP         putref_job                     ( spooler_com::Ijob* job )                  { Variant v = job; return put_job( &v ); }
    STDMETHODIMP            get_job                     ( spooler_com::Ijob** );
    
    STDMETHODIMP            put_state                   ( VARIANT* );
    STDMETHODIMP            get_state                   ( VARIANT* );
    
    STDMETHODIMP            put_state_text              ( BSTR );
    STDMETHODIMP            get_state_text              ( BSTR* );
    
    STDMETHODIMP            get_error                   ( spooler_com::Ierror** );
    
    STDMETHODIMP            put_payload                 ( VARIANT* );
    STDMETHODIMP         putref_payload                 ( IUnknown* );
    STDMETHODIMP            get_payload                 ( VARIANT* );
    
    STDMETHODIMP                payload_is_type         ( BSTR, VARIANT_BOOL* );

  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Order*                     _order;
};

//----------------------------------------------------------------------------------Com_order_queue

struct Com_order_queue : spooler_com::Iorder_queue, 
                         spooler_com::Ihas_java_class_name, 
                         Sos_ole_object               
{
                                Com_order_queue         ();


    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_java_class_name         ( BSTR* result )                            { return string_to_bstr( "sos.spooler.Order_queue", result ); }

    STDMETHODIMP                QueryInterface          ( REFIID, void** );

    STDMETHODIMP            get_length                  ( int* );
    STDMETHODIMP                add_order               ( VARIANT*, spooler_com::Iorder** );


  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
