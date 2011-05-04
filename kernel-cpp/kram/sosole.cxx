// $Id$

#include "precomp.h"

#define INC_AUTOMATION

#include "sosstrng.h"
#include <windows.h>
#include <ole2.h>

#pragma comment( lib, "ole32" )
#pragma comment( lib, "oleaut32" )

#include "sos.h"
#include "dynobj.h"
#include "com_simple_standards.h"
#include "olestd.h"
#include "sosole.h"

using namespace std;
namespace sos {


struct Ole_connection
{
                           ~Ole_connection          ();

    void                    init                    ();

    Bool                   _initialized;
};

static Ole_connection ole_connection;


const int lcid = LOCALE_SYSTEM_DEFAULT;

//--------------------------------------------------------------Ole_connection::~Ole_connection

Ole_connection::~Ole_connection()   
{ 
    if( _initialized ) {
        _initialized = false;
        CoUninitialize();
    }
}

//-------------------------------------------------------------------------Ole_connection::init

void Ole_connection::init() 
{
    // Fehlt: OLE-Version prüfen

    if( !_initialized ) 
    {
        CoInitialize( NULL );    // liefert keinen Fehler
        _initialized = true;
    }
}

//-----------------------------------------------------------------------Ole_method::Ole_method

Ole_method::Ole_method( Ole_object* o )
:
    _zero_  ( this+1 ),
    _object ( o )
{
    obj_const_name( "Ole_method" );
}

//----------------------------------------------------------------------Ole_method::~Ole_method

Ole_method::~Ole_method()
{
}
 
//--------------------------------------------------------------------------Ole_method::prepare
  
void Ole_method::prepare()
{
    int         ret;
    HRESULT     hr;
    OLECHAR     name [ 100+1 ];
    OLECHAR*    names [ 1 ];

    ret = MultiByteToWideChar( CP_ACP, 0, c_str( _name ), length( _name ), name, sizeof name );
    if( ret != length( _name ) ) {
        if( length( _name ) > NO_OF( name ) - 1 )  throw_xc( "Ole_method::prepare", "Methodenname zu lang", c_str( _name ) );
        throw_mswin_error( "MultiByteToWideChar" );
    }

    name[ length( _name ) ] = '\0';
    names[ 0 ] = name;

    // Besser ITypeLib::GetIDsOfNames() nutzen (wenn vorhanden), damit das entfernte Objekt 
    // nicht in Anspruch genommen werden muss.

    hr = _object->_pIDispatch->GetIDsOfNames( IID_NULL, names, 1, lcid, &_dispid );
    if( FAILED( hr ) )  throw_ole( hr, "MSWIN-GetIDsOfNames", c_str( _name ) );
}

//------------------------------------------------------------------Ole_method::throw_excepinfo

void Ole_method::throw_excepinfo( HRESULT hresult, EXCEPINFO* excepinfo )
{
    throw_ole_excepinfo( hresult, excepinfo, c_str( _name ), c_str( _object->_class_name ) );
}

//---------------------------------------------------------------------------Ole_method::invoke
                              
void Ole_method::invoke( Dyn_obj* result, const Sos_dyn_obj_array& params )
{
    HRESULT     hr;
    DISPPARAMS  dispparams;
    VARIANT     variant_array[ 100 ];                   // Anzahl der Parameter ist begrenzt!
    VARIANT     ole_result;
    EXCEPINFO   excepinfo;
    uint        error;
    int         i;

    if( params.count() > NO_OF( variant_array ) )  throw_xc( "Ole_method::invoke", "Zuviele Parameter" );


    // Parameter-Array aufbauen:
    for( i = 0; i < params.count(); i++ ) 
    {
        VARIANT* v = &variant_array[ params.count() - 1 - i ];   // Parameter werden rückwärts gespeichert

        VariantInit( v );
        V_VT( v ) = VT_BSTR;

        params[ i + params.first_index() ].write_text( &_object->_hilfspuffer );

        V_BSTR( v ) = SysAllocStringLen( (BSTR)NULL, _object->_hilfspuffer.length() );

        MultiByteToWideChar( CP_ACP, 0, _object->_hilfspuffer.char_ptr(), _object->_hilfspuffer.length(), 
                             V_BSTR( v ), _object->_hilfspuffer.length() );
    }

    dispparams.rgvarg            = variant_array;     // Array of arguments.
    dispparams.rgdispidNamedArgs = NULL;	          // Dispatch IDs of named arguments.
    dispparams.cArgs             = params.count();    // Number of arguments.
    dispparams.cNamedArgs        = 0;                 // Number of named arguments.

    VariantInit( &ole_result );

    hr = _object->_pIDispatch->Invoke( _dispid, IID_NULL, lcid, DISPATCH_METHOD, 
                                       &dispparams, &ole_result, &excepinfo, &error );

    // Parameter-Strings wieder freigeben:
    for( i = 0; i < params.count(); i++ ) {
        VariantClear( &variant_array[ i ] );
    }

    if( FAILED( hr ) ) {
        if( GetScode(hr) == DISP_E_EXCEPTION )  throw_excepinfo( hr, &excepinfo );
                                          else  throw_ole( hr, "IDispatch::Invoke", c_str( _name ) );
    }

    if( V_VT( &ole_result ) == VT_EMPTY 
     || V_VT( &ole_result ) == VT_BSTR  &&  !V_BSTR( &ole_result ) ) 
    {
        result->set_null();
    }
    else 
    {
        BSTR result_text;

        if( V_VT( &ole_result ) == VT_BSTR ) {    // ist schon Text?
            result_text = V_BSTR( &ole_result );
        } else {                                  // zu Text konvertieren
            VARIANT vt;
            VariantInit( &vt );
            hr = VariantChangeTypeEx( &vt, &ole_result, (LCID)0, 0, VT_BSTR );
            if( FAILED( hr ) )   throw_ole( hr, "VariantChangeTypeEx" );  

            result_text = V_BSTR( &vt );
            VariantClear( &vt );
        }

        int result_len = SysStringLen( result_text );
        _object->_hilfspuffer.allocate_min( result_len );

        int ret = WideCharToMultiByte( CP_ACP, 0, result_text, result_len, 
                                       _object->_hilfspuffer.char_ptr(), 
                                       _object->_hilfspuffer.size(),
                                       NULL, NULL );
        SysFreeString( result_text );
        if( ret != result_len )  throw_mswin_error( "WideCharToMultiByte" );


        result->assign( _object->_hilfspuffer.char_ptr(), result_len );
    }

    VariantClear( &ole_result );
}
 
//-----------------------------------------------------------------------Ole_object::Ole_object

Ole_object::Ole_object()
:
    _zero_ ( this+1 )
{
    obj_const_name( "Ole_object" );
    _methods.obj_const_name( "Ole_method::_methods" );
}

//-----------------------------------------------------------------------Ole_object::Ole_object

Ole_object::Ole_object( const Sos_string& class_name )
:
    _zero_ ( this+1 ),
    _class_name ( class_name )
{
    obj_const_name( "Ole_object" );
}

//----------------------------------------------------------------------Ole_object::~Ole_object 

Ole_object::~Ole_object()
{
    if( _pIDispatch ) {
        _pIDispatch->Release();
        _pIDispatch = NULL;
    }
}
 
//--------------------------------------------------------------------------Ole_object::prepare

void Ole_object::prepare()
{
    HRESULT hr;
    CLSID   clsid;

    ole_connection.init();

    if( length( _class_name ) == 0 )  throw_xc( "Ole_object::prepare", "Objektname fehlt" );

    if( _class_name[ 0 ] != '{' ) 
    {
        OLECHAR progid_text [ 100+1 ];
        ulong   progid_text_len = NO_OF( progid_text );

        if( length( _class_name ) > NO_OF( progid_text ) - 1 )  throw_xc( "Ole_object::prepare", "ProgId zu lang", c_str( _name ) );

        MultiByteToWideChar( CP_ACP, 0, c_str( _class_name ), length( _class_name ),
                             progid_text, length( _class_name ) );
        progid_text[ length( _class_name ) ] = '\0';

        hr = CLSIDFromProgID( progid_text, &clsid );
        if( FAILED( hr ) )  throw_ole( hr, "CLSIDFromProgID", c_str( _class_name ) );
/*
        Sos_string entry = _class_name;
        entry += "\\Clsid";
        ulong type = 0;
        long ret = RegQueryValueEx( HKEY_CLASSES_ROOT, c_str( entry ), NULL, &type, 
                                    (Byte*)clsid_text, &clsid_text_len );
        if( ret != ERROR_SUCCESS )  throw_mswin_error( "RegQueryValue" );
        if( type != REG_SZ )  throw_xc( "RegQueryValueEx", c_str( _name ), "type != REG_SZ" );    
*/
    } 
    else 
    {
        OLECHAR clsid_text [ 38+1 ];
        ulong   clsid_text_len = NO_OF( clsid_text );

        if( length( _class_name ) > NO_OF( clsid_text ) - 1 )  throw_xc( "Ole_object::prepare", "CLSID zu lang", c_str( _name ) );

        MultiByteToWideChar( CP_ACP, 0, c_str( _class_name ), length( _class_name ),
                             clsid_text, length( _class_name ) );
        clsid_text[ length( _class_name ) ] = '\0';

        hr = CLSIDFromString( clsid_text, &clsid );
        if( FAILED( hr ) )  throw_ole( hr, "CLSIDFromString", c_str( _class_name ) );
    }
        

    //CoCreateInstanceEx() richtet das Objekt auf einer beliebigen Maschine ein.

    hr = CoCreateInstance( clsid, NULL, CLSCTX_ALL, IID_IDispatch, (void**)&_pIDispatch );
    if( FAILED( hr ) )  throw_ole( hr, "CoCreateInstance", c_str( _class_name ) );
}
 
//---------------------------------------------------------------------------Ole_object::method

Ole_method* Ole_object::method( const char* name )
{
    Ole_method* m = NULL;
    for( int i = 0; i <= _methods.last_index(); i++ ) {
        m = _methods[ i ];
        if( stricmp( c_str( m->_name ), c_str( name ) ) == 0 )  return m;
    }

    Sos_ptr<Ole_method> method = SOS_NEW( Ole_method( this ) );
    
    method->_name = name;
    method->prepare();

    _methods.add( method );

    return method;
    //throw_xc( "SOS-1314", name, _class_name );
}

} //namespace sos
