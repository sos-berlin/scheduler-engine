// $Id: spooler_module_remote.cxx,v 1.23 2003/08/29 08:14:04 jz Exp $
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
    //HRESULT hr;

    Module_instance::init();

    if( _module->_reuse != Module::reuse_task )  throw_xc( "SPOOLER-192" );         // Problem u.a.: synchrones Release(), wenn Job gestoppt wird

/*
    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
    parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !log_filename().empty() )
    parameters.push_back( Parameter( "param", "-log=" + quoted_string( "+" + log_filename() ) ) );

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
*/
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
    
    if( _remote_instance )
    {
        LOGI( "*** Remote_module_instance_proxy::close(): _remote_instance->release()\n" );
        _remote_instance->release();
        _remote_instance = NULL;
    }

    _idispatch = NULL;

    if( _process )
    {
        _process->remove_module_instance( this );
        _process = NULL;
    }

    if( _session )
    {
        _session->close();
        _session = NULL;
    }

    Com_module_instance_base::close();
}

//------------------------------------------------------------Remote_module_instance_proxy::add_obj

void Remote_module_instance_proxy::add_obj( const ptr<IDispatch>& object, const string& name )
{
    _object_list.push_back( Object_list_entry( object, name ) );
}

//--------------------------------------------------------Remote_module_instance_proxy::name_exists

bool Remote_module_instance_proxy::name_exists( const string& name )
{
    if( !_remote_instance )  return false;

    return int_from_variant( _remote_instance->call( "name_exists", name ) ) != 0;
}

//---------------------------------------------------------------Remote_module_instance_proxy::call

Variant Remote_module_instance_proxy::call( const string& name )
{
    return _remote_instance->call( "call", "?" + name );     // "?": Methode ist optional. Wenn es sie nicht gibt, kommt VT_EMPTY zurück
}

//-------------------------------------------------------Remote_module_instance_proxy::begin__start

Async_operation* Remote_module_instance_proxy::begin__start()
{
    Module_instance::init();
/*
    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
    parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !log_filename().empty() )
    parameters.push_back( Parameter( "param", "-log=" + quoted_string( "+" + log_filename() ) ) );
*/
    if( !_session )
    {
        //_session  = Z_NEW( Session( start_process( parameters ) ) );
        _process = _spooler->new_process( true );     // temporary = true: Prozess schließen, wenn er nicht mehr gebraucht wird
        _process->add_module_instance( this );
        _session = _process->session(); 
        _pid = _session->connection()->pid();
    }


    _operation = +Z_NEW( Operation( this, Operation::c_begin ) );

    return +_operation;
}

//---------------------------------------------------------Remote_module_instance_proxy::begin__end

bool Remote_module_instance_proxy::begin__end()
{
    //if( _call_state != c_begin )  

    _operation->async_check_error();
    if( !_operation->async_finished() )  throw_xc( "SPOOLER-191", "begin__end", _operation->async_state_text() );
    _operation = NULL;

    return check_result( _remote_instance->call__end() );
}

//---------------------------------------------------------Remote_module_instance_proxy::end__start

Async_operation* Remote_module_instance_proxy::end__start( bool success )
{
    if( !_remote_instance )  return NULL;

    _end_success = success;
    _operation = +Z_NEW( Operation( this, Operation::c_end ) );

    //_operation = _remote_instance->call__start( "end", success );
    
    return _operation;
}

//-----------------------------------------------------------Remote_module_instance_proxy::end__end

void Remote_module_instance_proxy::end__end()
{
    if( !_remote_instance )  return;
  //if( _operation->_call_state != Operation::c_finished )  throw_xc( "SPOOLER-191", "end__end", state_name() );
    if( !_operation->async_finished() )  throw_xc( "SPOOLER-191", "end__end", _operation->async_state_text() );

    //_remote_instance->call__end();

    ptr<Async_operation> op = _operation;
    _operation = NULL;
    op->async_check_error();
}

//--------------------------------------------------------Remote_module_instance_proxy::step__start

Async_operation* Remote_module_instance_proxy::step__start()
{
    _operation = _remote_instance->call__start( "step" );

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::step__end

bool Remote_module_instance_proxy::step__end()
{
  //if( _call_state != c_finished )  throw_xc( "SPOOLER-191", "step__end", (int)_call_state );
    if( !_operation->async_finished() )  throw_xc( "SPOOLER-191", "step__end", _operation->async_state_text() );

    _operation = NULL;
    return check_result( _remote_instance->call__end() );
}

//------------------------------------------------Remote_module_instance_proxy::Operation::Operation

Remote_module_instance_proxy::Operation::Operation( Remote_module_instance_proxy* proxy, Call_state first_state )
:
    _zero_(this+1),
    _proxy(proxy),
    _call_state(first_state)
{
    //_operation = _proxy->_session->connect_server__start();
    //_operation->set_async_parent( this );
    async_continue();
}

//----------------------------------------------Remote_module_instance_proxy::Operation::begin__end
/*
bool Remote_module_instance_proxy::Operation::begin__end()
{
    if( _call_state != c_begin )  throw_xc( "SPOOLER-191", "begin__end", state_name() );

    return check_result( _remote_instance->call__end() );
}
*/
//-----------------------------------------Remote_module_instance_proxy::Operation::async_continue_

void Remote_module_instance_proxy::Operation::async_continue_( bool wait )
{ 
    bool loop = true;
    while( loop )
    {
        loop = false;

        if( _operation )        // _operation == NULL bei _call_state == c_begin oder c_end
        {
            _operation->async_continue( wait );

            if( wait )  _operation->async_finish();
            else  
            if( !_operation->async_finished() )  return;

            if( _operation->async_has_error() )  return;
        }

        switch( _call_state )
        {
            // begin__start() ... begin_end()

            case c_begin:
            {
                _operation = _proxy->_session->connect_server__start();
                _operation->set_async_parent( this );
                _call_state = c_connect;
                break;
            }


            case c_connect:
            {
                _operation = NULL;
                _proxy->_session->connect_server__end();
            }
            
            // Nächste Operation

            {
                _multi_qi.allocate( 1 );
                _multi_qi.set_iid( 0, spooler_com::IID_Iremote_module_instance_server );
                
                _operation = _proxy->_session->create_instance__start( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, NULL, 1, _multi_qi );
                _operation->set_async_parent( this );

                _call_state = c_create_instance;
                break;
            }


            case c_create_instance:
            {
                _operation = NULL;
                HRESULT hr = _proxy->_session->create_instance__end( 1, _multi_qi );
                if( FAILED(hr) )  throw_com( hr, "create_instance" );

                _proxy->_remote_instance = dynamic_cast<object_server::Proxy*>( _multi_qi[0].pItf );
                _proxy->_idispatch = _proxy->_remote_instance;
                _multi_qi.clear();
            }

            // Nächste Operation

            {
                Variant params ( Variant::vt_array, 8 );

                {
                    Locked_safearray params_array = V_ARRAY( &params );

                    params_array[0] = "language="        + _proxy->_module->_language;
                    params_array[1] = "com_class="       + _proxy->_module->_com_class_name;
                    params_array[2] = "filename="        + _proxy->_module->_filename;
                    params_array[3] = "java_class="      + _proxy->_module->_java_class_name;
                    params_array[4] = "java_class_path=" + _proxy->_module->_spooler->_java_vm->class_path();
                    params_array[5] = "java_work_dir="   + _proxy->_module->_spooler->_java_vm->work_dir();
                    params_array[6] = "recompile="       + as_string(_proxy->_module->_recompile);
                    params_array[7] = "script="          + _proxy->_module->_source.dom_doc().xml();
                }

                _operation = _proxy->_remote_instance->call__start( "construct", params );
                _operation->set_async_parent( this );

                _call_state = c_construct;
                loop = true;
                break;
            }



            case c_construct:
            {
                _operation = NULL;
                _proxy->_remote_instance->call__end();
            }
             
            // Nächste Operation

            {
                Variant objects ( Variant::vt_array, _proxy->_object_list.size() );
                Variant names   ( Variant::vt_array, _proxy->_object_list.size() );

                {
                    Locked_safearray objects_array = V_ARRAY( &objects );
                    Locked_safearray names_array   = V_ARRAY( &names   );

                    int i = 0;
                    FOR_EACH_CONST( Object_list, _proxy->_object_list, o )
                    {
                        objects_array[i] = o->_object;
                        names_array[i]   = o->_name;
                        i++;
                    }

                    _proxy->_object_list.clear();
                }

                _operation = _proxy->_remote_instance->call__start( "begin", objects, names );
                _operation->set_async_parent( this );

                _call_state = c_call_begin;
                break;
            }


            case c_call_begin:
            {
                _call_state = c_finished;
                break;
            }



            // end__start() .. end__end()

            case c_end:
            {
                _operation = _proxy->_remote_instance->call__start( "end", _proxy->_end_success );
                _operation->set_async_parent( this );
                _call_state = c_call_end;
                break;
            }


            case c_call_end:
            {
                _operation = NULL;
                _proxy->_remote_instance->call__end();
            }

            // Nächste Operation

            {
                if( _proxy->_close_instance_at_end )
                {
                    _operation = _proxy->_remote_instance->release__start();
                    _operation->set_async_parent( this );
                    _call_state = c_release;
                }
                else
                    _call_state = c_finished;
                break;
            }

            case c_release:
            {
                _operation = NULL;
                _proxy->_remote_instance->release__end();
                _proxy->_remote_instance = NULL;
                _proxy->_idispatch = NULL;
                break;
            }

            default:
                throw_xc( "Remote_module_instance_proxy::Operation::process" );
        }
    }
}

//-----------------------------------------Remote_module_instance_proxy::Operation::async_finished_

bool Remote_module_instance_proxy::Operation::async_finished_()
{ 
    return _call_state == c_finished  ||  _operation && _operation->async_has_error();
}

//---------------------------------------Remote_module_instance_proxy::Operation::async_state_text_

string Remote_module_instance_proxy::Operation::async_state_text_()
{ 
    string text = "Remote_module_instance_proxy(state=" + state_name();
    if( _operation )  text += "," + _operation->async_state_text();
    text += ")";

    return text;
}

//----------------------------------------------Remote_module_instance_proxy::Operation::state_name

string Remote_module_instance_proxy::Operation::state_name()
{
    switch( _call_state )
    {
        case c_none           : return "none";

        case c_call_begin     : return "call_begin";
        case c_connect        : return "connect";
        case c_create_instance: return "create_instance";
        case c_construct      : return "construct";
        case c_begin          : return "begin";

        case c_end            : return "end";
        case c_call_end       : return "call_end";
        case c_release        : return "release";

        case c_finished       : return "finished";
        default               : return as_string(_call_state);      // Für Microsoft
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
