// $Id: spooler_module_remote.cxx,v 1.13 2003/08/02 20:23:36 jz Exp $
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
}

//---------------------------------------------------------------Remote_module_instance_proxy::init
/*
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
*/
//---------------------------------------------------------------Remote_module_instance_proxy::load

void Remote_module_instance_proxy::load()
{
    //_remote_instance->call( "load" );
}

//--------------------------------------------------------------Remote_module_instance_proxy::close

void Remote_module_instance_proxy::close()
{
    Com_module_instance_base::close();

    _idispatch = NULL;
    _remote_instance = NULL;
}

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

//---------------------------------------------------------Remote_module_instance_proxy::call_async
/*
void Remote_module_instance_proxy::call_async( const string& name )
{
    if( _operation )  throw_xc( "Remote_module_instance_proxy::call_async" );
    _operation = _remote_instance->call_async( "call", "?" + name );     // "?": Methode ist optional. Wenn es sie nicht gibt, kommt VT_EMPTY zurück
}

//----------------------------------------------------------Remote_module_instance_proxy::call_wait

Variant Remote_module_instance_proxy::call_wait()
{
    ptr<Operation> op = _operation;
    _operation = NULL;
    return _remote_instance->call_wait( op );
}
*/
//-------------------------------------------------------Remote_module_instance_proxy::begin__start

void Remote_module_instance_proxy::begin__start( const Object_list& object_list )
{
    Module_instance::init();

    _object_list = object_list;


    HRESULT hr;


    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
    parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !_session )
    {
        _session  = Z_NEW( Session( start_process( params ) ) );
    }

    hr = _session->create_instance( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, 
                                    spooler_com::IID_Iremote_module_instance_server );


    _operation = _session->create_instance__start( const CLSID& clsid, IUnknown* outer, DWORD, COSERVERINFO*, unsigned long count, MULTI_QI* query_interfaces );
    _call_state = c_begin_create_instance;
}

//---------------------------------------------------------Remote_module_instance_proxy::begin__end

bool Remote_module_instance_proxy::begin__end()
{
    ptr<Operation> op = _operation;
    _operation = NULL;
    return check_result( _remote_instance->call__end( op ) );
}

//---------------------------------------------------------Remote_module_instance_proxy::end__start

void Remote_module_instance_proxy::end()
{
    _operation = _remote_instance->call( "end" );
}

//-----------------------------------------------------------Remote_module_instance_proxy::end__end

void Remote_module_instance_proxy::end__end()
{
    ptr<Operation> op = _operation;
    _operation = NULL;
    _remote_instance->call__end( op );
}

//--------------------------------------------------------Remote_module_instance_proxy::step__start

void Remote_module_instance_proxy::step__start()
{
    _operation = _remote_instance->call__start( "step" );
}

//----------------------------------------------------------Remote_module_instance_proxy::step__end

bool Remote_module_instance_proxy::step__end()
{
    return check_result( _remote_instance->call__end() );
}

//-------------------------------------------------Remote_module_instance_proxy::operation_finished

bool Remote_module_instance_proxy::operation_finished()
{ 
    if( !_operation->is_finished() )  return false;

    switch( _call_state )
    {
        case c_begin_create_instance:
        {
            {
                ptr<Operation> op = _operation;
                _operation = NULL;
                _remote_instance->create_instance__end( op, (void**)&_remote_instance );
            }


            Variant objects ( Variant::vt_array, object_list.size() );
            Variant names   ( Variant::vt_array, object_list.size() );

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

            _operation = _remote_instance->call__begin( "begin", objects, names );

            _call_state = c_begin_begin;
            break;
        }

        case c_begin_begin:
        {
            _call_state = c_null;

            ptr<Operation> op = _operation;
            _operation = NULL;
            return check_result( _remote_instance->call__end( op ) );
            break;
        }

    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
