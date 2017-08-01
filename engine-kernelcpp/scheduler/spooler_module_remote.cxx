#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------Remote_module_instance_proxy::~Remote_module_instance_proxy

Remote_module_instance_proxy::~Remote_module_instance_proxy()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//--------------------------------------------------------------Remote_module_instance_proxy::close

void Remote_module_instance_proxy::close()
{
    Com_module_instance_base::close();
}

//---------------------------------------------------------------Remote_module_instance_proxy::init

void Remote_module_instance_proxy::init()
{
    Module_instance::init();
}

//---------------------------------------------------------------Remote_module_instance_proxy::load

bool Remote_module_instance_proxy::load()
{
    return Com_module_instance_base::load();
}

//-------------------------------------------------------Remote_module_instance_proxy::close__start

Async_operation* Remote_module_instance_proxy::close__start()
{
    if (_remote_instance) {
        Z_LOGI2( "scheduler", "*** Remote_module_instance_proxy::close(): _remote_instance->release()\n" );
        try {
            _remote_instance->release();
        }
        catch(exception& x) {
            Z_LOG2( "scheduler", "Error ignored: " << x.what() << "\n" );       // Z.B. ERRNO-32 Broken pipe
        }
        _remote_instance = NULL;
    }

    if( _api_process  &&  _module->kind() != Module::kind_process )
    {
        _exit_code          = _api_process->exit_code();
        _termination_signal = _api_process->termination_signal();
    }

    _idispatch = NULL;

    if (_api_process)  
        return _api_process->close__start();
    else {
        _operation = Z_NEW(Sync_operation);
        return _operation;
    }
}

//---------------------------------------------------------Remote_module_instance_proxy::close__end

void Remote_module_instance_proxy::close__end()
{
    Com_module_instance_base::close__end();
}

//---------------------------------------------------------------Remote_module_instance_proxy::kill

bool Remote_module_instance_proxy::kill(int unix_signal)
{
    // Wenn noch andere Modulinstanzen (Tasks) im Prozess laufen sollten, sind die auch weg.

    return _api_process    ? _api_process->kill(unix_signal) :
         //_remote_instance? _remote_instance->kill_process() :
           _operation      ? _operation->async_kill() 
                           : false;
}

//----------------------------------------------------------Remote_module_instance_proxy::exit_code 

int Remote_module_instance_proxy::exit_code()
{
    if( _api_process )  _exit_code = _api_process->exit_code();
    return _exit_code;
}

//-------------------------------------------------Remote_module_instance_proxy::termination_signal

int Remote_module_instance_proxy::termination_signal()
{
    if( _api_process )  _termination_signal = _api_process->termination_signal();
    return _termination_signal;
}

//--------------------------------------------------------Remote_module_instance_proxy::stdout_path

File_path Remote_module_instance_proxy::stdout_path()                                      
{ 
    if (Local_api_process* local_api_process = dynamic_cast<Local_api_process*>(+_process)) 
        return local_api_process->stdout_path();
    else
        return File_path();
}

//--------------------------------------------------------Remote_module_instance_proxy::stderr_path

File_path Remote_module_instance_proxy::stderr_path()
{ 
    if (Local_api_process* local_api_process = dynamic_cast<Local_api_process*>(+_process)) 
        return local_api_process->stderr_path();
    else
        return File_path();
}

//---------------------------------------------------Remote_module_instance_proxy::try_delete_files

bool Remote_module_instance_proxy::try_delete_files( Has_log* log )
{
    if (Local_api_process* local_api_process = dynamic_cast<Local_api_process*>(+_process)) 
        return local_api_process->try_delete_files(log);
    else
        return true;
}

//----------------------------------------------------Remote_module_instance_proxy::undeleted_files

list<File_path> Remote_module_instance_proxy::undeleted_files()
{
    if (Local_api_process* local_api_process = dynamic_cast<Local_api_process*>(+_process)) 
        return local_api_process->undeleted_files();
    else
        return list<File_path>();
}

//------------------------------------------------------------Remote_module_instance_proxy::add_obj

void Remote_module_instance_proxy::add_obj( IDispatch* object, const string& name )
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

    _operation = +Z_NEW( Operation( this, c_begin ) );

    return +_operation;
}

//---------------------------------------------------------Remote_module_instance_proxy::begin__end

bool Remote_module_instance_proxy::begin__end()
{
    //_operation->async_check_error();   Nicht hier rufen!  call__end() prüft den Fehler und ruft vorher pop_operation().

    ptr<Async_operation> operation = _operation;
    _operation = NULL;

    // *** Sonderfall, weil es keine _connection für pop_operation gibt ***
    Operation* op = dynamic_cast<Operation*>( +operation );
    if( op  &&  op->_call_state == c_connect )  op->async_check_error();   
    // ***


    //if( !_operation->async_finished() )  z::throw_xc( "SCHEDULER-191", "begin__end", _operation->async_state_text() );

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
    if( !_remote_instance ) {
        _operation = Z_NEW(Sync_operation);
        return _operation;
    }

    _end_success = success;
    _operation = _remote_instance->call__start( "end", success );    
    return _operation;
}

//-----------------------------------------------------------Remote_module_instance_proxy::end__end

void Remote_module_instance_proxy::end__end()
{
    if( !_remote_instance )  return;
    if( !_operation->async_finished() )  z::throw_xc( "SCHEDULER-191", "end__end", _operation->async_state_text() );

    _operation = NULL;
    _remote_instance->call__end();
}

//--------------------------------------------------------Remote_module_instance_proxy::step__start

Async_operation* Remote_module_instance_proxy::step__start()
{
    if( !_remote_instance )  z::throw_xc( "SCHEDULER-200", "step__start" );

    _operation = _remote_instance->call__start( "step" );

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::step__end

Variant Remote_module_instance_proxy::step__end()
{
    if( !_remote_instance )  z::throw_xc( "SCHEDULER-200", "step__end" );
    if( !_operation )  z::throw_xc( "SCHEDULER-191", "step__end", "_operation==NULL" );
    if( !_operation->async_finished() )  z::throw_xc( "SCHEDULER-191", "step__end", _operation->async_state_text() );

    _operation = NULL;
    return _remote_instance->call__end();
}

//--------------------------------------------------------Remote_module_instance_proxy::call__start

Async_operation* Remote_module_instance_proxy::call__start( const string& method )
{
    if( !_remote_instance )  z::throw_xc( "SCHEDULER-200", "call", method.c_str() );

    _operation = _remote_instance->call__start( "call", method );

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::call__end

Variant Remote_module_instance_proxy::call__end()
{
    if( !_remote_instance )  z::throw_xc( "SCHEDULER-200", "call__end" );
    if( !_operation->async_finished() )  z::throw_xc( "SCHEDULER-191", "call__end", _operation->async_state_text() );

    _operation = NULL;
    return _remote_instance->call__end();
}

//-----------------------------------------------------Remote_module_instance_proxy::release__start

Async_operation* Remote_module_instance_proxy::release__start()
{
    if( _remote_instance )  _operation = _remote_instance->release__start();
                      else  _operation = Z_NEW(Sync_operation);

    return _operation;
}

//----------------------------------------------------------Remote_module_instance_proxy::call__end

void Remote_module_instance_proxy::release__end()
{
    if( !_operation->async_finished() )  z::throw_xc( "SCHEDULER-191", "release__end", _operation->async_state_text() );

    if (dynamic_cast<Sync_operation*>(+_operation))
    {
        _operation = NULL;
    }
    else
    {
        _operation = NULL;
        _remote_instance->release__end();
        _remote_instance = NULL;
    }

    Com_module_instance_base::release__end();
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

//-------------------------------------------------Remote_module_instance_proxy::try_to_get_process

bool Remote_module_instance_proxy::try_to_get_process(const Api_process_configuration* c)
{
    assert(!c);
    if (!_api_process) {
        Api_process_configuration process_configuration;
        process_configuration._remote_scheduler_address = _remote_scheduler_address;
        process_configuration._environment = new Com_variable_set(*_process_environment);
        process_configuration._java_options = trim(_spooler->settings()->_job_java_options + " " + _module->_java_options);
        process_configuration._java_classpath = _module->_java_class_path;
        process_configuration._priority = _module->_priority;
        process_configuration._login = _module->_login;
        process_configuration._has_api = _module->has_api();
        process_configuration._job_path = _task->job()->path();
        process_configuration._job_name = _job_name;
        process_configuration._task_id = _task_id;
        process_configuration._credentials_key = _module->_credentials_key;
        process_configuration._load_user_profile = _module->_load_user_profile;

        bool ok = Module_instance::try_to_get_process(&process_configuration);
        if (ok) {
            _api_process = dynamic_cast<Api_process*>(+_process);
            if (!_api_process) z::throw_xc(Z_FUNCTION);
            _api_process->start();
            _pid = _api_process->pid();
        }
    }
    if (_api_process) {
        _api_process->check_exception();    // Exception, wenn Agent das Kommando nicht akzeptiert hat
        return _api_process->is_started();  // false, wenn Agent noch nicht geantwortet hat.
    } else 
        return false;
}


void Remote_module_instance_proxy::detach_process() {
    if (_api_process) {
        _api_process->close_session();
        _api_process = NULL;
    }
    Module_instance::detach_process();
}

//-------------------------------------------Remote_module_instance_proxy::continue_async_operation

bool Remote_module_instance_proxy::continue_async_operation( Operation* operation, Async_operation::Continue_flags )
{ 

AGAIN:
    switch( operation->_call_state )
    {
        // begin__start() ... begin_end()

        case c_begin:
        {
            ptr<Async_operation> connection_operation = _api_process->session()->connect_server__start();
            connection_operation->set_async_manager( _spooler->_connection_manager );
            operation->set_async_child( connection_operation );            
            operation->_call_state = c_connect;

            #if defined Z_UNIX
                if (connection_operation->async_finished())     // Immer bei socketpair()
                    goto AGAIN;
            #endif
            break;
        }


        case c_connect:
        {
            operation->set_async_child( NULL );
            _api_process->session()->connect_server__end();
        }
        
        // Nächste Operation

        {
            operation->_multi_qi.allocate( 1 );
            operation->_multi_qi.set_iid( 0, spooler_com::IID_Iremote_module_instance_server );
            
            operation->set_async_child(_api_process->session()->create_instance__start(spooler_com::CLSID_Remote_module_instance_server, NULL, 0, NULL, 1, operation->_multi_qi));

            operation->_call_state = c_create_instance;
            break;
        }


        case c_create_instance:
        {
            operation->set_async_child( NULL );
            HRESULT hr = _api_process->session()->create_instance__end( 1, operation->_multi_qi );
            if( FAILED(hr) )  throw_com( hr, "create_instance", string_from_clsid( *operation->_multi_qi[ 0 ].pIID ) );

            _remote_instance = dynamic_cast<object_server::Proxy*>( operation->_multi_qi[0].pItf );
            _idispatch = _remote_instance;
            operation->_multi_qi.clear();
        }

        // Nächste Operation

        {
            Variant params ( Variant::vt_array, int_cast((18+2) + 10 * _module->_monitors->module_monitors().size()) );   // Wichtig: Größe anpassen!

            {
                Locked_safearray<Variant> params_array ( V_ARRAY( &params ) );
                int nr = 0;

                params_array[ nr++ ] = "language="        + _module->_language;
                params_array[ nr++ ] = "com_class="       + _module->_com_class_name;
                params_array[ nr++ ] = "filename="        + _module->_filename;
                params_array[ nr++ ] = "java_class="      + _module->_java_class_name;
                params_array[ nr++ ] = "java_options="    + _module->_java_options;
                params_array[ nr++ ] = "java_class_path=" + _module->_java_class_path;  // JS-540
                if (_module->_dotnet_class_name != "") params_array[ nr++ ] = "dotnet_class=" + _module->_dotnet_class_name;
                if (_module->_dll != "") params_array[ nr++ ] = "dll=" + _module->_dll;

                params_array[ nr++ ] = "script="          + _module->_text_with_includes.includes_resolved().xml_string();
                params_array[ nr++ ] = "job="             + _job_name;
                params_array[ nr++ ] = "task_id="         + as_string( _task_id );
                params_array[ nr++ ] = "environment="     + _process_environment->dom( "environment", "variable" ).xml_string();  // Wird bisher nur von Process_module_instance benutzt 2008-10-31

                if( _has_order )
                params_array[ nr++ ] = "has_order=1";
                if (_task->stderr_log_level() != log_info) 
                    params_array[nr++] = "stderr_log_level=" + as_string(_task->stderr_log_level());

                // prefix is transferred only if it is set in scheduler.xml otherwise the remote side will work with the default
                // that is because old agents could not work, if process.shell_variable_prefix is set
                if (_module->_process_shell_variable_prefix_is_configured)
                  params_array[ nr++ ] = "process.shell_variable_prefix=" + _module->_process_shell_variable_prefix;

                vector<Module_monitor*> module_monitors = _module->_monitors->module_monitors();
                for (int i = 0; i < module_monitors.size(); i++) {
                    Module_monitor* monitor = module_monitors[i];
                    params_array[ nr++ ] = "monitor.language="        + monitor->module()->_language;       // Muss der erste Parameter sein, legt den Module_monitor an
                    params_array[ nr++ ] = "monitor.name="            + monitor->monitor_name();           
                    params_array[ nr++ ] = "monitor.ordering="        + as_string(i);  // module_monitors() are ordered increasingly
                    params_array[ nr++ ] = "monitor.com_class="       + monitor->module()->_com_class_name;
                    params_array[ nr++ ] = "monitor.filename="        + monitor->module()->_filename;
                    params_array[ nr++ ] = "monitor.java_class="      + monitor->module()->_java_class_name;
                    params_array[ nr++ ] = "monitor.script="          + monitor->module()->_text_with_includes.includes_resolved().xml_string();    // JS-444  // Muss der letzte Parameter sein!
                    if (monitor->module()->_dotnet_class_name != "") params_array[ nr++ ] = "dotnet_class=" + monitor->module()->_dotnet_class_name;
                    if (monitor->module()->_dll != "") params_array[ nr++ ] = "dll=" + monitor->module()->_dll;
                }
            }

            // vgl. spooler_module_remote_server
            operation->set_async_child( _remote_instance->call__start( "construct", params ) );

            operation->_call_state = c_construct;

            break;
        }


        case c_construct:
        {
            operation->set_async_child( NULL );
            
            Variant ok = _remote_instance->call__end();
            
            if( !check_result( ok ) )
            {
                operation->_call_state = c_release_begin;
                goto AGAIN;
            }
        }
            
        // Nächste Operation

        {
            Variant objects ( Variant::vt_array, int_cast(_object_list.size()) );
            Variant names   ( Variant::vt_array, int_cast(_object_list.size()) );

            {
                Locked_safearray<Variant> objects_array ( V_ARRAY( &objects ) );
                Locked_safearray<Variant> names_array   ( V_ARRAY( &names   ) );

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

        case c_release_begin:     // Nur, wenn Construct() NULL geliefert hat (weil Module_monitor.spooler_task_before() false lieferte)
        {
            operation->set_async_child( NULL );
            operation->set_async_child( _remote_instance->release__start() );
            operation->_call_state = c_release;
            break;
        }

        case c_release:
        {
            _remote_instance->release__end();
            _remote_instance = NULL;
            _idispatch = NULL;
            operation->_call_state = c_finished;
        }

        default:
            throw_xc( "Remote_module_instance_proxy::Operation::process" );
    }

    return true;
}

//-------------------------------------------------------Remote_module_instance_proxy::process_name

string Remote_module_instance_proxy::process_name() const
{
    return _api_process? _api_process->short_name() : "";
}

//-----------------------------------------Remote_module_instance_proxy::Operation::async_finished_

bool Remote_module_instance_proxy::Operation::async_finished_() const
{ 
    if( _call_state == c_begin  &&  !_proxy->_api_process )
    {
        // Ein Sonderfall: async_continue() wird hier (statt oben im Hauptprogramm) gerufen,
        // weil die Operation nicht über Connection::current_super_operation() erreichbar ist.
        // Denn diese Operation hat ja noch keine Connection.
        // Ein zentrales Register aller offenen Operationen wäre gut.
        // Dann müsste das Hauptprogramm auch nicht die Verbindungen kennen und für jede
        // async_continue() aufrufen.

        // S.a. begin__end(): async_check_error() bei _state == c_connect

        const_cast<Operation*>( this ) -> async_continue();
    }

    return _call_state == c_finished;
}

//---------------------------------------Remote_module_instance_proxy::Operation::async_state_text_

string Remote_module_instance_proxy::Operation::async_state_text_() const
{ 
    string text = "Remote_module_instance_proxy(state=" + state_name();
    if( _call_state == c_begin  &&  !_proxy->_api_process )  text += ", waiting for available process in process class";
  //if( _operation )  text += "," + _operation->async_state_text();
    text += ")";

    return text;
}

//----------------------------------------------Remote_module_instance_proxy::Operation::state_name

string Remote_module_instance_proxy::Operation::state_name() const
{
    switch( _call_state )
    {
        case c_none           : return "none";
        case c_call_begin     : return "call_begin";
        case c_connect        : return "connect";
        case c_create_instance: return "create_instance";
        case c_construct      : return "construct";
        case c_begin          : return "begin";
        case c_release_begin  : return "release_begin";
        case c_release        : return "release";
        case c_finished       : return "finished";
        default               : return as_string(_call_state);      // Für Microsoft
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
