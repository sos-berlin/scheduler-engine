// $Id: spooler_module_java.cxx,v 1.33 2003/02/12 18:31:12 jz Exp $
/*
    Hier sind implementiert

    Java_vm
    Module::java_method_id
    Java_module_instance
*/

#include "spooler.h"
#include "../file/stdfile.h"    // make_path

#ifdef _DEBUG
#   include "Debug/sos/spooler/Idispatch.h"
#else
#   include "Release/sos/spooler/Idispatch.h"
#endif

#include <sys/stat.h>

#ifdef Z_WINDOWS
#   include <sys/utime.h>
#else
#   include <utime.h>
#   include <dlfcn.h>
#endif

#ifdef Z_HPUX
#   include <dl.h>
#endif

#ifndef JNI_VERSION_1_2
#   error "Der Spooler braucht jni.h Version 1.2"
#endif

using namespace std;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const static JNINativeMethod native_methods[] = 
{
    { "com_call", "(JLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;", (void*)Java_sos_spooler_Idispatch_com_1call }
};

//-------------------------------------------------------------------------------------------static

static Java_vm* java_vm;        // Für java_vfprintf() und für native Funktionen

//-------------------------------------------------------------------------------set_java_exception

static void set_java_exception( JNIEnv* jenv, const char* what )
{
    const char* exception_class_name = "java/lang/RuntimeException";
    jclass exception_class = jenv->FindClass( exception_class_name  );

    if( exception_class )  
    {
        int err = jenv->ThrowNew( exception_class, what ); 
        if( err == 0 )  return;
    }

    string w = what + string(" --- JNI.FindClass() liefert nicht Klasse ") + exception_class_name;

    if( jenv->ExceptionCheck() )  jenv->ExceptionDescribe();      // schreibt nach stderr

    jenv->FatalError( w.c_str() );
}

//-------------------------------------------------------------------------------set_java_exception

static void set_java_exception( JNIEnv* jenv, const exception& x )
{
    set_java_exception( jenv, x.what() );
}

//-------------------------------------------------------------------------------set_java_exception

static void set_java_exception( JNIEnv* jenv, const _com_error& x ) 
{
    string what = string_from_ole( x.Description() );
    set_java_exception( jenv, what.c_str() );
}

//------------------------------------------------------------------------------string_from_jstring

static string string_from_jstring( JNIEnv* jenv, const jstring& jstr )
{
    if( !jstr )  return "";

    const OLECHAR* str_w = jenv->GetStringChars( jstr, 0 );
   
    string result = string_from_ole( str_w );

    jenv->ReleaseStringChars( jstr, str_w );

    return result;
}

//----------------------------------------------------------------------------------jstring_to_bstr

static void jstring_to_bstr( JNIEnv* jenv, const jstring& jstr, BSTR* bstr )
{
    const OLECHAR* str_w = jenv->GetStringChars( jstr, 0 );
    
    HRESULT hr = string_to_bstr( str_w, bstr );
    
    jenv->ReleaseStringChars( jstr, str_w );

    if( FAILED(hr) )  throw_com( hr, "jstring_to_bstr/string_to_bstr" );
}

//--------------------------------------------------------------------------------jstring_from_bstr

inline jstring jstring_from_bstr( JNIEnv* jenv, const BSTR bstr )
{
//#   ifdef HOSTJAVA_OLECHAR_IS_WCHAR
        return jenv->NewString( bstr, SysStringLen(bstr) );
//#    else
//        std::string str = string_from_bstr( bstr );
//        return jenv->NewString( str.c_str() );
//#   endif
}

//------------------------------------------------------------------------------------java_vfprintf

static jint JNICALL java_vfprintf( FILE *fp, const char *format, va_list args )
{
    char buf[1024];

    int ret = _vsnprintf( buf, sizeof(buf), format, args );

    java_vm->_log( buf );

    return ret;
}

//-----------------------------------------------------------------------------jobject_from_variant

static jobject jobject_from_variant( JNIEnv* jenv, const VARIANT& v )
{
    jobject result = NULL;

    switch( v.vt )
    {
        case VT_EMPTY:
            return jenv->NewString( NULL, 0 );       // Für Job_chain_node.next_state, .error_state ("" wird zu VT_EMPTY)

        case VT_ERROR:
            if( v.scode == DISP_E_PARAMNOTFOUND ) return jenv->NewString( NULL, 0 );       // Für Job_chain_node.next_state, .error_state ("" wird zu VT_EMPTY)

            throw_com( v.scode, "Variant VT_ERROR" );

      //case VT_NULL: 
      //    return NULL;    //?

        case VT_I2:
        {
            jclass cls = jenv->FindClass( "java/lang/Short" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(S)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jshort)V_I2(&v) );
            break;
        }

        case VT_I4:
        {
            jclass cls = jenv->FindClass( "java/lang/Integer" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(I)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jint)V_I4(&v) );
            break;
        }

        case VT_R4:
        {
            jclass cls = jenv->FindClass( "java/lang/Float" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(F)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jfloat)V_R4(&v) );
            break;
        }

        case VT_R8:
        {
            jclass cls = jenv->FindClass( "java/lang/Double" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(D)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jdouble)V_R8(&v) );
            break;
        }

      //case VT_CY: 
      //case VT_DATE:       

        case VT_BOOL:
        {
            jclass cls = jenv->FindClass( "java/lang/Boolean" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(Z)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jboolean)V_BOOL(&v)? 1 : 0 );
            break;
        }

        case VT_I1:  
        {
            jclass cls = jenv->FindClass( "java/lang/Byte" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(B)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jbyte)V_I1(&v) );
            break;
        }

        case VT_UI1: 
        {
            jclass cls = jenv->FindClass( "java/lang/Short" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(S)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jshort)V_UI1(&v) );
            break;
        }

        case VT_UI2: 
        {
            jclass cls = jenv->FindClass( "java/lang/Integer" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(I)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jint)V_UI2(&v) );
            break;
        }

        case VT_UI4: 
        {
            jclass cls = jenv->FindClass( "java/lang/Long" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(J)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jlong)V_UI4(&v) );
            break;
        }

        case VT_I8:  
        {
            jclass cls = jenv->FindClass( "java/lang/Long" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(J)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jlong)V_I8(&v) );
            break;
        }

      //case VT_UI8: 
        case VT_INT: 
        {
            jclass cls = jenv->FindClass( "java/lang/Integer" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(I)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jint)V_INT(&v) );
            break;
        }

        case VT_UINT:
        {
            jclass cls = jenv->FindClass( "java/lang/Long" );
            if( !cls )  return NULL;

            jmethodID constructor_id = jenv->GetMethodID( cls, "<init>", "(J)V" );
            if( !constructor_id )  return NULL;

            result = jenv->NewObject( cls, constructor_id, (jlong)V_UINT(&v) );
            break;
        }

      //case VT_VOID:
      //case VT_HRESULT:
      //case VT_PTR:
      //case VT_FILETIME:

        case VT_BSTR: 
        {
            result = jstring_from_bstr( jenv, V_BSTR(&v) );
            break;
        }

        case VT_DISPATCH:
        {
            IDispatch* idispatch = V_DISPATCH( &v );
            if( !idispatch )  break;

            ptr<spooler_com::Ihas_java_class_name> j;
            HRESULT hr = idispatch->QueryInterface( spooler_com::IID_Ihas_java_class_name, (void**)&j );
            if( FAILED( hr ) )  break;

            Bstr java_class_name_bstr;
            hr = j->get_java_class_name( &java_class_name_bstr );
            if( FAILED(hr) )  break;

            string java_class_name = replace_regex( string_from_bstr( java_class_name_bstr ), "\\.", "/" ) ;
            ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( java_vm->_spooler, idispatch, java_class_name) );

            java_vm->env().add_object( java_idispatch );        // Lebensdauer nur bis Ende des Aufrufs der Java-Methode, s. Java_module_instance::call()

            result = *java_idispatch;

            break;
        }

      //case VT_VARIANT:
      //case VT_UNKNOWN:
      //case VT_DECIMAL:
      //case VT_SAFEARRAY:
      //case VT_CARRAY:
      //case VT_USERDEFINED:
      //case VT_LPSTR:
      //case VT_LPWSTR:
      //case VT_RECORD:
      //case VT_BLOB:
      //case VT_STREAM:
      //case VT_STORAGE:
      //case VT_STREAMED_OBJECT:
      //case VT_STORED_OBJECT:
      //case VT_BLOB_OBJECT:
      //case VT_CF:
      //case VT_CLSID:

        default:  
            throw_xc( "SPOOLER-178", variant_type_name(v) );
    }

    return result;
}

//--------------------------------------------------------------Java sos.spooler.Idispatch.com_call

extern "C"
JNIEXPORT jobject JNICALL Java_sos_spooler_Idispatch_com_1call( JNIEnv* jenv, jclass, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    try
    {
        HRESULT     hr;
        IDispatch*  idispatch = (IDispatch*)(size_t)jidispatch;
        Bstr        name_bstr;
        DWORD       context = 0;
        DISPID      dispid = 0;
        
        if( !idispatch )  throw_xc( "SPOOLER-176" );

        
        // GetIDsOfNames

        jstring_to_bstr( jenv, jname, &name_bstr );

        OLECHAR* name_ptr = name_bstr;

        if( !name_ptr )  throw_xc( "Idispatch.com_call", "Name fehlt" );

        if( name_ptr[0] == '<' )  context |= DISPATCH_PROPERTYGET, name_ptr++;
        else
        if( name_ptr[0] == '>' )  context |= DISPATCH_PROPERTYPUT, name_ptr++;
                            else  context |= DISPATCH_METHOD;

        hr = idispatch->GetIDsOfNames( IID_NULL, &name_ptr, 1, (LCID)0, &dispid );
        if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_bstr(name_ptr).c_str() );


        // Invoke

        Variant         result;
        Excepinfo       excepinfo;
        UINT            arg_nr;
        int             param_count = jenv->GetArrayLength( jparams );
        Dispparams      dispparams;

        dispparams.set_arg_count( param_count );
        if( context & DISPATCH_PROPERTYPUT )  dispparams.set_property_put();

        for( int i = 0; i < param_count; i++ )
        {
            jobject jparam = jenv->GetObjectArrayElement( jparams, i );
            if( !jparam )  throw_xc( "NULL-Pointer" );
            jclass  cls;

            if( cls = jenv->FindClass( "java/lang/String" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i].attach_bstr( NULL );
                jstring_to_bstr( jenv, (jstring)jparam, &dispparams[i].bstrVal );
            }
            else
            if( cls = jenv->FindClass( "java/lang/Boolean" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallBooleanMethod( jparam, jenv->GetMethodID( cls, "booleanValue", "()Z" ) ) != 0;
            }
            else
            if( cls = jenv->FindClass( "java/lang/Integer" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallIntMethod( jparam, jenv->GetMethodID( cls, "intValue", "()I" ) );
            }
            else
            if( cls = jenv->FindClass( "java/lang/Long" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallLongMethod( jparam, jenv->GetMethodID( cls, "longValue", "()J" ) );
            }
            else
            if( cls = jenv->FindClass( "java/lang/Double" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallDoubleMethod( jparam, jenv->GetMethodID( cls, "doubleValue", "()D" ) );
            }
            else
            if( cls = jenv->FindClass( "sos/spooler/Idispatch" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                jfieldID field_id = jenv->GetFieldID( cls, "_idispatch", "J" );
                if( field_id )
                {
                    dispparams[i] = (IDispatch*)(size_t)jenv->GetLongField( jparam, field_id );
                }
            }
            else
            {
                ptr<Java_object> o = Z_NEW( Java_object( java_vm->_spooler, jparam ) );
                o->set_global();
                dispparams[i] = o;
            }
        }

        if( jenv->ExceptionCheck() )  return NULL;

        hr = idispatch->Invoke( dispid, IID_NULL, (LCID)0, context, &dispparams, &result, &excepinfo, &arg_nr );
        if( FAILED(hr) )  throw_ole_excepinfo( hr, &excepinfo, "Invoke", string_from_bstr(name_bstr).c_str() );

        return jobject_from_variant( jenv, result );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }

    return NULL;
}

//-----------------------------------------------------------------------------Java_vm::get_options
/*
void Java_vm::get_options( const string& options )
{
    const char* p = options.c_str();

    while( *p )
    {
        string option;
        const char* q = strchr( p, '=' );
        if( q )  q++;
           else  q = p + strlen(p);

        option = string( p, q - p );
        if( !*q )  break;

        p = q;
        if( *p == '"' || *p == '\'' )
        {
            char quote = *p++;
            while( *p  &&  *p != quote )
            {
                if( *p == '\\' )  p++;
                option += *p++;
            }

            if( *p == quote )  p++;
        }
        else
        {
            q = strchr( p, ' ' );
            if( !q )  q = p + strlen(p);
            option.append( p, q - p );
            p = q;
        }

        while( *p == ' ' )  p++;

        _options.push_back( option );
    }
}
*/
//------------------------------------------------------------------------------------Java_vm::init
// Im Haupt-Thread zu rufen.

void Java_vm::init()
{
    java_vm = this;

    if( _vm )  return;

    _log.set_prefix( "Java" );

    int ret;


    _work_class_dir = _spooler->temp_dir() + Z_DIR_SEPARATOR "java";
    make_path( _work_class_dir );       // Java-VM prüft Vorhandensein der Verzeichnisse in classpath schon beim Start

        
    if( _filename == "" )  throw_xc( "SPOOLER-170" );
    string module_filename = _filename;


#   ifdef SYSTEM_HPUXxx

        // statisch einbinden, weil dlopen() den Thread Local Storage der libjvm.so nicht laden kann.

#   else     
      //typedef int JNICALL JNI_GetDefaultJavaVMInitArgs_func( JavaVMInitArgs* );
        typedef int JNICALL JNI_CreateJavaVM_func            ( JavaVM**, JNIEnv**, JavaVMInitArgs* );

      //JNI_GetDefaultJavaVMInitArgs_func*   JNI_GetDefaultJavaVMInitArgs;
        JNI_CreateJavaVM_func*               JNI_CreateJavaVM;

#       ifdef SYSTEM_WIN
        {    
            // Der Name des VM-Moduls steht vielleicht in der Registrierung unter
            // HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.4:RuntimeLib
            // HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment:CurrentVersion liefert z.B. "1.4"

            // Microsoft's VM-Modul scheint hier eingetragen zu sein: (ist c:\winnt\system32\msjava.dll).
            // HKEY_LOCAL_MACHINE\SOFTWARE\Clients\JavaVM\MSJavaVM\InstallInfo:VerifyFile

            LOG( "LoadLibrary " << _filename << '\n' );
            HINSTANCE vm_module = LoadLibrary( _filename.c_str() );
            if( !vm_module )  throw_mswin_error( "LoadLibrary", "Java Virtual Machine " + _filename );

            module_filename = filename_of_hinstance( vm_module );
            LOG( "HINSTANCE=" << (void*)vm_module << "  " << module_filename << "  " << file_version_info( module_filename ) << '\n' );

          //JNI_GetDefaultJavaVMInitArgs = (JNI_GetDefaultJavaVMInitArgs_func*)GetProcAddress( vm_module, "JNI_GetDefaultJavaVMInitArgs" );
          //if( !JNI_GetDefaultJavaVMInitArgs )  throw_mswin_error( "GetProcAddress", "JNI_GetDefaultJavaVMInitArgs" );

            JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)GetProcAddress( vm_module, "JNI_CreateJavaVM" );
            if( !JNI_CreateJavaVM )  throw_mswin_error( "GetProcAddress", "JNI_CreateJavaVM" );
        }
#       elif defined SYSTEM_HPUX
        {
            LOG( "shl_load " << _filename << '\n' );
            shl_t vm_module = shl_load( _filename.c_str(), BIND_IMMEDIATE | BIND_VERBOSE, 0 );
            if( !vm_module )  throw_xc( "SPOOLER-171", strerror(errno), _filename.c_str() );

            LOG( "shl_findsym JNI_CreateJavaVM\n" );
            int err = shl_findsym( &vm_module, "JNI_CreateJavaVM", TYPE_PROCEDURE, (void*)&JNI_CreateJavaVM );
            if( err )  throw_xc( "SPOOLER-171", strerror(errno), "JNI_CreateJavaVM" );
        }
#       else
        {
            LOG( "dlopen " << _filename << '\n' );
            void *vm_module = dlopen( _filename.c_str(), RTLD_LAZY );
            if( !vm_module )  throw_xc( "SPOOLER-171", dlerror(), _filename.c_str() );

          //JNI_GetDefaultJavaVMInitArgs = (JNI_GetDefaultJavaVMInitArgs_func*)dlsym( vm_module, "GetDefaultJavaVMInitArgs" );
          //if( !JNI_GetDefaultJavaVMInitArgs )  throw_xc( "SPOOLER-171", dlerror(), "GetDefaultJavaVMInitArgs" );

            JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)dlsym( vm_module, "JNI_CreateJavaVM" );
            if( !JNI_CreateJavaVM )  throw_xc( "SPOOLER-171", dlerror(), "JNI_CreateJavaVM" );
        }
#       endif
#   endif

  //if( _vm_args.classpath       )  complete_class_path = string(_vm_args.classpath) + Z_PATH_SEPARATOR;
    if( _work_class_dir != ""    )  _complete_class_path += _work_class_dir + Z_PATH_SEPARATOR;
    if( _ini_class_path != ""    )  _complete_class_path += _ini_class_path + Z_PATH_SEPARATOR;
    if( _config_class_path != "" )  _complete_class_path += _config_class_path + Z_PATH_SEPARATOR;
    _complete_class_path = replace_regex( _complete_class_path, Z_PATH_SEPARATOR "$", "" );


    _vm_args.version = JNI_VERSION_1_2;

  //get_options( _ini_options );        // Aus factory.ini
  //get_options( _config_options );     // Aus <config>

    _options.push_back( "-Djava.class.path=" + _complete_class_path );
    
    // _options.push_back( "-Xfuture" );   
    // Performs strict class-file format checks.  For purposes of backwards compatibility, the default format checks performed by the Java 2 SDK's virtual machine  are
    // no  stricter  than the checks performed by 1.1.x versions of the JDK software.  The -Xfuture flag turns on stricter class-file format checks that enforce closer
    // conformance to the class-file format specification.  Developers are encouraged to use this flag when developing new code because the stricter checks will become
    // the default in future releases of the Java application launcher.
    
    //_options.push_back( "-Xdebug" );    // Starts with the debugger enabled.

    Z_DEBUG_ONLY( _options.push_back( Option( "-verbose:class,gc,jni" ) ) );

    _options.push_back( Option( "vfprintf", (void*)java_vfprintf ) );

    _vm_args.nOptions = _options.size();
    delete _vm_args.options;
    _vm_args.options = new JavaVMOption[ _vm_args.nOptions ];
    for( int i = 0; i < _vm_args.nOptions; i++ )
    {
        _vm_args.options[i].optionString = (char*)_options[i]._option.c_str();
        _vm_args.options[i].extraInfo    =        _options[i]._extra;
        LOG( "Java option " << _vm_args.options[i].optionString << '\n' );
    }

/*  JDK 1.1
    _vm_args.version = JNI_VERSION_1_1;  //0x00010001; 
    ret = JNI_GetDefaultJavaVMInitArgs( &_vm_args );
    if( ret < 0 )  throw_java( ret, "JNI_GetDefaultJavaVMInitArgs" );

    _vm_args.classpath = (char*)complete_class_path.c_str();

    _vm_args.vfprintf = java_vfprintf;      // a hook for a function that redirects all VM messages.
    _vm_args.enableVerboseGC = true;        // whether GC messages will appear. 

    // Weitere Felder von _vm_args:
    // jint checkSource;            whether to check the Java source files are newer than compiled class files.
    // jint nativeStackSize;        maximum native stack size of Java-created threads.
    // jint javaStackSize; 
    // jint minHeapSize;            initial heap size.
    // jint maxHeapSize;            maximum heap size.
    // jint verifyMode;             controls whether Java byte code should be verified: 0 -- none, 1 -- remotely loaded code, 2 -- all code.
    // const char *classpath;       the local directory path for class loading. 
    // jint (*vfprintf)(FILE *fp, const char *format, va_list args);    a hook for a function that redirects all VM messages.
    // void (*exit)(jint code);     a VM exit hook.
    // void (*abort)();             a VM abort hook. 
    // jint enableClassGC;          whether to enable class GC.
    // jint enableVerboseGC;        whether GC messages will appear. 
    // jint disableAsyncGC;         whether asynchronous GC is allowed.
*/
    _thread_data->_env._java_vm = this;
    _thread_data->_env._jenv = NULL;

    LOG( "JNI_CreateJavaVM()\n" );
    ret = JNI_CreateJavaVM( &_vm, &_thread_data->_env._jenv, &_vm_args );

    LOG( "setlocale(LC_ALL,\"C\")\n" );
    const char* java_locale = setlocale( LC_ALL, "C" );
    if( strcmp( java_locale, "C" ) != 0 )  LOG( "Javas locale war " << java_locale << "\n" );

    if( ret < 0 )  throw_java( ret, "JNI_CreateJavaVM", module_filename );

    JNIEnv* jenv = env();

    int version = jenv->GetVersion();
    _log( "JNI " + as_string( version >> 16 ) + "." + as_string( version & 0xFFFF ) + " " + module_filename + " geladen" );

    _idispatch_jclass = jenv->FindClass( JAVA_IDISPATCH_CLASS );
    if( jenv->ExceptionCheck() )  throw_java( 0, "FindClass " JAVA_IDISPATCH_CLASS, module_filename );

    ret = env()->RegisterNatives( _idispatch_jclass, native_methods, NO_OF( native_methods ) );
    if( ret < 0 )  throw_java( ret, "RegisterNatives", module_filename );
}

//-----------------------------------------------------------------------------------Java_vm::close

void Java_vm::close()
{
    if( _vm )  
    {
        LOG( "DestroyJavaVM()\n" );

        int ret = _vm->DestroyJavaVM();
        if( ret < 0 )  _log.error( "DestroyJavaVM() liefert " + as_string(ret) + ". Java lässt sich nicht entladen" );
                 else  _vm = NULL;
    }

    _options.clear();
    delete _vm_args.options;  _vm_args.options = NULL;
}

//---------------------------------------------------------------------------Java_vm::attach_thread

void Java_vm::attach_thread( const string& thread_name )
{
    if( !_vm )  return;

    string           java_thread_name = "Spooler " + thread_name;
    JavaVMAttachArgs args;

    memset( &args, 0, sizeof args );
    args.version = JNI_VERSION_1_2;
    args.name    = (char*)java_thread_name.c_str();

    _thread_data->_env._java_vm = this;

    _thread_data->_env._jenv = NULL;

    int ret = _vm->AttachCurrentThread( (void**)&_thread_data->_env._jenv, &args ); 
    if( ret < 0 )  throw_java( ret, "AttachCurrentThread" );
}

//---------------------------------------------------------------------------Java_vm::detach_thread

void Java_vm::detach_thread()
{
    if( _vm )
    {
        int ret = _vm->DetachCurrentThread(); 
        if( ret < 0 )  throw_java( ret, "DetachCurrentThread" );
    }

    _thread_data.thread_detach();
}

//-------------------------------------------------------------------------------------Java_vm::env

Java_env& Java_vm::env()
{
    Java_env* env = &_thread_data->_env;

    if( !env ) 
    {
        if( !_vm )  throw_xc( "JAVA NICHT GESTARTET" );
        throw_xc( "JAVA-THREAD NICHT ZUGEORDNET" );
    }

    return *env;
}

//------------------------------------------------------------------------------Java_vm::throw_java

void Java_vm::throw_java( int return_value, const string& text1, const string& text2 )
{
    string ret_text;
    string java_text = "java";

    JNIEnv* env = _thread_data->_env;
    if( env )
    {
        jthrowable x = env->ExceptionOccurred();
        if( x )
        {
            env->ExceptionClear();

            jclass c = env->GetObjectClass(x);
            if( c ) 
            {
                jmethodID get_message_id = env->GetMethodID( c, "getMessage", "()Ljava/lang/String;" );
                if( get_message_id )
                {
                    java_text = string_from_jstring( env, (jstring)env->CallObjectMethod( x, get_message_id ) );
                }
            }
        }
    }

    switch( return_value )
    {
        case JNI_OK:        break;
        case JNI_ERR:       ret_text = "ret=JNI_ERR";                                           break;
        case JNI_EDETACHED: ret_text = "ret=JNI_EDETACHED \"thread detached from the VM\"";     break;
        case JNI_EVERSION:  ret_text = "ret=JNI_EVERSION \"JNI version error\"";                break;

#     ifdef JNI_VERSION_1_4
        case JNI_ENOMEM:    ret_text = "ret=JNI_ENOMEM \"not enough memory\"";                  break;
        case JNI_EEXIST:    ret_text = "ret=JNI_EEXIST \"VM already created\"";                 break;
        case JNI_EINVAL:    ret_text = "ret=JNI_EINVAL \"invalid arguments\"";                  break;
#     endif

        default:            ret_text = "ret=" + as_string(return_value);
    }

    Xc x ( "SPOOLER-175" );
    x.insert( java_text );
    x.insert( text1 );
    x.insert( text2 );
    x.insert( ret_text );
    throw_xc( x );
}

//-----------------------------------------------------------------------------Java_env::add_object

void Java_env::add_object( Java_idispatch* o )
{ 
    _java_idispatch_list.push_back(NULL); 
    *_java_idispatch_list.rbegin() = o; 

    o->set_global(); 
}

//-----------------------------------------------------------------------------Java_env::find_class

jclass Java_env::find_class( const string& name )
{
    jclass result = _jenv->FindClass( name.c_str() );
    if( !result )  _java_vm->throw_java( 0, "FindClass", name );

    return result;
}

//--------------------------------------------------------------------------Java_env::get_method_id

jmethodID Java_env::get_method_id( jclass cls, const string& name, const string& signature )
{
    jmethodID result = _jenv->GetMethodID( cls, name.c_str(), signature.c_str() );
    if( !result )  _java_vm->throw_java( 0, "GetMethodID", name );

    return result;
}

//-----------------------------------------------------------------------Java_env::get_object_class

jclass Java_env::get_object_class( jobject o )
{
    jclass result = _jenv->GetObjectClass( o );
    if( !result )  _java_vm->throw_java( 0, "GetObjectClass" );

    return result;
}

//-------------------------------------------------------------------------------Module::clear_java

void Module::clear_java()
{
    _java_class = NULL;
    _method_map.clear();
}

//--------------------------------------------------------------------------Module::make_java_class
// Quellcode compilieren

bool Module::make_java_class( bool force )
{
    string filename = _spooler->_java_vm._work_class_dir + Z_DIR_SEPARATOR + replace_regex( _java_class_name, "\\.", "/" );
    string java_filename  = filename + ".java";
    string class_filename = filename + ".class";
    string source;
    bool   do_compile = force;


    if( !do_compile )
    {
        struct stat s;
        int err = ::stat( class_filename.c_str(), &s );
        if( err )
        {
            do_compile = true;
        }
        else
        if( (time_t)_source._max_modification_time > s.st_mtime )
        {
            do_compile = true;
        }
        else
        {
/*
            source = _source.text();

            Mapped_file m ( java_filename, "r" );
            if( m.length() != source.length()  ||  memcmp( m.ptr(), source.data(), m.length() ) != 0 )
            {
                _log->warn( "Datei " + java_filename + " ist trotz gleichen Zeitstempels verschieden vom Java-Skript" );
                do_compile = true;
            }
*/
        }
    }

    if( do_compile )
    {
        if( source.empty() )  source = _source.text();

        make_path( directory_of_path( java_filename ) );

        File source_file ( java_filename, "wb" );
        source_file.print( source );
        source_file.close();

        //struct utimbuf utimbuf;
        //utimbuf.actime = utimbuf.modtime = (time_t)_source._max_modification_time;
        //utime( java_filename.c_str(), &utimbuf );

        string cmd = '"' + _spooler->_java_vm._javac + "\" -verbose -O -classpath " + _spooler->_java_vm._complete_class_path + ' ' + java_filename;
        _log->info( cmd );
        
        System_command c;
        c.set_throw( false );
        c.execute( cmd );

        if( c.stderr_text() != "" )  _log->debug( c.stderr_text() ),  _log->debug( "" );

        if( c.xc() )  throw *c.xc();

        //utime( class_filename.c_str(), &utimbuf );
    }

    return do_compile;
}

//--------------------------------------------------------------------------Module::java_method_id

jmethodID Module::java_method_id( const string& name )
{
    JNIEnv*   env = _spooler->_java_vm.env();
    jmethodID method_id;

    Method_map::iterator it = _method_map.find( name );
    if( it == _method_map.end() )  
    {
        int pos = name.find( '(' );
        if( pos == string::npos )  pos = name.length();
        
        method_id = env->GetMethodID( _java_class, name.substr(0,pos).c_str(), name.c_str()+pos );
        if( env->ExceptionCheck() ) env->ExceptionDescribe(), env->ExceptionClear();

        _method_map[name] = method_id;
    }
    else
    {
        method_id = it->second;
    }

    return method_id;
}

//-------------------------------------------------------------------------Java_object::Java_object

Java_object::Java_object( Spooler* spooler, jobject jo )
:
    _spooler( spooler ),
    _jobject( NULL )
{
    assign( jo );
}

//------------------------------------------------------------------------Java_object::~Java_object

Java_object::~Java_object()
{
    assign( NULL );
}

//----------------------------------------------------------------------Java_object::QueryInterface

STDMETHODIMP Java_object::QueryInterface( const IID& iid , void** result )
{
    if( iid == spooler_com::IID_Ihas_java_class_name )
    {
        AddRef();
        *result = (spooler_com::Ihas_java_class_name*)this;
        return S_OK;
    }

    return Object::QueryInterface( iid, result );
}

//------------------------------------------------------------------------------Java_object::assign

void Java_object::assign( jobject jo )
{
    JNIEnv* jenv = _spooler->_java_vm.env();

    if( _is_global )  jenv->DeleteGlobalRef( _jobject );

    _jobject = jo;
}

//--------------------------------------------------------------------------Java_object::set_global

void Java_object::set_global()
{
    if( _jobject )
    {
        _jobject = _spooler->_java_vm.env()->NewGlobalRef( _jobject );
        if( !_jobject )  _spooler->_java_vm.throw_java( 0, "NewGlobalRef" );

        _is_global = true;
    }
}

//-------------------------------------------------------------------Java_idispatch::Java_idispatch

Java_idispatch::Java_idispatch( Spooler* sp, IDispatch* idispatch, const string& subclass_name ) 
: 
    Java_object( sp ),
    _idispatch( idispatch ),
    _class_name( subclass_name )
{
    Java_env& jenv = _spooler->_java_vm.env();

    jclass subclass = jenv.find_class( subclass_name );

    jmethodID constructor_id = jenv.get_method_id( subclass, "<init>", "(J)V" );

    jobject jo = jenv->NewObject( subclass, constructor_id, (jlong)(size_t)idispatch );
    if( !jo )  _spooler->_java_vm.throw_java( 0, "NewObject", _class_name );

    assign( jo );
}

//------------------------------------------------------------------Java_idispatch::~Java_idispatch

Java_idispatch::~Java_idispatch()
{
    if( _jobject )
    {
        try
        {
            Java_env& jenv = _spooler->_java_vm.env();
        
            jclass object_class = jenv.get_object_class( _jobject );

            jmethodID method_id = jenv.get_method_id( object_class, "com_clear", "()V" );

            jenv->CallVoidMethod( _jobject, method_id );
            if( jenv->ExceptionCheck() )  _spooler->_java_vm.throw_java( 0, _class_name, "CallVoidMethod com_clear()" );

            assign( NULL );
            _idispatch = NULL;
        }
        catch( const exception& x )  { _spooler->_log.error( _class_name + "::~Java_idispatch: " + x.what() ); }
    }
}

//----------------------------------------------------------------------Java_module_instance::close

void Java_module_instance::close()
{
    Module_instance::close();

    _jobject = NULL;
}

//-----------------------------------------------------------------------Java_module_instance::init

void Java_module_instance::init()
{
    Module_instance::init();

    _java_vm = &_module->_spooler->_java_vm;
    if( !_java_vm->_vm )  throw_xc( "SPOOLER-177" );

    _env = &_java_vm->env();


    if( !_module->_java_class )
    {
        string class_name = replace_regex( _module->_java_class_name, "\\.", "/" );

        if( !_module->_source.empty() )
        {
            bool compiled = _module->make_java_class( _module->_recompile );     // Java-Klasse ggfs. übersetzen

            if( !compiled )
            {
                try 
                {
                    _module->_java_class = _env->find_class( class_name.c_str() );
                }
                catch( const exception& x )
                {
                    _java_vm->_log.warn( x.what() );
                    _java_vm->_log.warn( "Die Java-Klasse " + class_name + " konnte nicht geladen werden. Die Java-Quelle wird neu übersetzt, mal sehen, ob's dann geht" );
                    _module->make_java_class( true );       // force=true, Mod_time nicht berücksichtigen und auf jeden Fall kompilieren
                }
            }
        }

        _module->_java_class = _env->find_class( class_name.c_str() );
    }


    jmethodID method_id = _module->java_method_id( "<init>()V" );   // Konstruktor
    
    assert( _jobject == NULL );
    _jobject = _env->env()->NewObject( _module->_java_class, method_id );
    if( !_jobject )  _java_vm->throw_java( 0, _module->_java_class_name + " Konstruktor" );
}

//--------------------------------------------------------------------Java_module_instance::add_obj

void Java_module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    string java_class_name = "sos/spooler/" + replace_regex_ext( name, "^(spooler_)?(.*)$", "\\u\\2" );    // "spooler_task" -> "sos.spooler.Task"

    jclass cls = _env->env()->GetObjectClass( _jobject );
    if( !cls )  _module->_spooler->_java_vm.throw_java( 0, "GetMethodID" );

    string signature = string( "L" + java_class_name + ";" );
    jfieldID field_id = _env->env()->GetFieldID( cls, name.c_str(), signature.c_str() );
    if( !field_id )  _module->_spooler->_java_vm.throw_java( 0, "GetFieldID", name );

    ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( _module->_spooler, object, java_class_name ) );
    java_idispatch->set_global();

    _added_jobjects.push_back( java_idispatch );
                                                
    _env->env()->SetObjectField( _jobject, field_id, *java_idispatch );
    if( _env->env()->ExceptionCheck() )  _module->_spooler->_java_vm.throw_java( 0, "SetObjectField", name );

    //Com_module_instance_base::add_obj( object, name );
}

//-----------------------------------------------------------------------Java_module_instance::load

void Java_module_instance::load()
{
    Module_instance::load();
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name )
{
    Java_idispatch_stack_frame stack_frame ( _env );

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    Variant result;

    if( *name.rbegin() == 'Z' )
    {
        result = _env->env()->CallBooleanMethod( _jobject, method_id ) != 0;
    }
    else
    {
        _env->env()->CallVoidMethod( _jobject, method_id );
    }

    if( _env->env()->ExceptionCheck() )  _java_vm->throw_java( 0, name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, int param )
{
    Java_idispatch_stack_frame stack_frame ( _env );

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = _env->env()->CallBooleanMethod( _jobject, method_id, param ) != 0;

    if( _env->env()->ExceptionCheck() )  _java_vm->throw_java( 0, name );

    return result;
}

//----------------------------------------------------------------Java_module_instance::name_exists

bool Java_module_instance::name_exists( const string& name )
{
    return _module->java_method_id( name ) != NULL;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
