// $Id: spooler_module_remote_server.cxx,v 1.28 2003/09/27 18:54:11 jz Exp $
/*
    Hier sind implementiert

    Remote_module_instance_server
*/



#include "spooler.h"
#include "spooler_module_remote_server.h"
#include "../kram/sos_java.h"


namespace sos {
namespace spooler {

using namespace zschimmer::com::object_server;
using namespace spooler_com;

extern Typelib_descr spooler_typelib;

DESCRIBE_CLASS( &spooler_typelib, Remote_module_instance_server, remote_module_instance_server, CLSID_Remote_module_instance_server, "Spooler.Remote_module_instance_server", "1.0" )

//-------------------------------------Remote_module_instance_server::Remote_module_instance_server

Remote_module_instance_server::Remote_module_instance_server()
:
    Com_module_instance_base( Z_NEW( Module( NULL, NULL ) ) ),
    _zero_(_end_)
{
}

//------------------------------------Remote_module_instance_server::~Remote_module_instance_server

Remote_module_instance_server::~Remote_module_instance_server()
{
    try
    {
        close();
    }
    catch( exception& ) {}
}

//-------------------------------------------------------------Remote_module_instance_server::close

void Remote_module_instance_server::close()
{
    if( _module_instance )  _module_instance->close(), _module_instance = NULL;;

    Com_module_instance_base::close();
}

//---------------------------------------------------Remote_module_instance_server::load_implicitly
/*
void Remote_module_instance_server::load_implicitly()
{
    if( !_loaded_and_started )
    {
        if( _load_error )  throw_xc( "SPOOLER-185" );

        _load_error = true;
        
        _module_instance->load();
        _module_instance->start();

        _load_error = false;
        _loaded_and_started = true;
    }
}
*/
//------------------------------------------------------Com_remote_module_instance_server::_methods
#ifdef Z_COM

const Com_method Com_remote_module_instance_server::_methods[] =
{ 
   // _flags              , _name             , _method                                                         , _result_type  , _types        , _default_arg_count
    { DISPATCH_METHOD     , 1, "construct"    , (Com_method_ptr)&Com_remote_module_instance_server::construct   , VT_EMPTY      , { VT_BYREF|VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 2, "add_obj"      , (Com_method_ptr)&Com_remote_module_instance_server::add_obj     , VT_EMPTY      , { VT_DISPATCH, VT_BSTR } },
    { DISPATCH_METHOD     , 3, "name_exists"  , (Com_method_ptr)&Com_remote_module_instance_server::name_exists , VT_BOOL       , { VT_BSTR } },
    { DISPATCH_METHOD     , 4, "call"         , (Com_method_ptr)&Com_remote_module_instance_server::call        , VT_VARIANT    , { VT_BSTR } },
    { DISPATCH_METHOD     , 5, "begin"        , (Com_method_ptr)&Com_remote_module_instance_server::begin       , VT_VARIANT    , { VT_BYREF|VT_ARRAY|VT_VARIANT, VT_BYREF|VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 6, "end"          , (Com_method_ptr)&Com_remote_module_instance_server::end         , VT_VARIANT    , { VT_BOOL } },
    { DISPATCH_METHOD     , 7, "step"         , (Com_method_ptr)&Com_remote_module_instance_server::step        , VT_VARIANT    },
    {}
};

#endif
//-----------------------------------------------Com_remote_module_instance_server::create_instance

HRESULT Com_remote_module_instance_server::create_instance( const IID& iid, ptr<IUnknown>* result )
{
    if( iid == IID_Iremote_module_instance_server )
    {
        ptr<Iremote_module_instance_server> instance = new Com_remote_module_instance_server;
        *result = +instance;
        return S_OK;
    }

    return E_NOINTERFACE;
}

//-----------------------------Com_remote_module_instance_server::Com_remote_module_instance_server

Com_remote_module_instance_server::Com_remote_module_instance_server()
:
    Sos_ole_object( remote_module_instance_server_class_ptr, (Iremote_module_instance_server*)this )
{
}

//----------------------------Com_remote_module_instance_server::~Com_remote_module_instance_server

Com_remote_module_instance_server::~Com_remote_module_instance_server()
{
}

//------------------------------------------------Com_remote_module_instance_server::QueryInterface

STDMETHODIMP Com_remote_module_instance_server::QueryInterface( const IID& iid, void** result )
{
    if( iid == IID_IUnknown )
    {
        *result = (IUnknown*)(Iremote_module_instance_server*)this;
        ((IUnknown*)(Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    if( iid == IID_IDispatch )
    {
        *result = (IDispatch*)(Iremote_module_instance_server*)this;
        ((IDispatch*)(Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    else
    if( iid == IID_Iremote_module_instance_server )
    {
        *result = (Iremote_module_instance_server*)this;
        ((Iremote_module_instance_server*)this)->AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//-----------------------------------------------------Com_remote_module_instance_server::construct

STDMETHODIMP Com_remote_module_instance_server::construct( SAFEARRAY* safearray )
{
    HRESULT hr = NOERROR;

    try
    {
        string           java_class_path;
        string           java_work_dir;
        string           javac;
        string           java_options;
        Locked_safearray params ( safearray );

        for( int i = 0; i < params.count(); i++ )
        {
            if( params[i].vt != VT_EMPTY )
            {
                if( params[i].vt != VT_BSTR )  throw_xc( "_spooler_construct" );

                const OLECHAR* ole_value = wcschr( V_BSTR( &params[i] ), '=' );
                if( !ole_value )  throw_xc( "_spooler_construct" );
                string key_word = string_from_ole( V_BSTR( &params[i] ), ole_value - V_BSTR( &params[i] ) );
                ole_value++;
                string value = string_from_ole( ole_value );

                if( key_word == "language"        )  _server._module->_language        = value;
                else                                                                         
                if( key_word == "com_class"       )  _server._module->_com_class_name  = value;
                else                                                                         
                if( key_word == "filename"        )  _server._module->_filename        = value;
                else
                if( key_word == "java_class"      )  _server._module->_java_class_name = value;
                else
                if( key_word == "recompile"       )  _server._module->_recompile       = value[0] == '1';
                else
                if( key_word == "script"          )  _server._module->_source          = xml::Document_ptr( value ).documentElement();
                else
                if( key_word == "java_class_path" )  java_class_path                   = value;
                else
                if( key_word == "java_work_dir"   )  java_work_dir                     = value;
                else
                if( key_word == "javac"           )  javac                             = value;
                else
                if( key_word == "java_options"    )  java_options                      = value;
                else
                    throw_xc( "server::construct" );
            }
        }

        _server._module->init();
        _server._module->set_source_only( _server._module->_source );


        if( _server._module->_kind == Module::kind_java )
        {
            _server._module->_java_vm = get_java_vm( false );

            if( !_server._module->_java_vm->running() )
            {
                //java_vm->set_log( &_log );
                if( !java_work_dir.empty() )
                _server._module->_java_vm->set_work_dir( java_work_dir );

                if( !java_class_path.empty() )
                _server._module->_java_vm->set_class_path( java_class_path );

                _server._module->_java_vm->set_javac_filename( javac );
                _server._module->_java_vm->set_options( java_options );
                Java_module_instance::init_java_vm( _server._module->_java_vm );
            }
            else
            {
                LOG( "Com_remote_module_instance_server::construct: Die Java Virtual Machine läuft bereits.\n" );
                // Parameter für Java können nicht übernommen werden.
            }
        }


        _server._module_instance = _server._module->create_instance();
      //_server._module_instance->init();
      //_server._module_instance->_spooler_exit_called = true;            // Der Client wird spooler_exit() explizit aufrufen, um den Fehler zu bekommen.
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::construct" ); }

LOG( "Com_remote_module_instance_server::construct OK\n" );
    return hr;
}

//-------------------------------------------------------Com_remote_module_instance_server::add_obj

STDMETHODIMP Com_remote_module_instance_server::add_obj( IDispatch* object, BSTR name )
{
    HRESULT hr = NOERROR;

    try
    {
        _server._module_instance->add_obj( object, string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::add_obj" ); }

    return hr;
}

//---------------------------------------------------Com_remote_module_instance_server::name_exists

STDMETHODIMP Com_remote_module_instance_server::name_exists( BSTR name, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    try
    {
      //_server.load_implicitly();
        *result = _server._module_instance->name_exists( string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::name_exists" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::call

STDMETHODIMP Com_remote_module_instance_server::call( BSTR name, VARIANT* result )
{
    HRESULT hr = NOERROR;

  //In_call in_call ( this, name );

    try
    {
      //_server.load_implicitly();
      //_server._module_instance->call( string_from_bstr(name) ).CopyTo( result );

        _server._module_instance->call__start( string_from_bstr(name) ) -> async_finish();

        result->vt = VT_BOOL;
        V_BOOL( result ) = _server._module_instance->call__end();
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::call" ); }

    return hr;
}

//---------------------------------------------------------Com_remote_module_instance_server::begin

STDMETHODIMP Com_remote_module_instance_server::begin( SAFEARRAY* objects_safearray, SAFEARRAY* names_safearray, VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        Locked_safearray objects ( objects_safearray );
        Locked_safearray names   ( names_safearray );

        for( int i = 0; i < objects.count(); i++ )  
        {
            VARIANT* o = &objects[i];
            if( o->vt != VT_DISPATCH )  return DISP_E_BADVARTYPE;
            _server._module_instance->_object_list.push_back( Module_instance::Object_list_entry( V_DISPATCH(o), string_from_variant( names[i]) ) );
        }

        _server._module_instance->begin__start() -> async_finish();

        result->vt = VT_BOOL;
        V_BOOL( result ) = _server._module_instance->begin__end();
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::begin" ); }

    return hr;
}

//-----------------------------------------------------------Com_remote_module_instance_server::end

STDMETHODIMP Com_remote_module_instance_server::end( VARIANT_BOOL succeeded, VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        _server._module_instance->end__start( succeeded != 0 ) -> async_finish();
        _server._module_instance->end__end();
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::end" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::step

STDMETHODIMP Com_remote_module_instance_server::step( VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        _server._module_instance->step__start() -> async_finish();

        result->vt = VT_BOOL;
        V_BOOL( result ) = _server._module_instance->step__end();
    }
    catch( const exception& x ) { hr = com_set_error( x, "Remote_module_instance_server::step" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
