// $Id: spooler_module_java.cxx,v 1.2 2002/11/02 12:23:26 jz Exp $
/*
    Hier sind implementiert

    Java_vm
    Module::java_method_id
    Java_module_instance
*/


#include "spooler.h"

using namespace std;

namespace sos {
namespace spooler {

static Java_vm* java_vm;

//------------------------------------------------------------------------------------java_vfprintf

static jint JNICALL java_vfprintf( FILE *fp, const char *format, va_list args )
{
    char buf[1024];

    int ret = _vsnprintf( buf, sizeof(buf), format, args );

    java_vm->_log( buf );

    return ret;
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
    ret = JNI_CreateJavaVM( &_vm, &_thread_data->_env, &_vm_args );
    if( ret < 0 )  throw_java( ret, "JNI_CreateJavaVM" );
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

    int ret = _vm->AttachCurrentThread( (void**)&_thread_data->_env, &args ); 
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

JNIEnv* Java_vm::env()
{
    JNIEnv* env = _thread_data->_env;

    if( !env ) 
    {
        if( !_vm )  throw_xc( "JAVA NICHT GESTARTET" );
        throw_xc( "JAVA-THREAD NICHT ZUGEORDNET" );
    }

    return env;
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
}

//-----------------------------------------------------------------------Java_module_instance::load

void Java_module_instance::load()
{
    Module_instance::load();

    jmethodID method_id = _module->java_method_id( "<init>()V" );
    
    assert( _object == NULL );
    _object = _env->NewObject( _module->_java_class, method_id );
    if( !_object )  _java_vm->throw_java( 0, _module->_java_class_name + " Konstruktor" );

    _loaded = true;
}

//----------------------------------------------------------------------Java_module_instance::close

void Java_module_instance::close()
{
    _log->warn( "free jobject" );  // jobject freigeben!
    _object = NULL;

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
        result = _env->CallBooleanMethod( _object, method_id ) != 0;
    }
    else
    {
        _env->CallVoidMethod( _object, method_id );
    }

    if( _env->ExceptionOccurred() )  _java_vm->throw_java( 0, name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, int param )
{
    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = _env->CallBooleanMethod( _object, method_id, param ) != 0;

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
