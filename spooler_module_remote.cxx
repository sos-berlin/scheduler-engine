// $Id: spooler_module_remote.cxx,v 1.17 2003/08/25 20:41:26 jz Exp $
/*
    Hier sind implementiert

    Remote_module_instance_proxy
*/



#include "spooler.h"


namespace sos {
namespace spooler {

using namespace zschimmer::com::object_server;

//--------------------------------------Remote_module_instance_proxy::~Remote_module_instance_proxy

Remote_module_instance_proxy::~Remote_module_instance_proxy()
{
    close();
}

//---------------------------------------------------------------Remote_module_instance_proxy::init

void Remote_module_instance_proxy::init()
{
    HRESULT hr;

    Module_instance::init();

    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
    parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    hr = com_create_instance_in_separate_process( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, 
                                                  spooler_com::IID_Iremote_module_instance_server, (void**)&_remote_instance,
                                                  &_pid, parameters );
    if( FAILED(hr) )  throw_ole( hr, "com_create_instance_in_separate_process" );

    Variant params ( Variant::vt_array, 8 );

    {
        Locked_safearray params_array = V_ARRAY( &params );

        params_array[0] = "language="        + _module->_language;
        params_array[1] = "com_class="       + _module->_com_class_name;
        params_array[2] = "filename="        + _module->_filename;
        params_array[3] = "java_class="      + _module->_java_class_name;
        params_array[4] = "java_class_path=" + _module->_spooler->_java_vm->class_path();
        params_array[5] = "java_work_dir="   + _module->_spooler->_java_vm->work_dir();
        params_array[6] = "recompile="       + as_string(_module->_recompile);
        params_array[7] = "script="          + _module->_source.dom_doc().xml();
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
  //if( _session )  _session->close_current_operation();
    
    if( _remote_instance )  _remote_instance->close(), _remote_instance = NULL;
    _idispatch = NULL;

    Com_module_instance_base::close();
}

//------------------------------------------------------------Remote_module_instance_proxy::add_obj

void Remote_module_instance_proxy::add_obj( const ptr<IDispatch>& object, const string& name )
{
    //_remote_instance->call( "add_obj", +object, name );
    _object_list.push_back( Object_list_entry( object, name ) );
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

//-------------------------------------------------------Remote_module_instance_proxy::begin__start
/*
void Remote_module_instance_proxy::begin__start( const Object_list& object_list )
{
    _object_list = object_list;
}
*/
//-------------------------------------------------------Remote_module_instance_proxy::begin__start

void Remote_module_instance_proxy::begin__start()
{
    _error = NULL;

    Module_instance::init();

    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
    parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !_session )
    {
        _session  = Z_NEW( Session( start_process( parameters ) ) );
    }

    _multi_qi.allocate( 1 );
    _multi_qi.set_iid( 0, spooler_com::IID_Iremote_module_instance_server );
    
    _session->create_instance__start( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, NULL, 1, _multi_qi );
    _call_state = c_create_instance;
}

//---------------------------------------------------------Remote_module_instance_proxy::begin__end

bool Remote_module_instance_proxy::begin__end()
{
    if( _error )  throw *_error;
    if( _call_state != c_call_begin )  throw_xc( "begin__end" );

    return check_result( _remote_instance->call__end() );
}

//---------------------------------------------------------Remote_module_instance_proxy::end__start

void Remote_module_instance_proxy::end__start( bool success )
{
    _error = NULL;
    _remote_instance->call__start( "end", success );
}

//-----------------------------------------------------------Remote_module_instance_proxy::end__end

void Remote_module_instance_proxy::end__end()
{
    if( _error )  throw *_error;
    _remote_instance->call__end();
}

//--------------------------------------------------------Remote_module_instance_proxy::step__start

void Remote_module_instance_proxy::step__start()
{
    _error = NULL;
    _remote_instance->call__start( "step" );
}

//----------------------------------------------------------Remote_module_instance_proxy::step__end

bool Remote_module_instance_proxy::step__end()
{
    if( _error )  throw *_error;
    return check_result( _remote_instance->call__end() );
}

//-------------------------------------------------Remote_module_instance_proxy::operation_finished

bool Remote_module_instance_proxy::operation_finished()
{ 
    return _call_state == c_finished  ||  _error;
}

//------------------------------------------------------------Remote_module_instance_proxy::process

void Remote_module_instance_proxy::process( bool wait )
{ 
    if( _error )  return;

    try
    {
        if( wait )  _session->current_operation()->wait();
        else  
        if( !_session->current_operation()->is_finished() )  return;

        if( _session->current_operation()->has_error() )  return;

        switch( _call_state )
        {
            case c_create_instance:
            {
                HRESULT hr = _session->create_instance__end( 1, _multi_qi );
                if( FAILED(hr) )  throw_com( hr, "create_instance" );

                _remote_instance = dynamic_cast<object_server::Proxy*>( _multi_qi[0].pItf );
                _idispatch = _remote_instance;
                _multi_qi.clear();


                Variant objects ( Variant::vt_array, _object_list.size() );
                Variant names   ( Variant::vt_array, _object_list.size() );

                {
                    Locked_safearray objects_array = V_ARRAY( &objects );
                    Locked_safearray names_array   = V_ARRAY( &names   );

                    int i = 0;
                    FOR_EACH_CONST( Object_list, _object_list, o )
                    {
                        objects_array[i] = o->_object;
                        names_array[i]   = o->_name;
                        i++;
                    }
                }

                _object_list.clear();

                _remote_instance->call__start( "begin", objects, names );
                _call_state = c_call_begin;
                break;
            }

            case c_call_begin:
            {
                _call_state = c_finished;
                break;
            }

            default:
                throw_xc( "Remote_module_instance_proxy::process" );
        }
    }
    catch( const exception& x )
    {
        _error = x;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
