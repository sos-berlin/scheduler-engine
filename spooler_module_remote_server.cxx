// $Id$
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
        close__end();  // Synchron
    }
    catch( exception& ) {}
}

//--------------------------------------------------------Remote_module_instance_server::close__end

void Remote_module_instance_server::close__end()   // synchron
{
    if( _module_instance )  _module_instance->close(), _module_instance = NULL;;

    Com_module_instance_base::close__end();  // synchron
}

//---------------------------------------------------Remote_module_instance_server::load_implicitly
/*
void Remote_module_instance_server::load_implicitly()
{
    if( !_loaded_and_started )
    {
        if( _load_error )  throw_xc( "SCHEDULER-185" );

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
    { DISPATCH_METHOD     , 1, "construct"    , (Com_method_ptr)&Com_remote_module_instance_server::Construct   , VT_EMPTY      , { VT_BYREF|VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 2, "add_obj"      , (Com_method_ptr)&Com_remote_module_instance_server::Add_obj     , VT_EMPTY      , { VT_DISPATCH, VT_BSTR } },
    { DISPATCH_METHOD     , 3, "name_exists"  , (Com_method_ptr)&Com_remote_module_instance_server::Name_exists , VT_BOOL       , { VT_BSTR } },
    { DISPATCH_METHOD     , 4, "call"         , (Com_method_ptr)&Com_remote_module_instance_server::Call        , VT_VARIANT    , { VT_BSTR } },
    { DISPATCH_METHOD     , 5, "begin"        , (Com_method_ptr)&Com_remote_module_instance_server::Begin       , VT_VARIANT    , { VT_BYREF|VT_ARRAY|VT_VARIANT, VT_BYREF|VT_ARRAY|VT_VARIANT } },
    { DISPATCH_METHOD     , 6, "end"          , (Com_method_ptr)&Com_remote_module_instance_server::End         , VT_VARIANT    , { VT_BOOL } },
    { DISPATCH_METHOD     , 7, "step"         , (Com_method_ptr)&Com_remote_module_instance_server::Step        , VT_VARIANT    },
    {}
};

#endif
//-----------------------------------------------Com_remote_module_instance_server::create_instance

HRESULT Com_remote_module_instance_server::Create_instance( const IID& iid, ptr<IUnknown>* result )
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

STDMETHODIMP Com_remote_module_instance_server::Construct( SAFEARRAY* safearray )
{
    HRESULT hr = NOERROR;

    try
    {
        string           java_class_path;
        string           java_work_dir;
        string           javac;
        string           java_options;
        string           job_name;
        int              task_id = 0;
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
                if( key_word == "job"             )  job_name                          = value;
                else
                if( key_word == "task_id"         )  task_id                           = as_int( value );
                else
                    throw_xc( "server::construct" );
            }
        }

        _server._module->init();
        _server._module->set_source_only( _server._module->_source );


        _server._module->_java_vm = get_java_vm( false );

        if( !_server._module->_java_vm->running() )
        {
            // Java einstellen, falls der Job in Java geschrieben ist oder indirekt (über Javascript) Java benutzt.
            //java_vm->set_log( &_log );
            if( !java_work_dir.empty() )
            _server._module->_java_vm->set_work_dir( java_work_dir );

            if( !java_class_path.empty() )
            _server._module->_java_vm->set_class_path( java_class_path );

            _server._module->_java_vm->set_javac_filename( javac );
            _server._module->_java_vm->set_options( java_options );
        }


        if( _server._module->_kind == Module::kind_java )
        {
            if( !_server._module->_java_vm->running() )
            {
                Java_module_instance::init_java_vm( _server._module->_java_vm );
            }
            else
            {
                LOG( "Com_remote_module_instance_server::construct: Die Java Virtual Machine läuft bereits.\n" );
                // Parameter für Java können nicht übernommen werden.
            }
        }


        _server._module_instance = _server._module->create_instance();
        _server._module_instance->set_job_name( job_name );             // Nur zur Diagnose
        _server._module_instance->set_task_id( task_id );               // Nur zur Diagnose
      //_server._module_instance->init();
      //_server._module_instance->_spooler_exit_called = true;            // Der Client wird spooler_exit() explizit aufrufen, um den Fehler zu bekommen.
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::construct" ); }

    return hr;
}

//-------------------------------------------------------Com_remote_module_instance_server::add_obj

STDMETHODIMP Com_remote_module_instance_server::Add_obj( IDispatch* object, BSTR name )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server._module_instance )  throw_xc( "SCHEDULER-203", "add_obj", string_from_bstr(name) );
        _server._module_instance->add_obj( object, string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::add_obj" ); }

    return hr;
}

//---------------------------------------------------Com_remote_module_instance_server::name_exists

STDMETHODIMP Com_remote_module_instance_server::Name_exists( BSTR name, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server._module_instance )  throw_xc( "SCHEDULER-203", "name_exists", string_from_bstr(name) );
      //_server.load_implicitly();
        *result = _server._module_instance->name_exists( string_from_bstr(name) );
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::name_exists" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::call

STDMETHODIMP Com_remote_module_instance_server::Call( BSTR name, VARIANT* result )
{
    HRESULT hr = NOERROR;

  //In_call in_call ( this, name );

    try
    {
      //_server.load_implicitly();
      //_server._module_instance->call( string_from_bstr(name) ).CopyTo( result );

        if( !_server._module_instance )  
        {
            string nam = string_from_bstr( name );

            if( nam == spooler_close_name
             || nam == spooler_on_error_name
             || nam == spooler_exit_name     ) 
            {
                result->vt = VT_EMPTY;
                return hr;
            }

            throw_xc( "SCHEDULER-203", "call", string_from_bstr(name) );
        }

        _server._module_instance->call__start( string_from_bstr(name) ) -> async_finish();

        _server._module_instance->call__end().move_to( result );
        //result->vt = VT_BOOL;
        //V_BOOL( result ) = _server._module_instance->call__end();
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::call" ); }

    return hr;
}

//---------------------------------------------------------Com_remote_module_instance_server::begin

STDMETHODIMP Com_remote_module_instance_server::Begin( SAFEARRAY* objects_safearray, SAFEARRAY* names_safearray, VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server._module_instance )  throw_xc( "SCHEDULER-203", "begin" );

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
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::begin" ); }

    return hr;
}

//-----------------------------------------------------------Com_remote_module_instance_server::end

STDMETHODIMP Com_remote_module_instance_server::End( VARIANT_BOOL succeeded, VARIANT* result )
{
    HRESULT hr = NOERROR;
    
    if( result )  result->vt = VT_EMPTY;

    try
    {
        if( _server._module_instance )
        {
            _server._module_instance->end__start( succeeded != 0 ) -> async_finish();
            _server._module_instance->end__end();
        }
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::end" ); }

    return hr;
}

//----------------------------------------------------------Com_remote_module_instance_server::step

STDMETHODIMP Com_remote_module_instance_server::Step( VARIANT* result )
{
    HRESULT hr = NOERROR;

    try
    {
        if( !_server._module_instance )  throw_xc( "SCHEDULER-203", "step" );

        _server._module_instance->step__start() -> async_finish();

        _server._module_instance->step__end().move_to( result );
        //result->vt = VT_BOOL;
        //V_BOOL( result ) = _server._module_instance->step__end();
    }
    catch( const exception& x ) { hr = Com_set_error( x, "Remote_module_instance_server::step" ); }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
