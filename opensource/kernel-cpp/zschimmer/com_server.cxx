// $Id: com_server.cxx 13999 2010-09-02 10:53:42Z jz $

#include "zschimmer.h"
#include <stdio.h>
#include "com.h"
#include "log.h"

#ifdef Z_UNIX

#include "com_server.h"
#include "z_com_server.h"


using namespace std;

namespace zschimmer { 
namespace com { 

#define DEBUG_PRINTF( FORMAT, ... )  //fprintf( stderr, FORMAT, ##__VA_ARGS__ )

//--------------------------------------------------------------------------Apply_com_module_params

HRESULT Apply_com_module_params( Com_module_params* params )
{
    HRESULT hr = S_FALSE;

    if( params->_version == 2 )
    {
         //if( params->_subversion == 0 )
         //    Log_ptr::set_stream_and_system_mutex( params->_log_stream, params->_log_system_mutex );
         if( params->_subversion == 1 )
         {
             Log_ptr::set_log_context( params->_log_context );
         }

         set_com_context( params->_com_context );

         hr = S_OK;
    }

    return hr;
}

//-----------------------------------------------------------------------------Com_get_class_object

HRESULT Com_get_class_object( const Com_class_descriptor* class_descriptor, const CLSID& clsid, const IID& iid, void** result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !class_descriptor )
        {
            string name = string_from_clsid( clsid );
            Z_LOG2( "com", Z_FUNCTION << " CLASS_E_CLASSNOTAVAILABLE  Klasse " << name  << " ist nicht eingebunden\n" );
            fprintf( stderr, "Klasse %s ist nicht eingebunden\n", name.c_str() );
            return CLASS_E_CLASSNOTAVAILABLE;
        }

        if( clsid != class_descriptor->clsid() )
        {
            string name = string_from_clsid( clsid );
            Z_LOG2( "com", Z_FUNCTION << " Klasse " << name << ": clsid != class_descriptor->clsid()\n" );
            return CLASS_E_CLASSNOTAVAILABLE;
        }

        if( iid != IID_IClassFactory )  return E_NOINTERFACE;

        ptr<Com_class_factory> class_factory = Z_NEW( Com_class_factory( class_descriptor ) );
        *result = static_cast<IClassFactory*>( class_factory.take() );

        return S_OK;
    }
    catch( const exception&  x ) { hr = Com_set_error( x, "com_get_class_object" ); }
    catch( const _com_error& x ) { hr = Com_set_error( x, "com_get_class_object" ); }

    return hr;
}

//-----------------------------------------------------------------------------------Get_error_info

HRESULT Get_error_info( HRESULT error_code, EXCEPINFO* excepinfo )
{
    HRESULT hr = S_OK;

    memset( excepinfo, 0, sizeof *excepinfo );

    if( FAILED(error_code) )
    {
        excepinfo->scode = error_code;

        ptr<IErrorInfo> ierrorinfo;

        hr = GetErrorInfo( 0, ierrorinfo.pp() );
        if( SUCCEEDED(hr) )
        {
            ierrorinfo->GetSource     ( &excepinfo->bstrSource      );
            ierrorinfo->GetDescription( &excepinfo->bstrDescription );
            ierrorinfo->GetHelpFile   ( &excepinfo->bstrHelpFile    );
            ierrorinfo->GetHelpContext( &excepinfo->dwHelpContext   );
        }
    }

    return hr;
}

//-----------------------------------------------------------------------------------Com_get_dispid

HRESULT Com_get_dispid( const Com_method* methods, REFIID, LPOLESTR* rgszNames, UINT cNames, LCID, DISPID* rgDispId )
{
    if( !methods )
    {
        Z_LOG2( "com", Z_FUNCTION << " !methods\n" );
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if( cNames != 1 )  return DISP_E_UNKNOWNNAME;

    string name = string_from_ole( rgszNames[ 0 ] );

    HRESULT hr = DISP_E_UNKNOWNNAME;
    *rgDispId = DISPID_UNKNOWN;

    for( const Com_method* m = methods; m->_name; m++ )
    {
        if( stricmp( name.c_str(), m->_name ) == 0 )  { *rgDispId = m->_dispid;  hr = S_OK;  break; }
    }

    Z_LOG2( "com.invoke", "Com_get_dispid " << name << " ==> " << *rgDispId << "\n" );

    return hr;
}

//--------------------------------------------------------------------------------------------align

inline void** align( void** p, int alignment, int offset = 0 )
{
    size_t mask = alignment - 1;
    size_t result = ( ( (size_t)p + mask) & ~mask ) + offset;
    if( result - (size_t)p >= alignment )  result -= alignment;
    return (void**)result;
}

//---------------------------------------------------------------------------------------Com_invoke

HRESULT Com_invoke( IDispatch* idispatch, const Com_method* methods, DISPID dispid, 
                    REFIID iid, LCID lcid, WORD flags, DISPPARAMS* dispparams, uint* argnr,
                    VARIANT* result, EXCEPINFO* excepinfo )
{
    if( !methods )  return CLASS_E_CLASSNOTAVAILABLE;

    HRESULT             hr    = S_OK;
    Variant             vargs [ max_com_method_params ];

    // Muss auf double ausgerichtet sein!
    void*               args  [ max_com_method_params * sizeof(VARIANT)/sizeof(void*) + 1 ];    // Einer mehr für die Rückgabe
    void**              a     = args;

    // Muss auf double ausgerichtet sein!
    void*               ref_args[ max_com_method_params * sizeof(VARIANT)/sizeof(void*) ];
    void**              r     = ref_args;

    const Com_method*   m     = NULL;
    Variant             dummy_result;
    const char*         method_name = "";

    if( !result )  result = &dummy_result;


    if( excepinfo )  memset( excepinfo, 0, sizeof *excepinfo );
    memset( result       , 0, sizeof *result      );
    memset( args         , 0, sizeof args         );   // Vorsichtshalber, falls zu wenige Parameter in _methods deklariert sind.


    // METHODE SUCHEN mit passenden flags (DISPATCH_METHOD, DISPATCH_PROPERTYGET oder DISPATCH_PROPERTYPUT)
    
    for( m = methods; m->_method; m++ )
    {
        //Z_LOG2( "zschimmer", "Com_invoke dispid=" << m->_dispid << " flags=" << m->_flags << " name=" << m->_name << "\n" );
        if( m->_dispid == dispid )
        {
            method_name = m->_name;
            if( m->_flags & flags )  break;
        }
    }
    

    // PROTOKOLLIEREN UND PRÜFEN


    if( !m->_method )
    {
        string text;
        if( m->_flags )  hr = E_NOTIMPL,              text = "E_NOTIMPL";
                   else  hr = DISP_E_MEMBERNOTFOUND,  text = "DISP_E_MEMBERNOTFOUND";
        Z_LOG2( "com.invoke", "Com_invoke " << method_name << " dispid=" << dispid << " flags=" << flags << " ==> " << text << "\n" );
        return hr;
    }


    if( Log_ptr log = "com.invoke" )
    {
        log << "Com_invoke " << vartype_name( m->_result_type ) 
            << " (" << name_of_type( *idispatch ) << ")" 
            << (void*)idispatch << "->" 
            << method_name << '(';
        for( int i = 0; i < dispparams->cArgs; i++ )
        {
            if( i > 0 )  log << ',';
            log << vartype_name(m->_types[i]) << ':' << debug_string_from_variant( dispparams->rgvarg[dispparams->cArgs-1-i] ).substr(0,50);
        }
        log << ")\n";
    }
    DEBUG_PRINTF( "Com_invoke %s %s(", vartype_name( m->_result_type ).c_str(), m->_name );



    if( dispparams->cNamedArgs != 0 )
    {
        if( flags & DISPATCH_PROPERTYPUT  
         && dispparams->cNamedArgs == 1  
         && dispparams->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT ) ;  //ok
        else 
        {
            //jz 13.10.04  Sollten hier nicht auch mehrere Parameter möglich sein: obj.prop(47) = 23?
            Z_LOG2( "com.invoke", "Com_invoke: DISPATCH_PROPERTYPUT, aber nicht cNamedArgs == 1 oder rgdispidNamedArgs[0] == DISPID_PROPERTYPUT\n" );
            return DISP_E_BADPARAMCOUNT;
        }
    }


    // KONVERTIEREN


    int     i;

    for( i = 0; i < dispparams->cArgs; i++ )
    {
        VARTYPE  t = m->_types[i];
        VARIANT* p = &dispparams->rgvarg[ dispparams->cArgs - 1 - i ];

        DEBUG_PRINTF( "Com_invoke %d. soll=%s  ist=%s\n", (i+1), vartype_name(t).c_str(), vartype_name(p->vt).c_str() );

        if( t == (VT_BYREF|VT_VARIANT) )  
        {
            *a++ = p;
        }
        else
        if( ( t & ~VT_TYPEMASK ) == VT_ARRAY )
        {
            if( ( p->vt & ~VT_TYPEMASK ) != VT_ARRAY )
            {
                Z_LOG2( "com.invoke", "t=" << vartype_name(t) << ", aber p->vt=" << vartype_name( p->vt ) << "\n" );
                return DISP_E_TYPEMISMATCH;    
            }

            if( !V_ARRAY(p) )  return E_POINTER;

            VARTYPE safearray_vartype = vartype_from_safearray( V_ARRAY(p) );

            if( safearray_vartype != ( p->vt & ~VT_ARRAY ) )  // Hier sind wir erstmal pingelig, obwohl p->vt vielleicht egal sein kann, wichtig ist safearray_vartype
            {
                Z_LOG2( "com.invoke", "Pingelige Prüfung: safearray_vartype=" << vartype_name( safearray_vartype ) << " != p->vt=" << vartype_name( p->vt ) << "\n" );
                return DISP_E_TYPEMISMATCH;    
            }

            if( safearray_vartype != ( t & ~VT_ARRAY ) ) 
            {
                Z_LOG2( "com.invoke", "safearray_vartype=" << vartype_name( safearray_vartype ) << " != t=" << vartype_name( t ) << "\n" );
                return DISP_E_TYPEMISMATCH;       // VT_ARRAY und VT_TYPEMASK müssen gleich sein
            }

            *a++ = V_ARRAY(p);
        }
        else
        if( t == VT_EMPTY )
        {
            //Z_LOG2( "com.invoke", "Com_invoke: " << i << ". Parameter ist VT_EMPTY\n" );  //? jz 13.10.04
            return DISP_E_BADPARAMCOUNT;   // Zu viele Parameter angegeben
        }
        else
        {   
            if( ( t & ~VT_BYREF ) != p->vt )    // Typ ändern?
            {
                hr = VariantChangeType( &vargs[i], p, lcid, t & ~VT_BYREF );
                if( FAILED(hr) ) { 
                    fprintf(stderr,"VariantChangeType(VT=%s to VT=%s) hr=%x\n", vartype_name(p->vt).c_str(), vartype_name(t).c_str(), (uint)hr); 
                    if( argnr ) *argnr = dispparams->cArgs - 1 - i; 
                    return hr; 
                }

                p = &vargs[i];
            }

            switch( t )
            {
              //case VT_EMPTY:      return DISP_E_BADPARAMCOUNT;        // Zuviele Parameter angegeben
              //case VT_NULL:
                case VT_I2:         *a++ = (void*)(int)V_I2      ( p );      break;
                case VT_I4:         *a++ = (void*)     V_I4      ( p );      break;

                case VT_BYREF|VT_R4: *a++ = r;
                                     memcpy( r, &V_R4( p ), 4 );  r += 4;       break;

                case VT_BYREF|VT_R8: *a++ = r;
                                     memcpy( r, &V_R8( p ), 8 );  r += 8;       break;
                                      
#             if defined Z_HPUX_IA64
                case VT_R8:         *a++ = ((void**)&V_R8(p))[0];   // Das funktioniert anscheinend nicht. Also kein double verwenden!
                                    *a++ = ((void**)&V_R8(p))[1];
                                    break;
#             elif defined Z_HPUX_PARISC
                case VT_R8:         a = align( a, 8, 4 );
                                    *a++ = ((void**)&V_R8(p))[1];
                                    *a++ = ((void**)&V_R8(p))[0];
                                    break;
#              else
                case VT_R4:         memcpy( a, &V_R4( p ), 4 );  a += 1;     break;
                case VT_R8:         memcpy( a, &V_R8( p ), 8 );  a += 2;     break;
#             endif

              //case VT_CY:
              //case VT_DATE:
                case VT_BSTR:       *a++ = (void*)     V_BSTR    ( p );      break;
                case VT_DISPATCH:   *a++ = (void*)     V_DISPATCH( p );      break;
              //case VT_ERROR:
                case VT_BOOL:       *a++ = (void*)(int)V_BOOL    ( p );      break;
              //case VT_VARIANT|VT_BYREF: *a++ = p;                          break;
                case VT_VARIANT:    for( int j = 0; j < sizeof(VARIANT) / sizeof (void*); j++ )  *a++ = ((void**)p)[j];   break;
                case VT_UNKNOWN:    *a++ = (void*)     V_UNKNOWN ( p );      break;
              //case VT_DECIMAL:    *a++ = (void*)p->intVal;            break;
                case VT_I1:         *a++ = (void*)(int)V_I1      ( p );      break;
                case VT_UI1:        *a++ = (void*)(int)V_UI1     ( p );      break;
                case VT_UI2:        *a++ = (void*)(int)V_UI2     ( p );      break;
                case VT_UI4:        *a++ = (void*)     V_UI4     ( p );      break;
              //case VT_I8:         a = align( a, 8 );  
              //                    *a++ = (void*)     V_I8      ( p );      break;
              //case VT_UI8:        a = align( a, 8 );  
              //                    *a++ = (void*)     V_UI8     ( p );      break;
                case VT_INT:        *a++ = (void*)     V_INT     ( p );      break;
                case VT_UINT:       *a++ = (void*)     V_UINT    ( p );      break;
              //case VT_VOID:
              //case VT_HRESULT:
              //case VT_PTR:
              //case VT_SAFEARRAY:
              //case VT_CARRAY:
              //case VT_USERDEFINED:
              //case VT_LPSTR:
              //case VT_LPWSTR:
              //case VT_FILETIME:
              //case VT_BLOB:
              //case VT_STREAM:
              //case VT_STORAGE:
              //case VT_STREAMED_OBJECT:
              //case VT_STORED_OBJECT:
              //case VT_BLOB_OBJECT:
              //case VT_CF:
              //case VT_CLSID:

                default:
                    fprintf(stderr,"Com_invoke() Ungültiger Parametertyp %s für %d. Parameter der Methode %s\n", vartype_name(t).c_str(), i, (m->_name?m->_name:"") );
                    if( argnr ) *argnr = dispparams->cArgs - 1 - i; 
                    return DISP_E_TYPEMISMATCH;
            }
        }

        DEBUG_PRINTF( "%s:%s,", vartype_name(t).c_str(), debug_string_from_variant( *p ).substr(0,50).c_str() );
    }

    DEBUG_PRINTF( ")" );


    for(; i < max_com_method_params; i++ )      // Mit Default-Parametern auffüllen
    {
        VARTYPE t = m->_types[i];

        if( t == VT_EMPTY )  break;

        if( t == (VT_VARIANT|VT_BYREF) )  *a++ = (void*)&missing_variant;
        else
        if( t & VT_BYREF  )               *a++ = (void*)NULL;
        else
        switch( t )             // Standard-Defaultwerte (nicht änderbar):
        {
            case VT_I4:         *a++ = (void*)(int32)   0;      break;
            case VT_INT:        *a++ = (void*)(int)     0;      break;
            case VT_UI4:        *a++ = (void*)(uint32)  0;      break;
            case VT_UINT:       *a++ = (void*)(int)     0;      break;
            case VT_BSTR:       *a++ = (void*)          NULL;   break;
            case VT_SAFEARRAY:  *a++ = (void*)          NULL;   break;
            default:
            {
                if( ( t &~ VT_TYPEMASK ) == VT_ARRAY )  *a++ = (void*)NULL;   break;
                return DISP_E_BADPARAMCOUNT;  // Zu wenige Parameter     return DISP_E_BADVARTYPE;
            }
        }
    }


    // i == Anzahl der von der Methode verlangten Parameter (ohne Ergebnis)

    if( dispparams->cArgs < i - m->_default_arg_count ) 
    {
        Z_LOG2( "com.invoke", "Com_invoke: " << dispparams->cArgs << " Parameter übergeben, aber " << ( i - m->_default_arg_count ) << " erwartet\n" );
        return DISP_E_BADPARAMCOUNT;  // Zu wenige Parameter
    }



    if( m->_result_type != VT_VARIANT )  result->vt = m->_result_type;

    if( m->_result_type != VT_EMPTY )  
    {
        //*a++ = &return_value;
        *a++ = m->_result_type == VT_VARIANT? (void*)result 
             : m->_result_type == VT_DECIMAL? (void*)&result->decVal 
                                            : (void*)&result->intVal; 
    }


    typedef HRESULT (WINAPI IDispatch::* F)(void*,void*,void*,void*,void*,void*,void*,void*,void*,void*,void*);
    F f = (F)m->_method;


    // AUFRUF DER METHODE

    hr = (idispatch->*f)( args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);

    /* SAFEARRAY Vartype prüfen, noch nicht fertig. Auf Null-Rückgabe und Null-Result-Parameter achten
    if( m->_result_type & VT_ARRAY )
    {
        VARTYPE vt = 0;
        HRESULT hr = SafeArrayGetVartype( _safearray, &vt );
        if( FAILED( hr ) )  return hr;

        if( vt != ( m->_result_type & VT_TYPEMASK ) )  
        {
            Z_LOG( "com.invoke", "m->_result_type=" << vartype_name( result->vt ) << " != " << vartype_name( vt ) << "\n" );
            hr = DISP_E_BADVARTYPE;
        }
    }
    */

    Z_LOG2( "com.invoke", "Com_invoke ... hr=" << (void*)hr << " => " << debug_string_from_variant( *result ).substr(0,50) << '\n' );
    DEBUG_PRINTF( " hr=%X ", (uint)hr );
    DEBUG_PRINTF( " => %s", debug_string_from_variant( *result ).c_str() );

    if( FAILED(hr) && excepinfo )
    {
        ptr<IErrorInfo> error_info;

        HRESULT hr2 = GetErrorInfo( 0, error_info.pp() );
        if( hr2 == S_OK )
        {
            error_info->GetDescription( &excepinfo->bstrDescription );
            error_info->GetSource     ( &excepinfo->bstrSource );
            error_info->GetHelpFile   ( &excepinfo->bstrHelpFile );
            error_info->GetHelpContext( &excepinfo->dwHelpContext );
          //excepinfo->Scode = ?;

            Z_LOG2( "com.invoke", "   ERROR=" << excepinfo->bstrDescription << '\n' );
            DEBUG_PRINTF( " %s", string_from_bstr( excepinfo->bstrDescription ).c_str() );
        }
    }

    DEBUG_PRINTF( "\n" );

    return hr;
}

//--------------------------------------------------------------------------------Com_get_type_info

HRESULT Com_get_type_info_count( IDispatch* idispatch, const Com_method* methods, unsigned int* result )
{
    return E_NOTIMPL;
}

//--------------------------------------------------------------------------------com_get_type_info

HRESULT Com_get_type_info( IDispatch* idispatch, const Com_method* methods, unsigned int info_number, LCID lcid, ITypeInfo** result )
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer

#endif
