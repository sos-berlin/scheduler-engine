// $Id$

#include "hostjava_common.h"

//#ifdef _DEBUG
//#   include "Debug/java.h/Factory_processor.h"
//# else
//#   include "Release/java.h/Factory_processor.h"
//#endif

using namespace zschimmer;

#if 0
//-------------------------------------------------------------------------Series_factory.construct

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_construct( JNIEnv* jenv, jobject jo )
{
    try
    {
        HRESULT hr;
        Iseries_factory* series_factory = NULL;

        {
            ptr<Iseries_factory> p;

            hr = p.CoCreateInstance( L"hostWare.Series_factory" );
            if( FAILED(hr) )  throw_mswin( hr, "CoCreateInstance" );

            p.CopyTo( &series_factory );
        }

        jfieldID my_data_id = jenv->GetFieldID( jenv->GetObjectClass( jo ), "my_data", "I" );
        jenv->SetIntField( jo, my_data_id, (int)series_factory );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }
}

//--------------------------------------------------------------------Series_factory.order_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_order_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, order_filename )
}

//----------------------------------------------------------------Series_factory.set_order_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1order_1filename( JNIEnv* jenv, jobject jo, jstring order_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, order_filename )
}

//----------------------------------------------------------Series_factory.template_script_language

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_template_1script_1language( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, template_script_language )
}

//------------------------------------------------------Series_factory.set_template_script_language

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1template_1script_1language( JNIEnv* jenv, jobject jo, jstring template_script_language_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, template_script_language )
}

//------------------------------------------------------------------Series_factory.set_start_script

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_start_1script( JNIEnv* jenv, jobject jo, jstring start_script_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, start_script )
}

//----------------------------------------------------------------------Series_factory.template_dir

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_template_1dir( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, template_dir )
}

//------------------------------------------------------------------Series_factory.set_template_dir

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1template_1dir( JNIEnv* jenv, jobject jo, jstring template_dir_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, template_dir )
}

//-----------------------------------------------------------------Series_factory.template_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_template_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, template_filename )
}

//-------------------------------------------------------------Series_factory.set_template_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1template_1filename( JNIEnv* jenv, jobject jo, jstring template_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, template_filename )
}

//---------------------------------------------------------------------Series_factory.head_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_head_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, head_filename )
}

//-----------------------------------------------------------------Series_factory.set_head_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1head_1filename( JNIEnv* jenv, jobject jo, jstring head_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, head_filename )
}

//--------------------------------------------------------------------Series_factory.order_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_document_1dir( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, order_filename )
}

//----------------------------------------------------------------Series_factory.set_order_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1document_1dir( JNIEnv* jenv, jobject jo, jstring order_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, order_filename )
}

//-----------------------------------------------------------------Series_factory.document_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_document_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, document_filename )
}

//-------------------------------------------------------------Series_factory.set_document_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1document_1filename( JNIEnv* jenv, jobject jo, jstring document_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, document_filename )
}

//-----------------------------------------------------------------Series_factory.on_error_continue

JNIEXPORT jboolean JNICALL Java_sos_hostware_Series_1factory_on_1error_1continue( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BOOL( Iseries_factory, on_error_continue )
}

//-------------------------------------------------------------Series_factory.set_on_error_continue

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1on_1error_1continue( JNIEnv* jenv, jobject jo, jboolean on_error_continue_jbool )
{
    PUT_PROP_BOOL( Iseries_factory, on_error_continue )
}

//---------------------------------------------------------------------------Series_factory.context
/*
JNIEXPORT jobject JNICALL Java_sos_hostware_Series_1factory_context( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, order_filename )
}
*/
//------------------------------------------------------------------------Series_factory.set_contet

//JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1context(JNIEnv *, jobject, jobject)

//--------------------------------------------------------------------Series_factory.rerun_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Series_1factory_rerun_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Iseries_factory, rerun_filename )
}

//----------------------------------------------------------------Series_factory.set_rerun_filename

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1rerun_1filename( JNIEnv* jenv, jobject jo, jstring rerun_filename_jstr )
{
    PUT_PROP_BSTR( Iseries_factory, rerun_filename )
}

//---------------------------------------------------------------------------Series_factory.collect

JNIEXPORT jint JNICALL Java_sos_hostware_Series_1factory_collect( JNIEnv* jenv, jobject jo )
{
    GET_PROP_INT( Iseries_factory, collect )
}

//-----------------------------------------------------------------------Series_factory.set_collect

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_set_1collect( JNIEnv* jenv, jobject jo, jint collect )
{
    PUT_PROP_INT( Iseries_factory, collect )
}

//------------------------------------------------------------------------------Series_factory.open

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_open( JNIEnv* jenv, jobject jo )
{
    CALL_METHOD( Iseries_factory, open )
}

//-----------------------------------------------------------------------------Series_factory.close

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_close( JNIEnv* jenv, jobject jo )
{
    CALL_METHOD( Iseries_factory, close )
}

//---------------------------------------------------------------------------Series_factory.process

JNIEXPORT jboolean JNICALL Java_sos_hostware_Series_1factory_process( JNIEnv* jenv, jobject jo )
{
    VARIANT_BOOL ok = false;

    try                                                                                
    {                                                                                  
        Iseries_factory* series_factory = (Iseries_factory*)get_my_data( jenv, jo );   
                                                                                                
        HRESULT hr = series_factory->process( &ok );                                   
        if( FAILED(hr) )  throw_com( hr, "Series_factory.process" );
    }                                                                                           
    catch( const exception&  x ) { set_java_exception( jenv, x ); }                             
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }                             

    return ok;
}

//-----------------------------------------------------------------------Series_factory.process_all

JNIEXPORT void JNICALL Java_sos_hostware_Series_1factory_process_1all( JNIEnv* jenv, jobject jo )
{
    CALL_METHOD( Iseries_factory, process_all )
}

//-----------------------------------------------------------------Series_factory.factory_processor

JNIEXPORT jobject JNICALL Java_sos_hostware_Series_1factory_processor( JNIEnv* jenv, jobject jo )
{
    jobject processor_j = NULL;

    try
    {
        Iseries_factory*    series_factory = (Iseries_factory*)get_my_data( jenv, jo );
        Ifactory_processor* processor      = NULL;

        HRESULT hr = series_factory->get_processor( &processor );
        if( FAILED(hr) )  throw_com( hr, "Series_factory.processor" );

        jclass cls = jenv->FindClass( "sos/hostware/Factory_processor" );

        jmethodID method = jenv->GetMethodID( cls, "<init>", "()V");
        processor_j = jenv->NewObject( cls, method );

        jfieldID my_data_id = jenv->GetFieldID( cls, "my_data", "I" );
        jenv->SetIntField( processor_j, my_data_id, (int)processor );
    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }

    return processor_j;
}

//-------------------------------------------------------------------------Series_factory.record_nr

JNIEXPORT jint JNICALL Java_sos_hostware_Series_1factory_record_1nr( JNIEnv* jenv, jobject jo )
{
    GET_PROP_INT( Iseries_factory, record_nr )
}

//-------------------------------------------------------------------------Series_factory.rerunning

JNIEXPORT jboolean JNICALL Java_sos_hostware_Series_1factory_rerunning( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BOOL( Iseries_factory, rerunning )
}

//-------------------------------------------------------------------------------Series_factory.eof

JNIEXPORT jboolean JNICALL Java_sos_hostware_Series_1factory_eof( JNIEnv* jenv, jobject jo )
{
    VARIANT_BOOL eof = false;

    try
    {
        Iseries_factory* series_factory = (Iseries_factory*)get_my_data( jenv, jo );

        HRESULT hr = series_factory->eof( &eof );
        if( FAILED(hr) )  throw_com( hr, "Series_factory.processor" );

    }
    catch( const exception&  x ) { set_java_exception( jenv, x ); }
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }

    return eof;
}

//-------------------------------------------------------------------------------------------------
#endif
