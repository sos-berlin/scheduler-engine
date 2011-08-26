// $Id: java_Idispatch.cxx 12516 2007-01-16 10:56:47Z jz $

#include "z_java.h"

/*
#   ifdef _DEBUG
#       include "Debug/Idispatch.h"
#    else
#       include "Release/Idispatch.h"
#   endif
*/

using namespace zschimmer;
using namespace zschimmer::com;

//------------------------------------------------------------------------------Idispatch.construct

JNIEXPORT void JNICALL Java_sos_zschimmer_com_Idispatch_com_1construct( JNIEnv* jenv, jclass, jlong jidispatch )
{
    IDispatch* idispatch = (IDispatch*)jidispatch;

    if( idispatch )  idispatch->AddRef();
}

//-------------------------------------------------------------------------------Idispatch.destruct

JNIEXPORT void JNICALL Java_zschimmer_com_Idispatch_destruct( JNIEnv* jenv, jclass, jint jidispatch )
{
    IDispatch* idispatch = (IDispatch*)jidispatch;

    if( idispatch )  idispatch->Release();
}

//--------------------------------------------------------------------------Idispatch.com_method_id

JNIEXPORT jint JNICALL Java_zschimmer_com_Idispatch_com_1method_1id( JNIEnv* jenv, jobject jo, jint jidispatch, jint context, jstring name )
{
    try
    {
        HRESULT    hr;
        IDispatch* idispatch = (IDispatch*)jidispatch;
        Bstr       name_bstr = bstr_from_jstring( name );

        if( !idispatch )  throw_xc( "Idispatch::method_id" );

        DISPID dispid = 0;
        hr = idispatch->GetIDsOfNames( IID_NULL, &name_bstr, 1, STANDARD_LCID, &disp_id );
        if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_jstring(name).c_str() );

        return disp_id;
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-------------------------------------------------------------------------------Idispatch.com_call

JNIEXPORT jobject JNICALL Java_zschimmer_com_Idispatch_com_1call( JNIEnv* jenv, jobject jo, jint method_id, jint context )
{
    try
    {
        HRESULT    hr;
        IDispatch* idispatch = (IDispatch*)jidispatch;
        Variant    result;
        Excepinfo  excepinfo;
        UINT       arg_nr = -1;

        if( !idispatch )  throw_xc( "Idispatch::com_call" );

        hr = idispatch->Invoke( method_id, IID_NULL, STANDARD_LCID, context, params, &result, &excepinfo, &arg_nr );
        if( FAILED(hr) )  throw_com( hr, "Invoke", string_from_bstr(name).c_str() );

        return disp_id;
    }
    catch( const exception&  x ) { env.set_java_exception( x ); }
    catch( const _com_error& x ) { env.set_java_exception( x ); }
}

//-------------------------------------------------------------------------------------------------
