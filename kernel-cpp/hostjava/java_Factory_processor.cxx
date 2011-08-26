// $Id: java_Factory_processor.cxx 13912 2010-06-30 15:41:14Z ss $
// §1800

#include "hostjava_common.h"

//#ifdef _DEBUG
//#   include "Debug/java.h/Factory_processor.h"
//# else
//#   include "Release/java.h/Factory_processor.h"
//#endif

using namespace zschimmer;
using namespace zschimmer::com;
using namespace zschimmer::javabridge;
using namespace sos::hostjava;

//----------------------------------------------------------------------Factory_processor.construct
/*
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_construct( JNIEnv* jenv, jobject jo )
{
    try
    {
        HRESULT                 hr                  = NOERROR;
        Ifactory_processor*     factory_processor   = NULL;
        ptr<Ifactory_processor> p;
        bool                    coinitialize_called = false;

        hr = p.CoCreateInstance( CLSID_Factory_processor );

        if( hr == CO_E_NOTINITIALIZED  &&  !coinitialize_called )
        {
          //hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );  if( FAILED(hr) )  throw_com( hr, "CoInitializeEx(,COINIT_MULTITHREADED)" );
            hr = CoInitialize(NULL);  if( FAILED(hr) )  throw_com( hr, "CoInitialize()" );

            hr = p.CoCreateInstance( CLSID_Factory_processor );
        }
            
        if( FAILED(hr) )  throw_com( hr, ( "CoCreateInstance CLSID_Factory_processor " + string_from_clsid( CLSID_Factory_processor ) ).c_str() );

        p.move_to( &factory_processor );

        jfieldID my_idispatch_id = jenv->GetFieldID( jenv->GetObjectClass( jo ), "_idispatch", "J" );
        jenv->SetLongField( jo, my_idispatch_id, (int)factory_processor );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}
*/
//-----------------------------------------------------------------------Factory_processor.destruct
/*
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_destruct( JNIEnv* jenv, jobject jo )
{
    Java_sos_hostware_Factory_1processor_close( jenv, jo );
}
*/
//-------------------------------------------------------------------Factory_processor.close_native

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_close_1native( JNIEnv* jenv, jobject jo )
{
    Com_env env = jenv;

    try
    {
        HRESULT  hr;
        jfieldID my_idispatch_id = 0;
        Ifactory_processor* factory_processor = (Ifactory_processor*)get_my_idispatch( jenv, jo, &my_idispatch_id );

        if( factory_processor )
        {
            hr = factory_processor->Close();
          //factory_processor->Release();
          //jenv->SetLongField( jo, my_idispatch_id, 0 );

            if( FAILED(hr) )  throw_com( hr, "Ifactory_processor.close" );
        }
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//---------------------------------------------------------------------------Factory_processor.eval

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_eval( JNIEnv* jenv, jobject jo, jstring expr_j, jint flags )
{
    Com_env env = jenv;

    try
    {
        HRESULT     hr;
        Bstr        expr_bstr;
        jfieldID    my_idispatch_id = 0;
        Ifactory_processor* factory_processor = (Ifactory_processor*)get_my_idispatch( jenv, jo, &my_idispatch_id );   //jenv->GetIntField( jo, my_idispatch_id );

        env.jstring_to_bstr( expr_j, &expr_bstr );

        Variant result;

        hr = factory_processor->Eval( expr_bstr, (Scripttext_flags)flags, &result );
        if( FAILED(hr) )  throw_com( hr, "Factory_processor.eval" );

        hr = result.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_com( hr, "Factory_processor.eval" );

        return env.jstring_from_bstr( V_BSTR( &result ) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return NULL;
}

//--------------------------------------------------------------------------Factory_processor.parse

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_parse( JNIEnv* jenv, jobject jo, jstring script_text_j, jint flags )
{
    Com_env env = jenv;
    HRESULT hr;

    try
    {
        Bstr                script_text_bstr;
        Ifactory_processor* factory_processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );   //jenv->GetIntField( jo, my_idispatch_id );

        env.jstring_to_bstr( script_text_j, &script_text_bstr );

        Variant result;
        hr = factory_processor->Parse( script_text_bstr, (Scripttext_flags)flags, &result );
        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parse" );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-----------------------------------------------------------------Factory_processor.add_parameters

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_add_1parameters( JNIEnv* jenv, jobject jo )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* factory_processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        HRESULT             hr = NOERROR;

        hr = factory_processor->Add_parameters();

        if( FAILED(hr) )  throw_com( hr, "Ifactory_processor.add_parameters" );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//------------------------------------------------------------------------Factory_processor.process

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_process( JNIEnv* jenv, jobject jo )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* factory_processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        HRESULT             hr = NOERROR;

        hr = factory_processor->Process();

        if( FAILED(hr) )  throw_com( hr, "Ifactory_processor.process" );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//--------------------------------------------------------------------Factory_processor.script_text

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_script_1text( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Script_text )
}

//-----------------------------------------------------------------Factory_processor.error_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_error_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Error_filename )
}

//-----------------------------------------------------------------Factory_processor.error_document

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_error_1document( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Error_document )
}

//----------------------------------------------------------Factory_processor.set_document_filename

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1document_1filename( JNIEnv* jenv, jobject jo, jstring Document_filename_jstr )
{
	
    #if defined Z_UNIX && defined _DEBUG	// Gnu Java (gji) schreibt nicht das Null-Byte am Stringende:
    {
        const OLECHAR* str_w = jenv->GetStringChars( Document_filename_jstr, 0 );
        Z_LOG("sos.hostware.Factory_processor.set_document_filename \"" ); //" << str_w << "\"\n");
        for( const OLECHAR* o = str_w; *o; o++ ) Z_LOG((char)*o);
        Z_LOG("\"\n");
    }
    #endif

    PUT_PROP_BSTR( Ifactory_processor, Document_filename )
}

//--------------------------------------------------------------Factory_processor.document_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_document_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Document_filename )
}

//--------------------------------------------------------------Factory_processor.set_head_filename

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1head_1filename( JNIEnv* jenv, jobject jo, jstring Head_filename_jstr )
{
    PUT_PROP_BSTR( Ifactory_processor, Head_filename )
}

//------------------------------------------------------------------Factory_processor.head_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_head_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Head_filename )
}

//-------------------------------------------------------------------Factory_processor.set_language

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1language( JNIEnv* jenv, jobject jo, jstring Language_jstr )
{
    PUT_PROP_BSTR( Ifactory_processor, Language )
}

//-----------------------------------------------------------------------Factory_processor.language

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_language( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Language )
}

//----------------------------------------------------------------------Factory_processor.set_param

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1param( JNIEnv* jenv, jobject jo, jstring Param_jstr )
{
    PUT_PROP_BSTR( Ifactory_processor, Param )
}

//------------------------------------------------------------------Factory_processor.set_parameter

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter( JNIEnv* jenv, jobject jo, jstring name_jstr, jstring value_jstr )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor   = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;
        HRESULT             hr          = NOERROR;

        env.jstring_to_bstr( name_jstr , &name_bstr  );

        value_vt.vt = VT_BSTR;
        env.jstring_to_bstr( value_jstr, &V_BSTR(&value_vt) );

        hr = processor->put_Parameter( name_bstr, &value_vt );

        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//------------------------------------------------------------------Factory_processor.set_parameter_bool

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter_1bool( JNIEnv* jenv, jobject jo, jstring name_jstr, jboolean value )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor   = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;
        HRESULT             hr          = NOERROR;

        env.jstring_to_bstr( name_jstr , &name_bstr  );
        value_vt = (bool)( value != 0 );

        hr = processor->put_Parameter( name_bstr, &value_vt );

        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//--------------------------------------------------------------Factory_processor.set_parameter_int

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter_1int( JNIEnv* jenv, jobject jo, jstring name_jstr, jint value )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;
        HRESULT             hr        = NOERROR;

        env.jstring_to_bstr( name_jstr , &name_bstr );
        value_vt = value;

        hr = processor->put_Parameter( name_bstr, &value_vt );

        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-------------------------------------------------------------Factory_processor.set_parameter_long
/*
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter_1long( JNIEnv* jenv, jobject jo, jstring name_jstr, jlong value )
{
    try
    {
        Ifactory_processor* processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;

        env.jstring_to_bstr( name_jstr , &name_bstr  );
        value_vt = value;

        HRESULT hr = processor->put_parameter( name_bstr, &value_vt );
        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}
*/
//-----------------------------------------------------------Factory_processor.set_parameter_double

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter_1double( JNIEnv* jenv, jobject jo, jstring name_jstr, jdouble value )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;
        HRESULT             hr        = NOERROR;

        env.jstring_to_bstr( name_jstr , &name_bstr  );
        value_vt = value;

        hr = processor->put_Parameter( name_bstr, &value_vt );

        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//---------------------------------------------------------Factory_processor.set_parameter_currency

extern "C"
JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1parameter_1currency( JNIEnv* jenv, jobject jo, jstring name_jstr, jlong value )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Bstr                name_bstr;
        Variant             value_vt;
        HRESULT             hr        = NOERROR;

        env.jstring_to_bstr( name_jstr , &name_bstr  );

/*
        jclass decimal_cls = env->FindClass( "java.math.BigDecimal" );
        if( !decimal_cls )  return;

        #if( !env->IsInstanceOf( value, decimal_cls ) )  throw_xc( "No-BigDecimal" );

        jclass oclass = env->GetObjectClass( value );
        if( !oclass )  return;

        jmethod to_string = env->GetMethodID( oclass, "toString", "()Ljava.lang.string;" );
        if( !to_string )  return;

        jstring value_jstr = env->CallVoidMethod( value, to_string );
        if( env->ExceptionOccurred() )  return;

        env.jstring_to_bstr( name_jstr , &name_bstr  );
*/
        value_vt.vt = VT_CY;
        value_vt.cyVal.int64 = value;

        hr = processor->put_Parameter( name_bstr, &value_vt );

        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//------------------------------------------------------------Factory_processor.parameter_as_string

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_parameter_1as_1string( JNIEnv* jenv, jobject jo, jstring name_jstr )
{
    Com_env env = jenv;

    try
    {
        Ifactory_processor* processor = (Ifactory_processor*)get_my_idispatch( jenv, jo );
        Ivariable*          variable  = NULL;
        Bstr                name_bstr;   env.jstring_to_bstr( name_jstr, &name_bstr );
        HRESULT             hr        = NOERROR;

        Variant             empty_vt;
        Variant             value_vt;

        hr = processor->get_Parameter( name_bstr, &variable );
        if( FAILED(hr) )  throw_com( hr, "Factory_processor.parameter", name_bstr );

        hr = variable->get_Value( &empty_vt, &value_vt );
        if( FAILED(hr) )  throw_com( hr, "Variable.value", name_bstr );

        hr = value_vt.ChangeType( VT_BSTR );
        if( FAILED(hr) )  throw_com( hr, "VariantChangeType" );

        return env.jstring_from_bstr( V_BSTR(&value_vt) );
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }

    return NULL;
}

//----------------------------------------------------------Factory_processor.set_template_filename

JNIEXPORT void JNICALL Java_sos_hostware_Factory_1processor_set_1template_1filename( JNIEnv* jenv, jobject jo, jstring Template_filename_jstr )
{
    PUT_PROP_BSTR( Ifactory_processor, Template_filename )
}

//--------------------------------------------------------------Factory_processor.template_filename

JNIEXPORT jstring JNICALL Java_sos_hostware_Factory_1processor_template_1filename( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ifactory_processor, Template_filename )
}
