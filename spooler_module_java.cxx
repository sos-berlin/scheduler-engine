// $Id: spooler_module_java.cxx,v 1.50 2003/04/09 10:46:38 jz Exp $
/*
    Hier sind implementiert

    Java_vm
    Module::java_method_id
    Java_module_instance
*/

#include "spooler.h"
#include "../file/stdfile.h"    // make_path

#include "../zschimmer/java.h"

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
#endif

#ifndef JNI_VERSION_1_2
#   error "Der Spooler braucht jni.h Version 1.2"
#endif

using namespace std;
using namespace zschimmer::java;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const static JNINativeMethod native_methods[] = 
{
    { "com_call", "(JLjava/lang/String;[Ljava/lang/Object;)Ljava/lang/Object;", (void*)Java_sos_spooler_Idispatch_com_1call }
};

//-------------------------------------------------------------------------------------------static

zschimmer::Thread_data<Java_thread_data> thread_data;

//-------------------------------------------------------------------------------set_java_exception

static void set_java_exception( JNIEnv* jenv, const char* what )
{
    const char* exception_class_name = "java/lang/RuntimeException";
    jclass exception_class = jenv->FindClass( exception_class_name  );

    if( exception_class )  
    {
        int err = jenv->ThrowNew( exception_class, what ); 
        jenv->DeleteLocalRef( exception_class );

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

//-----------------------------------------------------------------------------jobject_from_variant

static jobject jobject_from_variant( JNIEnv* jenv, const VARIANT& v )
{
    if( v.vt == VT_DISPATCH )
    {
        IDispatch* idispatch = V_DISPATCH( &v );
        if( !idispatch )  return NULL;

        ptr<spooler_com::Ihas_java_class_name> j;
        HRESULT hr = idispatch->QueryInterface( spooler_com::IID_Ihas_java_class_name, (void**)&j );
        if( FAILED( hr ) )  throw_ole( hr, "IID_Ihas_java_class_name" );

        Bstr java_class_name_bstr;
        hr = j->get_java_class_name( &java_class_name_bstr );
        if( FAILED(hr) )  throw_ole( hr, "get_java_class_name" );

        string java_class_name = replace_regex( string_from_bstr( java_class_name_bstr ), "\\.", "/" ) ;
        ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( spooler_ptr->_java_vm, idispatch, java_class_name ) );

        thread_data->add_object( java_idispatch );        // Lebensdauer nur bis Ende des Aufrufs der Java-Methode, s. Java_module_instance::call()

        return *java_idispatch;
    }
    else
    {
        return z::java::jobject_from_variant( jenv, v );
    }
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
        if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_ole(name_ptr).c_str() );


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
            if( jenv->DeleteLocalRef( cls ), cls = jenv->FindClass( "java/lang/Boolean" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallBooleanMethod( jparam, jenv->GetMethodID( cls, "booleanValue", "()Z" ) ) != 0;
            }
            else
            if( jenv->DeleteLocalRef( cls ), cls = jenv->FindClass( "java/lang/Integer" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallIntMethod( jparam, jenv->GetMethodID( cls, "intValue", "()I" ) );
            }
            else
            if( jenv->DeleteLocalRef( cls ), cls = jenv->FindClass( "java/lang/Long" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallLongMethod( jparam, jenv->GetMethodID( cls, "longValue", "()J" ) );
            }
            else
            if( jenv->DeleteLocalRef( cls ), cls = jenv->FindClass( "java/lang/Double" ), !cls )  return NULL;
            else
            if( jenv->IsInstanceOf( jparam, cls ) )
            {
                dispparams[i] = jenv->CallDoubleMethod( jparam, jenv->GetMethodID( cls, "doubleValue", "()D" ) );
            }
            else
            if( jenv->DeleteLocalRef( cls ), cls = jenv->FindClass( "sos/spooler/Idispatch" ), !cls )  return NULL;
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
                //ptr<Java_global_object> o = Z_NEW( Java_global_object( spooler_ptr->_java_vm, jparam ) );
                //dispparams[i] = o;
                throw_xc( "SPOOLER-178", i );
            }


            jenv->DeleteLocalRef( cls );
            jenv->DeleteLocalRef( jparam );
        }

        if( jenv->ExceptionCheck() )  return NULL;

        hr = idispatch->Invoke( dispid, IID_NULL, (LCID)0, context, &dispparams, &result, &excepinfo, &arg_nr );
        if( FAILED(hr) )  throw_ole_excepinfo( hr, &excepinfo, "Invoke", string_from_bstr(name_bstr).c_str() );

        return spooler::jobject_from_variant( jenv, result );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }

    return NULL;
}

//----------------------------------------------------------------------------Spooler::init_java_vm

void Spooler::init_java_vm()
{
    _java_vm->set_log( &_prefix_log );

    _java_vm->start();

    make_path( _java_work_dir );       // Java-VM prüft Vorhandensein der Verzeichnisse in classpath schon beim Start

    Env e = _java_vm->env();

    //_idispatch_jclass = e->FindClass( JAVA_IDISPATCH_CLASS );
    Class idispatch_class ( _java_vm, JAVA_IDISPATCH_CLASS );
    if( e->ExceptionCheck() )  e.throw_java( "FindClass " JAVA_IDISPATCH_CLASS );

    int ret = e->RegisterNatives( idispatch_class, native_methods, NO_OF( native_methods ) );
    if( ret < 0 )  throw_java_ret( ret, "RegisterNatives" );
}

//---------------------------------------------------------------------Java_thread_data::add_object

void Java_thread_data::add_object( Java_idispatch* o )
{ 
    _java_idispatch_list.push_back(NULL); 
    *_java_idispatch_list.rbegin() = o; 

    //o->set_global(); 
}

//-------------------------------------------------------------------Java_idispatch::Java_idispatch

Java_idispatch::Java_idispatch( Vm* vm, IDispatch* idispatch, const string& subclass_name ) 
: 
    Global_jobject( vm ),
    _idispatch( idispatch ),
    _class_name( subclass_name )
{
    Env e = env();

    jclass subclass = e.find_class( subclass_name );

    jmethodID constructor_id = e.get_method_id( subclass, "<init>", "(J)V" );

    jobject jo = e->NewObject( subclass, constructor_id, (jlong)(size_t)idispatch );
    if( !jo )  e.throw_java( "NewObject", _class_name );

    assign( jo );
    e->DeleteLocalRef( subclass );
}

//------------------------------------------------------------------Java_idispatch::~Java_idispatch

Java_idispatch::~Java_idispatch()
{
    if( get_jobject() )
    {
        try
        {
            Env e = env();
        
            jclass object_class = e.get_object_class( get_jobject() );

            jmethodID method_id = e.get_method_id( object_class, "com_clear", "()V" );

            e->DeleteLocalRef( object_class ), object_class = NULL;

            e->CallVoidMethod( get_jobject(), method_id );
            if( e->ExceptionCheck() )  e.throw_java( _class_name, "CallVoidMethod com_clear()" );

            assign( NULL );
            _idispatch = NULL;

        }
        catch( const exception& x )  { vm()->_log.error( _class_name + "::~Java_idispatch: " + x.what() ); }
    }
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
    string filename = _spooler->_java_work_dir + Z_DIR_SEPARATOR + replace_regex( _java_class_name, "\\.", "/" );
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

        string cmd = '"' + _spooler->_java_vm->javac_filename() + "\" -verbose -O -classpath " + _spooler->_java_vm->class_path() + ' ' + java_filename;
        _log->info( cmd );
        
        System_command c;
        c.set_throw( false );
        c.execute( cmd );

        if( c.stderr_text() != "" )  _log->info( c.stderr_text() ),  _log->info( "" );
        if( c.stdout_text() != "" )  _log->info( c.stdout_text() ),  _log->info( "" );

        if( c.xc() )  throw *c.xc();

        //utime( class_filename.c_str(), &utimbuf );
    }

    return do_compile;
}

//--------------------------------------------------------------------------Module::java_method_id

jmethodID Module::java_method_id( const string& name )
{
    JNIEnv*   env = _spooler->_java_vm->jenv();
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

//----------------------------------------------------------------------Java_object::QueryInterface
/*
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
*/
//-------------------------------------------------------------------Java_idispatch::Java_idispatch
/*
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
    if( !jo )  _spooler->_java_vm.throw_java( "NewObject", _class_name );

    assign( jo );
    jenv->DeleteLocalRef( subclass );
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

            jenv->DeleteLocalRef( object_class ), object_class = NULL;

            jenv->CallVoidMethod( _jobject, method_id );
            if( jenv->ExceptionCheck() )  _spooler->_java_vm.throw_java( _class_name, "CallVoidMethod com_clear()" );

            assign( NULL );
            _idispatch = NULL;

        }
        catch( const exception& x )  { _spooler->_log.error( _class_name + "::~Java_idispatch: " + x.what() ); }
    }
}
*/
//-------------------------------------------------------Java_module_instance::Java_module_instance

Java_module_instance::Java_module_instance( Vm* vm, Module* module )
: 
    Module_instance(module),
    Has_vm(vm),
    _zero_(this+1), 
    _jobject(_module->_spooler->_java_vm) 
{
}

//----------------------------------------------------------------------Java_module_instance::close

void Java_module_instance::close()
{
    Module_instance::close();

    _added_jobjects.clear();
    _jobject = NULL;
}

//-----------------------------------------------------------------------Java_module_instance::init

void Java_module_instance::init()
{
    Env e = env();

    Module_instance::init();

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
                    _module->_java_class = e.find_class( class_name.c_str() );
                }
                catch( const exception& x )
                {
                    _java_vm->_log.warn( x.what() );
                    _java_vm->_log.warn( "Die Java-Klasse " + class_name + " konnte nicht geladen werden. Die Java-Quelle wird neu übersetzt, mal sehen, ob's dann geht" );
                    _module->make_java_class( true );       // force=true, Mod_time nicht berücksichtigen und auf jeden Fall kompilieren
                }
            }
        }

        _module->_java_class = e.find_class( class_name.c_str() );
    }


    jmethodID method_id = _module->java_method_id( "<init>()V" );   // Konstruktor
    
    assert( _jobject == NULL );
    _jobject = e->NewObject( _module->_java_class, method_id );
    if( !_jobject )  e.throw_java( _module->_java_class_name + " Konstruktor" );
}

//--------------------------------------------------------------------Java_module_instance::add_obj

void Java_module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    Env e = env();

    string java_class_name = "sos/spooler/" + replace_regex_ext( name, "^(spooler_)?(.*)$", "\\u\\2" );    // "spooler_task" -> "sos.spooler.Task"

    //LOGI( "Java_module_instance::add_obj " << java_class_name << "\n" );

    jclass cls = e->GetObjectClass( _jobject );
    if( !cls )  e.throw_java( "GetMethodID" );

    string signature = "L" + java_class_name + ";";

    jfieldID field_id = e->GetFieldID( cls, name.c_str(), signature.c_str() );
    e->DeleteLocalRef( cls );

    if( !field_id )  e.throw_java( "GetFieldID", name );

    ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( vm(), object, java_class_name ) );
    //java_idispatch->set_global();

    _added_jobjects.push_back( java_idispatch );
                         
    e->SetObjectField( _jobject, field_id, *java_idispatch );
    if( e->ExceptionCheck() )  e.throw_java( "SetObjectField", name );

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
    Env e = env();
    Java_idispatch_stack_frame stack_frame;

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    Variant result;

    if( *name.rbegin() == 'Z' )
    {
        result = e->CallBooleanMethod( _jobject, method_id ) != 0;
    }
    else
    {
        e->CallVoidMethod( _jobject, method_id );
    }

    if( e->ExceptionCheck() )  e.throw_java( name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, int param )
{
    Env e = env();
    Java_idispatch_stack_frame stack_frame;

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SPOOLER-174", name, _module->_java_class_name.c_str() );

    bool result = e->CallBooleanMethod( _jobject, method_id, param ) != 0;

    if( e->ExceptionCheck() )  e.throw_java( name );

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
