// $Id$
/*
    Hier sind implementiert

    Java_vm
    Module::java_method_id
    Java_module_instance
*/

#include "spooler.h"
#include "../file/stdfile.h"    // make_path

#include "../kram/sos_java.h"
#include "../zschimmer/java.h"
#include "../zschimmer/system_command.h"

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
#   error "Der Scheduler braucht jni.h Version 1.2"
#endif

#ifdef Z_HPUX
#   include "../hostjava/hostjava.h"
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

static zschimmer::Thread_data<Java_thread_data> thread_data;

//-----------------------------------------------------------------------------jobject_from_variant

static jobject jobject_from_variant( JNIEnv* jenv, const VARIANT& v, Java_idispatch_container* java_idispatch_container )
{
    if( v.vt == VT_EMPTY )
    {
        return jenv->NewString( NULL, 0 );       // Für Job_chain_node.next_state, .error_state ("" wird zu VT_EMPTY)
    }
    else
    if( v.vt == VT_ERROR  && v.scode == DISP_E_PARAMNOTFOUND ) 
    {
        return jenv->NewString( NULL, 0 );       // Für Job_chain_node.next_state, .error_state ("" wird zu VT_EMPTY)
    }
    else
    {
        return z::java::jobject_from_variant( jenv, v, java_idispatch_container );
    }
}

//--------------------------------------------------------------Java sos.spooler.Idispatch.com_call

extern "C"
//JNIEXPORT
jobject JNICALL Java_sos_spooler_Idispatch_com_1call( JNIEnv* jenv, jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    try
    {
        return sos::spooler::jobject_from_variant( jenv, variant_java_com_call( jenv, cls, jidispatch, jname, jparams ), &thread_data->_idispatch_container );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }

    return NULL;
}

//------------------------------------------Java_idispatch_stack_frame::~Java_idispatch_stack_frame

Java_idispatch_stack_frame::~Java_idispatch_stack_frame()
{ 
    thread_data->_idispatch_container.release_objects(); 
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
    if( _java_vm->work_dir().empty() )  throw_xc( "SCHEDULER-201" );    // Vorsichtshalber, sollte nicht passieren.

    string filename = _java_vm->work_dir() + Z_DIR_SEPARATOR + replace_regex( _java_class_name, "\\.", "/" );
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
                _log.warn( "Datei " + java_filename + " ist trotz gleichen Zeitstempels verschieden vom Java-Skript" );
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

        string cmd = '"' + _java_vm->javac_filename() + "\" -g -classpath " + _java_vm->class_path() + ' ' + java_filename;     // + " -verbose"
        _log.info( cmd );
        
        System_command c;
        c.set_throw( false );
        c.execute( cmd );

        if( c.stderr_text() != "" )  _log.log( c.exit_code() != 0? log_error : log_info, c.stderr_text() ),  _log.info( "" );
        if( c.stdout_text() != "" )  _log.log( c.exit_code() != 0? log_error : log_info, c.stdout_text() ),  _log.info( "" );

        if( c.xc() )  throw *c.xc();

        //utime( class_filename.c_str(), &utimbuf );
    }

    return do_compile;
}

//--------------------------------------------------------------------------Module::java_method_id

jmethodID Module::java_method_id( const string& name )
{
    JNIEnv*   env = _java_vm->jenv();
    jmethodID method_id;

    Method_map::iterator it = _method_map.find( name );
    if( it == _method_map.end() )  
    {
        int pos = name.find( '(' );
        if( pos == string::npos )  pos = name.length();
        
        if( !_java_class )  throw_xc( "SCHEDULER-197", name );
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
//---------------------------------------------------------------Java_module_instance::init_java_vm

void Java_module_instance::init_java_vm( java::Vm* java_vm )
{
    string work_dir = java_vm->work_dir();
    if( !work_dir.empty() )
    {
        try
        {
            //fprintf( stderr, "make_path %s\n", work_dir.c_str() );
            make_path( work_dir );  // Verzeichnis muss beim Start von Java vorhanden sein, damit Java es in classpath berücksichtigt.
        }
        catch( const exception& x ) { java_vm->_log.warn( "mkdir " + work_dir + " => " + x.what() ); }
    }

    java_vm->start();

  //make_path( _java_vm->work_dir() );       // Java-VM prüft Vorhandensein der Verzeichnisse in classpath schon beim Start

    Env e = java_vm->env();

    //_idispatch_jclass = e->FindClass( JAVA_IDISPATCH_CLASS );
    Class idispatch_class ( java_vm, JAVA_IDISPATCH_CLASS );
    if( e->ExceptionCheck() )  e.throw_java( "FindClass " JAVA_IDISPATCH_CLASS );

    int ret = e->RegisterNatives( idispatch_class, native_methods, NO_OF( native_methods ) );
    if( ret < 0 )  throw_java_ret( ret, "RegisterNatives" );


#   ifdef Z_HPUX
        Z_LOG( "init_hostjava()\n" );
        hostjava::init_hostjava( java_vm );     // Weil gcc 3.2 in libhostjava.sl die statischen Variablen nicht initialisiert, bin wir das Module ein.
#   endif
}

//-------------------------------------------------------Java_module_instance::Java_module_instance

Java_module_instance::Java_module_instance( Vm* vm, Module* module )
: 
    Module_instance(module),
    Has_vm(vm),
    _zero_(_end_), 
    _jobject( _java_vm ) 
{
}

//-----------------------------------------------------------------Java_module_instance::close__end

void Java_module_instance::close__end()  // Synchron
{
    close_monitor();
    Module_instance::close__end();   // Synchron

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
            bool compiled = _module->make_java_class( _module->_recompile & !_module->_compiled );     // Java-Klasse ggfs. übersetzen
            _module->_compiled |= compiled;

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

void Java_module_instance::add_obj( IDispatch* object, const string& name )
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

    ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( vm(), object, true, java_class_name.c_str() ) );

    _added_jobjects.push_back( java_idispatch );
                         
    e->SetObjectField( _jobject, field_id, java_idispatch->get_jobject() );
    if( e->ExceptionCheck() )  e.throw_java( "SetObjectField", name );


    Module_instance::add_obj( object, name );
}

//-----------------------------------------------------------------------Java_module_instance::load

bool Java_module_instance::load()
{
    return Module_instance::load();
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name_par )
{
    Env    e = env();
    Java_idispatch_stack_frame stack_frame;
    bool   is_optional = false;
    string name = name_par;

    if( name[0] == '?' )  is_optional = true,  name.erase( 0, 1 );

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  
    {
        if( is_optional )  return Variant();
        throw_xc( "SCHEDULER-174", name, _module->_java_class_name.c_str() );
    }

    Variant result;

    if( *name.rbegin() == 'Z' )
    {
        In_call in_call ( this, name );  //.substr( 0, name.length() - 1 ) );
        result = e->CallBooleanMethod( _jobject, method_id ) != 0;
    }
    else
    {
        In_call in_call ( this, name );
        e->CallVoidMethod( _jobject, method_id );
    }

    if( e->ExceptionCheck() )  e.throw_java( name );

    return result;
}

//-----------------------------------------------------------------------Java_module_instance::call

Variant Java_module_instance::call( const string& name, bool param )
{
    Env e = env();
    Java_idispatch_stack_frame stack_frame;

    jmethodID method_id = _module->java_method_id( name );
    if( !method_id )  throw_xc( "SCHEDULER-174", name, _module->_java_class_name.c_str() );

    bool result;
    
    {
        In_call in_call ( this, name );  //name.substr( 0, name.length() - 1 ) );      // "Z" abschneiden
        result = e->CallBooleanMethod( _jobject, method_id, param ) != 0;
    }

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
