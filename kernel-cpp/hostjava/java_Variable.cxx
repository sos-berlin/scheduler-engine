// $Id$

#include "hostjava_common.h"

using namespace zschimmer;
using namespace zschimmer::com;
using namespace zschimmer::javabridge;
using namespace sos::hostjava;

//-----------------------------------------------------------------------------------variable_value

static jstring variable_value( JNIEnv* jenv, jobject jo, VARIANT* index_vt )
{
    Com_env env = jenv;

    try                                                                                         
    {                                                                                           
        Ivariable*  v = (Ivariable*)get_my_idispatch( jenv, jo );               

        //Z_MUTEX( hostjava_mutex )
        {
            HRESULT     hr;
            Variant     value_vt;

            hr = v->get_Value( index_vt, &value_vt );
            if( FAILED(hr) )  throw_com( hr, "Variable::value" );

            hr = value_vt.ChangeType( VT_BSTR );
            if( FAILED(hr) )  throw_com( hr, "VariantChangeType" );

            return env.jstring_from_bstr( V_BSTR(&value_vt) );
        }
    }                                                                                           
    catch( const exception&  x ) { env.set_java_exception( x ); }                             
    catch( const _com_error& x ) { env.set_java_exception( x ); }                             
                                                                                                
    return NULL;
}

//-----------------------------------------------------------------------------------Variable.value

JNIEXPORT jstring JNICALL Java_sos_hostware_Variable_value__( JNIEnv* jenv, jobject jo )
{
    VARIANT empty_vt;  VariantInit( &empty_vt );
    return variable_value( jenv, jo, &empty_vt );
}

//-----------------------------------------------------------------------------------Variable.value

JNIEXPORT jstring JNICALL Java_sos_hostware_Variable_value__I( JNIEnv* jenv, jobject jo, jint index )
{
    Variant index_vt = index; 
    return variable_value( jenv, jo, &index_vt );
}

//-------------------------------------------------------------------------------variable_set_value

void variable_set_value( JNIEnv* jenv, jobject jo, VARIANT* index_vt, jstring value_jstr )
{
    Com_env env = jenv;

    try                                                                                         
    {                                                                                           
        HRESULT     hr      = NOERROR;
        Variant     value_vt;
        Ivariable*  v       = (Ivariable*)get_my_idispatch( jenv, jo );                                 
        
        value_vt.vt = VT_BSTR;
        env.jstring_to_bstr( value_jstr, &V_BSTR( &value_vt ) );

        //Z_MUTEX( hostjava_mutex )
        {
            hr = v->put_Value( index_vt, &value_vt );
        }

        if( FAILED(hr) )  throw_com( hr, "Variable::value" );
    }                                                                                           
    catch( const exception&  x ) { env.set_java_exception( x ); }                             
    catch( const _com_error& x ) { env.set_java_exception( x ); }                             
}

//-------------------------------------------------------------------------------Variable.set_value

JNIEXPORT void JNICALL Java_sos_hostware_Variable_set_1value__Ljava_lang_String_2( JNIEnv* jenv, jobject jo, jstring value )
{
    VARIANT empty_vt;  VariantInit( &empty_vt );
    variable_set_value( jenv, jo, &empty_vt, value );
}

//-------------------------------------------------------------------------------Variable.set_value

JNIEXPORT void JNICALL Java_sos_hostware_Variable_set_1value__ILjava_lang_String_2( JNIEnv* jenv, jobject jo, jint index, jstring value_jstr )
{
    Variant index_vt = index; 
    variable_set_value( jenv, jo, &index_vt, value_jstr );
}

//-------------------------------------------------------------------------------------Variable.dim

JNIEXPORT void JNICALL Java_sos_hostware_Variable_dim( JNIEnv* jenv, jobject jo, jint dim )
{
    Com_env env = jenv;

    try                                                                                         
    {                                                                                           
        HRESULT     hr;
        Ivariable*  v = (Ivariable*)get_my_idispatch( jenv, jo );                                 
        
        //Z_MUTEX( hostjava_mutex )
        {
            hr = v->Dim( dim );
        }

        if( FAILED(hr) )  throw_com( hr, "Variable::dim" );
    }                                                                                           
    catch( const exception&  x ) { env.set_java_exception( x ); }                             
    catch( const _com_error& x ) { env.set_java_exception( x ); }                             
}

//------------------------------------------------------------------------------------Variable.name

JNIEXPORT jstring JNICALL Java_sos_hostware_Variable_name( JNIEnv* jenv, jobject jo )
{
    GET_PROP_BSTR( Ivariable, Name )
}

//-------------------------------------------------------------------------------------------------
