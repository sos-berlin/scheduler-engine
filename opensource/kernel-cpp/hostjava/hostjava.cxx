// $Id: hostjava.cxx 13912 2010-06-30 15:41:14Z ss $

#include "hostjava_common.h"
#include "../zschimmer/com.h"
#include "../zschimmer/java.h"
#include "../hostole/hostole_ptr.h"


using namespace zschimmer;
using namespace zschimmer::com;
using namespace zschimmer::javabridge;
using namespace std;



namespace sos {
namespace hostjava {

//-------------------------------------------------------------------------------------------static

static Mutex                    hostjava_mutex ( "hostjava" );
static Ihostware*               hostware_ptr;                       // Nicht Klasse Hostware, damit Destruktor und dadurch entladene hostole.dll nicht gerufen wird
static ptr<javabridge::Vm>      java_vm;
//const JNINativeMethod           native_methods[];                   // Siehe am Ende von hostjava.cxx

//--------------------------------------------------------------------------------------------const
/*
struct Hostjava_init
{
    Hostjava_init()   
    { 
/ *
#       ifdef Z_WINDOWS
            string log_file = read_profile_string( "", "hostjava", "log", "" );
            if( !log_file.empty() )  log_start( log_file );
#       endif
* /
        //CoInitialize(NULL); 
    }

    ~Hostjava_init()
    { 
        //if( object_count > 0 )  fprintf( stderr, "Hostjava: %d Objekte nicht freigegeben\n", object_count );
        //Log_ptr::set_stream_and_system_mutex( NULL, NULL );     // Kein Thread darf laufen!

        // Nicht in DllMain() rufen! CoUninitialize(); 
    }
};
static Hostjava_init hostjava_init;
*/

//---------------------------------------------------------------------------------------JNI_OnLoad

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* jvm, void* )
{
    //HRESULT hr;

  //try
    {
        zschimmer_init();

        java_vm = Z_NEW( javabridge::Vm( jvm ) );
        java_vm._ptr->AddRef();                 // Damit bei Programmende nicht Release gerufen wird (die Java-DLL ist dann vielleicht schon entladen)
        //java::Vm::set_jvm( jvm );
    }
  //catch( const exception&  x ) { env.set_java_exception( x );  return 0; }
  //catch( const _com_error& x ) { env.set_java_exception( x );  return 0; }

    //log_categories.set( "mutex" );

    return JNI_VERSION_1_4;
}

//-------------------------------------------------------------------------------------JNI_OnUnload

extern "C" JNIEXPORT void JNICALL JNI_OnUnload( JavaVM*, void* )
{
    // Wird nur gerufen, wenn Javas Garbage Collector den Class Loader entlädt. Das passiert offenbar selten.

    if( hostware_ptr )
    {
        hostware_ptr->Release();
        hostware_ptr = NULL;

        if( java_vm )  java_vm._ptr->Release();
        java_vm = NULL;
    }

    zschimmer_terminate();
    //CoUninitialize();
}

//------------------------------------------------------------------------------------init_hostware

static void init_hostware( JNIEnv* jenv )
{
    Z_MUTEX( hostjava_mutex )
    {
        if( !hostware_ptr )
        {
            hostole::Hostware hostware;
            hostware.create_instance();
            hostware.need_version( "1.6.107" );

            JavaVM* java_vm = NULL;
            jenv->GetJavaVM( &java_vm );
            
            //hostware._ptr->putref_Java_vm( java_vm );
            //hostware.set_java_vm( java_vm );
            hostware.set_com_context();
          
            hostware_ptr = hostware.take();
        }
    }
}

//-------------------------------------------------------------sos.hostware.Idispatch.com_construct

extern "C"
JNIEXPORT jlong JNICALL Java_sos_hostware_Idispatch_com_1construct( JNIEnv* jenv, jclass, jstring class_name_j )
{
    Com_env env    = jenv;
    jlong   result = 0;

    try
    {
        HRESULT         hr;
        ptr<IDispatch>  idispatch;
        Bstr            class_name_bstr;
        bool            coinitialize_called = false;

        env.jstring_to_bstr( class_name_j, &class_name_bstr );

        hr = idispatch.CoCreateInstance( class_name_bstr );

        if( hr == CO_E_NOTINITIALIZED  &&  !coinitialize_called )
        {
            //hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );  if( FAILED(hr) )  throw_com( hr, "CoInitializeEx(,COINIT_MULTITHREADED)" );
            hr = CoInitialize( NULL );  if( FAILED(hr) )  throw_com( hr, "CoInitialize()" );

            hr = idispatch.CoCreateInstance( class_name_bstr );
        }

        if( FAILED(hr) )  throw_com( hr, "CoCreateInstance", string_from_bstr( class_name_bstr ) );


        if( !hostware_ptr )  init_hostware( jenv );


        result = (jlong)(size_t)+idispatch;
        idispatch._ptr = NULL;
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//---------------------------------------------------------------sos.hostware.Idispatch.com_release
// Kann von einem anderen Thread (vom Garbage collector) gerufen werden!

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Idispatch_com_1release( JNIEnv* jenv, jclass, jlong idispatch_long )
{
    Com_env env = jenv;

    try
    {
        IDispatch* idispatch = (IDispatch*)(size_t)idispatch_long;

        if( !idispatch_long )  { env.set_NullPointerException( "hostjava" ); return; }

        idispatch->Release();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//----------------------------------------------------------------sos.hostware.Idispatch.com_get_id

extern "C"
JNIEXPORT jint JNICALL Java_sos_hostware_Idispatch_com_1get_1id( JNIEnv* jenv, jclass, jlong jidispatch, jstring jname )
{
    Com_env env    = jenv;
    jint    result = 0;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        result = env.java_com_get_dispid( jidispatch, jname );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//---------------------------------------------------------------sos.hostware.Idispatch.com_call_id

extern "C"
JNIEXPORT jobject JNICALL Java_sos_hostware_Idispatch_com_1call_1id( JNIEnv* jenv, jclass cls, jlong jidispatch, jint dispid, jint com_context, jobjectArray jparams )
{
    Com_env env    = jenv;
    jobject result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return NULL; }

        Variant variant = env.variant_java_com_call( cls, jidispatch, dispid, (WORD)com_context, jparams );
        result = env.jobject_from_variant( variant );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//------------------------------------------------------------------sos.hostware.Idispatch.com_call

extern "C"
JNIEXPORT jobject JNICALL Java_sos_hostware_Idispatch_com_1call( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    Com_env env    = jenv;
    jobject result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return NULL; }

        Variant variant = env.variant_java_com_call( cls, jidispatch, jname, jparams );
        result = env.jobject_from_variant( variant );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//-------------------------------------------------------sos.hostware.Idispatch.boolean_com_call_id

extern "C"
JNIEXPORT jboolean JNICALL Java_sos_hostware_Idispatch_boolean_1com_1call_1id( JNIEnv* jenv, jclass cls, jlong jidispatch, jint dispid, int com_context, jobjectArray jparams )
{
    Com_env  env    = jenv;
    jboolean result = false;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        Variant variant_result = env.variant_java_com_call( cls, jidispatch, dispid, (WORD)com_context, jparams );
        variant_result.ChangeType( VT_BOOL );
        result = V_BOOL( &variant_result ) != 0;
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//----------------------------------------------------------sos.hostware.Idispatch.boolean_com_call

extern "C"
JNIEXPORT jboolean JNICALL Java_sos_hostware_Idispatch_boolean_1com_1call( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    Com_env  env    = jenv;
    jboolean result = false;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        Variant variant_result = env.variant_java_com_call( cls, jidispatch, jname, jparams );
        variant_result.ChangeType( VT_BOOL );
        result = V_BOOL( &variant_result ) != 0;
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//--------------------------------------------------------sos.hostware.Idispatch.string_com_call_id

extern "C"
JNIEXPORT jstring JNICALL Java_sos_hostware_Idispatch_string_1com_1call_1id( JNIEnv* jenv, jclass cls, jlong jidispatch, jint dispid, jint com_context, jobjectArray jparams )
{
    Com_env env    = jenv;
    jstring result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        Variant variant = env.variant_java_com_call( cls, jidispatch, dispid, (WORD)com_context, jparams );

        if( variant.vt == VT_NULL
         || variant.vt == VT_DISPATCH  &&  V_DISPATCH( &variant ) == NULL )
        {
            result = NULL;
        }
        else
            result = env.jstring_from_string( string_from_variant( variant ) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//-----------------------------------------------------------sos.hostware.Idispatch.string_com_call

extern "C"
JNIEXPORT jstring JNICALL Java_sos_hostware_Idispatch_string_1com_1call( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    Com_env env    = jenv;
    jstring result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        Variant variant = env.variant_java_com_call( cls, jidispatch, jname, jparams );

        if( variant.vt == VT_NULL
            || variant.vt == VT_DISPATCH  &&  V_DISPATCH( &variant ) == NULL )
        {
            result = NULL;
        }
        else
            result = env.jstring_from_string( string_from_variant( variant ) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//-------------------------------------------------sos.hostware.Idispatch.string_com_call_id_string

extern "C"
JNIEXPORT jstring JNICALL Java_sos_hostware_Idispatch_string_1com_1call_1id_1string( JNIEnv* jenv, jclass cls, jlong jidispatch, jint dispid, jint com_context, jstring par1 )
{
    Com_env env    = jenv;
    jstring result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        Variant variant = env.variant_java_com_call( cls, jidispatch, dispid, (WORD)com_context, par1 );

        if( variant.vt == VT_BSTR )
        {
            result = env.jstring_from_bstr( V_BSTR( &variant ) );
        }
        else
        if( variant.vt == VT_NULL
         || variant.vt == VT_DISPATCH  &&  V_DISPATCH( &variant ) == NULL )
        {
            result = NULL;
        }
        else
        {
            result = env.jstring_from_string( string_from_variant( variant ) );
        }
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//----------------------------------------------------sos.hostware.Idispatch.string_com_call_string

extern "C"
JNIEXPORT jstring JNICALL Java_sos_hostware_Idispatch_string_1com_1call_1string( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jstring par1 )
{
    Com_env env    = jenv;
    jstring result = NULL;

    try
    {
        if( !jidispatch )  { env.set_NullPointerException( "hostjava" ); return result; }

        WORD   com_context = 0;
        DISPID dispid = env.java_com_get_dispid( jidispatch, jname, &com_context );
        result = Java_sos_hostware_Idispatch_string_1com_1call_1id_1string( jenv, cls, jidispatch, dispid, com_context, par1 );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//------------------------------------------------sos.hostware.Idispatch.string_com_call_string_int
/*
extern "C"
JNIEXPORT jstring JNICALL Java_sos_hostware_Idispatch_string_1com_1call_1string( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jint par1 )
{
    jstring result = NULL;

    try
    {
        result = jstring_from_string( jenv, string_from_variant( variant_java_com_call( jenv, cls, jidispatch, jname, jparams ) ) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}
*/
//------------------------------------------------------------------------------------enter_object
/*
void enter_object( IUnknown* object )
{
    thread_map[ GetCurrentThreadId() ].push_back( object );
}

//-----------------------------------------------------------------------------------remove_object

void remove_object( IUnknown* object )
{
    thread_map[ GetCurrentThreadId() ].push_back( object );
}


//object_is_valid

bool object_is_valid( const IUnknown* object )
{
    Object_map om = thread_map[ GetCurrentThreadId() ];
    Object_map::iterator o = om.find( object );
    return o != om.end();
}
*/
//-----------------------------------------------------------------------------------------DllMain
#ifdef SYSTEM_WIN
/*
extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, LPVOID )
{
    switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH: 
        {
            _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );   // Speicherprüfung bei _DEBUG
            break;
        }

    	case DLL_PROCESS_DETACH: 
        {
            break;
        }

		case DLL_THREAD_ATTACH:
        {
            break;
        }

    	case DLL_THREAD_DETACH: 
        {
            // Wenn der Thread endet, dann alle Hostole-Objekte schließen. Denn sonst ruft der Garbage Collector 
            // die finalize() Methode in einem anderen Thread auf, und da knallt's, weil COM die Objekte selbst
            // schon geschlossen hat (das liegt wohl am Appartement Model).
            // In Java sollte _immer_ die close() Methode gerufen werden.

            Thread_id thread_id = GetCurrentThreadId();

            THREAD_LOCK( thread_map_sema )
            {
                Thread_map::iterator t = thread_map.find( thread_id );
                if( t != thread_map.end() ) 
                {
                    Z_FOR_EACH( Object_map, *t, o )  o->Release();
                    thread_map.erase( t );
                }
            }

            break;
        }

		default: 
            break;
	} 

    return TRUE;
}
*/
#endif
//--------------------------------------------------------------------------------Hostware_std_type
/*
enum Hostware_std_type
{
    hwst_none       = 0,
    hwst_char       = 1,
    hwst_varchar    = 2,
    hwst_decimal    = 3,
    hwst_integer    = 4,
    hwst_float      = 5,
    hwst_date       = 6,
    hwst_time       = 7,
    hwst_date_time  = 8,
    hwst_boolean    = 9
};
*/
//----------------------------------------------------------------------------------------Type_info
/*
public class Type_info
{
    public native String        name                        ();
};
*/
//---------------------------------------------------------------------------------------Type_param
/*

public class Type_param
{
        [propget] HRESULT std_type      ( [out,retval] Hostware_std_type* o );
        [propget] HRESULT size          ( [out,retval] long* o );
        [propget] HRESULT display_size  ( [out,retval] long* o );
        [propget] HRESULT precision     ( [out,retval] long* o );
        [propget] HRESULT radix         ( [out,retval] long* o );
        [propget] HRESULT scale         ( [out,retval] VARIANT* o );     // long oder NULL
        [propget] HRESULT usigned       ( [out,retval] VARIANT_BOOL* o );
        [propget] HRESULT info          ( [out,retval] Ihostware_type_info** o );
};

*/
//---------------------------------------------------------------------------------------Field_type
/*
public class Field_type
{
        [propget] HRESULT field_size    ( [out,retval] long* o );
        [propget] HRESULT info          ( [out,retval] Ihostware_type_info** o );
        [propget] HRESULT param         ( [out,retval] Ihostware_type_param** o );
};
*/
//--------------------------------------------------------------------------------------Record_type
/*
public class Record_Type
{
    public native void          name( [out,retval] BSTR* name );
    public native void          field_count( [out,retval] long* number );
    public native void          field_descr( [in] long i, [out,retval] Ihostware_field_descr** o );
    public native void          add_field_descr( [in] Ihostware_field_descr* o );
    public native void          add_field( [in] BSTR name, [in] BSTR type );
    public native void          param( [out,retval] Ihostware_type_param** o );
};
*/
//--------------------------------------------------------------------------------------Field_descr
/*
public class Field_descr
{
    public native void          name( [out,retval] BSTR* name );
    public native void          type( [out,retval] Ihostware_field_type** type );
    public native void          offset( [out,retval] long* o );
    public native void          remark( [out,retval] BSTR* o );
};
*/
//------------------------------------------------------------------------------------Script_object
/*
public class Script_object
{
    public native void          obj_close();
    public native void          obj_set_language( [in] BSTR scripting_engine_name );
    public native void          obj_language( [out,retval] BSTR* scripting_engine_name );
    public native void          obj_parse( [in] BSTR script_text, [in,defaultvalue(scripttext_isvisible)] enum Scripttext_flags, [out,retval] VARIANT* result );
    public native void          obj_eval( [in] BSTR script_text, [in,defaultvalue(0)] enum Scripttext_flags, [out,retval] VARIANT* result );
    public native void          obj_name_exists( [in] BSTR sub_name, [out,retval] VARIANT_BOOL* );
};
*/
//----------------------------------------------------------------------------------------Variables
/*
public class Variables
{
    public native void          set_var                     ( [in] BSTR name, [in] VARIANT* value );
    public native void          set_value                   ( [in] BSTR name, [in] VARIANT* value );
    public native void          value                       ( [in] BSTR name, [out,retval] Ivariable** result );
    public native void          count                       ( [out,retval] int* value );

    public native void          clone                       ( [out,retval] Ivariables** result );

        [id(DISPID_NEWENUM),propget,restricted]
    public native void          _NewEnum                    ( [out,retval] IUnknown** enumerator );
};
*/
//----------------------------------------------------------Hostware_variables_enumerator
/*
    interface Ivariables_enumerator : IEnumVARIANT
    {
    public native void          Next                    ( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched );
    public native void          Skip                    ( unsigned long celt );
    public native void          Reset                   ();
    public native void          Clone                   ( Ivariables_enumerator** ppenum );
    }
*/
//---------------------------------------------------------------------------------Word_application
/*
public class Word_application
{
    public native void          kill_all_words          ( [in,defaultvalue("")] BSTR empty, [out,retval] int* count );
    public native void          load                    ();
    public native void          app                     ( [out,retval] IDispatch** word_application_interface );
    public native void          print                   ( [in] BSTR filename, [in] BSTR parameters );
}
*/
//--------------------------------------------------------------------------------------Ghostscript
/*
public class Ghostscript
{
    public native void          run                     ( [in] BSTR parameters );
    public native void          collect_stdout          ( [out,retval] VARIANT_BOOL* collect );
    public native void          set_collect_stdout          ( [in] VARIANT_BOOL collect );
    public native void          stdout                  ( [out,retval] BSTR* stdout_text );
    public native void          init                    ();
    public native void          close                   ();
};
*/
//-------------------------------------------------------------------------------------------------



//-----------------------------------------------------------------------------------------init_com
/*
void init_com()
{
    static 
    //Z_MICROSOFT_ONLY( __declspec(thread) ) 
    bool com_initialized = false;

    if( !com_initialized )  CoInitialize(NULL), com_initialized = true;
}
*/
//----------------------------------------------------------------------------------jstring_to_bstr
/*
void jstring_to_bstr( JNIEnv* jenv, const jstring& jstr, BSTR* bstr )
{
    const OLECHAR* str_w = jenv->GetStringChars( jstr, 0 );
    
    *bstr = SysAllocString( str_w );
    
    jenv->ReleaseStringChars( jstr, str_w );
}
*/
//--------------------------------------------------------------------------------bstr_from_jstring
/*
inline Bstr bstr_from_jstring( JNIEnv* jenv, const jstring& jstr )
{
    Bstr bstr;
    jstring_to_bstr( jenv, jstr, &bstr );
    return bstr;
}
*/
//-------------------------------------------------------------------------------set_java_exception
/*
void set_java_exception( JNIEnv* jenv, const char* what )
{
    const char* exception_class_name = "java/lang/RuntimeException";
    jclass exception_class = jenv->FindClass( exception_class_name  );
  //jclass exception_class = jenv->FindClass( "java.lang.Throwable" );

    if( exception_class )  
    {
        int err = jenv->ThrowNew( exception_class, what ); 
        if( err == 0 )  return;
    }

    string w = what + string(" --- JNI.FindClass() liefert nicht Klasse ") + exception_class_name;

    if( jenv->ExceptionOccurred() )  jenv->ExceptionDescribe();      // schreibt nach stderr

    jenv->FatalError( w.c_str() );
}

//-------------------------------------------------------------------------------set_java_exception

void set_java_exception( JNIEnv* jenv, const exception& x )
{
    env.set_java_exception( x.what() );
}

//-------------------------------------------------------------------------------set_java_exception

void set_java_exception( JNIEnv* jenv, const _com_error& x ) 
{
    string what = string_from_ole( x.Description() );
    env.set_java_exception( what.c_str() );
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace hostjava
} //namespace sos

//------------------------------------------------------------------------------------------DllMain
/*
extern "C" BOOL WINAPI DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )
{
    BOOL result = TRUE;

    switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH: 
        {
            try 
            {
#               ifdef SYSTEM_MICROSOFT
                    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );   // Speicherprüfung bei _DEBUG
#               endif

                CoInitialize(NULL);
            }
            catch( const Xc& ) { result = FALSE; }
            break;
        }

    	case DLL_PROCESS_DETACH: 
        {
            CoUninitialize();
            break;
        }

        case DLL_THREAD_ATTACH : CoInitialize(NULL);   break;
        case DLL_THREAD_DETACH : CoUninitialize(); break;

		default: break;
	} 

    return result;
}

*/

//-------------------------------------------------------------------------------------------------

namespace sos {
namespace hostjava {

//---------------------------------------------------------------------------------get_my_idispatch

void* get_my_idispatch( JNIEnv* jenv, jobject jo, jfieldID* field_id )
{
    jfieldID my_data_id = jenv->GetFieldID( jenv->GetObjectClass( jo ), "_idispatch", "J" );
    if( field_id )  *field_id = my_data_id;

    return (void*)(size_t)jenv->GetLongField( jo, my_data_id );
}

//--------------------------------------------------------------------------------------------const
// Für HP-UX: libhostjava.sl lässt sich nicht binden (statische Variablen werden nicht initialisiert), deshalb wird's in den Scheduler eingebunden

const static JNINativeMethod idispatch_native_methods[] = 
{
    // sos.hostware.Idispatch
    { "com_construct"           , "(Ljava/lang/String;)J"                                       , (void*)Java_sos_hostware_Idispatch_com_1construct },
    { "com_release"             , "(J)J"                                                        , (void*)Java_sos_hostware_Idispatch_com_1release },
    { "com_call"                , "(JLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;"  , (void*)Java_sos_hostware_Idispatch_com_1call },
    { "boolean_com_call"        , "(JLjava/lang/String;[Ljava/lang/Object;)Z"                   , (void*)Java_sos_hostware_Idispatch_boolean_1com_1call },
    { "string_com_call"         , "(JLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;"  , (void*)Java_sos_hostware_Idispatch_string_1com_1call },
    { "string_com_call_string"  , "(JLjava/lang/String;Ljava/lang/String;)Ljava/lang/String;"   , (void*)Java_sos_hostware_Idispatch_string_1com_1call_1string },
};

const static JNINativeMethod factory_processor_native_methods[] = 
{
    // sos.hostware.Factory_processor
    { "close_native"            , "()V"                                     , (void*)Java_sos_hostware_Factory_1processor_close_1native },
    { "eval"                    , "(Ljava/lang/String;I)Ljava/lang/String;" , (void*)Java_sos_hostware_Factory_1processor_eval },
    { "parse"                   , "(Ljava/lang/String;I)V"                  , (void*)Java_sos_hostware_Factory_1processor_parse },
    { "add_parameters"          , "()V"                                     , (void*)Java_sos_hostware_Factory_1processor_add_1parameters },
    { "process"                 , "()V"                                     , (void*)Java_sos_hostware_Factory_1processor_process },
    { "script_text"             , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_script_1text },
    { "error_filename"          , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_error_1filename },
    { "error_document"          , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_error_1document },
    { "set_document_filename"   , "(Ljava/lang/String;)V"                   , (void*)Java_sos_hostware_Factory_1processor_set_1document_1filename },
    { "document_filename"       , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_document_1filename },
    { "set_head_filename"       , "(Ljava/lang/String;)V"                   , (void*)Java_sos_hostware_Factory_1processor_set_1head_1filename },
    { "head_filename"           , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_head_1filename },
    { "set_language"            , "(Ljava/lang/String;)V"                   , (void*)Java_sos_hostware_Factory_1processor_set_1language },
    { "language"                , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_language },
    { "set_param"               , "(Ljava/lang/String;)V"                   , (void*)Java_sos_hostware_Factory_1processor_set_1param },
    { "set_parameter"           , "(Ljava/lang/String;Ljava/lang/String;)V" , (void*)Java_sos_hostware_Factory_1processor_set_1parameter },
    { "set_parameter_bool"      , "(Ljava/lang/String;Z)V"                  , (void*)Java_sos_hostware_Factory_1processor_set_1parameter_1bool },
    { "set_parameter_int"       , "(Ljava/lang/String;I)V"                  , (void*)Java_sos_hostware_Factory_1processor_set_1parameter_1int },
    { "set_parameter_double"    , "(Ljava/lang/String;D)V"                  , (void*)Java_sos_hostware_Factory_1processor_set_1parameter_1double },
    { "set_parameter_currency"  , "(Ljava/lang/String;J)V"                  , (void*)Java_sos_hostware_Factory_1processor_set_1parameter_1currency },
    { "parameter_as_string"     , "(Ljava/lang/String;)Ljava/lang/String;"  , (void*)Java_sos_hostware_Factory_1processor_parameter_1as_1string },
    { "set_template_filename"   , "(Ljava/lang/String;)V"                   , (void*)Java_sos_hostware_Factory_1processor_set_1template_1filename },
    { "template_filename"       , "()Ljava/lang/String;"                    , (void*)Java_sos_hostware_Factory_1processor_template_1filename },
};

//------------------------------------------------------------------------------------init_hostjava

void register_natives( javabridge::Vm* java_vm, const string& class_path, const JNINativeMethod* native_methods, int native_methods_count )
{
    Env env = java_vm->jni_env();

    Class my_class ( class_path.c_str() );
    if( env->ExceptionCheck() )  env.throw_java( "FindClass " + class_path );

    if( Log_ptr log = "java" )
    {
        log << "RegisterNatives \"" << class_path << "\",\n";
        for( int i = 0; i < native_methods_count; i++ )  log << "    " << native_methods[i].name << native_methods[i].signature << "\n";
    }

    int ret = env->RegisterNatives( my_class, native_methods, native_methods_count );
    if( ret < 0 )  throw_java_ret( ret, "RegisterNatives" );
}

//------------------------------------------------------------------------------------init_hostjava

void init_hostjava( javabridge::Vm* java_vm )
{
    register_natives( java_vm, "sos/hostware/Idispatch"        , idispatch_native_methods        , NO_OF( idispatch_native_methods )  );
    register_natives( java_vm, "sos/hostware/Factory_processor", factory_processor_native_methods, NO_OF( factory_processor_native_methods ) );
    init_hostjava_process_with_pid( java_vm );
}

//-------------------------------------------------------------------------------------------------

} //namespace hostjava
} //namespace sos

