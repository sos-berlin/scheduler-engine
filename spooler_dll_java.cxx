// $Id: spooler_dll_java.cxx,v 1.1 2003/09/04 15:56:11 jz Exp $

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


//-------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------JNI_OnLoad

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM* jvm, void* )
{
    CoInitialize(NULL);
    zschimmer_init();
    java::Vm::set_jvm( jvm );
    return JNI_VERSION_1_2;
}

//-------------------------------------------------------------------------------------JNI_OnUnload

extern "C" JNIEXPORT void JNICALL JNI_OnUnload( JavaVM* jvm, void* )
{
    zschimmer_terminate();
    CoUninitialize();
}

//-------------------------------------------------------------sos.spooler.Spooler_program.construct

JNIEXPORT void JNICALL Java_sos_spooler_Spooler_1program_construct( JNIEnv* jenv, jobject jo, jstring parameters_jstr )
{
    try
    {
        int ret = sos::spooler_main( NULL, NULL, string_from_jstring( jenv, parameters_jstr ) );
    }
    catch( const exception&  ) {} //set_java_exception( jenv, x ); }
    catch( const _com_error& ) {} //set_java_exception( jenv, x ); }
}

//-------------------------------------------------------------------------------------------------

#endif
