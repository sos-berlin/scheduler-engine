// $Id: spooler_module_java.cxx,v 1.1 2002/11/01 09:27:11 jz Exp $
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

//------------------------------------------------------------------------------------Spooler::init

void Java_vm::init()
{
    if( _vm )  return;

    int            ret;
    JDK1_1InitArgs vm_args;
    string         class_path;

    typedef int JNI_GetDefaultJavaVMInitArgs_func( JDK1_1InitArgs* );
    typedef int JNI_CreateJavaVM_func            ( JavaVM**, JNIEnv**, JDK1_1InitArgs* );

    JNI_GetDefaultJavaVMInitArgs_func*   JNI_GetDefaultJavaVMInitArgs;
    JNI_CreateJavaVM_func*               JNI_CreateJavaVM;

    
    if( _filename == "" )  throw_xc( "SPOOLER-170" );

#   ifdef SYSTEM_WIN
    {    
        // Der Name des VM-Moduls steht vielleicht in der Registrierung unter
        // HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.3 RuntimeLib    (Borland) oder
        // HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.3.0 RuntimeLib   (Sun)

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


    vm_args.version = 0x00010001; 
    ret = JNI_GetDefaultJavaVMInitArgs( &vm_args );
    if( ret < 0 )  throw_xc( "SPOOLER-169", "JNI_GetDefaultJavaVMInitArgs", "Version nicht aktuell?" );

    class_path = string(vm_args.classpath) + Z_PATH_SEPARATOR + _class_path;
    vm_args.classpath = (char*)class_path.c_str();

    // Weitere Felder von vm_args:
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

    ret = JNI_CreateJavaVM( &_vm, &_thread_data->_env, &vm_args );
    if( ret < 0 )  throw_xc( "SPOOLER-169", "JNI_CreateJavaVM" );
}

//--------------------------------------------------------------------------------------------close

void Java_vm::close()
{
    if( !_vm )  return;

    _vm->DestroyJavaVM();
    _vm = NULL;
}

//---------------------------------------------------------------------------Java_vm::attach_thread

JNIEnv* Java_vm::attach_thread()
{
    JNIEnv* env = NULL;
    
    int ret = _vm->AttachCurrentThread( (void**)&_thread_data->_env, NULL ); 
    if( !ret )  throw_xc( "Java.AttachCurrentThread" );

    return env;
}

//---------------------------------------------------------------------------Java_vm::detach_thread

void Java_vm::detach_thread()
{
    if( _vm )
    {
        int ret = _vm->DetachCurrentThread(); 
        if( ret < 0 )  throw_xc( "Java.DetachCurrentThread" );

    }

    _thread_data.thread_detach();
}

//-------------------------------------------------------------------------------------Java_vm::env

JNIEnv* Java_vm::env()
{
    JNIEnv* env = _thread_data->_env;

    if( !env )  throw_xc( "Java_vm::env" );

    return env;
}

//------------------------------------------------------------------------------Java_vm::throw_java

void Java_vm::throw_java( const string& text )
{
    env()->ExceptionDescribe();
    throw_xc( "SPOOLER-175", text );
}

//--------------------------------------------------------------------------Module::java_method_id

jmethodID Module::java_method_id( const string& name )
{
    jmethodID method_id;

    Method_map::iterator it = _method_map.find( name );
    if( it == _method_map.end() )  
    {
        int pos = name.find( '(' );
        if( pos == string::npos )  pos = name.length();
        method_id = _spooler->_java_vm.env()->GetMethodID( _java_class, name.substr(0,pos).c_str(), name.c_str()+pos );
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

    _env = _module->_spooler->_java_vm.env();

    if( !_module->_java_class )
    {
        _module->_java_class = _env->FindClass( _module->_java_class_name.c_str() );
    
        if( !_module->_java_class   ||  _env->ExceptionOccurred() )  throw_xc( "Java::FindClass", _module->_java_class_name.c_str() );
    }
}

//-----------------------------------------------------------------------Java_module_instance::load

void Java_module_instance::load()
{
    Module_instance::load();

    jmethodID method_id = _module->java_method_id( "<init>()v" );
    _object = _env->NewObject( _module->_java_class, method_id );
    if( !_object )  _module->_spooler->_java_vm.throw_java( _module->_java_class_name + " Konstruktor" );
}

//----------------------------------------------------------------------Java_module_instance::close

void Java_module_instance::close()
{
    Module_instance::close();
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name )
{
    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = _env->CallBooleanMethod( _object, method_id ) != 0;

    if( _env->ExceptionOccurred() )  _module->_spooler->_java_vm.throw_java( name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, int param )
{
    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = _env->CallBooleanMethod( _object, method_id, param ) != 0;

    if( _env->ExceptionOccurred() )  _module->_spooler->_java_vm.throw_java( name );

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
