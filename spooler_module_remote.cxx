// $Id: spooler_module_remote.cxx,v 1.3 2003/05/31 10:01:13 jz Exp $
/*
    Hier sind implementiert

    Remote_module_instance_proxy
*/



#include "spooler.h"


namespace sos {
namespace spooler {

using namespace zschimmer::com::object_server;

//-------------------------------------------------------------------------------------------------

//--------------------------------------Remote_module_instance_proxy::~Remote_module_instance_proxy

Remote_module_instance_proxy::~Remote_module_instance_proxy()
{
}

//---------------------------------------------------------------Remote_module_instance_proxy::init

void Remote_module_instance_proxy::init()
{
    HRESULT hr;

    hr = com_create_instance_in_separate_process( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, spooler_com::IID_Iremote_module_instance_server, (void**)&_remote_instance );
    if( FAILED(hr) )  throw_ole( hr, "com_create_instance_in_separate_process" );

    Variant params ( Variant::vt_array, 6 );

    {
        Locked_safearray params_array = V_ARRAY( &params );

        params_array[0] = "language="   + _module->_language;
        params_array[1] = "com_class="  + _module->_com_class_name;
        params_array[2] = "filename="   + _module->_filename;
        params_array[3] = "java_class=" + _module->_java_class_name;
        params_array[4] = "recompile="  + as_string(_module->_recompile);
        params_array[5] = "script="     + _module->_source.text();
    }

    _remote_instance->call( "construct", params );

    _idispatch = _remote_instance;
}

//---------------------------------------------------------------Remote_module_instance_proxy::load

void Remote_module_instance_proxy::load()
{
    //_remote_instance->call( "load" );
}

//--------------------------------------------------------------Remote_module_instance_proxy::close

void Remote_module_instance_proxy::close()
{
    _remote_instance = NULL;
}

//-----------------------------------------------------------Remote_module_instance_proxy::dispatch
/*
IDispatch* Remote_module_instance_proxy::dispatch() const
{ 
    return _idispatch; 
}
*/
//------------------------------------------------------------Remote_module_instance_proxy::add_obj

void Remote_module_instance_proxy::add_obj( const ptr<IDispatch>& object, const string& name )
{
    _remote_instance->call( "add_obj", +object, name );
}

//--------------------------------------------------------Remote_module_instance_proxy::name_exists

bool Remote_module_instance_proxy::name_exists( const string& name )
{
    return int_from_variant( _remote_instance->call( "name_exists", name ) ) != 0;
}

//---------------------------------------------------------------Remote_module_instance_proxy::call

Variant Remote_module_instance_proxy::call( const string& name )
{
    return _remote_instance->call( "call", "?" + name );     // "?": Methode ist optional. Wenn es sie nicht gibt, kommt VT_EMPTY zurück
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
