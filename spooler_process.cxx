// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
// §1172
// §1206

#include "spooler.h"


namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int connection_retry_time = 60;

Process_class_subsystem    ::Class_descriptor    Process_class_subsystem    ::class_descriptor ( &typelib, "Spooler.Process_classes", Process_class_subsystem::_methods );
Process_class_configuration::Class_descriptor    Process_class_configuration::class_descriptor ( &typelib, "Spooler.Process_class"  , Process_class          ::_methods );

//----------------------------------------------------------------Process_class_subsystem::_methods

const Com_method Process_class_subsystem::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Process_class_subsystem,  1, Java_class_name      , VT_BSTR    , 0 ),
    COM_PROPERTY_GET( Process_class_subsystem,  0, Process_class        , VT_DISPATCH, 0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_subsystem,  2, Process_class_or_null, VT_DISPATCH, 0, VT_BSTR ),
    COM_METHOD      ( Process_class_subsystem,  3, Create_process_class , VT_DISPATCH, 0 ),
    COM_METHOD      ( Process_class_subsystem,  4, Add_process_class    , VT_EMPTY   , 0, VT_DISPATCH ),
#endif
    {}
};

//------------------------------------------------------------Process_class_configuration::_methods

const Com_method Process_class_configuration::_methods[] =
{ 
#ifdef COM_METHOD
    COM_PROPERTY_GET( Process_class_configuration,  1, Java_class_name     , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  2, Name                ,              0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_configuration,  2, Name                , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  3, Remote_scheduler    ,              0, VT_BSTR ),
    COM_PROPERTY_GET( Process_class_configuration,  3, Remote_scheduler    , VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Process_class_configuration,  4, Max_processes       ,              0, VT_INT ),
    COM_PROPERTY_GET( Process_class_configuration,  4, Max_processes       , VT_INT    , 0 ),
    COM_METHOD      ( Process_class_configuration,  5, Remove              , VT_EMPTY   , 0 ),
#endif
    {}
};

//--------------------------------------------------------Process::Close_operation::Close_operation

Process::Close_operation::Close_operation( Process* p, bool run_independently )
: 
    _zero_(this+1), 
    _process(p)
{
    if( run_independently )
    {
        _hold_self = this;
    }
}

//--------------------------------------------------------rocess::Close_operation::~Close_operation
    
Process::Close_operation::~Close_operation()
{
}

//--------------------------------------------------------Process::Close_operation::async_continue_

bool Process::Close_operation::async_continue_( Async_operation::Continue_flags )
{
    return _process->continue_close_operation( this );
}

//----------------------------------------------------------------Process::continue_close_operation

bool Process::continue_close_operation( Process::Close_operation* op )
{
    bool something_done = false;

    if( op->_state == Close_operation::s_initial )
    {
        if( _session )
        {
            op->_close_session_operation = _session->close__start();
    
            if( !op->_close_session_operation->async_finished() )
            {
                op->_close_session_operation->set_async_parent( op );
                op->_close_session_operation->set_async_manager( _spooler->_connection_manager );
            }

            something_done = true;
        }

        op->_state = Close_operation::s_closing_session;
    }

    if( op->_state == Close_operation::s_closing_session )
    {
#       ifdef Z_WINDOWS
            if( op->_close_session_operation )  op->_close_session_operation->async_continue();       // Falls wir wegen Prozess-Event aufgerufen worden sind
#       endif

        if( !op->_close_session_operation  ||  op->_close_session_operation->async_finished() )
        {
            if( op->_close_session_operation )
            {
                op->_close_session_operation->set_async_parent( NULL );
                op->_close_session_operation = NULL;
                _session->close__end();
                _session = NULL;

                something_done = true;
            }

            if( _async_remote_operation )
            {
                _async_remote_operation->close_remote_task();
                _async_remote_operation->set_async_parent( op );
                something_done = true;
            }

            op->_state = Close_operation::s_closing_remote_process;
        }
    }

    if( op->_state == Close_operation::s_closing_remote_process )
    {
        if( !_async_remote_operation || _async_remote_operation->async_finished() )
        {
            if( _async_remote_operation )  _async_remote_operation->set_async_parent( NULL );

            op->_state = Close_operation::s_finished;

            ptr<Process> process = op->_process;
            op->_process = NULL;

            if( op->_hold_self )    // run_independently
            {
                process->close__end();
                process->_close_operation = NULL;
                op->_hold_self = NULL;
                // this ist jetzt ungültig!
            }

            something_done = true;
        }
    }

    return something_done;
}

//--------------------------------------------------------Process::Close_operation::async_finished_

bool Process::Close_operation::async_finished_() const
{
    return _state == s_finished;
}

//------------------------------------------------------Process::Close_operation::async_state_text_

string Process::Close_operation::async_state_text_() const
{
    S result;

    result << "Process::Close_operation ";
    result << string_from_state( _state );
    if( _close_session_operation )  result << " " << _close_session_operation->async_state_text();
    if( _process  &&  _process->_async_remote_operation )  result << " " << _process->_async_remote_operation->async_state_text();

    return result;
}

//------------------------------------------------------Process::Close_operation::string_from_state
    
string Process::Close_operation::string_from_state( State state )
{
    switch( state )
    {
        case s_initial:                 return "initial";
        case s_closing_session:         return "closing_session";
        case s_closing_remote_process:  return "closing_remote_process";
        case s_finished:                return "finished";
        default:                        return S() << "State(" << state << ")";
    }
}

//------------------------------------------Process::Async_remote_operation::Async_remote_operation
    
Process::Async_remote_operation::Async_remote_operation( Process* p ) 
:                        
    _zero_(this+1), 
    _process(p) 
{
    _process->_spooler->_process_count++;       // Jeder Prozess hat zwei Verbindungen: Zum Prozess und Xml_client_connection zum Scheduler
}

//-----------------------------------------Process::Async_remote_operation::~Async_remote_operation
    
Process::Async_remote_operation::~Async_remote_operation()
{
    --_process->_spooler->_process_count;

    if( _process && _process->_xml_client_connection && _process->_xml_client_connection->async_parent() == this )  
    {
        _process->_xml_client_connection->set_async_parent( NULL );
    }
}

//------------------------------------------------------Process::Async_remote_operation::state_name
    
string Process::Async_remote_operation::state_name( State state )
{
    string result;

    switch( state )
    {
        case s_not_connected:   result = "not_connected";   break;
        case s_connecting:      result = "connecting";      break;
        case s_starting:        result = "starting";        break;
        case s_running:         result = "running";         break;
        case s_closing:         result = "closing";         break;
        case s_closed:          result = "closed";          break;
        default:                result = "Async_remote_operation-" + as_string( state );
    }

    return result;
}

//-----------------------------------------------Process::Async_remote_operation::async_state_text_

string Process::Async_remote_operation::async_state_text_() const
{
    S result;
    result << "Async_remote_operation " << state_name( _state );
    if( _process )  result << " " << _process->obj_name();
    return result;
}

//----------------------------------------------------Process::Com_server_thread::Com_server_thread

Process::Com_server_thread::Com_server_thread( object_server::Connection_to_own_server_thread* c ) 
: 
    Base_class(c),
    _zero_(this+1)
{
    set_thread_name( "scheduler::Process::Com_server_thread" );
}

//----------------------------------------------------------Process::Com_server_thread::thread_main
    
int Process::Com_server_thread::thread_main()
{
    int result = 0;

    Com_initialize com_initialize;

    try
    {
        _object_server = Z_NEW( Object_server() );  // Bis zum Ende des Threads stehenlassen, wird von anderem Thread benutzt: Process::pid()
        _object_server->set_stdin_data( _connection->stdin_data() );
        _server = +_object_server;

        result = run_server();
    }
    catch( exception& x )
    {
        S s; 
        s << "ERROR in Com_server_thread: " << x.what() << "\n";
        Z_LOG2( "scheduler", s );
        cerr << s;
    }

    return result;
}

//---------------------------------------------------------------------------------Process::Process

Process::Process( Spooler* sp )
: 
    Scheduler_object( sp, this, type_process ), 
    _zero_(this+1),
    _process_id( _spooler->get_next_process_id() )
{
}

//--------------------------------------------------------------------------------Process::~Process

Process::~Process()
{
    if( _async_remote_operation )
    {
        _async_remote_operation->set_async_manager( NULL );
        _async_remote_operation = NULL;
    }

    if( _process_handle_copy )  _spooler->unregister_process_handle( _process_handle_copy );
    if( _xml_client_connection )  _xml_client_connection->set_async_manager( NULL );

    if( _process_class )  _process_class->remove_process( this );
}

//-----------------------------------------------------------------------------Process::close_async

void Process::close_async()
{
    if( !_close_operation )
    {
        if( !_async_remote_operation  &&  !is_terminated() )
        {
            log()->warn( message_string( "SCHEDULER-850", obj_name()) );

            try
            {
                kill();     // Nicht bei _async_remote_operation aufrufen, dass eine TCP-Nachricht starten würde
            }
            catch( exception& x ) { _log->warn( x.what() ); }
        }


        close__start( true );
    }
}

//----------------------------------------------------------------------------Process::close__start

Async_operation* Process::close__start( bool run_independently )
{
    _close_operation = Z_NEW( Close_operation( this, run_independently ) );
    _close_operation->set_async_manager( _spooler->_connection_manager );
    _close_operation->async_continue();

    return _close_operation;
}

//------------------------------------------------------------------------------Process::close__end

void Process::close__end()
{
    _close_operation = NULL;
    _session = NULL;
}

//---------------------------------------------------------------------Process::add_module_instance

void Process::add_module_instance( Module_instance* module_instance )
{ 
    if( _module_instance_count != 0 )  assert(0), throw_xc( "Process::add_module_instance" );

    InterlockedIncrement( &_module_instance_count ); 

    _module_instance = module_instance;
}

//------------------------------------------------------------------Process::remove_module_instance

void Process::remove_module_instance( Module_instance* )
{ 
    _module_instance = NULL;

    InterlockedDecrement( &_module_instance_count ); 

    if( _temporary  &&  _module_instance_count == 0 )  
    {
        if( _session )
        {
            if( _session->connection() )  _exit_code          = _session->connection()->exit_code(),  
                                          _termination_signal = _session->connection()->termination_signal();

            _session->close__start() -> async_finish();
            _session->close__end();
            _session = NULL;
        }

        if( Process_class* process_class = _process_class )  
        {
            _process_class = NULL;
            process_class->remove_process( this );
        }
    }
}

//-----------------------------------------------------------------------------------Process::start

void Process::start()
{
    if( started() )  assert(0), z::throw_xc( Z_FUNCTION );

    if( _process_class )  _remote_scheduler = _process_class->remote_scheduler();

    if( is_remote_host() )
    {
        assert( _process_class );
        async_remote_start();
    }
    else
    if( _run_in_thread )
    {
        start_local_thread();
    }
    else
    {
        start_local_process();
    }

#   ifdef Z_WINDOWS
        if( _spooler )  _connection->set_event( &_spooler->_event );
#   endif

    _log->set_prefix( obj_name() );

    _session = Z_NEW( object_server::Session( _connection ) );
    _session->set_connection_has_only_this_session();

    _running_since = Time::now();
}

//---------------------------------------------------------------------Process::start_local_process

void Process::start_local_process()
{
    ptr<object_server::Connection_to_own_server_process> connection = _spooler->_connection_manager->new_connection_to_own_server_process();
    object_server::Parameters                            parameters;

    if( !_spooler->_sos_ini.empty() )
    parameters.push_back( object_server::Parameter( "param", "-sos.ini=" + _spooler->_sos_ini ) );   // Muss der erste Parameter sein! (für sos_main0()).

    if( !_spooler->_factory_ini.empty() )
    parameters.push_back( object_server::Parameter( "param", "-ini=" + _spooler->_factory_ini ) );

    parameters.push_back( object_server::Parameter( "param", "-O" ) );

    if( !_job_name.empty() )
    parameters.push_back( object_server::Parameter( "param", "-job=" + _job_name ) );

    if( _task_id )
    parameters.push_back( object_server::Parameter( "param", "-task-id=" + as_string(_task_id) ) );

    if( !log_filename().empty() )
    parameters.push_back( object_server::Parameter( "param", "-log=" + log_categories_as_string() + " >+" + log_filename() ) );

    parameters.push_back( object_server::Parameter( "program", _spooler->_my_program_filename ) );


    connection->open_stdout_stderr_files();      //Nicht in einem remote_scheduler (File_logger übernimmt stdout): 
    fill_connection( connection );

    #ifdef Z_HPUX
        connection->set_ld_preload( static_ld_preload );
    #endif

    connection->set_priority( _priority );
    connection->start_process( parameters );

    _connection = +connection;

    _process_handle_copy = _connection->process_handle();
    _spooler->register_process_handle( _process_handle_copy );

    _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
}

//----------------------------------------------------------------------Process::start_local_thread

void Process::start_local_thread()
{
    ptr<object_server::Connection_to_own_server_thread> connection = _spooler->_connection_manager->new_connection_to_own_server_thread();
   
    fill_connection( connection );

    _com_server_thread = Z_NEW( Com_server_thread( connection ) );
    connection->start_thread( _com_server_thread );


    _connection = +connection;

    _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
}

//-------------------------------------------------------------------------Process::fill_connection

void Process::fill_connection( object_server::Connection* connection )
{
    xml::Xml_string_writer stdin_xml_writer;

    stdin_xml_writer.set_encoding( scheduler_character_encoding );
    stdin_xml_writer.write_prolog();

    stdin_xml_writer.begin_element( "task_process" );
    {
        stdin_xml_writer.set_attribute_optional( "include_path"          , _spooler->include_path() );
        stdin_xml_writer.set_attribute_optional( "java_options"          , _spooler->_config_java_options );
        stdin_xml_writer.set_attribute_optional( "java_class_path"       , _spooler->java_subsystem()->java_vm()->class_path() );
        stdin_xml_writer.set_attribute_optional( "javac"                 , _spooler->java_subsystem()->java_vm()->javac_filename() );
        stdin_xml_writer.set_attribute_optional( "java_work_dir"         , _spooler->java_work_dir() );
        stdin_xml_writer.set_attribute_optional( "scheduler.directory"   , _spooler->directory() );      // Für Com_spooler_proxy::get_Directory
        stdin_xml_writer.set_attribute_optional( "scheduler.log_dir"     , _spooler->_log_directory );   // Für Com_spooler_proxy::get_Log_dir
        stdin_xml_writer.set_attribute_optional( "scheduler.include_path", _spooler->_include_path );    // Für Com_spooler_proxy::get_Include_path
        stdin_xml_writer.set_attribute_optional( "scheduler.ini_path"    , _spooler->_factory_ini );     // Für Com_spooler_proxy::get_Ini_path

        if( object_server::Connection_to_own_server_process* c = dynamic_cast<object_server::Connection_to_own_server_process*>( connection ) )
        {
            stdin_xml_writer.set_attribute( "stdout_path", c->stdout_path() );
            stdin_xml_writer.set_attribute( "stderr_path", c->stderr_path() );

            if( _log_stdout_and_stderr )
                stdin_xml_writer.set_attribute( "log_stdout_and_stderr", "yes" );
        }

        if( _environment )  
        {
            xml::Document_ptr dom_document = _environment->dom( "environment", "variable" );
            stdin_xml_writer.write_element( dom_document.documentElement() );
        }
    }

    stdin_xml_writer.end_element( "task_process" );
    stdin_xml_writer.flush();

    connection->set_stdin_data( stdin_xml_writer.to_string() );

    if( _controller_address )  connection->set_controller_address( _controller_address );
}

//----------------------------------------------------------------------Process::async_remote_start

void Process::async_remote_start()
{
    ptr<object_server::Connection> c = _spooler->_connection_manager->new_connection();

    assert( _process_class );
    c->set_remote_host( _remote_scheduler._host );
    c->listen_on_tcp_port( INADDR_ANY );


    _connection = +c;

    _async_remote_operation = Z_NEW( Async_remote_operation( this ) );
    _async_remote_operation->async_wake();
    _async_remote_operation->set_async_manager( _spooler->_connection_manager );
}

//------------------------------------------------------------------------------Process::is_started

//bool Process::is_started()
//{
//    bool result = false;
//
//    if( _async_remote_operation )
//    {
//        _async_remote_operation->async_check_exception( Z_FUNCTION );
//
//        result = _async_remote_operation->async_finished();
//     
//        //if( result )
//        //{
//        //    _async_remote_operation->set_async_manager( NULL );
//        //    _async_remote_operation = NULL;
//        //}
//    }
//    else
//    {
//        result = true;
//    }
//
//    return result;
//}

//-------------------------------------------------------------Process::async_remote_start_continue

bool Process::async_remote_start_continue( Async_operation::Continue_flags )
{
    bool something_done = true;     // spooler.cxx ruft async_continue() auf

    if( _xml_client_connection )  _xml_client_connection->async_check_exception();

    switch( _async_remote_operation->_state )
    {
        case Async_remote_operation::s_not_connected:
        {
            // Hier fehlt noch das Register für gemeinsame Benutzung

            _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _remote_scheduler ) );
            _xml_client_connection->set_async_parent( _async_remote_operation );
            _xml_client_connection->set_async_manager( _spooler->_connection_manager );
            _xml_client_connection->set_wait_for_connection( connection_retry_time );
            _xml_client_connection->connect();

            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_connecting;
        }

        case Async_remote_operation::s_connecting:
        {
            if( _xml_client_connection->state() != Xml_client_connection::s_connected )  break;

            xml::Xml_string_writer xml_writer;
            xml_writer.set_encoding( scheduler_character_encoding );
            xml_writer.write_prolog();
            xml_writer.begin_element( "remote_scheduler.start_remote_task" );
            xml_writer.set_attribute( "tcp_port", _connection->tcp_port() );
            if( _module_instance->_module->kind() == Module::kind_process )  xml_writer.set_attribute( "kind", "process" );
            xml_writer.end_element( "remote_scheduler.start_remote_task" );
            xml_writer.close();
            _xml_client_connection->send( xml_writer.to_string() );

            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_starting;
        }

        case Async_remote_operation::s_starting:
        {
            xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document();
            if( !dom_document )  break;

            Z_LOG2( "scheduler", Z_FUNCTION << " XML-Antwort: " << dom_document.xml() );

            //if( _spooler->_validate_xml )  _spooler->_schema.validate( dom_document );

            _remote_process_id = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "process_id", 0 );
            _remote_pid        = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "pid", 0 );

            _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_running;
        }

        case Async_remote_operation::s_running:    
        {
            if( _module_instance  &&  _module_instance->_task )  _module_instance->_task->signal( Z_FUNCTION );
            break;
        }

        case Async_remote_operation::s_closing:
        {
            if( xml::Document_ptr dom_document = _xml_client_connection->fetch_received_dom_document() )  
            {
                Z_LOG2( "joacim", Z_FUNCTION << " XML-Antwort: " << dom_document.xml() );
                //_spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert

                //_remote_stdout_file.open_temporary( File::open_unlink_later );
                //_remote_stdout_file.print( string_from_hex( dom_document.select_element_strict( "//ok/file [ @name='stdout' and @encoding='hex' ]" ).text() ) );
                //_remote_stdout_file.close();

                //_remote_stderr_file.open_temporary( File::open_unlink_later );
                //_remote_stderr_file.print( string_from_hex( dom_document.select_element_strict( "//ok/file [ @name='stderr' and @encoding='hex' ]" ).text() ) );
                //_remote_stderr_file.close();

                something_done = true;
                _async_remote_operation->_state = Async_remote_operation::s_closed;
            }

            break;
        }

        default:
            break;
    }

    return something_done;
}

//--------------------------------------------------------------------------Process::async_continue

bool Process::async_continue()
{
    return _connection? _connection->async_continue() 
                      : false;
}

//-----------------------------------------------Process::Async_remote_operation::close_remote_task

void Process::Async_remote_operation::close_remote_task( bool kill )
{
    if( _state <= s_connecting )
    {
        if( _process->_xml_client_connection )  
        {
            _process->_xml_client_connection->close();
            _process->_xml_client_connection->set_async_parent( NULL );
            _process->_xml_client_connection = NULL;
            _state = s_closed;
        }
    }
    else
    if( _state >= s_starting  &&  _state < s_closing )
    {
        if( _process->_xml_client_connection  &&  _process->_xml_client_connection->is_send_possible() )
        {
            try
            {
                S xml;
                xml << "<remote_scheduler.remote_task.close process_id='" << _process->_remote_process_id << "'";
                if( kill )  xml << " kill='yes'";
                xml << "/>";

                _process->_xml_client_connection->send( xml );

                _state = s_closing;
            }
            catch( exception& x )  { _process->_log->warn( x.what() ); }
        }
        else
        { 
            //_process->_xml_client_connection->close();
            //_state = s_closed;
        }
    }
}

//--------------------------------------------------------------------------------Process::end_task

void Process::end_task()
{
    assert( _module_instance );

    if( _module_instance )  _module_instance->end_task();
}

//------------------------------------------------------------------------------------Process::kill

bool Process::kill()
{
    bool result = false;

    if( !_is_killed  &&  _connection )
    {
        if( is_remote_host() )
        {
            // Async_operation (vorhandene oder neue, besser neue)
            // mit <kill_task> starten. Nur, wenn noch nicht gestartet
            // if( !_kill_task_operation )   _kill_task_operation = Z_NEW( Kill_task_operation );
            // <remote_scheduler.task.close pid=" _remote_pid

            if( _async_remote_operation  &&  _async_remote_operation->_state != Async_remote_operation::s_closed )
                _async_remote_operation->close_remote_task( true );

            result = true;
        }
        else
        if( object_server::Connection_to_own_server_process* c = dynamic_cast<object_server::Connection_to_own_server_process*>( +_connection ) )
        {
            result = c->kill_process();
        }
        else
        if( dynamic_cast<object_server::Connection_to_own_server_thread*>( +_connection ) )
        {
            if( pid_t pid = this->pid() )
                try_kill_process_immediately( pid );   // Für kind_remote kind_process (Process_module_instance)
        }
    }

    if( result )  _is_killed = true;

    return result;
}

//-------------------------------------------------------------------------------------Process::pid

int Process::pid() const
{ 
    int result = 0;

    //if( _module_instance  &&  _module_instance->kind() == Module::kind_process )
    //{
    //    if( _connection )  assert( dynamic_cast<object_server::Connection_to_own_server_thread*>( +_connection ) );
    //    result = _module_instance->pid();
    //}
    //else
    if( _connection )
    {
        if( _com_server_thread )
        {
            if( ptr<Object_server> object_server = _com_server_thread->_object_server )
            {
                // Hmm, scheint der einzige Weg zu sein, an die Pid der Process_module_instance heranzukommen.

                Object* o = object_server->get_class_object_or_null( spooler_com::CLSID_Remote_module_instance_server );

                if( Com_remote_module_instance_server::Class_data* class_data = dynamic_cast<Com_remote_module_instance_server::Class_data*>( o ) )
                {
                    result = class_data->_remote_instance_pid;
                }
            }
        }
        else
        {
            //assert( _module_instance  &&  _module_instance->kind() == Module::kind_remote );
            result = _connection->pid();
        }
    }

    return result;
}

//---------------------------------------------------------------------------Process::is_terminated

bool Process::is_terminated()
{
    return !_connection  ||  _connection->process_terminated();
}

//-------------------------------------------------------------------------------Process::exit_code

int Process::exit_code()
{
    if( _connection )  _exit_code = _connection->exit_code();

    return _exit_code;
}

//-----------------------------------------------------------------------Process::termination_signal

int Process::termination_signal()
{
    if( _connection )  _termination_signal = _connection->termination_signal();

    return _termination_signal;
}

//--------------------------------------------------------------------------Process::is_remote_host

bool Process::is_remote_host() const
{ 
    return _remote_scheduler; 
}

//-----------------------------------------------------------------------------Process::stdout_path

File_path Process::stdout_path()
{
    File_path result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->stdout_path();
    }
    //else
    //if( object_server::Connection_to_own_server_thread* c = dynamic_cast< object_server::Connection_to_own_server_thread* >( +_connection ) )
    //{
    //    assert(!"Connection_to_own_server_thread");
    //}
    //else
    //    result = _remote_stdout_file.path();

    return result;
}

//-----------------------------------------------------------------------------Process::stderr_path

File_path Process::stderr_path()
{
    File_path result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->stderr_path();
    }
    //else
    //if( object_server::Connection_to_own_server_thread* c = dynamic_cast< object_server::Connection_to_own_server_thread* >( +_connection ) )
    //{
    //    assert(!"Connection_to_own_server_thread");
    //}
    //else
    //    result = _remote_stderr_file.path();

    return result;
}

//---------------------------------------------------------------------Process::delete_files__start

bool Process::try_delete_files( Has_log* log )
{
    bool result = true;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->try_delete_files( log );
    }
    //else
    //{
    //    if( _remote_stdout_file.is_to_be_unlinked() )  result &= _remote_stdout_file.try_unlink();
    //    if( _remote_stderr_file.is_to_be_unlinked() )  result &= _remote_stderr_file.try_unlink();
    //}

    return result;
}

//-------------------------------------------------------------------------Process::undeleted_files

list<File_path> Process::undeleted_files()
{
    list<File_path> result;

    if( object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection ) )
    {
        result = c->undeleted_files();
    }
    //else
    //{
    //    if( _remote_stdout_file.is_to_be_unlinked() )  result.push_back( _remote_stdout_file.path() );
    //    if( _remote_stderr_file.is_to_be_unlinked() )  result.push_back( _remote_stderr_file.path() );
    //}

    return result;
}

//-----------------------------------------------------------------------------Process::dom_element

xml::Element_ptr Process::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr process_element = document.createElement( "process" );

    if( _connection )
    process_element.setAttribute( "pid"              , _connection->pid() );
  //process_element.setAttribute( "module_instances" , _module_instance_count );

    if( !_job_name.empty() )
    process_element.setAttribute( "job"              , _job_name );

    if( _task_id )
    process_element.setAttribute( "task"             , _task_id ),
    process_element.setAttribute( "task_id"          , _task_id );          // VERALTET!
    process_element.setAttribute_optional( "remote_scheduler", _remote_scheduler.as_string() );

    if( _connection )
    {
        process_element.setAttribute( "operations", _connection->operation_count() );
        process_element.setAttribute( "callbacks", _connection->callback_count() );
    }

    process_element.setAttribute( "running_since", _running_since.as_string() );

    Async_operation* operation = _connection? _connection->current_super_operation() : NULL;
    if( operation )
    process_element.setAttribute( "operation"        , operation->async_state_text() );

    return process_element;
}

//--------------------------------------------------------------------------------Process::obj_name

string Process::obj_name() const
{
    return "Process " + short_name();
}

//------------------------------------------------------------------------------Process::short_name

string Process::short_name() const
{
    S result;

    if( _connection )  result << _connection->short_name();
    if( _remote_pid )  result << ",pid=" << _remote_pid;

    return result;
}

//-----------------------------------------Process_class_configuration::Process_class_configuration

Process_class_configuration::Process_class_configuration( Scheduler* scheduler, const string& name )
: 
    Idispatch_implementation( &class_descriptor ),
    file_based<Process_class_configuration,Process_class_folder,Process_class_subsystem>( scheduler->process_class_subsystem(), static_cast< spooler_com::Iprocess_class* >( this ), type_process_class ),
    _zero_(this+1),
    _max_processes(30)
{ 
    if( name != "" )  set_name( name );
}

//---------------------------------------------------Process_class_configuration::set_max_processes

void Process_class_configuration::set_max_processes( int max_processes )
{
    if( max_processes < 0 )  z::throw_xc( "SCHEDULER-420", "Process_class.max_processes", max_processes );
    check_max_processes( max_processes );
    
    _max_processes = max_processes;
}

//------------------------------------------------Process_class_configuration::set_remote_scheduler

void Process_class_configuration::set_remote_scheduler( const Host_and_port& remote_scheduler )
{
    check_remote_scheduler( remote_scheduler );

    _remote_scheduler = remote_scheduler;
    if( _remote_scheduler._host  &&  _remote_scheduler._port == 0 )  _remote_scheduler._port = _spooler->_tcp_port;
}

//------------------------------------------------------------Process_class_configuration::obj_name

string Process_class_configuration::obj_name() const
{
    S result;

    result << "Process_class " << path().without_slash();

    return result;
}

//-------------------------------------------------------------Process_class_configuration::set_dom

void Process_class_configuration::set_dom( const xml::Element_ptr& e )
{
    if( !e )  return;
    if( !e.nodeName_is( "process_class" ) )  z::throw_xc( "SCHEDULER-409", "process_class", e.nodeName() );

    clear_source_xml();

    string name = e.getAttribute( "name" );
    if( name != "" )  set_name( name );         // Leere Name steht für die Default-Prozessklasse

    set_max_processes   ( (int)e.uint_getAttribute( "max_processes"   , _max_processes ) );
    set_remote_scheduler(      e.     getAttribute( "remote_scheduler", _remote_scheduler.as_string() ) );
}

//---------------------------------------------------------Process_class_configuration::dom_element

xml::Element_ptr Process_class_configuration::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = document.createElement( "process_class" );
        
    fill_file_based_dom_element( result, show_what );
  //result.setAttribute         ( "name"            , name() );
    result.setAttribute         ( "max_processes"   , _max_processes );
    result.setAttribute_optional( "remote_scheduler", _remote_scheduler.as_string() );

    return result;
}

//------------------------------------------------------------Process_class_configuration::put_Name

STDMETHODIMP Process_class_configuration::put_Name( BSTR name_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_name( string_from_bstr( name_bstr ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------Process_class_configuration::put_Remote_scheduler

STDMETHODIMP Process_class_configuration::put_Remote_scheduler( BSTR remote_scheduler_bstr )
{
    HRESULT hr = S_OK;

    try
    {
        set_remote_scheduler( string_from_bstr( remote_scheduler_bstr ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//---------------------------------------------------Process_class_configuration::put_Max_processes

STDMETHODIMP Process_class_configuration::put_Max_processes( int max_processes )
{
    HRESULT hr = S_OK;

    try
    {
        set_max_processes( max_processes );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//--------------------------------------------------------------Process_class_configuration::Remove

STDMETHODIMP Process_class_configuration::Remove()
{
    HRESULT hr = S_OK;

    try
    {
        remove( File_based::rm_base_file_too );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------------------------Process_class::Process_class

Process_class::Process_class( Scheduler* scheduler, const string& name )
:
    Process_class_configuration( scheduler, name ),
    _zero_(this+1)
{
}

//--------------------------------------------------------------------Process_class::~Process_class
    
Process_class::~Process_class()
{
    Z_DEBUG_ONLY( assert( _process_set.empty() ) );
    Z_DEBUG_ONLY( assert( _waiting_jobs.empty() ) );

    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << " ERROR " << x.what() << "\n" ); }
}

//-----------------------------------------------------------------Process_class::set_configuration

void Process_class::set_configuration( const Process_class_configuration& configuration )
{
    if( normalized_path() != configuration.normalized_path() )  assert(0), z::throw_xc( Z_FUNCTION );

    // Erst die Warnungen
    check_max_processes   ( configuration.max_processes() );
    check_remote_scheduler( configuration.remote_scheduler() );

    // Jetzt ändern. Es sollte keine Exception auftreten.
    set_max_processes   ( configuration.max_processes() );
    set_remote_scheduler( configuration.remote_scheduler() );
}

//-----------------------------------------------------------------------------Process_class::close

void Process_class::close()
{
    Z_FOR_EACH( Process_set, _process_set, it )
    {
        if( Process* process = *it )
        {
            log()->warn( message_string( "SCHEDULER-871", process->obj_name() ) );      // Das sollte nicht passieren
        }
    }
}

//---------------------------------------------------------------------Process_class::on_initialize

bool Process_class::on_initialize()
{
    return true;
}

//---------------------------------------------------------------------------Process_class::on_load

bool Process_class::on_load()
{
    return true;
}

//-----------------------------------------------------------------------Process_class::on_activate

bool Process_class::on_activate()
{
    return true;
}

//----------------------------------------------------------------Process_class::can_be_removed_now

bool Process_class::can_be_removed_now()
{
    return _process_set.empty();
}

//----------------------------------------------------------------Process_class::prepare_to_replace

void Process_class::prepare_to_replace()
{
    assert( replacement() );
}

//---------------------------------------------------------------Process_class::can_be_replaced_now

bool Process_class::can_be_replaced_now()
{
    return true;
}

//--------------------------------------------------------------------Process_class::on_replace_now

Process_class* Process_class::on_replace_now()
{
    set_configuration( *replacement() );
    set_replacement( NULL );

    return this;
}

//---------------------------------------------------------------Process_class::check_max_processes

void Process_class::check_max_processes( int ) const
{
    //if( _remove )  z::throw_xc( "SCHEDULER-421", obj_name() );
}

//-----------------------------------------------------------------Process_class::set_max_processes

void Process_class::set_max_processes( int max_processes )
{
    // Keine Exception bei Aufruf aus set_configuration() auslösen!

    if( _process_set.size() > max_processes )  log()->warn( message_string( "SCHEDULER-419", max_processes, _process_set.size() ) );

    _max_processes = max_processes;
}

//------------------------------------------------------------Process_class::check_remote_scheduler

void Process_class::check_remote_scheduler( const Host_and_port& ) const
{
    //if( _remove )  z::throw_xc( "SCHEDULER-421", obj_name() );
}

//-----------------------------------------------------------------------Process_class::add_process

void Process_class::add_process( Process* process )
{
    process->_process_class = this;
    _process_set.insert( process );
}

//--------------------------------------------------------------------------Spooler::remove_process

void Process_class::remove_process( Process* process )
{
    Process_set::iterator it = _process_set.find( process );
    if( it == _process_set.end() )  z::throw_xc( Z_FUNCTION );

    process->_process_class = NULL; 
    _process_set.erase( it ); 

    if( is_to_be_removed()  &&  can_be_removed_now() )
    {
        remove();
        // this ist ungültig!
    }
    else
    if( !_waiting_jobs.empty() )
    {
        Job* job = *_waiting_jobs.rbegin();
        job->notify_a_process_is_idle();
    }
}

//------------------------------------------------------------------------Process_class::new_process

Process* Process_class::new_process()
{
    assert_is_active();

    ptr<Process> process;

    process = Z_NEW( Process( _spooler ) );        

    process->set_temporary( true );      // Zunächst nach der Task beenden. (Problem mit Java, 1.9.03)
  //process->start();

    add_process( process );

    return process;
}

//--------------------------------------------------------Process_class::select_process_if_available

Process* Process_class::select_process_if_available()
{
    Process* process = NULL;

    if( !is_to_be_removed()  &&                                    // remove_process() könnte sonst Process_class löschen.
        file_based_state() == File_based::s_active )  
    {
        FOR_EACH( Process_set, _process_set, p )
        {
            if( (*p)->_module_instance_count == 0 )  { process = *p; break; }
        }

        if( process )
        {
            if( process->_connection && process->_connection->has_error() )
            {
                _spooler->log()->warn( message_string( "SCHEDULER-299", process->short_name() ) );   // "Prozess pid=$1 wird nach Fehler entfernt"

                process->kill();
                remove_process( process );
                process = NULL;
            }
        }

        if( !process  
         && _process_set.size()      < _max_processes  
         && _spooler->_process_count < scheduler::max_processes  )  return new_process();
    }

    return process;
}

//-----------------------------------------------------------------Process_class::process_available

bool Process_class::process_available( Job* for_job )
{ 
    if( file_based_state() != File_based::s_active )  return false;
    if( _process_set.size()      >= _max_processes )  return false;
    if( _spooler->_process_count >= scheduler::max_processes )  return false;

    if( _waiting_jobs.empty() )  return true;

    // Warten Jobs auf einen freien Prozess? 
    // Dann liefern wir nur true, wenn dieser Job der erste in der Warteschlange ist.
    return *_waiting_jobs.rbegin() == for_job;
}

//---------------------------------------------------------------Process_class::enqueue_waiting_job

void Process_class::enqueue_waiting_job( Job* job )
{
    _waiting_jobs.push_back( job );
}

//----------------------------------------------------------------Process_class::remove_waiting_job

void Process_class::remove_waiting_job( Job* waiting_job )
{
    _waiting_jobs.remove( waiting_job );

    if( _process_set.size() < _max_processes  &&  !_waiting_jobs.empty() )
    {
        Job* job = *_waiting_jobs.rbegin();
        job->notify_a_process_is_idle();
    }
}

//----------------------------------------------------------------------Process_class::need_process

bool Process_class::need_process()
{ 
/*
    for( Job_list::iterator j = _waiting_jobs.begin(); j != _waiting_jobs.end(); )
    {
        if( !(*j)->_waiting_for_process )  _waiting_jobs.erase( j );   // Hat sich erledigt
                                     else  j++;
    }
*/
    return !_waiting_jobs.empty(); 
}

//----------------------------------------------------------Process_class::notity_a_process_is_idle

void Process_class::notify_a_process_is_idle()
{
    if( !_waiting_jobs.empty() )  (*_waiting_jobs.begin())->notify_a_process_is_idle();
}

//-----------------------------------------------------------------------Process_class::dom_element

xml::Element_ptr Process_class::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = Process_class_configuration::dom_element( document, show_what );

    result.setAttribute( "processes", (int)_process_set.size() );

    if( !_process_set.empty() )
    {
        xml::Element_ptr processes_element = result.append_new_element( "processes" );
        FOR_EACH( Process_set, _process_set, it )  processes_element.appendChild( (*it)->dom_element( document, show_what ) );
    }

    if( !_waiting_jobs.empty() )
    {
        xml::Element_ptr waiting_jobs_element = document.createElement( "waiting_jobs" );
        result.appendChild( waiting_jobs_element );

        FOR_EACH( Job_list, _waiting_jobs, j )  //waiting_jobs_element.appendChild( (*j)->dom_element( document, show_standard ) );
        {
            xml::Element_ptr job_element = document.createElement( "job" );
            job_element.setAttribute( "job", (*j)->name() );
            waiting_jobs_element.appendChild( job_element );
        }
    }

    return result;
}

//-----------------------------------------------------------------------Process_class::execute_xml

xml::Element_ptr Process_class::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& )
{
    if( element.nodeName_is( "process_class.remove" ) ) 
    {
        remove( File_based::rm_base_file_too );
    }
    else
        z::throw_xc( "SCHEDULER-409", "process_class.remove", element.nodeName() );

    return command_processor->_answer.createElement( "ok" );
}

//-------------------------------------------------------Process_class_folder::Process_class_folder

Process_class_folder::Process_class_folder( Folder* folder )
:
    typed_folder<Process_class>( folder->spooler()->process_class_subsystem(), folder, type_process_class_folder )
{
}

//------------------------------------------------------Process_class_folder::~Process_class_folder

Process_class_folder::~Process_class_folder()
{
}

//-------------------------------------------------Process_class_subsystem::Process_class_subsystem

Process_class_subsystem::Process_class_subsystem( Scheduler* scheduler )
:
    _zero_(this+1),
    Idispatch_implementation( &class_descriptor ),
    file_based_subsystem<Process_class>( scheduler, static_cast<Iprocess_classes*>( this ), type_process_class_subsystem )
{
}

//----------------------------------------------------Process_class_subsystem::subsystem_initialize

bool Process_class_subsystem::subsystem_initialize()
{
    set_subsystem_state( subsys_initialized );
    file_based_subsystem<Process_class>::subsystem_initialize();

    spooler()->root_folder()->process_class_folder()->add_process_class( Z_NEW( Process_class( spooler() ) ) );     // Default-Prozessklasse ohne Namen
    spooler()->root_folder()->process_class_folder()->add_process_class( Z_NEW( Process_class( spooler(), temporary_process_class_name ) ) );

    return true;
}

//----------------------------------------------------------Process_class_subsystem::subsystem_load

bool Process_class_subsystem::subsystem_load()
{
    set_subsystem_state( subsys_loaded );
    file_based_subsystem<Process_class>::subsystem_load();
    return true;
}

//------------------------------------------------------Process_class_subsystem::subsystem_activate

bool Process_class_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Process_class>::subsystem_activate();

    return true;
}

//-------------------------------------------------------------------Process_class_subsystem::close

void Process_class_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Process_class>::close();
}

//----------------------------------------------------------Process_class_subsystem::async_continue

bool Process_class_subsystem::async_continue()
{
    bool something_done = false;

    FOR_EACH_FILE_BASED( Process_class, process_class )
    {
        FOR_EACH( Process_class::Process_set, process_class->_process_set, p )
        {
            something_done |= (*p)->async_continue();
        }
    }

    return something_done;
}

//---------------------------------------------------Process_class_subsystem::new_temporary_process

Process* Process_class_subsystem::new_temporary_process()
{
    ptr<Process> process = Z_NEW( Process( _spooler ) );

    process->set_temporary( true );
    temporary_process_class()->add_process( process );

    return process;
}

//-------------------------------------------------------------Process_class_subsystem::execute_xml

xml::Element_ptr Process_class_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "process_class" ) ) 
    {
        //result = spooler()->root_folder()->process_class_folder()->execute_xml_process_class( command_processor, element );
        spooler()->root_folder()->process_class_folder()->add_or_replace_file_based_xml( element );
        result = command_processor->_answer.createElement( "ok" );
    }
    else
    if( string_begins_with( element.nodeName(), "process_class." ) ) 
    {
        result = process_class( Absolute_path( root_path, element.getAttribute( "process_class" ) ) )->execute_xml( command_processor, element, show_what );
    }
    else
        z::throw_xc( "SCHEDULER-113", element.nodeName() );

    return result;
}

//-------------------------------------------------------Process_class_subsystem::get_Process_class

STDMETHODIMP Process_class_subsystem::get_Process_class( BSTR path_bstr, spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = process_class( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-----------------------------------------------Process_class_subsystem::get_Process_class_or_null

STDMETHODIMP Process_class_subsystem::get_Process_class_or_null( BSTR path_bstr, spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        *result = process_class_or_null( Absolute_path( root_path, string_from_bstr( path_bstr ) ) );
        if( *result )  (*result)->AddRef();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//----------------------------------------------------Process_class_subsystem::Create_process_class

STDMETHODIMP Process_class_subsystem::Create_process_class( spooler_com::Iprocess_class** result )
{
    HRESULT hr = S_OK;

    try
    {
        ptr<Process_class> process_class = Z_NEW( Process_class( spooler() ) );
        //nicht nötig  process_class->set_folder_path( root_path );
        *result = process_class.take();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------Process_class_subsystem::Add_process_class

STDMETHODIMP Process_class_subsystem::Add_process_class( spooler_com::Iprocess_class* iprocess_class )
{
    HRESULT hr = S_OK;

    try
    {
        Process_class* process_class = dynamic_cast<Process_class*>( iprocess_class );
        if( !process_class )  return E_POINTER;

        Folder* folder = spooler()->root_folder();
        process_class->set_folder_path( folder->path() );
        process_class->initialize();

        Process_class* current_process_class = process_class_or_null( process_class->path() );
        if( current_process_class  &&  current_process_class->is_to_be_removed() )
        {
            current_process_class->replace_with( process_class );
        }
        else
        {
            folder->process_class_folder()->add_process_class( process_class );
            process_class->activate();
        }
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------Process_class_subsystem::temporary_process_class

Process_class* Process_class_subsystem::temporary_process_class()
{ 
    return spooler()->root_folder()->process_class_folder()->process_class( temporary_process_class_name ); 
}

//-----------------------------------------------------Process_class_subsystem::try_to_free_process

//bool Process_class_subsystem::try_to_free_process( Job* for_job, Process_class* process_class, const Time& now )
//{
//    return _spooler->task_subsystem()->try_to_free_process( for_job, process_class, now );
//}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
