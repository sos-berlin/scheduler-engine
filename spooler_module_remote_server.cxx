// $Id: spooler_module_remote_server.cxx,v 1.3 2003/05/31 10:01:13 jz Exp $
/*
    Hier sind implementiert

    Remote_module_instance_server
*/



#include "spooler.h"
#include "spooler_module_remote_server.h"

namespace sos {
namespace spooler {

using namespace zschimmer::com::object_server;
using namespace spooler_com;

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Remote_module_instance_server, remote_module_instance_server, CLSID_Remote_module_instance_server, "Spooler.Remote_module_instance_server", "1.0" )

//---------------------------------------------------Remote_module_instance_server::create_instance

HRESULT Remote_module_instance_server::create_instance( const IID& iid, ptr<IUnknown>* result )
{
    if( iid == IID_Iremote_module_instance_server )
    {
        ptr<Iremote_module_instance_server> instance = new Remote_module_instance_server;
        *result = (spooler_com::Iremote_module_instance_server*)instance;
        return S_OK;
    }

    return E_NOINTERFACE;
}

//-------------------------------------Remote_module_instance_server::Remote_module_instance_server

Remote_module_instance_server::Remote_module_instance_server()
:
    Com_module_instance_base( Z_NEW( Module( NULL, NULL ) ) ),
    Sos_ole_object( remote_module_instance_server_class_ptr, (Iremote_module_instance_server*)this ),
    _zero_(this+1)
{
}

//------------------------------------Remote_module_instance_server::~Remote_module_instance_server

Remote_module_instance_server::~Remote_module_instance_server()
{
    try
    {
        _module_instance->close();
    }
    catch( exception& ) {}

    _module_instance = NULL;
}

//----------------------------------------------------Remote_module_instance_server::QueryInterface

STDMETHODIMP Remote_module_instance_server::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_IUnknown )
    {
        *result = (IUnknown*)(Iremote_module_instance_server*)this;
        AddRef();
        return S_OK;
    }
    if( iid == IID_IDispatch )
    {
        *result = (IDispatch*)(Iremote_module_instance_server*)this;
        AddRef();
        return S_OK;
    }
    else
    if( iid == IID_Iremote_module_instance_server )
    {
        *result = (Iremote_module_instance_server*)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return Object::QueryInterface( iid, result );
    }
}

//----------------------------------------------------------Remote_module_instance_server::_methods
#ifdef Z_COM

const Com_method Remote_module_instance_server::_methods[] =
{ 
   // _flags              , _name             , _method                                        , _result_type, _types        , _default_arg_count
/*
    { DISPATCH_PROPERTYGET, 1, "is_error"        , (Com_method_ptr)&Com_error::get_is_error       , VT_BOOL },
    { DISPATCH_PROPERTYGET, 2, "code"            , (Com_method_ptr)&Com_error::get_code           , VT_BSTR }, 
    { DISPATCH_PROPERTYGET, 3, "text"            , (Com_method_ptr)&Com_error::get_text           , VT_BSTR }, 
*/
    {}
};

#endif


//---------------------------------------------------------Remote_module_instance_server::construct

STDMETHODIMP Remote_module_instance_server::construct( SAFEARRAY* safearray )
{
    HRESULT hr = NOERROR;

    try
    {
        Locked_safearray params ( safearray );

        for( int i = 0; i < params.count(); i++ )
        {
            if( params[i].vt != VT_BSTR )  throw_xc( "_spooler_construct" );

            const OLECHAR* value = wcschr( V_BSTR( &params[i] ), '=' );
            if( !value )  throw_xc( "_spooler_construct" );
            value++;

            if( olestring_begins_with( V_BSTR( &params[i] ), "language="   ) )  _module->_language        = string_from_ole( value );
            else                                                                         
            if( olestring_begins_with( V_BSTR( &params[i] ), "com_class="  ) )  _module->_com_class_name  = string_from_ole( value );
            else                                                                         
            if( olestring_begins_with( V_BSTR( &params[i] ), "filename="   ) )  _module->_filename        = string_from_ole( value );
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "java_class=" ) )  _module->_java_class_name = string_from_ole( value );
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "recompile="  ) )  _module->_recompile       = value[0] == '1';
            else
            if( olestring_begins_with( V_BSTR( &params[i] ), "script="     ) )  _module->_source          = string_from_ole( value );
            else
                throw_xc( "server::construct" );
        }

        _module->init();
        _module->set_source_only( _module->_source );
        _module_instance = _module->create_instance();
        _module_instance->init();
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::construct" ); }

    return hr;
}

//-----------------------------------------------------------Remote_module_instance_server::add_obj

STDMETHODIMP Remote_module_instance_server::add_obj( IDispatch* object, BSTR name )
{
    HRESULT hr = NOERROR;

    try
    {
        _module_instance->add_obj( object, string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::add_obj" ); }

    return hr;
}


//---------------------------------------------------Remote_module_instance_server::load_implicitly

void Remote_module_instance_server::load_implicitly()
{
    if( !_loaded_and_started )
    {
        _module_instance->load();
        _module_instance->start();
        _loaded_and_started = true;
    }
}

//-------------------------------------------------------Remote_module_instance_server::name_exists

STDMETHODIMP Remote_module_instance_server::name_exists( BSTR name, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    try
    {
        load_implicitly();
        *result = _module_instance->name_exists( string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::name_exists" ); }

    return hr;
}

//--------------------------------------------------------------Remote_module_instance_server::call

STDMETHODIMP Remote_module_instance_server::call( BSTR name, VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        load_implicitly();
        _module_instance->call( string_from_bstr(name) ).CopyTo( result );
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::call" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
