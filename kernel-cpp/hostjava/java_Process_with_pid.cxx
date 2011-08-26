// $Id: java_Process_with_pid.cxx 13912 2010-06-30 15:41:14Z ss $

#include "hostjava_common.h"
#include "../zschimmer/z_process.h"

using namespace std;
using namespace zschimmer;
using namespace zschimmer::com;
using namespace zschimmer::javabridge;
using namespace sos::hostjava;

//-----------------------------------------------------------------------Process_with_pid.construct

extern "C"
JNIEXPORT jlong JNICALL Java_sos_hostware_Process_1with_1pid_construct( JNIEnv* jenv, jobject )
{
    jlong result = 0;
    Env   env    = jenv;

    try
    {
        ptr<Process> process = Z_NEW( Process );
        result = (jlong)(size_t)process.take();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//---------------------------------------------------------------------------Process_with_pid.close

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_close( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env env = jenv;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;

        if( process )  process->Release();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//---------------------------------------------------------------------------Process_with_pid.start

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_start( JNIEnv* jenv, jobject, jlong process_ptr, jstring cmd_line_jstr )
{
    Env env = jenv;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return; }

        process->start( env.string_from_jstring( cmd_line_jstr ) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//---------------------------------------------------------------------Process_with_pid.start_array

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_start_1array( JNIEnv* jenv, jobject, jlong process_ptr, jobjectArray cmd_array_j )
{
    Env env = jenv;

    try
    {
        Process* process  = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return; }

        int            len      = jenv->GetArrayLength( cmd_array_j );
        vector<string> cmd_array ( len );

        for( int i = 0; i < len; i++ )
        {
            jstring jparam = static_cast<jstring>( jenv->GetObjectArrayElement( cmd_array_j, i ) );
            cmd_array[ i ] = env.string_from_jstring( jparam );
            jenv->DeleteLocalRef( jparam );
        }

        process->start( cmd_array );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-----------------------------------------------------------------------------Process_with_pid.pid

extern "C"
JNIEXPORT jint JNICALL Java_sos_hostware_Process_1with_1pid_pid( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env  env    = jenv;
    jint result = 0;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return result; }

        result = process->pid();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//----------------------------------------------------------------------Process_with_pid.terminated

extern "C"
JNIEXPORT jboolean JNICALL Java_sos_hostware_Process_1with_1pid_terminated( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env      env    = jenv;
    jboolean result = false;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return result; }

        result = process->terminated();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//-------------------------------------------------------------------------Process_with_pid.destroy

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_destroy( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env env = jenv;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return; }

        process->try_kill();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-------------------------------------------------------------------------Process_with_pid.waitFor

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_waitFor( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env env = jenv;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return; }

        process->wait();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-----------------------------------------------------------------------Process_with_pid.exitValue

extern "C"
JNIEXPORT jint JNICALL Java_sos_hostware_Process_1with_1pid_exitValue( JNIEnv* jenv, jobject, jlong process_ptr )
{
    Env  env    = jenv;
    jint result = 0;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return result; }

        result = process->exit_code();
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//-----------------------------------------------------------Process_with_pid.set_own_process_group

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_set_1own_1process_1group( JNIEnv* jenv, jobject, jlong process_ptr, jboolean on )
{
    Env env = jenv;

    try
    {
        Process* process = (Process*)(size_t)process_ptr;
        if( !process )  { env.set_NullPointerException( "hostjava" );  return; }

        process->set_own_process_group( on? true : false );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//------------------------------------------------------------------------Process_with_pid.kill_pid

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Process_1with_1pid_kill_1pid( JNIEnv* jenv, jclass, jint pid )
{
    Env env = jenv;

    try
    {
        kill_process_immediately( pid );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//--------------------------------------------------------------------Process_with_pid.try_kill_pid

extern "C"
JNIEXPORT jboolean JNICALL Java_sos_hostware_Process_1with_1pid_try_1kill_1pid( JNIEnv* jenv, jclass, jint pid )
{
    Env      env    = jenv;
    jboolean result = false;

    try
    {
        result = try_kill_process_immediately( pid );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return result;
}

//--------------------------------------------------------------------------------------------const

namespace sos {
namespace hostjava {

const static JNINativeMethod native_methods[] = 
{
    // sos.hostware.Process_with_pid
    { "close"                   , "(J)V"                    , (void*)Java_sos_hostware_Process_1with_1pid_close },
    { "start"                   , "(JLjava/lang/String;)V"  , (void*)Java_sos_hostware_Process_1with_1pid_start },
    { "start_array"             , "(J[Ljava/lang/String;)V" , (void*)Java_sos_hostware_Process_1with_1pid_start_1array },
    { "pid"                     , "(J)I"                    , (void*)Java_sos_hostware_Process_1with_1pid_pid },
    { "terminated"              , "(J)Z"                    , (void*)Java_sos_hostware_Process_1with_1pid_terminated },
    { "destroy"                 , "(J)V"                    , (void*)Java_sos_hostware_Process_1with_1pid_destroy },
    { "waitFor"                 , "(J)I"                    , (void*)Java_sos_hostware_Process_1with_1pid_waitFor },
    { "exitValue"               , "(J)I"                    , (void*)Java_sos_hostware_Process_1with_1pid_exitValue },
    { "kill_pid"                , "(I)V"                    , (void*)Java_sos_hostware_Process_1with_1pid_kill_1pid },
    { "try_kill_pid"            , "(I)Z"                    , (void*)Java_sos_hostware_Process_1with_1pid_try_1kill_1pid },
};

//------------------------------------------------------------------------------------init_hostjava

void init_hostjava_process_with_pid( javabridge::Vm* java_vm )
{
    register_natives( java_vm, "sos/hostware/Process_with_pid", native_methods, NO_OF( native_methods ) );
}

} //namespace hostjava
} //namespace sos

//-------------------------------------------------------------------------------------------------

