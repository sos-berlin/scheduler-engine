// $Id: java_com.cxx 14174 2011-01-13 16:26:26Z ss $

#include "zschimmer.h"
#include "threads.h"
#include "z_com.h"
#include "java.h"
#include "java_com.h"
#include "java_odl.h"

#include <math.h>

#ifdef Z_UNIX
#   include <dlfcn.h>
#endif

#ifdef Z_HPUX
#   include <dl.h>
#endif



namespace zschimmer {
namespace javabridge {

using namespace std;
using namespace com;

//----------------------------------------------------------------------------------Class_container
/*
struct Standard_classes : Object
{
    Standard_classes( JNIEnv* jenv )
    {
        _java_lang_String = jenv->FindClass( 
        jenv->NewGlobalRef( _java_lang_String );
    }


    jclass                     _java_lang_String;
    Class                      _java_lang_Boolean;
    Class                      _java_lang_Integer;
    Class                      _java_lang_Double;
    Class                      _sos_spooler_Idispatch;

};
*/
//--------------------------------------------------------------------Com_env::jstring_from_variant

jstring Com_env::jstring_from_variant( const VARIANT& v )
{
    if( V_VT( &v ) == VT_BSTR ) 
    {
        return jstring_from_bstr( V_BSTR( &v ) );
    }
    else
    {
        Variant bstr_variant;
        bstr_variant.change_type( VT_BSTR, v );
        return jstring_from_bstr( V_BSTR( &bstr_variant ) );
    }
}

//-------------------------------------------------------------------------Com_env::jstring_to_bstr

void Com_env::jstring_to_bstr( const jstring& jstr, BSTR* bstr )
{
    JNIEnv* jenv = jni_env();

    int len = jenv->GetStringLength( jstr );

    {
        Read_jstring_critical jchars ( jstr );    // Ab jetzt kein JNI-Aufruf!
        HRESULT hr = String_to_bstr( (const OLECHAR*)(const jchar*)jchars, len, bstr );    
        if( FAILED(hr) )  throw_com( hr, "jstring_to_bstr/string_to_bstr" );
    }
}

//--------------------------------------------------------------------Com_env::jobject_from_variant

jobject Com_env::jobject_from_variant( const VARIANT& v, Java_idispatch_container* java_idispatch_container )
{
    Env     env = jni_env();
    jobject result = NULL;

    if( Vm::static_vm )
    {
        Vm::Standard_classes* stdcls = Vm::static_vm->standard_classes();

        switch( v.vt )
        {
            case VT_EMPTY:
                return NULL;

            case VT_ERROR:
                if( v.scode == DISP_E_PARAMNOTFOUND ) return NULL;

                throw_com( v.scode, "Variant VT_ERROR" );

            case VT_NULL: 
                return NULL;

            case VT_I2:
            {
                result = env->NewObject( stdcls->_java_lang_short_class, stdcls->_java_lang_short_constructor_id, (jshort)V_I2(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Short" );
                break;
            }

            case VT_I4:
            {
                result = env->NewObject( stdcls->_java_lang_integer_class, stdcls->_java_lang_integer_constructor_id, (jint)V_I4(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Integer" );
                break;
            }

            case VT_R4:
            {
                result = env->NewObject( stdcls->_java_lang_float_class, stdcls->_java_lang_float_constructor_id, (jfloat)V_R4(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Float" );
                break;
            }

            case VT_R8:
            {
                result = env->NewObject( stdcls->_java_lang_double_class, stdcls->_java_lang_double_constructor_id, (jdouble)V_R8(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Double" );
                break;
            }
            
          //case VT_CY: 

            case VT_DATE:       
            {
                int64 msec = (int64)( 1000 * seconds_since_1970_from_local_com_date( V_DATE( &v ) ) );
                result = env->NewObject( stdcls->_java_util_date_class, stdcls->_java_util_date_constructor_id, (jlong)msec );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.util.Date" );
                break;
            }

            case VT_BOOL:
            {
                result = env->NewObject( stdcls->_java_lang_boolean_class, stdcls->_java_lang_boolean_constructor_id, (jboolean)V_BOOL(&v)? 1 : 0 );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Boolean" );
                break;
            }

            case VT_I1:  
            {
                result = env->NewObject( stdcls->_java_lang_byte_class, stdcls->_java_lang_byte_constructor_id, (jbyte)V_I1(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Byte" );
                break;
            }

            case VT_UI1: 
            {
                result = env->NewObject( stdcls->_java_lang_short_class, stdcls->_java_lang_short_constructor_id, (jshort)V_UI1(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Short" );
                break;
            }

            case VT_UI2: 
            {
                result = env->NewObject( stdcls->_java_lang_integer_class, stdcls->_java_lang_integer_constructor_id, (jint)V_UI2(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Integer" );
                break;
            }

            case VT_UI4: 
            {
                result = env->NewObject( stdcls->_java_lang_long_class, stdcls->_java_lang_long_constructor_id, (jlong)V_UI4(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Long" );
                break;
            }

            case VT_I8:  
            {
                result = env->NewObject( stdcls->_java_lang_long_class, stdcls->_java_lang_long_constructor_id, (jlong)V_I8(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Long" );
                break;
            }

          //case VT_UI8: 
            case VT_INT: 
            {
                result = env->NewObject( stdcls->_java_lang_integer_class, stdcls->_java_lang_integer_constructor_id, (jint)V_INT(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Integer" );
                break;
            }

            case VT_UINT:
            {
                result = env->NewObject( stdcls->_java_lang_long_class, stdcls->_java_lang_long_constructor_id, (jlong)V_UINT(&v) );
                if( env->ExceptionCheck() )  env.throw_java( "NewObject", "java.lang.Long" );
                break;
            }

          //case VT_VOID:
          //case VT_HRESULT:
          //case VT_PTR:
          //case VT_FILETIME:

            case VT_BSTR: 
            {
                result = jstring_from_bstr( V_BSTR(&v) );
                break;
            }

            case VT_DISPATCH:
            {
                string      java_class_name;
                const char* java_class_name_ptr = NULL;

                IDispatch* idispatch = V_DISPATCH( &v );
                if( !idispatch )  return NULL;

                ptr<odl::Ihas_java_class_name> j;
                HRESULT hr = j.Assign_qi( idispatch );
                
                if( !FAILED( hr ) )
                {
                    //Bstr java_class_name_bstr;
                    //hr = j->get_java_class_name( &java_class_name_bstr );
                    //if( FAILED(hr) )  throw_com( hr, "get_java_class_name" );

                    //java_class_name = string_from_bstr( java_class_name_bstr );
                    java_class_name_ptr = j->const_java_class_name();
                }
                else
                {
                    // Vielleicht gibt's die Eigenschaft java_class_name per IDispatch (so bei einem Proxy des Objektservers)

                    java_class_name = string_from_variant( com_property_get( idispatch, "java_class_name" ) );
                    java_class_name_ptr = java_class_name.c_str();
                }

                //for( uint i = 0; i < java_class_name.length(); i++ )  if( java_class_name[i] == '.' )  java_class_name[i] = '/';

                ptr<Java_idispatch> java_idispatch = Z_NEW( Java_idispatch( idispatch, java_idispatch_container != NULL, java_class_name_ptr ) );

                if( java_idispatch_container )      // Für den Scheduler (s. spooler_module_java.cxx)
                {
                    java_idispatch_container->add_object( java_idispatch );        // Lebensdauer nur bis Ende des Aufrufs der Java-Methode, s. Java_module_instance::call()
                }
                result = java_idispatch->get_jobject();
                break;
            }

            case VT_ARRAY | VT_UI1:
            {
                Locked_safearray<Byte> safearray ( V_ARRAY( &v ) );

                int        n           = safearray.count();
                jbyteArray jbyte_array = env->NewByteArray( n );
                
                env->SetByteArrayRegion( jbyte_array, 0, n, reinterpret_cast<signed char*>( &safearray[0] ) );

                result = jbyte_array;
                break;
            }

          //case VT_VARIANT:
          //case VT_UNKNOWN:
          //case VT_DECIMAL:
          //case VT_SAFEARRAY:
          //case VT_CARRAY:
          //case VT_USERDEFINED:
          //case VT_LPSTR:
          //case VT_LPWSTR:
          //case VT_RECORD:
          //case VT_BLOB:
          //case VT_STREAM:
          //case VT_STORAGE:
          //case VT_STREAMED_OBJECT:
          //case VT_STORED_OBJECT:
          //case VT_BLOB_OBJECT:
          //case VT_CF:
          //case VT_CLSID:

            default:  
                throw_xc( "Z-JAVA-104", variant_type_name(v) );
        }
    }

    return result;
}

//---------------------------------------------------------------------Com_env::java_com_get_dispid

DISPID Com_env::java_com_get_dispid( jlong jidispatch, jstring jname, WORD* context )
{
    Env     env    = jni_env();
    DISPID  dispid = 0;

    HRESULT     hr;
    IDispatch*  idispatch = (IDispatch*)(size_t)jidispatch;
    OLECHAR     name [ 200+1 ];


    if( !idispatch )  
        throw_xc( "Z-JAVA-111" );
    if( !jname )  
        throw_xc( "Idispatch.com_call", "Name fehlt" );

    {
        int                   len         = env->GetStringLength( jname );
        Read_jstring_critical name_jchars ( jname );                  // Ab hier kein JNI-Aufruf!

        const OLECHAR* name_ptr = (const OLECHAR*)(const jchar*)name_jchars;

        //cerr << Z_FUNCTION << " jidispatch=0x" << hex << jidispatch << dec << " name=" << string_from_ole( name_ptr ) << endl;

        if( context )
        {
            *context = 0;
            if( name_ptr[0] == '<' )  *context |= DISPATCH_PROPERTYGET, name_ptr++, len--;
            else
            if( name_ptr[0] == '>' )  *context |= DISPATCH_PROPERTYPUT, name_ptr++, len--;
                                else  *context |= DISPATCH_METHOD;
        }

        if( len > NO_OF( name ) - 1 )  throw_xc( "Z-JAVA-113", string_from_jstring( jname ) );
        memcpy( name, name_ptr, len * sizeof (OLECHAR) );
        name[len] = 0;
    }

    if( name[0] == '\0' )
    {
        dispid = DISPID_VALUE;
    }
    else
    {
        OLECHAR* names[1] = { name };
        hr = idispatch->GetIDsOfNames( IID_NULL, names, 1, STANDARD_LCID, &dispid );
        if( FAILED(hr) )  throw_com( hr, "GetIDsOfNames", string_from_ole(name).c_str() );
    }

    //cerr << Z_FUNCTION << " dispid=" << dispid << endl;

    return dispid;
}


//----------------------------------------------------------------------Com_env::jobject_to_variant
com::Variant Com_env::jobject_to_variant(jobject jparam)
{
    Variant result               = empty_variant;
    Env     env                  = jni_env();
    Vm::Standard_classes* stdcls = Vm::static_vm->standard_classes();

    if( !jparam )  throw_xc( "NULL-Pointer" );

    if( env->IsInstanceOf( jparam, stdcls->_java_lang_string_class ) )
    {
        BSTR str_result;
        jstring_to_bstr( (jstring)jparam, &str_result );
        result.attach_bstr( str_result );
    }
    else
    if( env->IsInstanceOf( jparam, stdcls->_java_lang_boolean_class ) )
    {
        result = env->CallBooleanMethod( jparam, env->GetMethodID( stdcls->_java_lang_boolean_class, "booleanValue", "()Z" ) ) != 0;
        if( env->ExceptionCheck() )  env.throw_java( "CallBooleanMethod", "booleanValue" );
    }
    else
    if( env->IsInstanceOf( jparam, stdcls->_java_lang_integer_class ) )
    {
        result = env->CallIntMethod( jparam, env->GetMethodID( stdcls->_java_lang_integer_class, "intValue", "()I" ) );
        if( env->ExceptionCheck() )  env.throw_java( "CallIntMethod", "intValue" );
    }
    else
    if( env->IsInstanceOf( jparam, stdcls->_java_lang_long_class ) )
    {
        result = env->CallLongMethod( jparam, env->GetMethodID( stdcls->_java_lang_long_class, "longValue", "()J" ) );
        if( env->ExceptionCheck() )  env.throw_java( "CallLongMethod", "longValue" );
    }
    else
    if( env->IsInstanceOf( jparam, stdcls->_java_lang_double_class ) )
    {
        result = env->CallDoubleMethod( jparam, env->GetMethodID( stdcls->_java_lang_double_class, "doubleValue", "()D" ) );
        if( env->ExceptionCheck() )  env.throw_java( "CallDoubleMethod", "doubleValue" );
    }
    else
    if( local_jobject<jclass> cls = get_spooler_idispatch_class_if_is_instance_of( jparam ) )
    {
        jfieldID field_id = env->GetFieldID( cls, "_idispatch", "J" );
        if( field_id )
        {
            result = (IDispatch*)(size_t)env->GetLongField( jparam, field_id );
            if( env->ExceptionCheck() )  env.throw_java( "GetLongField" );
        }
    }
    else
    if( is_byte_array_class( jparam ) )
    {
        jbyteArray jbyte_array = static_cast<jbyteArray>( jparam );

        jsize n = env->GetArrayLength( jbyte_array );

        Locked_safearray<Byte> safearray ( n );
        env->GetByteArrayRegion( jbyte_array, 0, n, reinterpret_cast<signed char*>( &safearray[0] ) );
        if( env->ExceptionCheck() )  env.throw_java( "GetByteArrayRegion" );
        
        result.vt     = VT_ARRAY | VT_UI1;
        result.parray = safearray.take_safearray();
    }
    else
    if( is_string_array_class( jparam ) )
    {
        jobjectArray jobject_array = static_cast<jobjectArray>( jparam );

        jsize n = env->GetArrayLength( jobject_array );

        Locked_safearray<BSTR> safearray ( n );
        for( jsize j = 0; j < n; j++ )
        {
            Local_jstring jstr ( (jstring)env->GetObjectArrayElement( jobject_array, j ) );
            if( env->ExceptionCheck() )  env.throw_java( "GetObjectArrayElement" );
            jstring_to_bstr( jstr, &safearray[ (int)j ] );
        }
        
        result.vt     = VT_ARRAY | VT_BSTR;
        result.parray = safearray.take_safearray();
    }
    else
    {
        ptr<Jobject_idispatch> jobject_idispatch = Z_NEW( Jobject_idispatch( jparam ) );
        result = (IDispatch*)+jobject_idispatch;
        //Z_LOG2( "zschimmer", Z_FUNCTION << ": " << vartype_name( (*dispparams)[i].vt ) << ":" << (*dispparams)[i] << "\n" );

        //throw_xc( "Z-JAVA-111", i );
    }

    return result;
}

//--------------------------------------------------------------Com_env::jobjectarray_to_dispparams

bool Com_env::jobjectarray_to_dispparams( jobjectArray jparams, Dispparams* dispparams )
{
    Env     env         = jni_env();
    int     param_count = jparams? env->GetArrayLength( jparams ) : 0;

    dispparams->set_arg_count( param_count );

    if( Vm::static_vm  &&  param_count > 0 )
    {

        for( int i = 0; i < param_count; i++ )
        {
            jobject jparam = env->GetObjectArrayElement( jparams, i );
            if( env->ExceptionCheck() )  env.throw_java( "GetObjectArrayElement" );
            (*dispparams)[i] = jobject_to_variant( jparam );
            env->DeleteLocalRef( jparam );
        }

        if( env->ExceptionCheck() )  return false;
    }

    return true;
}

//-------------------------------------------------------------------Com_env::jobject_java_com_call

jobject Com_env::jobject_java_com_call( jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    WORD    context = 0;

    DISPID dispid = java_com_get_dispid( jidispatch, jname, &context );

    return jobject_java_com_call( cls, jidispatch, dispid, context, jparams );
}

//-------------------------------------------------------------------Com_env::jobject_java_com_call

jobject Com_env::jobject_java_com_call( jclass cls, jlong jidispatch, DISPID dispid, WORD context, jobjectArray jparams )
{
    try
    {
        return jobject_from_variant( variant_java_com_call( cls, jidispatch, dispid, context, jparams ) );
    }
    catch( const exception&  x ) { set_java_exception( x ); }
    catch( const _com_error& x ) { set_java_exception( x ); }

    return NULL;
}

//-------------------------------------------------------------------Com_env::variant_java_com_call

Variant Com_env::variant_java_com_call( jclass, jlong jidispatch, DISPID dispid, WORD context, Dispparams* dispparams )
{
    HRESULT     hr;
    IDispatch*  idispatch = (IDispatch*)(size_t)jidispatch;
    
/*
    if( Log_ptr log = "java.com_call" )
    {
        log << "com_call " << (void*)(size_t)jidispatch << '.' << string_from_jstring(jenv,jname) << '(';
        for( int i = 0; i < dispparams->arg_count(); i++ )  log << debug_string_from_variant( (*dispparams)[i] ) << ',';
        log << ")\n";
    }
*/
    // Invoke

    Variant         result;
    Excepinfo       excepinfo;
    UINT            arg_nr;

    if( context & DISPATCH_PROPERTYPUT )  dispparams->set_property_put();


    hr = idispatch->Invoke( dispid, IID_NULL, STANDARD_LCID, context, dispparams, &result, &excepinfo, &arg_nr );
    if( FAILED(hr) )
    {
        if( hr == DISP_E_EXCEPTION )  throw_com_excepinfo( hr, &excepinfo, "", NULL );  //string_from_ole(name).c_str() );
                                else  throw_com_excepinfo( hr, &excepinfo, "Invoke", ( "dispid=" + hex_from_int(dispid) ).c_str() );  //string_from_ole(name).c_str() );
    }

    return result;
}

//-------------------------------------------------------------------Com_env::variant_java_com_call

Variant Com_env::variant_java_com_call( jclass cls, jlong jidispatch, jstring jname, jobjectArray jparams )
{
    WORD context = 0;

    DISPID dispid = java_com_get_dispid( jidispatch, jname, &context );

    return variant_java_com_call( cls, jidispatch, dispid, context, jparams );
}

//-------------------------------------------------------------------Com_env::variant_java_com_call

Variant Com_env::variant_java_com_call( jclass jcls, jlong jidispatch, DISPID dispid, WORD context, jobjectArray jparams )
{
    Dispparams dispparams;

    bool ok = jobjectarray_to_dispparams( jparams, &dispparams );
    if( !ok )  return Variant::vt_missing;

    return variant_java_com_call( jcls, jidispatch, dispid, context, &dispparams );
}

//-------------------------------------------------------------------Com_env::variant_java_com_call

Variant Com_env::variant_java_com_call( jclass cls, jlong jidispatch, jstring jname, jstring par1 )
{
    WORD context = 0;

    DISPID dispid = java_com_get_dispid( jidispatch, jname, &context );

    return variant_java_com_call( cls, jidispatch, dispid, context, par1 );
}

//-------------------------------------------------------------------Com_env::variant_java_com_call

Variant Com_env::variant_java_com_call( jclass jcls, jlong jidispatch, DISPID dispid, WORD context, jstring par1 )
{
    Dispparams dispparams;

    dispparams.set_arg_count( 1 );

    dispparams[0] = (BSTR)NULL;
    jstring_to_bstr( par1, &V_BSTR( &dispparams[0] ) );

    return variant_java_com_call( jcls, jidispatch, dispid, context, &dispparams );
}

//-------------------------------------------------------------------Java_idispatch::Java_idispatch

Java_idispatch::Java_idispatch( IDispatch* idispatch, bool is_owner, const char* subclass_name ) 
: 
    _idispatch( idispatch )
    //_class_name( subclass_name )
{
    Env e = jni_env();

    jclass subclass = e.find_class( subclass_name );

    jmethodID constructor_id = e.get_method_id( subclass, "<init>", "(J)V" );

    //LOG( "new Java_idispatch(" << subclass_name << ")\n" );
    jobject jo = e->NewObject( subclass, constructor_id, (jlong)(size_t)idispatch );
    if( !jo || e->ExceptionCheck() )  e.throw_java( "NewObject", subclass_name );

    _is_owner = is_owner;

    if( is_owner )  
    {
        _global_jobject = jo;           // C++-Java_idispatch ist Eigentümer von Java-Idispatch
        _jobject        = _global_jobject;
    }
    else
    {
        _jobject = jo;
        idispatch->AddRef();            // Java-Idispatch ist Eigentümer von COM-IDispatch
    }

    e->DeleteLocalRef( subclass );
}

//------------------------------------------------------------------Java_idispatch::~Java_idispatch

Java_idispatch::~Java_idispatch()
{
    if( _is_owner  &&  _global_jobject )
    {
        Env e = jni_env();
        
        try
        {
            jmethodID method_id = e.get_method_id( _global_jobject.get_jclass(), "com_clear", "()V" );

            e->CallVoidMethod( _global_jobject, method_id );
            if( e->ExceptionCheck() )  e.throw_java( "~Java_idispatch", "CallVoidMethod com_clear()" );

            _jobject        = NULL;     // Ist Kopie von _global_jobject
            _global_jobject = NULL;
            _idispatch      = NULL;

        }
        catch( const exception& x )  { if( Vm::static_vm )  Vm::static_vm->_log.error( string("~Java_idispatch: ") + x.what() ); }
    }
}

//-------------------------------------------------------------Java_idispatch_container::add_object

void Java_idispatch_container::add_object( Java_idispatch* o )
{ 
    _java_idispatch_list.push_back( NULL ); 
    *_java_idispatch_list.rbegin() = o; 

    //o->set_global(); 
}

//-------------------------------------------------------------Jobject_idispatch::Jobject_idispatch

Jobject_idispatch::Jobject_idispatch( jobject jo )
: 
    _jobject( Env().new_global_ref(jo) ) 
{
}

//------------------------------------------------------------Jobject_idispatch::~Jobject_idispatch

Jobject_idispatch::~Jobject_idispatch()
{ 
    if( _jobject  &&  Vm::static_vm  &&  Vm::static_vm->vm() )
    {
        Env().delete_global_ref(_jobject);
        //if( JNIEnv* jenv = jni_env() )  jenv->DeleteGlobalRef( _jobject ); 
    }
}

//----------------------------------------------------------------Jobject_idispatch::QueryInterface
/*
STDMETHODIMP Jobject_idispatch::QueryInterface( const IID& iid, void** result )
{ 
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Jobject_idispatch, result );
    
    return Object::QueryInterface( iid, result ); 
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer
