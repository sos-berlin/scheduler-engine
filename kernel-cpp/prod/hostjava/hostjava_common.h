// $Id: hostjava.h 4008 2005-11-07 14:29:54Z jz $

#ifndef __HOSTJAVA_COMMON_H
#define __HOSTJAVA_COMMON_H


#include "../zschimmer/com.h"
#include "../hostole/hostole.h"
#include "../zschimmer/z_com.h"
#include "../zschimmer/java.h"
#include "../zschimmer/java_com.h"
#include "hostjava.h"

#ifdef _DEBUG
#   include "Debug/sos/hostware/Scripttext_flags.h"
#   include "Debug/sos/hostware/Factory_processor.h"
#   include "Debug/sos/hostware/File.h"
# else
#   include "Release/sos/hostware/Scripttext_flags.h"
#   include "Release/sos/hostware/Factory_processor.h"
#   include "Release/sos/hostware/File.h"
#endif


/*
#ifdef _DEBUG
#   include <fstream>
    extern std::ofstream hostjava_log;
#   define LOG(X)  hostjava_log << X
# else
#   define LOG(X)
#endif
*/
//-------------------------------------------------------------------------------------------------

namespace sos {
namespace hostjava {

//------------------------------------------------------------------------------------PUT_PROP_BSTR

#define PUT_PROP_BSTR( INTERFACE, PROPERTY )                                                    \
                                                                                                \
    Com_env env = jenv;                                                                         \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
        Bstr PROPERTY##_bstr;                                                                   \
        env.jstring_to_bstr( PROPERTY##_jstr, &PROPERTY##_bstr );                               \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->put_##PROPERTY( PROPERTY##_bstr );                                      \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY, PROPERTY##_bstr );           \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { env.set_java_exception( x ); }                             

//------------------------------------------------------------------------------------GET_PROP_BSTR

#define GET_PROP_BSTR( INTERFACE, PROPERTY )                                                    \
                                                                                                \
    Com_env env = jenv;                                                                         \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
        Bstr PROPERTY##_bstr;                                                                   \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->get_##PROPERTY( &PROPERTY##_bstr );                                     \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY );                            \
                                                                                                \
        return env.jstring_from_bstr( PROPERTY##_bstr );                                        \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { env.set_java_exception( x ); }                               \
                                                                                                \
    return NULL;

//-------------------------------------------------------------------------------------PUT_PROP_INT

#define PUT_PROP_INT( INTERFACE, PROPERTY )                                                     \
                                                                                                \
    Com_env env = jenv;                                                                         \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->put_##PROPERTY( PROPERTY );                                             \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY );                            \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }                             

//-------------------------------------------------------------------------------------GET_PROP_INT

#define GET_PROP_INT( INTERFACE, PROPERTY )                                                     \
                                                                                                \
    Com_env env = jenv;                                                                         \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
        int        PROPERTY = 0;                                                                \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->get_##PROPERTY( &PROPERTY );                                            \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY );                            \
                                                                                                \
        return PROPERTY;                                                                        \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { env.set_java_exception( x ); }                               \
                                                                                                \
    return 0;

//------------------------------------------------------------------------------------PUT_PROP_BOOL

#define PUT_PROP_BOOL( INTERFACE, PROPERTY )                                                    \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->put_##PROPERTY( PROPERTY##_jbool );                                     \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY );                            \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }                             

//------------------------------------------------------------------------------------GET_PROP_BOOL

#define GET_PROP_BOOL( INTERFACE, PROPERTY )                                                    \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
        VARIANT_BOOL PROPERTY = false;                                                          \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->get_##PROPERTY( &PROPERTY );                                            \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #PROPERTY );                            \
                                                                                                \
        return PROPERTY;                                                                        \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { env.set_java_exception( x ); }                               \
                                                                                                \
    return 0;

//--------------------------------------------------------------------------------------CALL_METHOD

#define CALL_METHOD( INTERFACE, METHOD )                                                        \
                                                                                                \
    try                                                                                         \
    {                                                                                           \
        INTERFACE* iface = (INTERFACE*)sos::hostjava::get_my_idispatch( jenv, jo );             \
                                                                                                \
        HRESULT hr = NOERROR;                                                                   \
                                                                                                \
        {                                                                                       \
            hr = iface->METHOD();                                                               \
        }                                                                                       \
                                                                                                \
        if( FAILED(hr) )  throw_com( hr, #INTERFACE "." #METHOD );                              \
    }                                                                                           \
    catch( const exception&  x ) { env.set_java_exception( x ); }                               \
    catch( const _com_error& x ) { set_java_exception( jenv, x ); }                             

//-------------------------------------------------------------------------------------------------
/*
#ifdef _WIN32
    typedef DWORD                           Thread_id;
    typedef list< IUnknown* >               Object_list;
    typedef map< Thread_id, Object_list >   Thread_map;
#endif
*/

//-------------------------------------------------------------------------------------------------

//void                            init_com                    ();
void*                           get_my_idispatch            ( JNIEnv*, jobject, jfieldID* = NULL );

//-------------------------------------------------------------------------------------------------

} //namespace hostjava
} //namespace sos

#endif
