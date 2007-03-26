// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
// §1172
// §1206

#include "spooler.h"


namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const
    
const int connection_retry_time = 60;

//---------------------------------------------------------rocess::Close_operation::Close_operation

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

//---------------------------------------------------------------------------------Process::Process

Process::Process( Spooler* sp )
: 
    Scheduler_object( sp, this, type_process ), 
    _zero_(this+1)
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
    Async_operation* result = NULL;

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
    if( _module_instance_count != 0 )  throw_xc( "Process::add_module_instance" );

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

        if( _process_class )  
        {
            _process_class->remove_process( this );
        }
    }
}

//-----------------------------------------------------------------------------------Process::start

void Process::start()
{
    if( is_remote_host() )
    {
        assert( _process_class );
        async_remote_start();
    }
    else
    {
        start_local();
    }

#   ifdef Z_WINDOWS
        _connection->set_event( &_spooler->_event );
#   endif

    _log->set_prefix( obj_name() );

    _session = Z_NEW( object_server::Session( _connection ) );
    _session->set_connection_has_only_this_session();

    _running_since = Time::now();
}

//-----------------------------------------------------------------------------Process::start_local
    
void Process::start_local()
{
    if( started() )  throw_xc( __FUNCTION__ );


    if( !_server_hostname.empty() )
    {
        assert(0); // War ein experiment
        //_connection = Z_NEW( object_server::Connection( _spooler->_connection_manager ) );
        //_connection->connect( _server_hostname, _server_port );
        //_connection->set_async();
    }
    else
    {
        ptr<object_server::Connection_to_own_server_process> c = _spooler->_connection_manager->new_connection_to_own_server_process();
        c->open_stdout_stderr_files();


        object_server::Parameters parameters;

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

       
        io::String_writer string_writer;
        xml::Xml_writer xml_writer ( &string_writer );
        xml_writer.begin_element( "task_process" );
        {
            xml_writer.set_attribute_optional( "include_path"   , _spooler->include_path() );
            xml_writer.set_attribute_optional( "java_options"   , _spooler->_config_java_options );
            xml_writer.set_attribute_optional( "java_class_path", _spooler->java_subsystem()->java_vm()->class_path() );
            xml_writer.set_attribute_optional( "javac"          , _spooler->java_subsystem()->java_vm()->javac_filename() );
            xml_writer.set_attribute_optional( "java_work_dir"  , _spooler->java_work_dir() );
            xml_writer.set_attribute         ( "stdout_path"    , c->stdout_path() );
            xml_writer.set_attribute         ( "stderr_path"    , c->stderr_path() );
        }
        xml_writer.end_element( "task_process" );
        xml_writer.flush();



        c->set_stdin_data    ( string_writer.to_string() );
        //c->attach_stdout_file( _stdout_file );
        //c->attach_stderr_file( _stderr_file );
        c->set_priority      ( _priority );
        if( _controller_address )  c->set_controller_address( _controller_address );


        c->start_process( parameters );


        _connection = +c;

        _process_handle_copy = _connection->process_handle();
        _spooler->register_process_handle( _process_handle_copy );
    }

    _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
}

//----------------------------------------------------------------------Process::async_remote_start

void Process::async_remote_start()
{
    ptr<object_server::Connection> c = _spooler->_connection_manager->new_connection();

  //c->set_stdin_data( _task_process_xml );
    assert( _process_class );
    c->set_remote_host( _process_class->_remote_scheduler._host );
    c->listen_on_tcp_port( INADDR_ANY );


    _connection = +c;

    _async_remote_operation = Z_NEW( Async_remote_operation( this ) );
    _async_remote_operation->async_wake();
    _async_remote_operation->set_async_manager( _spooler->_connection_manager );
}

//------------------------------------------------------------------------------Process::is_started

bool Process::is_started()
{
    bool result = false;

    if( _async_remote_operation )
    {
        _async_remote_operation->async_check_exception( __FUNCTION__ );

        result = _async_remote_operation->async_finished();
     
        //if( result )
        //{
        //    _async_remote_operation->set_async_manager( NULL );
        //    _async_remote_operation = NULL;
        //}
    }
    else
    {
        result = true;
    }

    return result;
}

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

            _xml_client_connection = Z_NEW( Xml_client_connection( _spooler, _process_class->_remote_scheduler ) );
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

            // Xml_writer benutzen
            // password übergeben?
            string xml_text = S() << "<remote_scheduler.start_remote_task tcp_port='" << _connection->tcp_port() << "'/>";
            _xml_client_connection->send( xml_text );

            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_starting;
        }

        case Async_remote_operation::s_starting:
        {
            xml::Document_ptr dom_document = _xml_client_connection->received_dom_document();
            if( !dom_document )  break;

            Z_LOG2( "scheduler", __FUNCTION__ << " XML-Antwort: " << dom_document.xml() );

            //if( _spooler->_validate_xml )  _spooler->_schema.validate( dom_document );

            _remote_pid = dom_document.select_element_strict( "spooler/answer/process" ).int_getAttribute( "pid" );

            _spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert
            something_done = true;
            _async_remote_operation->_state = Async_remote_operation::s_running;
        }

        case Async_remote_operation::s_running:    
        {
            if( _module_instance  &&  _module_instance->_task )  _module_instance->_task->signal( __FUNCTION__ );
            break;
        }

        case Async_remote_operation::s_closing:
        {
            if( xml::Document_ptr dom_document = _xml_client_connection->received_dom_document() )  
            {
                Z_LOG2( "joacim", __FUNCTION__ << " XML-Antwort: " << dom_document.xml() );
                //_spooler->log()->debug9( message_string( "SCHEDULER-948", _connection->short_name() ) );  // pid wird auch von Task::set_state(s_starting) mit log_info protokolliert

                _remote_stdout_file.open_temporary( File::open_unlink_later );
                _remote_stdout_file.print( string_from_hex( dom_document.select_element_strict( "//ok/file [ @name='stdout' and @encoding='hex' ]" ).text() ) );
                _remote_stdout_file.close();

                _remote_stderr_file.open_temporary( File::open_unlink_later );
                _remote_stderr_file.print( string_from_hex( dom_document.select_element_strict( "//ok/file [ @name='stderr' and @encoding='hex' ]" ).text() ) );
                _remote_stderr_file.close();

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
    if( _process->_xml_client_connection )
    {
        if( _process->_xml_client_connection->is_send_possible() )
        {
            try
            {
                S xml;
                xml << "<remote_scheduler.remote_task.close pid='" << _process->_remote_pid << "'";
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
    }

    if( result )  _is_killed = true;

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
    return _process_class &&  _process_class->_remote_scheduler; 
}

//-----------------------------------------------------------------------------Process::stdout_path

File_path Process::stdout_path()
{
    object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection );
    return c? c->stdout_path() : _remote_stdout_file.path();
}

//-----------------------------------------------------------------------------Process::stderr_path

File_path Process::stderr_path()
{
    object_server::Connection_to_own_server_process* c = dynamic_cast< object_server::Connection_to_own_server_process* >( +_connection );
    return c? c->stderr_path() : _remote_stderr_file.path();
}

//-----------------------------------------------------------------------------Process::dom_element

xml::Element_ptr Process::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr process_element = document.createElement( "process" );

    //process_element.setAttribute( "name"           , _name );

    if( _connection )
    process_element.setAttribute( "pid"              , _connection->pid() );
  //process_element.setAttribute( "module_instances" , _module_instance_count );

    if( !_job_name.empty() )
    process_element.setAttribute( "job"              , _job_name );

    if( _task_id )
    process_element.setAttribute( "task"             , _task_id ),
    process_element.setAttribute( "task_id"          , _task_id );          // VERALTET!

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

//------------------------------------------------------------------------------Process_class::init

void Process_class::init()
{
    _max_processes = 10;
}

//-----------------------------------------------------------------------Process_class::add_process

void Process_class::add_process( Process* process )
{
    process->_process_class = this;
    _process_list.push_back( process );
}

//--------------------------------------------------------------------------Spooler::remove_process

void Process_class::remove_process( Process* process )
{
    FOR_EACH( Process_list, _process_list, p )
    {
        if( *p == process )  
        { 
            process->_process_class = NULL; 
            _process_list.erase( p ); 

            if( !_waiting_jobs.empty() )
            {
                Job* job = *_waiting_jobs.rbegin();
                job->notify_a_process_is_idle();
                //job->signal( S() << __FUNCTION__  << "  " << process->obj_name() );
            }

            return; 
        }
    }

    throw_xc( "Process_class::remove_process" );
}

//------------------------------------------------------------------------Process_class::new_process

Process* Process_class::new_process()
{
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

    FOR_EACH( Process_list, _process_list, p )
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
     && _process_list.size()      < _max_processes  
     && _spooler->_process_count  < max_processes  )  return new_process();

    return process;
}

//-----------------------------------------------------------------Process_class::process_available

bool Process_class::process_available( Job* for_job )
{ 
    if( _process_list.size()     >= _max_processes )  return false;
    if( _spooler->_process_count >= max_processes )  return false;

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

    if( _process_list.size() < _max_processes  &&  !_waiting_jobs.empty() )
    {
        Job* job = *_waiting_jobs.rbegin();
        //job->signal( S() << __FUNCTION__ << "  " << waiting_job->obj_name() );
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

//---------------------------------------------------------------------------Process_class::set_dom

void Process_class::set_dom( const xml::Element_ptr& e )
{
    if( !e )  return;

    if( _name.empty() )     // neu?
    {
        _name = e.getAttribute( "name" );
    }

    _max_processes = (int)e.uint_getAttribute( "max_processes", _max_processes );

    _remote_scheduler = e.getAttribute( "remote_scheduler", _remote_scheduler.as_string() );
    if( _remote_scheduler._host  &&  _remote_scheduler._port == 0 )  _remote_scheduler._port = _spooler->_tcp_port;
}

//----------------------------------------------------------------------------------Spooler::as_dom
// Anderer Thread

xml::Element_ptr Process_class::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr element = document.createElement( "process_class" );
        
    element.setAttribute         ( "name"            , _name );
    element.setAttribute         ( "processes"       , (int)_process_list.size() );
    element.setAttribute         ( "max_processes"   , _max_processes );
    element.setAttribute_optional( "remote_scheduler", _remote_scheduler.as_string() );

    xml::Element_ptr processes_element = document.createElement( "processes" );
    element.appendChild( processes_element );

    FOR_EACH( Process_list, _process_list, it )  processes_element.appendChild( (*it)->dom_element( document, show ) );

    if( !_waiting_jobs.empty() )
    {
        xml::Element_ptr waiting_jobs_element = document.createElement( "waiting_jobs" );
        element.appendChild( waiting_jobs_element );

        FOR_EACH( Job_list, _waiting_jobs, j )  //waiting_jobs_element.appendChild( (*j)->dom_element( document, show_standard ) );
        {
            xml::Element_ptr job_element = document.createElement( "job" );
            job_element.setAttribute( "job", (*j)->name() );
            waiting_jobs_element.appendChild( job_element );
        }
    }

    return element;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
