// script_object.cxx                                 (C)2000 SOS GmbH Berlin
// $Id$

#include "precomp.h"

#include <wchar.h>
#include <ctype.h>

#if !defined STRICT
#   define STRICT
#endif


#include "../kram/sos.h"
#include "../kram/log.h"
#include "../file/anyfile.h"
#include "../kram/sosscrpt.h"

#include <windows.h>
#include <ole2ver.h>

#include "../kram/oleserv.h"
#include "../kram/olestd.h"
#include "hostole.h"
#define INITGUIDS
#include "hostole2.h"
#include "hostole_odl.h"
#include "variables.h"


namespace sos {

//-----------------------------------------------------------------------------Script_object_site

struct Script_object_site : Script_site
{
                                Script_object_site      ()                      : _zero_(this+1) {}
                             //~Script_object_site      ();

    Fill_zero                  _zero_;
    ptr<IDispatch>             _engine_idispatch;
};

//----------------------------------------------------------------------------------Script_object

struct Script_object : Iscript_object,
                       Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "hostWare.Script_object" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Script_object           ( IUnknown* = NULL );
                               ~Script_object           ();

    USE_SOS_OLE_OBJECT_ADDREF_RELEASE   
    USE_SOS_OLE_OBJECT_GETTYPEINFO      

    STDMETHODIMP                QueryInterface          ( REFIID, void** );
    STDMETHODIMP                GetIDsOfNames           ( REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHODIMP                Invoke                  ( DISPID, REFIID, LCID, unsigned short,
                                                          DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );

    STDMETHODIMP                Obj_close               ();
    STDMETHODIMP            put_Obj_language            ( BSTR );
    STDMETHODIMP            get_Obj_language            ( BSTR* );
    STDMETHODIMP                Obj_add_object          ( IDispatch*, BSTR, Script_item_flags );
    STDMETHODIMP                Obj_parse               ( BSTR, ::Scripttext_flags, VARIANT* );
    STDMETHODIMP                Obj_parse_only          ( BSTR, ::Scripttext_flags );
    STDMETHODIMP                Obj_parse2              ( BSTR, ::Scripttext_flags, bool start, VARIANT* );
    STDMETHODIMP                Obj_eval                ( BSTR source, ::Scripttext_flags flags, VARIANT* result ) { return Obj_parse( source, ::Scripttext_flags( flags | ::scripttext_isexpression ), result ); }
    STDMETHODIMP                Obj_name_exists         ( BSTR sub_name, VARIANT_BOOL* );
    STDMETHODIMP                Obj_add_variables       ( Ivariables2* );


    Fill_zero                  _zero_;
    ptr<Script_object_site>    _site;
    bool                       _language_set;
    bool                       _initialized;
    bool                       _started;
};

//------------------------------------------------------------------------Hostware::create_object

STDMETHODIMP Hostware::Create_object( BSTR script_text_or_class_name_bstr, BSTR language, IDispatch** script_object )
{
    HRESULT              hr = NOERROR;
    sos::Script_object*  o = NULL;

    LOGI( "Hostware::create_object()\n" );

    try {
        const wchar_t* p = script_text_or_class_name_bstr;
        while( iswalnum( p[0] )  || p[0] == L'.' )  p++;   // Klassenname: Buchstaben, Ziffern, Punkte?
        if( p[0] == 0 )
        {
            string class_name = bstr_as_string( script_text_or_class_name_bstr );
            CLSID clsid = string_as_clsid( class_name );

            hr = CoCreateInstance( clsid, NULL, CLSCTX_ALL, IID_IDispatch, (void**)script_object );
            if( FAILED( hr ) )  throw_ole( hr, "CoCreateInstance", c_str(class_name) );
        }
        else
        {
            o = new sos::Script_object;
            o->AddRef();

            hr = o->put_Obj_language( language );
            if( FAILED( hr ) )  throw_ole( hr, "obj_language" );

            Variant result;
            hr = o->Obj_parse( script_text_or_class_name_bstr, ::scripttext_isvisible, &result );
            if( !FAILED( hr ) )  *script_object = o;
        }
    }
    catch( const exception& x )   { return _set_excepinfo( x, "hostWare.Script_object::create_object" ); }

    if( FAILED( hr )  &&  o )  o->Release();
    return hr;
}

//----------------------------------------------------------------------------------Typbibliothek

DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Script_object, script_object, CLSID_Script_object, "hostWare.Script_object", "1.0" );

//--------------------------------------------------------------------Script_object::Script_object

Script_object::Script_object( IUnknown* )
:
    Sos_ole_object( &script_object_class, this, NULL ),
    _zero_(this+1)
{
    _site = new Script_object_site();
    if( !_site )  throw_no_memory_error();
    _site->init();
}

//-------------------------------------------------------------------Script_object::~Script_object

Script_object::~Script_object()
{
    if( _site ) 
    {
        _site->_engine_idispatch = NULL;
        _site->close_engine();
        _site = NULL;
    }
}

//--------------------------------------------------------------------Script_object::obj_add_close

STDMETHODIMP Script_object::Obj_close()
{
    HRESULT hr = NOERROR;

    try {
        if( _site )  _site->close_engine(), _site = NULL;
    }
    catch( const exception& x )  { hr = _set_excepinfo( x, "hostWare.Script_object::obj_close" ); }

    return hr;
}

//-----------------------------------------------------------------Script_object::put_obj_language

STDMETHODIMP Script_object::put_Obj_language( BSTR language_bstr )
{
    if( !_site )  return E_POINTER;

    try
    {
        string language = bstr_as_string( language_bstr );
        if( !language.empty() )  _language_set = true;
        _site->set_engine( language );
    }
    catch( const exception& x )  { return _set_excepinfo( x, "hostWare.Script_object::obj_language" ); }

    return NOERROR;
}

//-----------------------------------------------------------------Script_object::get_obj_language

STDMETHODIMP Script_object::get_Obj_language( BSTR* language_bstr )
{
    if( !_site )  return E_POINTER;

    *language_bstr = SysAllocString_string( _site->_engine_name );

    return NOERROR;
}

//----------------------------------------------------------------------Script_object::Obj_add_obj
// Code gesperrt, weil Objekte nicht freigegeben werden, trotz obj_close(). jz 21.2.01

STDMETHODIMP Script_object::Obj_add_object( IDispatch* object, BSTR name_bstr, Script_item_flags flags )
{
    if( !_site )  return E_POINTER;

    HRESULT hr = NOERROR;

    try 
    {
        if( !_initialized )
        {
            _site->init_engine();
            _initialized = true;
        }

        string name = bstr_as_string( name_bstr );
        if( name.empty() )  flags = Script_item_flags( flags | sif_globalmembers );

        _site->add_obj( object, name_bstr, flags );
    }
    catch( const exception& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//------------------------------------------------------------------------Script_object::obj_parse

STDMETHODIMP Script_object::Obj_parse( BSTR script_text_bstr, ::Scripttext_flags flags, VARIANT* result )
{
    return Obj_parse2( script_text_bstr, flags, true, result );
}

//--------------------------------------------------------------------Script_object::Obj_parse_only

STDMETHODIMP Script_object::Obj_parse_only( BSTR script_text_bstr, ::Scripttext_flags flags )
{
    return Obj_parse2( script_text_bstr, flags, false, NULL );
}

//------------------------------------------------------------------------Script_object::obj_parse

STDMETHODIMP Script_object::Obj_parse2( BSTR script_text_bstr, ::Scripttext_flags flags, bool start, VARIANT* result )
{
    if( !_site )  return E_POINTER;

    HRESULT hr = NOERROR;

    try
    {
        EXCEPINFO excep;  memset( &excep, 0, sizeof excep );


        if( !_initialized )
        {
            if( !_language_set ) 
            {
                _language_set = true;
                _site->set_engine( detect_script_language( bstr_as_string( script_text_bstr ) ) );
            }

            _site->init_engine();
            _initialized = true;
        }

        if( !_started  &&  start )
        {
            _started = true;
            hr = _site->_script->SetScriptState( SCRIPTSTATE_STARTED );
            if( _site->_script_exception )   throw *_site->_script_exception;
            if( FAILED( hr ) )  throw_ole( hr, "IActiveScript::SetScriptState", "SCRIPTSTATE_STARTED" );
        }


        Variant  dummy_result;
        if( !result )  result = &dummy_result;


        hr = _site->_script_parse->ParseScriptText( script_text_bstr,
                                                    NULL, 
                                                    NULL, 
                                                    NULL, 
                                                    NULL, 
                                                    NULL, 
                                                    flags, 
                                                    result,
                                                    &excep
                                                  );
        
        
        if( _site->_script_exception ) {
            SysFreeString( excep.bstrSource );
            SysFreeString( excep.bstrDescription );
            SysFreeString( excep.bstrHelpFile );
            throw *_site->_script_exception;
        }

        if( FAILED( hr ) )  throw_ole_excepinfo( hr, &excep, "IActiveScriptParse::ParseScriptText" ); 

        hr = _site->_script->GetScriptDispatch( NULL, _site->_engine_idispatch.pp() );
        if( FAILED( hr ) )  throw_ole( hr, "QueryInterface" );
    }
    catch( const exception& x )   { return _set_excepinfo( x, "hostWare.Script_object::obj_parse" ); }

    return hr;
}

//------------------------------------------------------------------Script_object::Query_interface

STDMETHODIMP Script_object::QueryInterface( REFIID iid, void** obj )
{
    if( iid == IID_Iscript_object ) 
    {
        *obj = this;
        AddRef();
        return NOERROR;
    }

    return Sos_ole_object::QueryInterface( iid, obj );                                                                             \
}

//--------------------------------------------------------------------Script_object::GetIDsOfNames

STDMETHODIMP Script_object::GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                           LCID lcid, DISPID* rgDispID )
{
    HRESULT hr = Sos_ole_object::GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispID );
    
    if( hr == (HRESULT)DISP_E_UNKNOWNNAME
     || hr == (HRESULT)DISP_E_MEMBERNOTFOUND)  // Dieser Fehlercode kommt, wenn die Methode nicht bekannt ist.
    {
        if( !_site->_engine_idispatch )  return ERROR;
        return _site->_engine_idispatch->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispID );
    }

    return hr;
}

//---------------------------------------------------------------------------Script_object::Invoke

STDMETHODIMP Script_object::Invoke( DISPID dispID, REFIID riid, LCID lcid,
                                    unsigned short wFlags, DISPPARAMS* pDispParams,
                                    VARIANT* pVarResult, EXCEPINFO* excepinfo,
                                    UINT* puArgErr )
{
    if( dispID >= SCRIPT_OBJECT_DISPID  &&  dispID < SCRIPT_OBJECT_DISPID + 100 )    // Standard-Methode von Script_object, z.B. obj_parse()
    {  
        return Sos_ole_object::Invoke( dispID, riid, lcid, wFlags, pDispParams, pVarResult, excepinfo, puArgErr );  
    }

    if( !_site )  return E_POINTER;
    if( !_site->_engine_idispatch )  return E_POINTER;
    return _site->_engine_idispatch->Invoke( dispID, riid, lcid, wFlags, pDispParams, pVarResult, excepinfo, puArgErr );
}

//-----------------------------------------------------------------------Script_object::name_exists
// Vgl. Processor::name_exists 

HRESULT Script_object::Obj_name_exists( BSTR sub_name, VARIANT_BOOL* result )
{
    HRESULT     hr;
    OLECHAR*    names [ 1 ];
    DISPID      dispid;
    OLECHAR     object_name [ 256+1 ];
    OLECHAR*    punkt;

    LOGI( "Script_object::name_exists " << sub_name << '\n' );

    punkt = wmemchr( sub_name, L'.', SysStringLen( sub_name ) );
    if( punkt ) {
        int len = min( punkt - sub_name, (int)NO_OF( object_name ) - 1 );
        wmemcpy( object_name, sub_name, len );
        object_name[ len ] = 0;
        names[0] = punkt + 1;
    }
    else 
    {
        object_name[0] = 0;
        names[0] = sub_name;
    }

    if( !_site )  return E_POINTER;
    if( !_site->_engine_idispatch )  return E_POINTER;

    hr = _site->_engine_idispatch->GetIDsOfNames( IID_NULL, names, 1, LOCALE_SYSTEM_DEFAULT, &dispid );

    if( FAILED( hr ) ) 
    {
        if( hr != DISP_E_UNKNOWNNAME )  return hr;
        hr = NOERROR;
        *result = 0;
    } 
    else 
    {
        *result = -1;
    }

    return hr;
}

//-----------------------------------------------------------------Script_object::Obj_add_variables

STDMETHODIMP Script_object::Obj_add_variables( Ivariables2* ivariables )
{
    HRESULT hr = S_OK;

    hr = Obj_add_object( ivariables, Bstr( "obj_parameters" ), Script_item_flags( sif_isvisible | sif_globalmembers ) );        // Parameter als OBJEKTE übergeben (call-by-ref).

    /*
    Hostware_variables* variables = DYNAMIC_CAST( Hostware_variables*, ivariables );

    for( Hostware_variables::Map::iterator it = variables->_map.begin(); it != variables->_map.end(); it++ )
    {
        if( it->second )
        {
#           ifdef Z_WINDOWS
                hr = Obj_add_object( static_cast<Ivariable2*>( it->second ), it->first, sif_isvisible );        // Parameter als OBJEKTE übergeben (call-by-ref).
                if( FAILED(hr) )  return hr;
#            else
                // s. factory/factory_processor.add_parameters() 
                int Obj_add_variables_IST_NICHT_IMPLEMENTIERT;
#           endif
        }
    }
    */

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos