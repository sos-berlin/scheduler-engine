// $Id$

#ifndef __SPOOLER_COM_H
#define __SPOOLER_COM_H

#include "../zschimmer/com.h"
#include "../zschimmer/com_server.h"
#include "../zschimmer/z_com_server.h"
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

//typedef spooler_com::Log_level Log_level;
/*
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
*/

const Log_level log_debug_spooler = log_debug3;

struct Prefix_log;
struct Log;
struct Object_set;
struct Job;
struct Task;
struct Spooler;


#ifndef Z_WINDOWS
    struct IXMLDOMDocument;    // Dummy
#endif

//-------------------------------------------------------------------------------------------------

extern Typelib_ref              typelib;

//----------------------------------------------------------------------------------------Com_error

struct Com_error: spooler_com::Ierror, 
                  spooler_com::Ihas_java_class_name, 
                  Sos_ole_object
{
                                Com_error                   ( const Xc_copy& );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                          { return (char*)"sos.spooler.Error"; }

    STDMETHODIMP            get_Is_error                    ( VARIANT_BOOL* );
    STDMETHODIMP            get_Code                        ( BSTR* );
    STDMETHODIMP            get_Text                        ( BSTR* );

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
    Z_GNU_ONLY(                 Com_variable                ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Com_variable                ( const BSTR name, const VARIANT& );
                                Com_variable                ( const Com_variable& );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Variable"; }

    STDMETHODIMP            put_Value                       ( VARIANT* v )                          { HRESULT hr = NOERROR; THREAD_LOCK(_lock) _value = *v; return hr; }
    STDMETHODIMP            get_Value                       ( VARIANT* result )                     { HRESULT hr = NOERROR; THREAD_LOCK(_lock) hr = VariantCopy( result, &_value ); return hr; }
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return _name.CopyTo(result); }
    STDMETHODIMP                Clone                       ( spooler_com::Ivariable** );

    string                      string_value                () const;

//private:
    Thread_semaphore           _lock;
    Bstr                       _name;
    Variant                    _value;
};

//----------------------------------------------------------------------------------Com_variable_set

struct Com_variable_set: spooler_com::Ivariable_set, 
                         spooler_com::Ihas_java_class_name, 
                         Sos_ole_object,
                         zschimmer::Get_string_by_name_interface
{
                                Com_variable_set            ();
                                Com_variable_set            ( const xml::Element_ptr&, const string& variable_element_name = "param" );
                                Com_variable_set            ( const Com_variable_set& );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Variable_set"; }

    STDMETHODIMP            get_Dom                         ( IXMLDOMDocument** );
    void                    set_dom                         ( const xml::Element_ptr&, const string& variable_element_name = "param" );

    xml::Document_ptr           dom                         ( const string& element_name, const string& subelement_name );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const string& element_name, const string& subelement_name );

    void                        set_var                     ( const string& name, const Variant& value );
    void                        get_var                     ( BSTR name, VARIANT* value ) const;
    string                      get_string                  ( const string& name );
    string                      get_string_by_name          ( const string& name, bool* name_found ) const;
    void                        merge                       ( const Com_variable_set* );

    STDMETHODIMP                Set_var                     ( BSTR name, VARIANT* value )           { return put_Var( name, value ); }

    STDMETHODIMP            put_Value                       ( VARIANT*, VARIANT* );
    STDMETHODIMP            get_Value                       ( VARIANT*, VARIANT* );

    STDMETHODIMP            put_Var                         ( BSTR, VARIANT* );
    STDMETHODIMP            get_Var                         ( BSTR, VARIANT* );

    STDMETHODIMP            get_Count                       ( int* );

    STDMETHODIMP                Clone                       ( spooler_com::Ivariable_set** );
    STDMETHODIMP                Merge                       ( spooler_com::Ivariable_set* );
    STDMETHODIMP            get__NewEnum                    ( IUnknown** );    
    STDMETHODIMP            put_Xml                         ( BSTR xml_text );
    STDMETHODIMP            get_Xml                         ( BSTR* xml_text );
    void                    set_xml                         ( const string& xml_text );
  //STDMETHODIMP            get_Names_array                 ( SAFEARRAY** );
    STDMETHODIMP            get_Names                       ( BSTR* );


    static const string         xml_element_name            ()                                      { return "sos.spooler.variable_set"; }


//private:
    friend struct               Com_variable_set_enumerator;

    typedef stdext::hash_map< Bstr, ptr<Com_variable> >  Map;

    void                        operator =                  ( const Com_variable_set& );

    mutable Thread_semaphore   _lock;
    Map                        _map;
    bool                       _ignore_case;
};

//----------------------------------------------------------------------Com_variable_set_enumerator

struct Com_variable_set_enumerator : spooler_com::Ivariable_set_enumerator, Sos_ole_object
{
    STDMETHODIMP_(ULONG)        AddRef                  ()                                          { return Sos_ole_object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                 ()                                          { return Sos_ole_object::Release(); }
    STDMETHODIMP                QueryInterface          ( const IID&, void** );
    
    //USE_SOS_OLE_OBJECT_WITHOUT_QI

                                Com_variable_set_enumerator();

    STDMETHODIMP                Next                    ( ulong32 celt, VARIANT* rgvar, ulong32* pceltFetched );
    STDMETHODIMP                Skip                    ( ulong32 celt );
    STDMETHODIMP                Reset                   ();
    STDMETHODIMP                Clone                   ( IEnumVARIANT** ppenum );

    void                        initialize              ( Com_variable_set* );

    ptr<Com_variable_set>           _variable_set;
    Com_variable_set::Map::iterator _iterator;
};

//------------------------------------------------------------------------------------------Com_log

struct Com_log : spooler_com::Ilog, 
                 spooler_com::Ihas_java_class_name, 
                 z::com::object_server::Ihas_reference_with_properties,
                 Sos_ole_object               
{
                                Com_log                     ( Prefix_log* = NULL );
                             //~Com_log                     ();

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
   
    USE_SOS_OLE_OBJECT_WITHOUT_QI


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Log"; }
  //STDMETHODIMP            get_Java_object_is_owner        ( BSTR* result )                        { *result = VARIANT_FALSE;  return S_OK; }

    // interface Ireference_with_properties
    ptr<z::com::object_server::Reference_with_properties> get_reference_with_properties();


    // interface Ilog
    void                    set_log                         ( Prefix_log* );

    STDMETHODIMP                Debug9                      ( BSTR );
    STDMETHODIMP                Debug8                      ( BSTR );
    STDMETHODIMP                Debug7                      ( BSTR );
    STDMETHODIMP                Debug6                      ( BSTR );
    STDMETHODIMP                Debug5                      ( BSTR );
    STDMETHODIMP                Debug4                      ( BSTR );
    STDMETHODIMP                Debug3                      ( BSTR );
    STDMETHODIMP                Debug2                      ( BSTR );
    STDMETHODIMP                Debug1                      ( BSTR );
    STDMETHODIMP                Debug                       ( BSTR );
    STDMETHODIMP                Info                        ( BSTR );
    STDMETHODIMP                Msg                         ( BSTR );
    STDMETHODIMP                Warn                        ( BSTR );
    STDMETHODIMP                Error                       ( BSTR );
  //STDMETHODIMP                Fatal                       ( BSTR );
    STDMETHODIMP                Log                         ( spooler_com::Log_level, BSTR line );
    STDMETHODIMP                Log_file                    ( BSTR path );

    STDMETHODIMP            get_Mail                        ( spooler_com::Imail** );

    STDMETHODIMP            put_Mail_on_error               ( VARIANT_BOOL );
    STDMETHODIMP            get_Mail_on_error               ( VARIANT_BOOL* );
    
    STDMETHODIMP            put_Mail_on_success             ( VARIANT_BOOL );
    STDMETHODIMP            get_Mail_on_success             ( VARIANT_BOOL* );

    STDMETHODIMP            put_Mail_on_process             ( int );
    STDMETHODIMP            get_Mail_on_process             ( int* );

    STDMETHODIMP            put_Level                       ( int );
    STDMETHODIMP            get_Level                       ( int* );

    STDMETHODIMP            get_Filename                    ( BSTR* );

    STDMETHODIMP            put_New_filename                ( BSTR );
    STDMETHODIMP            get_New_filename                ( BSTR* );

    STDMETHODIMP            put_Collect_within              ( VARIANT* );
    STDMETHODIMP            get_Collect_within              ( double* );

    STDMETHODIMP            put_Collect_max                 ( VARIANT* );
    STDMETHODIMP            get_Collect_max                 ( double* );

    STDMETHODIMP            put_Mail_it                     ( VARIANT_BOOL );

    STDMETHODIMP            get_Last_error_line             ( BSTR* );

    STDMETHODIMP            get_Last                        ( VARIANT*, BSTR* );

    STDMETHODIMP            put_Mail_on_warning             ( VARIANT_BOOL );
    STDMETHODIMP            get_Mail_on_warning             ( VARIANT_BOOL* );

    STDMETHODIMP                Start_new_file              ();
    
  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Prefix_log*                _log;
};

//------------------------------------------------------------------------------------Com_log_proxy

Z_DEFINE_GUID( CLSID_Com_log_proxy, 0xfeee47a6, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );   // {feee47a6-6c1b-11d8-8103-000476ee8afb}


struct Com_log_proxy: object_server::Proxy 
{
                              //Com_log                     ();

    static HRESULT              Create_instance             ( const IID& iid, ptr<IUnknown>* result );


    void                    set_property                    ( const string& name, const Variant& value );

  //STDMETHODIMP                QueryInterface              ( const IID&, void** );
   
  //STDMETHODIMP                GetIDsOfNames               ( const IID&, OLECHAR** rgszNames, UINT cNames, LCID, DISPID* );

    STDMETHODIMP                Invoke                      ( DISPID, const IID&, LCID, unsigned short wFlags, DISPPARAMS*,
                                                              VARIANT* pVarResult, EXCEPINFO*, UINT* puArgErr );
/*
    STDMETHODIMP                Debug9                      ( BSTR line )                           { log( log_debug9, line ); }
    STDMETHODIMP                Debug8                      ( BSTR line )                           { log( log_debug8, line ); }
    STDMETHODIMP                Debug7                      ( BSTR line )                           { log( log_debug7, line ); }
    STDMETHODIMP                Debug6                      ( BSTR line )                           { log( log_debug6, line ); }
    STDMETHODIMP                Debug5                      ( BSTR line )                           { log( log_debug5, line ); }
    STDMETHODIMP                Debug4                      ( BSTR line )                           { log( log_debug4, line ); }
    STDMETHODIMP                Debug3                      ( BSTR line )                           { log( log_debug3, line ); }
    STDMETHODIMP                Debug2                      ( BSTR line )                           { log( log_debug2, line ); }
    STDMETHODIMP                Debug1                      ( BSTR line )                           { log( log_debug1, line ); }
    STDMETHODIMP                Debug                       ( BSTR line )                           { log( log_debug , line ); }
    STDMETHODIMP                Info                        ( BSTR line )                           { log( log_info  , line ); }
    STDMETHODIMP                Msg                         ( BSTR line )                           { log( log_info  , line ); }
    STDMETHODIMP                Warn                        ( BSTR line )                           { log( log_warn  , line ); }
    STDMETHODIMP                Error                       ( BSTR line )                           { log( log_error , line ); }
  //STDMETHODIMP                Fatal                       ( BSTR );
    STDMETHODIMP                Log                         ( spooler_com::Log_level, BSTR line );

    STDMETHODIMP            put_Level                       ( int );
    STDMETHODIMP            get_Level                       ( int* );
*/

  private:
  //static const z::com::Com_method    _methods[];

    int                        _level;
};

//----------------------------------------------------------------------------------Com_object_set

struct Com_object_set : spooler_com::Iobject_set, 
                      //spooler_com::Ihas_java_class_name, 
                        Sos_ole_object               
{
    Z_GNU_ONLY(                 Com_object_set              ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Com_object_set              ( Object_set* );

    USE_SOS_OLE_OBJECT

  //STDMETHODIMP            get_java_class_name             ( BSTR* result );

    void                        clear                       ()                                      { THREAD_LOCK(_lock)  _object_set = NULL; }

    STDMETHODIMP            get_Low_level                   ( int* );
    STDMETHODIMP            get_High_level                  ( int* );


  private:
    Thread_semaphore           _lock;
    Object_set*                _object_set;
};

//------------------------------------------------------------------------------------------Com_job

struct Com_job : spooler_com::Ijob, 
                 spooler_com::Ihas_java_class_name, 
                 Sos_ole_object               
{
    Z_GNU_ONLY(                 Com_job                     ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Com_job                     ( Job* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Job"; }

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _job = NULL; }

    STDMETHODIMP                Start_when_directory_changed( BSTR directory_name, BSTR filename_pattern );
    STDMETHODIMP                Clear_when_directory_changed();
    STDMETHODIMP                Start                       ( VARIANT*, spooler_com::Itask** );
    STDMETHODIMP                Wake                        ();
  //STDMETHODIMP            get_Thread                      ( spooler_com::Ithread** );
  //STDMETHODIMP            put_Include_path                ( BSTR );
    STDMETHODIMP            get_Include_path                ( BSTR* );
    STDMETHODIMP            get_Name                        ( BSTR* );
    STDMETHODIMP            put_State_text                  ( BSTR );
    STDMETHODIMP            get_Title                       ( BSTR* );
    STDMETHODIMP            put_Delay_after_error           ( int error_steps, VARIANT* time );
    STDMETHODIMP            get_Order_queue                 ( spooler_com::Iorder_queue** );
    STDMETHODIMP            put_Delay_order_after_setback   ( int setback_number, VARIANT* time );
    STDMETHODIMP            put_Max_order_setbacks          ( int );
    STDMETHODIMP                Clear_delay_after_error     ();
    STDMETHODIMP                Remove                      ();
    STDMETHODIMP                Execute_command             ( BSTR );


  private:
    Thread_semaphore           _lock;
    Job*                       _job;                        // Es gibt nur einen Com_job pro Job
};

//-----------------------------------------------------------------------------------------Com_task

struct Com_task : spooler_com::Itask, 
                  spooler_com::Ihas_java_class_name, 
                  z::com::object_server::Ihas_reference_with_properties,
                  Sos_ole_object               
{
                                Com_task                    ( Task* = NULL );
                               ~Com_task                    ()                                      {}

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI


    // Interface Ihas_java_class_name

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Task"; }

    
    // Interface Ireference_with_properties
    
    ptr<z::com::object_server::Reference_with_properties> get_reference_with_properties();


    // Interface Itask

    void                        set_task                    ( Task* );
    Task*                       task                        ()                                      { return _task; }

    STDMETHODIMP            get_Object_set                  ( spooler_com::Iobject_set** );
    STDMETHODIMP            put_Error                       ( VARIANT* error_text );
    STDMETHODIMP            get_Error                       ( spooler_com::Ierror** );
    STDMETHODIMP            get_Job                         ( spooler_com::Ijob** );
    STDMETHODIMP            get_Params                      ( spooler_com::Ivariable_set** );
    STDMETHODIMP                Wait_until_terminated       ( double wait_time, VARIANT_BOOL* ok );
    STDMETHODIMP                End                         ();
    STDMETHODIMP            put_Result                      ( VARIANT* value );
    STDMETHODIMP            get_Result                      ( VARIANT* value );
    STDMETHODIMP            put_Repeat                      ( double seconds );
    STDMETHODIMP            put_History_field               ( BSTR name, VARIANT* value );
    STDMETHODIMP            get_Id                          ( int* value );
    STDMETHODIMP            put_Delay_spooler_process       ( VARIANT* time );
    STDMETHODIMP            put_Close_engine                ( VARIANT_BOOL );
    STDMETHODIMP            get_Order                       ( spooler_com::Iorder** );
    STDMETHODIMP            get_Changed_directories         ( BSTR* );
    STDMETHODIMP                Add_pid                     ( int, VARIANT* );
    STDMETHODIMP                Remove_pid                  ( int );
    STDMETHODIMP            get_Stderr_text                 ( BSTR* );
    STDMETHODIMP            get_Stderr_path                 ( BSTR* );
    STDMETHODIMP            get_Stdout_text                 ( BSTR* );
    STDMETHODIMP            get_Stdout_path                 ( BSTR* );
    STDMETHODIMP            get_Stderr_or_stdout_text_or_path( BSTR*, bool get_stderr, bool get_text );
    STDMETHODIMP                Create_subprocess           ( VARIANT*, spooler_com::Isubprocess** );
    STDMETHODIMP                Add_subprocess              ( int, double, VARIANT_BOOL, VARIANT_BOOL, BSTR );
    STDMETHODIMP            put_Priority                    ( int );
    STDMETHODIMP            get_Priority                    ( int* );
    STDMETHODIMP            put_Priority_class              ( BSTR );
    STDMETHODIMP            get_Priority_class              ( BSTR* );
    STDMETHODIMP            get_Step_count                  ( int* );
    STDMETHODIMP                Set_error_code_and_text     ( BSTR, BSTR );
    STDMETHODIMP            get_Web_service                 ( spooler_com::Iweb_service** );
    STDMETHODIMP            get_Web_service_or_null         ( spooler_com::Iweb_service** );

  private:
    Thread_semaphore           _lock;
    ptr<Task>                  _task;
};

//-----------------------------------------------------------------------------------Com_task_proxy
                                    
//Z_DEFINE_GUID( CLSID_Com_spooler_proxy, 0xfeee47aa, 0x6c1b, 0x11d8, 0x81, 0x03, 0x00, 0x04, 0x76, 0xee, 0x8a, 0xfb );   // {feee47aa-6c1b-11d8-8103-000476ee8afb}

struct Com_task_proxy : object_server::proxy_with_local_methods< Com_task_proxy, spooler_com::Itask_proxy >
{
    static Class_descriptor     class_descriptor;
    static const com::Com_method _methods[];


    static HRESULT              Create_instance             ( const IID& iid, ptr<IUnknown>* result );


                                Com_task_proxy              ();


    STDMETHODIMP                Create_subprocess           ( VARIANT* program_and_parameters, spooler_com::Isubprocess** result );
    STDMETHODIMP            put_Priority                    ( int );
    STDMETHODIMP            get_Priority                    ( int* );
    STDMETHODIMP            put_Priority_class              ( BSTR );
    STDMETHODIMP            get_Priority_class              ( BSTR* );

    void                        wait_for_subprocesses       ();


    ptr<Subprocess_register>   _subprocess_register;
};

//---------------------------------------------------------------------------------------Com_thread
/*
struct Com_thread : spooler_com::Ithread, 
                    spooler_com::Ihas_java_class_name, 
                    Sos_ole_object               
{
                                Com_thread                  ( Spooler_thread* );

    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_java_class_name             ( BSTR* result )                        { return String_to_bstr( "sos.spooler.Thread", result ); }

    void                        close                       ()                                      { THREAD_LOCK(_lock)  _thread = NULL; }

    STDMETHODIMP                get_Log                     ( spooler_com::Ilog** );
    STDMETHODIMP                get_Script                  ( IDispatch** );
  //STDMETHODIMP                put_Include_path            ( BSTR );
    STDMETHODIMP                get_Include_path            ( BSTR* );
    STDMETHODIMP                get_Name                    ( BSTR* );

  protected:
    Thread_semaphore           _lock;
    Spooler_thread*            _thread;                     // Es gibt nur einen Com_thread pro Spooler_thread
};
*/
//--------------------------------------------------------------------------------------Com_spooler

struct Com_spooler : spooler_com::Ispooler, 
                     spooler_com::Ihas_java_class_name, 
                     z::com::object_server::Ihas_reference_with_properties,
                     Sos_ole_object               
{
                                Com_spooler                 ();                                     // Für gcc 3.2. Nicht implementiert.
                                Com_spooler                 ( Spooler* ); 

    STDMETHODIMP                QueryInterface              ( const IID&, void** );
    
    USE_SOS_OLE_OBJECT_WITHOUT_QI

    // Interface Ihas_reference_with_properties
    ptr<object_server::Reference_with_properties> get_reference_with_properties();


    // interface Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Spooler"; }


    STDMETHODIMP            get_Log                         ( spooler_com::Ilog** );
    STDMETHODIMP            get_Param                       ( BSTR* );
    STDMETHODIMP            get_Id                          ( BSTR* );
    STDMETHODIMP            get_Script                      ( IDispatch** );
    STDMETHODIMP            get_Job                         ( BSTR job_name, spooler_com::Ijob** );
    STDMETHODIMP                Create_variable_set         ( spooler_com::Ivariable_set** );
  //STDMETHODIMP            put_Include_path                ( BSTR );
    STDMETHODIMP            get_Include_path                ( BSTR* );
    STDMETHODIMP            get_Log_dir                     ( BSTR* );
    STDMETHODIMP                Let_run_terminate_and_restart();
    STDMETHODIMP            get_Variables                   ( spooler_com::Ivariable_set** );
    STDMETHODIMP            put_Var                         ( BSTR name, VARIANT* value );
    STDMETHODIMP            get_Var                         ( BSTR name, VARIANT* value );
    STDMETHODIMP            get_Db_name                     ( BSTR* );
    STDMETHODIMP                Create_job_chain            ( spooler_com::Ijob_chain** );
    STDMETHODIMP                Add_job_chain               ( spooler_com::Ijob_chain* );
    STDMETHODIMP            get_Job_chain                   ( BSTR, spooler_com::Ijob_chain** );
    STDMETHODIMP                Create_order                ( spooler_com::Iorder** );
    STDMETHODIMP            get_Is_service                  ( VARIANT_BOOL* );
    STDMETHODIMP            get_Directory                   ( BSTR* );
    STDMETHODIMP                Job_chain_exists            ( BSTR name, VARIANT_BOOL* );
    STDMETHODIMP            get_Hostname                    ( BSTR* );
    STDMETHODIMP                Terminate                   ( int timeout );
    STDMETHODIMP                Terminate_and_restart       ( int timeout );
    STDMETHODIMP                Abort_immediately           ();
    STDMETHODIMP                Abort_immediately_and_restart();
    STDMETHODIMP            get_Db_variables_table_name     ( BSTR* );
    STDMETHODIMP            get_Db_tasks_table_name         ( BSTR* );
    STDMETHODIMP            get_Db_orders_table_name        ( BSTR* );
    STDMETHODIMP            get_Db_history_table_name       ( BSTR* );
    STDMETHODIMP            get_Db_order_history_table_name ( BSTR* );
    STDMETHODIMP            get_Ini_path                    ( BSTR* );
    STDMETHODIMP                Execute_xml                 ( BSTR, BSTR* );
    STDMETHODIMP            get_Tcp_port                    ( int* );
    STDMETHODIMP            get_Udp_port                    ( int* );
    STDMETHODIMP                Create_xslt_stylesheet      ( spooler_com::Ixslt_stylesheet** );


    void                        close                       ()                                      { THREAD_LOCK(_lock)  _spooler = NULL; }
    string                      stdout_path                 ();
    string                      stderr_path                 ();


  protected:
    Thread_semaphore           _lock;
    Spooler*                   _spooler;                    // Es gibt nur einen Com_spooler
};

//-----------------------------------------------------------------------------------Com_task_proxy

struct Com_spooler_proxy : object_server::proxy_with_local_methods< Com_spooler_proxy, spooler_com::Ispooler_proxy >
{
    static Class_descriptor     class_descriptor;
    static const com::Com_method _methods[];


    static HRESULT              Create_instance             ( const IID& iid, ptr<IUnknown>* result );


                                Com_spooler_proxy           ();


    STDMETHODIMP                Create_xslt_stylesheet      ( spooler_com::Ixslt_stylesheet** );
};

//--------------------------------------------------------------------------------------Com_context

struct Com_context : spooler_com::Ispooler_context, Sos_ole_object               
{
                                Com_context                 ();


    void                        close                       ();

    USE_SOS_OLE_OBJECT

    STDMETHODIMP            get_Log                         ( IDispatch** o )        { return _log    .CopyTo(o); }
    STDMETHODIMP            get_Spooler                     ( IDispatch** o )        { return _spooler.CopyTo(o); }
  //STDMETHODIMP            get_Thread                      ( IDispatch** o )        { return _thread .CopyTo(o); }
    STDMETHODIMP            get_Job                         ( IDispatch** o )        { return _job    .CopyTo(o); }
    STDMETHODIMP            get_Task                        ( IDispatch** o )        { return _task   .CopyTo(o); }


    Thread_semaphore           _lock;

    ptr<IDispatch>             _log;
    ptr<IDispatch>             _spooler;
  //ptr<IDispatch>             _thread;
    ptr<IDispatch>             _job;
    ptr<IDispatch>             _task;
/*
    ptr<spooler_com::Ilog>     _log;
    ptr<spooler_com::Ispooler> _spooler;
  //ptr<spooler_com::Ithread>  _thread;
    ptr<spooler_com::Ijob>     _job;
    ptr<spooler_com::Itask>    _task;
*/
};

//------------------------------------------------------------------------------------Com_job_chain

struct Com_job_chain : spooler_com::Ijob_chain, 
                       spooler_com::Ihas_java_class_name, 
                       Sos_ole_object               
{
                                Com_job_chain           ( Job_chain* );

    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    void                        close                   ()                                          { THREAD_LOCK( _lock )  _job_chain = NULL; }

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                          { return (char*)"sos.spooler.Job_chain"; }

    STDMETHODIMP            put_Name                    ( BSTR );
    STDMETHODIMP            get_Name                    ( BSTR* );

    STDMETHODIMP            get_Order_count             ( int* );
    
    STDMETHODIMP            get_Order_queue             ( VARIANT* state, spooler_com::Iorder_queue** );
    
    STDMETHODIMP            get_Node                    ( VARIANT* state, spooler_com::Ijob_chain_node** );

    STDMETHODIMP                Add_job                 ( VARIANT*, VARIANT*, VARIANT*, VARIANT* );
    STDMETHODIMP                Add_end_state           ( VARIANT* );

    STDMETHODIMP                Add_order               ( VARIANT* order_or_payload, spooler_com::Iorder** );
    STDMETHODIMP                Add_or_replace_order    ( spooler_com::Iorder* );

    STDMETHODIMP                Try_add_order           ( spooler_com::Iorder* order_or_payload, VARIANT_BOOL* );

    STDMETHODIMP            put_Orders_recoverable      ( VARIANT_BOOL );
    STDMETHODIMP            get_Orders_recoverable      ( VARIANT_BOOL* );

    STDMETHODIMP                Remove_all_pending_orders( int* result );

    STDMETHODIMP                Remove                  ();

    Thread_semaphore           _lock;
    Job_chain*                 _job_chain;
};

//-------------------------------------------------------------------------------Com_job_chain_node

struct Com_job_chain_node : spooler_com::Ijob_chain_node, 
                            spooler_com::Ihas_java_class_name, 
                            Sos_ole_object               
{
                                Com_job_chain_node      ();

    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                          { return (char*)"sos.spooler.Job_chain_node"; }

    STDMETHODIMP            get_State                   ( VARIANT* );
    STDMETHODIMP            get_Next_node               ( spooler_com::Ijob_chain_node** );
    STDMETHODIMP            get_Error_node              ( spooler_com::Ijob_chain_node** );
    STDMETHODIMP            get_Job                     ( spooler_com::Ijob** );
    STDMETHODIMP            get_Next_state              ( VARIANT* );
    STDMETHODIMP            get_Error_state             ( VARIANT* );
};

//----------------------------------------------------------------------------------------Com_order

struct Com_order : spooler_com::Iorder, 
                   spooler_com::Ihas_java_class_name, 
                   Sos_ole_object
{
                                Com_order               ( Order* );
                              //Com_order               ();

    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    void                        close                   ()                                          { THREAD_LOCK( _lock )  _order = NULL; }

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                          { return (char*)"sos.spooler.Order"; }

    STDMETHODIMP            put_Id                      ( VARIANT* );
    STDMETHODIMP            get_Id                      ( VARIANT* );

    STDMETHODIMP            put_Title                   ( BSTR );
    STDMETHODIMP            get_Title                   ( BSTR* );
    
    STDMETHODIMP            put_Priority                ( int );
    STDMETHODIMP            get_Priority                ( int* );
    
    STDMETHODIMP            get_Job_chain               ( spooler_com::Ijob_chain** );

    STDMETHODIMP            get_Job_chain_node          ( spooler_com::Ijob_chain_node** );
    
    STDMETHODIMP            put_Job                     ( VARIANT* );
    STDMETHODIMP         putref_Job                     ( spooler_com::Ijob* job )                  { Variant v = job; return put_Job( &v ); }
    STDMETHODIMP            get_Job                     ( spooler_com::Ijob** );
    
    STDMETHODIMP            put_State                   ( VARIANT* );
    STDMETHODIMP            get_State                   ( VARIANT* );
    
    STDMETHODIMP            put_State_text              ( BSTR );
    STDMETHODIMP            get_State_text              ( BSTR* );
    
    STDMETHODIMP            get_Error                   ( spooler_com::Ierror** );
    
    STDMETHODIMP            put_Payload                 ( VARIANT* );
    STDMETHODIMP         putref_Payload                 ( IUnknown* );
    STDMETHODIMP            get_Payload                 ( VARIANT* );
    
    STDMETHODIMP                Payload_is_type         ( BSTR, VARIANT_BOOL* );

    STDMETHODIMP                Setback                 (); 

    STDMETHODIMP            put_At                      ( VARIANT* );
    STDMETHODIMP            get_At                      ( DATE* );

    STDMETHODIMP            get_Run_time                ( spooler_com::Irun_time** );

    STDMETHODIMP                Remove_from_job_chain   ();

    STDMETHODIMP            get_String_next_start_time  ( BSTR* );

  //STDMETHODIMP            get_Log                     ( spooler_com::Ilog** );

    STDMETHODIMP            get_Xml                     ( BSTR, BSTR* );

    STDMETHODIMP        get_Web_service                 ( spooler_com::Iweb_service** );
    STDMETHODIMP        get_Web_service_or_null         ( spooler_com::Iweb_service** );

    STDMETHODIMP        get_Web_service_operation        ( spooler_com::Iweb_service_operation** );
    STDMETHODIMP        get_Web_service_operation_or_null( spooler_com::Iweb_service_operation** );

    STDMETHODIMP            put_Xml_payload             ( BSTR );
    STDMETHODIMP            get_Xml_payload             ( BSTR* );
    

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


    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                          { return (char*)"sos.spooler.Order_queue"; }

    STDMETHODIMP            get_Length                  ( int* );
    STDMETHODIMP                Add_order               ( VARIANT*, spooler_com::Iorder** );


  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
};

//-----------------------------------------------------------------------------------Com_subprocess

struct Com_subprocess : spooler_com::Isubprocess, 
                        spooler_com::Ihas_java_class_name, 
                        Sos_ole_object               
{
                                Com_subprocess          ();


    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                            { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                          { return (char*)"sos.spooler.Subprocess"; }

    STDMETHODIMP                Close                   ();
    STDMETHODIMP            get_Pid                     ( int* );
    STDMETHODIMP            get_Terminated              ( VARIANT_BOOL* );
    STDMETHODIMP            get_Exit_code               ( int* );
  //STDMETHODIMP            get_Stdout_path             ( BSTR* );
  //STDMETHODIMP            get_Stderr_path             ( BSTR* );
    STDMETHODIMP            put_Ignore_error            ( VARIANT_BOOL );
    STDMETHODIMP            get_Ignore_error            ( VARIANT_BOOL* );
    STDMETHODIMP            put_Ignore_signal           ( VARIANT_BOOL );
    STDMETHODIMP            get_Ignore_signal           ( VARIANT_BOOL* );
    STDMETHODIMP                Wait_for_termination    ( VARIANT*, VARIANT_BOOL* );
    STDMETHODIMP                Kill                    ( int signal );

  private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    ptr<Subprocess>            _subprocess;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
