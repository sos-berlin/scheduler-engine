// $Id: spooler_dll_java.cxx,v 1.5 2004/10/15 13:08:18 jz Exp $

#include "spooler.h"
#include "../zschimmer/java.h"


#ifdef Z_WINDOWS

#ifdef _DEBUG
#   include "Debug/sos/spooler/Spooler_program.h"
#else
#   include "Release/sos/spooler/Spooler_program.h"
#endif

namespace sos 
{
    int sos_main( int argc, char** argv );
}


using namespace zschimmer::java;


//-------------------------------------------------------------------------------------------static

static ptr<java::Vm>            static_java_vm;

//---------------------------------------------------------------------------------------JNI_OnLoad

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* jvm, void* )
{
    CoInitialize(NULL);
    zschimmer_init();

    static_java_vm = Z_NEW( java::Vm( jvm ) );
    static_java_vm._ptr->AddRef();                 // Damit bei Programmende nicht Release gerufen wird (die Java-DLL ist dann vielleicht schon entladen)
    //java::Vm::set_jvm( jvm );

    return JNI_VERSION_1_2;
}

//-------------------------------------------------------------------------------------JNI_OnUnload

extern "C" JNIEXPORT void JNICALL JNI_OnUnload( JavaVM*, void* )
{
    if( static_java_vm )  static_java_vm._ptr->Release();
    static_java_vm = NULL;

    zschimmer_terminate();
    CoUninitialize();
}

//-------------------------------------------------------------sos.spooler.Spooler_program.construct

JNIEXPORT void JNICALL Java_sos_spooler_Spooler_1program_construct( JNIEnv* jenv, jobject, jstring parameters_jstr )
{
    try
    {
        int ret = sos::spooler_main( NULL, NULL, string_from_jstring( jenv, parameters_jstr ) );
    }
    catch( const exception&  ) {} //set_java_exception( jenv, x ); }
    catch( const _com_error& ) {} //set_java_exception( jenv, x ); }
}

//--------------------------------------------------------sos.spooler.Spooler_program.construct_argv

JNIEXPORT void JNICALL Java_sos_spooler_Spooler_1program_construct_1argv( JNIEnv* jenv, jobject, jobjectArray jparams )
{
    char** argv = NULL;
    int    argc = 0;

    try
    {
        int count = jenv->GetArrayLength( jparams );

        argv = new char*[ count ];

        for( int i = 0; i < count; i++ )
        {
            jobject jparam = jenv->GetObjectArrayElement( jparams, i );
            string arg = string_from_jstring( jenv, (jstring)jparam );
            argv[i] = new char[ arg.length() + 1 ];
            strcpy( argv[i], arg.c_str() );
          //fprintf(stderr,"%d: %s\n", i, argv[i]);
            argc++;
        }

        int ret = sos::spooler_main( argc, argv, "" );
    }
    catch( const exception&  ) {} //set_java_exception( jenv, x ); }
    catch( const _com_error& ) {} //set_java_exception( jenv, x ); }

    for( int i = 0; i < argc; i++ )  delete argv[i];
    delete[] argv;
}

//-------------------------------------------------------------------------------------------------

#endif
