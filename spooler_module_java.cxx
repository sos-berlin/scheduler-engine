// $Id: spooler_module_java.cxx,v 1.4 2002/11/07 17:57:24 jz Exp $
/*
    Hier sind implementiert

    Java_vm
    Module::java_method_id
    Java_module_instance
*/

#include "spooler.h"

#ifdef _DEBUG
#   include "Debug/Idispatch.h"
#else
#   include "Release/Idispatch.h"
#endif

using namespace std;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const static JNINativeMethod native_methods[] = 
{
    { "com_call", "(JLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;", Java_sos_spooler_Idispatch_com_1call }
};

//-------------------------------------------------------------------------------------------static

static Java_vm* java_vm;        // Für java_vfprintf()

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

    if( jenv->ExceptionOccurred() )  jenv->ExceptionDescribe();      // schreibt nach stderr

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

//----------------------------------------------------------------------------------jstring_to_bstr

static void jstring_to_bstr( JNIEnv* jenv, const jstring& jstr, BSTR* bstr )
{
    const OLECHAR* str_w = jenv->GetStringChars( jstr, 0 );
    
    *bstr = SysAllocString( str_w );
    
    jenv->ReleaseStringChars( jstr, str_w );
}

//------------------------------------------------------------------------------------java_vfprintf

static jint JNICALL java_vfprintf( FILE *fp, const char *format, va_list args )
{
    char buf[1024];

    int ret = _vsnprintf( buf, sizeof(buf), format, args );

    java_vm->_log( buf );

    return ret;
}

//--------------------------------------------------------------Java sos.spooler.Idispatch.com_call

extern "C"
JNIEXPORT jobject JNICALL Java_sos_spooler_Idispatch_com_1call( JNIEnv* jenv, jclass, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    try
    {
        HRESULT     hr;
        IDispatch*  idispatch = (IDispatch*)jidispatch;
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
        if( name_ptr[1] == '>' )  context |= DISPATCH_PROPERTYPUT, name_ptr++;
                            else  context |= DISPATCH_METHOD;

        hr = idispatch->GetIDsOfNames( IID_NULL, &name_ptr, 1, (LCID)0, &dispid );
        if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_bstr(name_bstr).c_str() );


        // Invoke

        Variant         result;
        Excepinfo       excepinfo;
        UINT            arg_nr;
        int             param_count = jenv->GetArrayLength( jparams );
        Dispparams      dispparams;

        dispparams.set_arg_count( param_count );

        for( int i = 0; i < param_count; i++ )
        {
            jobject jparam = jenv->GetObjectArrayElement( jparams, i );
            if( !jparam )  throw_xc( "NULL-Pointer" );
            jclass  cls;

            if( cls = jenv->FindClass( "java/lang/Integer" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallIntMethod( jparam, jenv->GetMethodID( cls, "intValue", "()I" ) );
            }
            else
            if( cls = jenv->FindClass( "java/lang/Boolean" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallBooleanMethod( jparam, jenv->GetMethodID( cls, "booleanValue", "()Z" ) ) != 0;
            }
            else
            if( cls = jenv->FindClass( "java/lang/String" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i].attach_bstr( NULL );
                jstring_to_bstr( jenv, (jstring)jparam, &dispparams[i].bstrVal );
            }
        }

        if( jenv->ExceptionOccurred() )  return NULL;

        hr = idispatch->Invoke( dispid, IID_NULL, (LCID)0, context, &dispparams, &result, &excepinfo, &arg_nr );
        if( FAILED(hr) )  throw_com( hr, "Invoke" );
        
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
    if( _vm )  return;

    java_vm = this;

    _log.set_prefix( "Java" );

    int ret;

    typedef int JNICALL JNI_GetDefaultJavaVMInitArgs_func( JavaVMInitArgs* );
    typedef int JNICALL JNI_CreateJavaVM_func            ( JavaVM**, JNIEnv**, JavaVMInitArgs* );

    JNI_GetDefaultJavaVMInitArgs_func*   JNI_GetDefaultJavaVMInitArgs;
    JNI_CreateJavaVM_func*               JNI_CreateJavaVM;

    
    if( _filename == "" )  throw_xc( "SPOOLER-170" );

#   ifdef SYSTEM_WIN
    {    
        // Der Name des VM-Moduls steht vielleicht in der Registrierung unter
        // HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.4:RuntimeLib
        // HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment:CurrentVersion liefert z.B. "1.4"

        // Microsoft's VM-Modul scheint hier eingetragen zu sein: (ist c:\winnt\system32\msjava.dll).
        // HKEY_LOCAL_MACHINE\SOFTWARE\Clients\JavaVM\MSJavaVM\InstallInfo:VerifyFile

        LOG( "LoadLibrary " << _filename << '\n' );
        HINSTANCE vm_module = LoadLibrary( _filename.c_str() );
        if( !vm_module )  throw_mswin_error( "LoadLibrary", "Java Virtual Machine " + _filename );

        JNI_GetDefaultJavaVMInitArgs = (JNI_GetDefaultJavaVMInitArgs_func*)GetProcAddress( vm_module, "JNI_GetDefaultJavaVMInitArgs" );
        if( !JNI_GetDefaultJavaVMInitArgs )  throw_mswin_error( "GetProcAddress", "JNI_GetDefaultJavaVMInitArgs" );

        JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)GetProcAddress( vm_module, "JNI_CreateJavaVM" );
        if( !JNI_CreateJavaVM )  throw_mswin_error( "GetProcAddress", "JNI_CreateJavaVM" );
    }
#   else
    {
        LOG( "dlopen " << _filename << '\n' );
        void *vm_module = dlopen( _filename.c_str(), RTLD_LAZY );
        if( !vm_module )  throw_xc( "SPOOLER-171", dlerror(), _filename.c_str() );

        JNI_GetDefaultJavaVMInitArgs = (JNI_GetDefaultJavaVMInitArgs_func*)dlsym( vm_module, "JNI_CreateJavaVM" );
        if( !JNI_GetDefaultJavaVMInitArgs )  throw_xc( "SPOOLER-171", dlerror(), "JNI_CreateJavaVM" );

        JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)dlsym( vm_module, "JNI_CreateJavaVM" );
        if( !JNI_CreateJavaVM )  throw_xc( "SPOOLER-171", dlerror(), "JNI_CreateJavaVM" );
    }
#   endif

    string complete_class_path = "";
  //if( _vm_args.classpath       )  complete_class_path = string(_vm_args.classpath) + Z_PATH_SEPARATOR;
    if( _ini_class_path != ""    )  complete_class_path += _ini_class_path + Z_PATH_SEPARATOR;
    if( _config_class_path != "" )  complete_class_path += _config_class_path + Z_PATH_SEPARATOR;
    complete_class_path = replace_regex( complete_class_path, Z_PATH_SEPARATOR "$", "" );


    _vm_args.version = JNI_VERSION_1_2;

  //get_options( _ini_options );        // Aus factory.ini
  //get_options( _config_options );     // Aus <config>

    _options.push_back( "-Djava.class.path=" + complete_class_path );
    Z_DEBUG_ONLY( _options.push_back( Option( "-verbose:class,gc,jni" ) ) );
    _options.push_back( Option( "vfprintf", java_vfprintf ) );

    _vm_args.nOptions = _options.size();
    delete _vm_args.options;
    _vm_args.options = new JavaVMOption[ _vm_args.nOptions ];
    for( int i = 0; i < _options.size(); i++ )
    {
        _vm_args.options[i].optionString = (char*)_options[i]._option.c_str();
        _vm_args.options[i].extraInfo    =        _options[i]._extra;
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

    ret = JNI_CreateJavaVM( &_vm, &_thread_data->_env._jenv, &_vm_args );
    if( ret < 0 )  throw_java( ret, "JNI_CreateJavaVM" );

    JNIEnv* jenv = env();

    int version = jenv->GetVersion();
    LOG( "Java JNI " << ( version >> 16 ) << "." << ( version & 0xFFFF ) << " geladen\n" );

    _idispatch_jclass = jenv->FindClass( JAVA_IDISPATCH_CLASS );
    if( jenv->ExceptionOccurred() )  throw_java( 0, "FindClass", JAVA_IDISPATCH_CLASS );

    ret = env()->RegisterNatives( _idispatch_jclass, native_methods, NO_OF( native_methods ) );
    if( ret < 0 )  throw_java( ret, "RegisterNatives" );
}

//-----------------------------------------------------------------------------------Java_vm::close

void Java_vm::close()
{
    java_vm = NULL;

    delete _vm_args.options;  _vm_args.options = NULL;

    if( !_vm )  return;

    int ret = _vm->DestroyJavaVM();
    if( ret < 0 )  _log.error( "DestroyJavaVM() liefert " + as_string(ret) + ". Java lässt sich nicht entladen" );
    _vm = NULL;
}

//---------------------------------------------------------------------------Java_vm::attach_thread

void Java_vm::attach_thread( const string& thread_name )
{
    string           java_thread_name = "Spooler " + thread_name;
    JavaVMAttachArgs args;

    memset( &args, 0, sizeof args );
    args.version = JNI_VERSION_1_2;
    args.name    = (char*)java_thread_name.c_str();

    _thread_data->_env._java_vm = this;

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

    JNIEnv* env = _thread_data->_env;
    if( env )
    {
        if( env->ExceptionOccurred() )
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }

    switch( return_value )
    {
        case JNI_OK:        break;
        case JNI_ERR:       ret_text = "ret=JNI_ERR";                                           break;
        case JNI_EDETACHED: ret_text = "ret=JNI_EDETACHED \"thread detached from the VM\"";     break;
        case JNI_EVERSION:  ret_text = "ret=JNI_EVERSION \"JNI version error\"";                break;
        case JNI_ENOMEM:    ret_text = "ret=JNI_ENOMEM \"not enough memory\"";                  break;
        case JNI_EEXIST:    ret_text = "ret=JNI_EEXIST \"VM already created\"";                 break;
        case JNI_EINVAL:    ret_text = "ret=JNI_EINVAL \"invalid arguments\"";                  break;
        default:            ret_text = "ret=" + as_string(return_value);
    }

    Xc x ( "SPOOLER-175" );
    x.insert( text1 );
    x.insert( text2 );
    x.insert( ret_text );
    throw_xc( x );
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
        if( env->ExceptionOccurred() ) env->ExceptionDescribe(), env->ExceptionClear();

        _method_map[name] = method_id;
    }
    else
    {
        method_id = it->second;
    }

    return method_id;
}

//-----------------------------------------------------------Java_global_object::Java_global_object

Java_global_object::Java_global_object( Spooler* spooler, jobject jo )
:
    _spooler( spooler ),
    _jobject( NULL )
{
    assign( jo );
}

//----------------------------------------------------------Java_global_object::~Java_global_object

Java_global_object::~Java_global_object()
{
    assign( NULL );
}

//-----------------------------------------------------------------------Java_global_object::assign

void Java_global_object::assign( jobject jo )
{
    JNIEnv* jenv = _spooler->_java_vm.env();

    if( _jobject )  jenv->DeleteGlobalRef( jo ), _jobject = NULL;

    if( jo )
    {
        _jobject = jenv->NewGlobalRef( jo );
        if( !_jobject )  _spooler->_java_vm.throw_java( 0, "NewGlobalRef" );
    }
}

//-------------------------------------------------------------------Java_idispatch::Java_idispatch

Java_idispatch::Java_idispatch( Spooler* sp, IDispatch* idispatch, const string& subclass_name ) 
: 
    Java_global_object( sp ),
    _idispatch( idispatch )
{
    Java_env jenv = _spooler->_java_vm.env();

    jclass subclass = jenv.find_class( subclass_name );

    jmethodID constructor_id = jenv.get_method_id( subclass, "<init>", "(J)V" );

    jobject jo = jenv->NewObject( subclass, constructor_id, (long)(IDispatch*)idispatch );
    if( !jo )  _spooler->_java_vm.throw_java( 0, "NewObject", JAVA_IDISPATCH_CLASS );

    assign( jo );
}

//------------------------------------------------------------------Java_idispatch::~Java_idispatch

Java_idispatch::~Java_idispatch()
{
    if( _jobject )
    {
        Java_env jenv = _spooler->_java_vm.env();
        
        jclass object_class = jenv.get_object_class( _jobject );

        jmethodID method_id = jenv.get_method_id( object_class, "com_clear", "()V" );


        jenv->CallVoidMethod( _jobject, method_id );
        if( jenv->ExceptionOccurred() )  _spooler->_java_vm.throw_java( 0, "CallVoidMethod", "com_clear()" );

        assign( NULL );
        _idispatch = NULL;
    }
}

//-----------------------------------------------------------------------Java_module_instance::init

void Java_module_instance::init()
{
    Module_instance::init();

    _java_vm = &_module->_spooler->_java_vm;
    _env     = _java_vm->env();

    if( !_module->_java_class )
    {
        string class_name = replace_regex( _module->_java_class_name, "\\.", "/" );
        _module->_java_class = _env->FindClass( class_name.c_str() );
    
        if( _env->ExceptionOccurred() )  _java_vm->throw_java( 0, "FindClass()", _module->_java_class_name.c_str() );
        if( !_module->_java_class )  throw_xc( "Java::FindClass", _module->_java_class_name.c_str() );
    }


    jmethodID method_id = _module->java_method_id( "<init>()V" );
    
    assert( _jobject == NULL );
    _jobject = _env->NewObject( _module->_java_class, method_id );
    if( !_jobject )  _java_vm->throw_java( 0, _module->_java_class_name + " Konstruktor" );
}

//-----------------------------------------------------------------------Java_module_instance::load

void Java_module_instance::load()
{
    Module_instance::load();

    _loaded = true;
}

//--------------------------------------------------------------------Java_module_instance::add_obj

void Java_module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    string java_class_name = "sos/spooler/" + replace_regex_ext( name, "^(spooler_)?(.*)$", "\\u\\2" );    // "spooler_task" -> "sos.spooler.Task"

    jclass cls = _env->GetObjectClass( _jobject );
    if( !cls )  _module->_spooler->_java_vm.throw_java( 0, "GetMethodID" );

    string signature = string( "L" + java_class_name + ";" );
    jfieldID field_id = _env->GetFieldID( cls, name.c_str(), signature.c_str() );
    if( !field_id )  _module->_spooler->_java_vm.throw_java( 0, "GetFieldID", name );

    _added_jobjects.push_back( Z_NEW( Java_idispatch( _module->_spooler, object, java_class_name ) ) );
                                                
    _env->SetObjectField( _jobject, field_id, **_added_jobjects.rbegin() );
    if( _env->ExceptionOccurred() )  _module->_spooler->_java_vm.throw_java( 0, "SetObjectField", name );

    //Com_module_instance_base::add_obj( object, name );
}

//----------------------------------------------------------------------Java_module_instance::close

void Java_module_instance::close()
{
    _log->warn( "free jobject" );  // jobject freigeben!
    _jobject = NULL;

    Module_instance::close();
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name )
{
    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    Variant result;

    if( *name.rbegin() == 'Z' )
    {
        result = _env->CallBooleanMethod( _jobject, method_id ) != 0;
    }
    else
    {
        _env->CallVoidMethod( _jobject, method_id );
    }

    if( _env->ExceptionOccurred() )  _java_vm->throw_java( 0, name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, int param )
{
    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = _env->CallBooleanMethod( _jobject, method_id, param ) != 0;

    if( _env->ExceptionOccurred() )  _java_vm->throw_java( 0, name );

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
