// $Id$
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

    if( _process )
    {
        _process->remove_module_instance( this );
        _process = NULL;
    }
}

//---------------------------------------------------------------Remote_module_instance_proxy::init

void Remote_module_instance_proxy::init()
{
    //HRESULT hr;
/*
    if( getenv( "SPOOLER_SERVER" ) )
    {
        _server_hostname = getenv( "SPOOLER_SERVER" );
        _server_port     = 9000;
    }
*/
    Module_instance::init();

    if( _module->_reuse != Module::reuse_task )  throw_xc( "SCHEDULER-192" );         // Problem u.a.: synchrones Release(), wenn Job gestoppt wird
}

//---------------------------------------------------------------Remote_module_instance_proxy::load

void Remote_module_instance_proxy::load()
{
    //_remote_instance->call( "load" );
}

//-------------------------------------------------------Remote_module_instance_proxy::close__start

Async_operation* Remote_module_instance_proxy::close__start()
{
  //if( _session )  _session->close_current_operation();
    
    if( _remote_instance )
    {
        LOGI( "*** Remote_module_instance_proxy::close(): _remote_instance->release()\n" );

        try
        {
            _remote_instance->release();
        }
        catch( exception& x )
        {
            LOG( "Fehler wird ignoriert: " << x.what() << "\n" );       // Z.B. ERRNO-32 Broken pipe
        }

        _remote_instance = NULL;
    }

    _idispatch = NULL;

    if( _session )
    {
        return _session->close__start();
    }

    return &dummy_sync_operation;
}

//---------------------------------------------------------Remote_module_instance_proxy::close__end

void Remote_module_instance_proxy::close__end()
{
    _session = NULL;

    if( _process )
    {
/*
        _process->remove_module_instance( this );
        exit_code();   // Exit code merken
        _process = NULL;
*/
    }
/*
  //if( _session )
    {
  //    _session->close();
        _session = NULL;
    }
*/
    //Com_module_instance_base::close();
}

//---------------------------------------------------------------Remote_module_instance_proxy::kill

bool Remote_module_instance_proxy::kill()
{
    // Wenn noch andere Modulinstanzen (Tasks) im Prozess laufen sollten, sind die auch weg.

    return _process        ? _process->kill() :
         //_remote_instance? _remote_instance->kill_process() :
           _operation      ? _operation->async_kill() 
                           : false;
}

//----------------------------------------------------------Remote_module_instance_proxy::exit_code 

int Remote_module_instance_proxy::exit_code()
{
    if( _process )  _exit_code = _process->exit_code();
    return _exit_code;
}

//-------------------------------------------------Remote_module_instance_proxy::termination_signal

int Remote_module_instance_proxy::termination_signal()
{
    if( _process )  _termination_signal = _process->termination_signal();
    return _termination_signal;
}

//----------------------------------------------------Remote_module_instance_proxy::stdout_filename

string Remote_module_instance_proxy::stdout_filename()                                      
{ 
    return _process? _process->stdout_filename() : "";
}

//----------------------------------------------------Remote_module_instance_proxy::stderr_filename

string Remote_module_instance_proxy::stderr_filename()
{ 
    return _process? _process->stderr_filename() : "";
}

//------------------------------------------------------------Remote_module_instance_proxy::add_obj

void Remote_module_instance_proxy::add_obj( IDispatch* object, const string& name )
{
    _object_list.push_back( Object_list_entry( object, name ) );
}

//--------------------------------------------------------Remote_module_instance_proxy::add_log_obj
/*
void Remote_module_instance_proxy::add_log_obj( Com_log* log, const string& name )
{
    typedef object_server::Reference_with_properties  Ref;
    ptr<Ref> remote_ref = Z_NEW( Ref( "sos::spooler::Log", log ) );
    
    int level = 0;
    log->get_level( &level );

    remote_ref->set_property( "level", level );

    _object_list.push_back( Object_list_entry( remote_ref, name ) );
}
*/
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

/*
    if( !_process )
    {
        if( _process_class_name.empty()  ||  _module->_separate_process )
        {
            _process = _spooler->new_temporary_process();
        }
        else
        {
            _process = Z_NEW( Process( _spooler ) );        
            //_process = _spooler->process_class( _process_class_name ) -> select_process();
            //if( !_process )            
            // Erstmal immer nur eine Task pro Prozess. 
            // Mehrere Tasks pro Prozess erst, wenn sichergestellt ist, dass jede Operation die Antwort liest (v.a. im Fehlerfall),
            // sodass nicht eine nicht beendete Operation den Prozess blockiert.

            //_process = _spooler->process_class( _process_class_name ) -> select_process();
        }

        _process->start();
        _process->add_module_instance( this );
    }

    _session = _process->session(); 
    _pid = _session->connection()->pid();
*/
    _operation = +Z_NEW( Operation( this, c_begin ) );

    return +_operation;
}

//---------------------------------------------------------Remote_module_instance_proxy::begin__end

bool Remote_module_instance_proxy::begin__end()
{
    //if( _call_state != c_begin )  

    //_operation->async_check_error();   Nicht hier rufen!  call__end() prüft den Fehler und ruft vorher pop_operation().

    ptr<Async_operation> operation = _operation;
    _operation = NULL;

    // *** Sonderfall, weil es keine _connection für pop_operation gibt ***
    Operation* op = dynamic_cast<Operation*>( +operation );
    if( op  &&  op->_call_state == c_connect )  op->async_check_error();   
    // ***


    //if( !_operation->async_finished() )  throw_xc( "SCHEDULER-191", "begin__end", _operation->async_state_text() );

/*
    operation->async_check_error();
    return dynamic_cast<Operation*>( +operation )->_bool_result;
*/

    //?if( !_remote_instance )
    //  operation->async_check_error();  // Wenn create_instance() fehlgeschlagen ist

    bool result = false;


    // _remote_instance->call__end() ist nicht gut, wenn _remote_instance->call__start() einen Fehler gemeldet hat. Das sollte anders codiert werden.
    // Ein Fehler in _remote_instance->call__start() wird durch den Fehler "pop_operation() bei leerem Stack" überdeckt.
    // Das hier sollte überarbeitet werden. Im Fehlerfall ist mal eine Operation offen, die beendet werden muss, und mal nicht.
    if( !operation->async_child() )  operation->async_check_error();
    if( _remote_instance )  result = check_result( _remote_instance->call__end() );   // call__end() vor der Fehlerprüfung rufen, sonst werden untere Operationen nicht beendet. 12.11.03
    operation->async_check_error();  // Wenn create_instance() fehlgeschlagen ist

    return result;
}

//---------------------------------------------------------Remote_module_instance_proxy::end__start

Async_operation* Remote_module_instance_proxy::end__start( bool success )
{
    if( !_remote_instance )  return &dummy_sync_operation; //NULL;

    _end_success = success;
  //_operation = +Z_NEW( Operation( this, c_end ) );

    _operation = _remote_instance->call__start( "end", success );
    
    return _operation;
}

//-----------------------------------------------------------Remote_module_instance_proxy::end__end

void Remote_module_instance_proxy::end__end()
{
    if( !_remote_instance )  return;
  //if( _operation->_call_state != Operation::c_finished )  throw_xc( "SCHEDULER-191", "end__end", state_name() );
    if( !_operation->async_finished() )  throw_xc( "SCHEDULER-191", "end__end", _operation->async_state_text() );

    _operation = NULL;
    _remote_instance->call__end();

  //ptr<Async_operation> op = _operation;
  //_operation = NULL;
  //op->async_check_error();
}

//--------------------------------------------------------Remote_module_instance_proxy::step__start

Async_operation* Remote_module_instance_proxy::step__start()
{
    if( !_remote_instance )  throw_xc( "SCHEDULER-200", "step__start" );

    _operation = _remote_instance->call__start( "step" );

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::step__end

Variant Remote_module_instance_proxy::step__end()
{
    if( !_remote_instance )  throw_xc( "SCHEDULER-200", "step__end" );

  //if( _call_state != c_finished )  throw_xc( "SCHEDULER-191", "step__end", (int)_call_state );
    if( !_operation->async_finished() )  throw_xc( "SCHEDULER-191", "step__end", _operation->async_state_text() );

    _operation = NULL;
    return _remote_instance->call__end();
}

//--------------------------------------------------------Remote_module_instance_proxy::call__start

Async_operation* Remote_module_instance_proxy::call__start( const string& method )
{
    if( !_remote_instance )  throw_xc( "SCHEDULER-200", "call", method.c_str() );

    _operation = _remote_instance->call__start( "call", method );

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::call__end

Variant Remote_module_instance_proxy::call__end()
{
    if( !_remote_instance )  throw_xc( "SCHEDULER-200", "call__end" );

  //if( _call_state != c_finished )  throw_xc( "SCHEDULER-191", "step__end", (int)_call_state );
    if( !_operation->async_finished() )  throw_xc( "SCHEDULER-191", "call__end", _operation->async_state_text() );

    _operation = NULL;
    return _remote_instance->call__end();
}

//-----------------------------------------------------Remote_module_instance_proxy::release__start

Async_operation* Remote_module_instance_proxy::release__start()
{
    if( _remote_instance )  _operation = _remote_instance->release__start();
                      else  _operation = &dummy_sync_operation;

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::call__end

void Remote_module_instance_proxy::release__end()
{
    if( !_operation->async_finished() )  throw_xc( "SCHEDULER-191", "release__end", _operation->async_state_text() );

    if( _operation == &dummy_sync_operation )
    {
        _operation = NULL;
    }
    else
    {
        _operation = NULL;
        _remote_instance->release__end();
        _remote_instance = NULL;
    }
}

//---------------------------------------------Remote_module_instance_proxy::check_connection_error

void Remote_module_instance_proxy::check_connection_error()
{
    if( _remote_instance )  _remote_instance->check_connection_error();
}

//----------------------------------------------Remote_module_instance_proxy::Operation::~Operation

Remote_module_instance_proxy::Operation::~Operation()
{
}

//-----------------------------------------------Remote_module_instance_proxy::Operation::Operation

Remote_module_instance_proxy::Operation::Operation( Remote_module_instance_proxy* proxy, Call_state first_state )
:
    _zero_(this+1),
    _proxy(proxy),
    _call_state(first_state)
{
    async_continue();
}

//----------------------------------------------Remote_module_instance_proxy::Operation::begin__end
/*
bool Remote_module_instance_proxy::Operation::begin__end()
{
    if( _call_state != c_begin )  throw_xc( "SCHEDULER-191", "begin__end", state_name() );

    return check_result( _remote_instance->call__end() );
}
*/
//-------------------------------------------------Remote_module_instance_proxy::try_to_get_process

bool Remote_module_instance_proxy::try_to_get_process()
{
    if( _process )  return true;

    if( _module->_separate_process 
     || _module->_process_class_name.empty()  && !_spooler->process_class_or_null("") )   // Namenlose Prozessklasse nicht bekannt? Dann temporäre Prozessklasse verwenden
    {
        _process = _spooler->new_temporary_process();
    }
    else
    {
        //_process = Z_NEW( Process( _spooler ) );        
        _process = _spooler->process_class( _module->_process_class_name ) -> select_process_if_available();
        if( !_process )  return false;

        // Erstmal immer nur eine Task pro Prozess. 
        // Mehrere Tasks pro Prozess erst, wenn sichergestellt ist, dass jede Operation die Antwort liest (v.a. im Fehlerfall),
        // sodass nicht eine nicht beendete Operation den Prozess blockiert.

        //_process = _spooler->process_class( _process_class_name ) -> select_process();
    }

    _process->add_module_instance( this );

    if( !_process->started() )
    {
        if( !_server_hostname.empty() )
        {
            _process->set_server( _server_hostname, _server_port );
        }

        _process->set_job_name( _job_name );
        _process->set_task_id ( _task_id  );
        _process->start();
    }

    _session = _process->session(); 
    _pid     = _session->connection()->pid();

    return true;
}

//-----------------------------------------Remote_module_instance_proxy::Operation::async_continue_

bool Remote_module_instance_proxy::continue_async_operation( Operation* operation, bool wait )
{ 
    switch( operation->_call_state )
    {
        // begin__start() ... begin_end()

        case c_begin:
        {
/*
            if( !_process )
            {
                try_to_get_process();
                if( !_process )  break;
            }
*/
            operation->set_async_child( _session->connect_server__start() );

            operation->_call_state = c_connect;
            break;
        }


        case c_connect:
        {
            operation->set_async_child( NULL );
            _session->connect_server__end();
        }
        
        // Nächste Operation

        {
            operation->_multi_qi.allocate( 1 );
            operation->_multi_qi.set_iid( 0, spooler_com::IID_Iremote_module_instance_server );
            
            operation->set_async_child( _session->create_instance__start( spooler_com::CLSID_Remote_module_instance_server, NULL, 0, NULL, 1, operation->_multi_qi ) );

            operation->_call_state = c_create_instance;
            break;
        }


        case c_create_instance:
        {
            operation->set_async_child( NULL );
            HRESULT hr = _session->create_instance__end( 1, operation->_multi_qi );
            if( FAILED(hr) )  throw_com( hr, "create_instance" );

            _remote_instance = dynamic_cast<object_server::Proxy*>( operation->_multi_qi[0].pItf );
            _idispatch = _remote_instance;
            operation->_multi_qi.clear();
        }

        // Nächste Operation

        {
            Variant params ( Variant::vt_array, 10 );

            {
                Locked_safearray params_array = V_ARRAY( &params );

                params_array[0] = "language="        + _module->_language;
                params_array[1] = "com_class="       + _module->_com_class_name;
                params_array[2] = "filename="        + _module->_filename;
                params_array[3] = "java_class="      + _module->_java_class_name;

                if( _server_hostname.empty() )
                {
                    params_array[4] = "java_class_path=" + _module->_spooler->_java_vm->class_path();
                    params_array[5] = "java_work_dir="   + _module->_spooler->_java_vm->work_dir();
                }

                params_array[6] = "recompile="       + as_string( _module->_recompile && !_module->_compiled );
                params_array[7] = "script="          + _module->_source.dom_document().xml();
                params_array[8] = "job="             + _job_name;
                params_array[9] = "task_id="         + as_string( _task_id );
            }

            operation->set_async_child( _remote_instance->call__start( "construct", params ) );

            operation->_call_state = c_construct;

            break;
        }


        case c_construct:
        {
            operation->set_async_child( NULL );
            _remote_instance->call__end();

            _module->_compiled = true;
        }
            
        // Nächste Operation

        {
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

                _object_list.clear();
            }

            operation->set_async_child( _remote_instance->call__start( "begin", objects, names ) );
            operation->_call_state = c_call_begin;
            break;
        }


        case c_call_begin:
        {
            operation->set_async_child( NULL );
            operation->_call_state = c_finished;
            break;
        }

        //operation->_bool_result = check_result( _remote_instance->call__end() );


        // end__start() .. end__end()
/*
        case c_end:
        {
            operation->set_async_child( _remote_instance->call__start( "end", _end_success ) );
            operation->_call_state = c_call_end;
          //something_done = true;
            break;
        }


        case c_call_end:
        {
            operation->set_async_child( NULL );
            _remote_instance->call__end();
          //something_done = true;
        }
*/
        // Nächste Operation
/*
        {
            if( _close_instance_at_end )
            {
                operation->set_async_child( _remote_instance->release__start() );
                operation->_call_state = c_release;
            }
            else
                operation->_call_state = c_finished;
            break;
        }


        case c_release:
        {
            operation->set_async_child( NULL );
            _remote_instance->release__end();
            _remote_instance = NULL;
            _idispatch = NULL;
        }
*/
//      operation->_call_state = c_finished;
//      break;


        default:
            throw_xc( "Remote_module_instance_proxy::Operation::process" );
    }

    return true;
}

//-----------------------------------------Remote_module_instance_proxy::Operation::async_finished_

bool Remote_module_instance_proxy::Operation::async_finished_()
{ 

    if( _call_state == c_begin  &&  !_proxy->_process ) 
    {
        // Ein Sonderfall: async_continue() wird hier (statt oben im Hauptprogramm) gerufen,
        // weil die Operation nicht über Connection::current_super_operation() erreichbar ist.
        // Denn diese Operation hat ja noch keine Connection.
        // Ein zentrales Register aller offenen Operationen wäre gut.
        // Dann müsste das Hauptprogramm auch nicht die Verbindungen kennen und für jede
        // async_continue() aufrufen.

        // S.a. begin__end(): async_check_error() bei _state == c_connect

        async_continue();
    }

    return _call_state == c_finished;  //  ||  _operation && _operation->async_has_error();
}

//---------------------------------------Remote_module_instance_proxy::Operation::async_state_text_

string Remote_module_instance_proxy::Operation::async_state_text_()
{ 
    string text = "Remote_module_instance_proxy(state=" + state_name();
    if( _call_state == c_begin  &&  !_proxy->_process )  text += ",Warten auf verfügbaren Prozess der Prozessklasse";
  //if( _operation )  text += "," + _operation->async_state_text();
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

      //case c_end            : return "end";
      //case c_call_end       : return "call_end";
      //case c_release        : return "release";

        case c_finished       : return "finished";
        default               : return as_string(_call_state);      // Für Microsoft
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
