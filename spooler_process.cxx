// $Id: spooler_process.cxx,v 1.17 2003/10/18 21:23:17 jz Exp $

#include "spooler.h"


namespace sos {
namespace spooler {

using namespace object_server;

//------------------------------------------------------------------Process::remove_module_instance

void Process::remove_module_instance( Module_instance* )
{ 
    InterlockedDecrement( &_module_instance_count ); 

    if( _temporary  &&  _module_instance_count == 0 )  
    {
        if( _session )
        {
            _session->close();
            _session = NULL;
        }

        if( _process_class )  
        {
            _process_class->remove_process( this );
            _spooler->signal( "Process available" );
        }
    }
}

//-----------------------------------------------------------------------------------Process::start

void Process::start()
{
    if( started() )  throw_xc( "Process::start" );


    if( _server_hostname.empty() )
    {
        Parameters parameters;

        parameters.push_back( Parameter( "param", "-O" ) );

        if( !_job_name.empty() )
        parameters.push_back( Parameter( "param", "-job=" + quoted_string( _job_name ) ) );

        if( _task_id )
        parameters.push_back( Parameter( "param", "-task-id=" + as_string( _task_id ) ) );

        if( !log_filename().empty() )
        parameters.push_back( Parameter( "param", "-log=" + /*quoted_string*/( "+" + log_filename() ) ) );   // -log="+xxx" funktioniert in Linux nicht, die Anführungszeichen kommen in log.cxx an

        parameters.push_back( Parameter( "program", _spooler->_my_program_filename ) );


        _connection = _spooler->_connection_manager->start_process( parameters );
    }
    else
    {
        _connection = Z_NEW( Connection( _spooler->_connection_manager ) );
        _connection->connect( _server_hostname, _server_port );
        _connection->set_async();
    }


#   ifdef Z_WINDOWS
        _connection->set_event( &_spooler->_event );
#   endif

    _session  = Z_NEW( Session( _connection ) );
    _session->set_connection_has_only_this_session();

    _running_since = Time::now();

    _spooler->_log.debug( "Prozess pid=" + as_string( pid() ) + " gestartet" );
}

//--------------------------------------------------------------------------Process::async_continue

bool Process::async_continue()
{
    return _connection->async_continue();
}

//------------------------------------------------------------------------------------Process::kill

bool Process::kill()
{
    return _connection->kill_process();
}

//-------------------------------------------------------------------------------------Process::dom

xml::Element_ptr Process::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr process_element = document.createElement( "process" );

    THREAD_LOCK( _lock )
    {
        //process_element.setAttribute( "name"           , _name );

        if( _connection )
        process_element.setAttribute( "pid"              , _connection->pid() );
      //process_element.setAttribute( "module_instances" , _module_instance_count );

        if( !_job_name.empty() )
        process_element.setAttribute( "job"              , _job_name );

        if( _task_id )
        process_element.setAttribute( "task_id"          , _task_id );

        if( _connection )
        {
            process_element.setAttribute( "operations", _connection->operation_count() );
            process_element.setAttribute( "callbacks", _connection->callback_count() );
        }

        process_element.setAttribute( "running_since", _running_since.as_string() );

        Async_operation* operation = _connection? _connection->current_super_operation() : NULL;
        if( operation )
        process_element.setAttribute( "operation"        , operation->async_state_text() );

        //process_element.setAttribute( "steps"          , _step_count );
        //process_element.setAttribute( "started_tasks"  , _task_count );
    }

    return process_element;
}

//-----------------------------------------------------------------------Process_class::add_process

void Process_class::add_process( Process* process )
{
    THREAD_LOCK( _lock )
    {
        process->_process_class = this;
        _process_list.push_back( process );
    }
}

//--------------------------------------------------------------------------Spooler::remove_process

void Process_class::remove_process( Process* process )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Process_list, _process_list, p )
        {
            if( *p == process )  
            { 
                process->_process_class = NULL; 
                _process_list.erase( p ); 
                //notify_a_process_is_idle();
                return; 
            }
        }
    }

    throw_xc( "Process_class::remove_process" );
}

//------------------------------------------------------------------------Process_class::new_process

Process* Process_class::new_process()
{
    ptr<Process> process;

    THREAD_LOCK( _lock )
    {
        process = Z_NEW( Process( _spooler ) );        

        process->set_temporary( true );      // Zunächst nach der Task beenden. (Problem mit Java, 1.9.03)
      //process->start();

        add_process( process );
    }

    return process;
}

//--------------------------------------------------------Process_class::select_process_if_available

Process* Process_class::select_process_if_available()
{
    Process* process = NULL;

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Process_list, _process_list, p )
        {
            if( (*p)->_module_instance_count == 0 )  { process = *p; break; }
        }

        if( process )
        {
            if( process->_connection->has_error() )
            {
                _spooler->_log.warn( "Prozess pid=" + as_string( process->pid() ) + " wird nach Fehler entfernt" );

                process->_connection->kill_process();
                remove_process( process );
                process = NULL;
            }
        }

        if( !process  &&  _process_list.size() < _max_processes )  return new_process();

    }

    return process;
}

//-----------------------------------------------------------------Process_class::process_available

bool Process_class::process_available( Job* for_job )
{ 
    if( _process_list.size() >= _max_processes )  return false;

    if( _waiting_jobs.empty() )  return true;

    // Warten Jobs auf einen freien Prozess? 
    // Dann liefern wir nur true, wenn dieser Job der erste in der Warteschlange ist.
    return *_waiting_jobs.rbegin() == for_job;

    //FOR_EACH( Job_list, _waiting_jobs, j )  if( *j == job )  return true;
}

//---------------------------------------------------------------Process_class::enqueue_waiting_job

void Process_class::enqueue_waiting_job( Job* job )
{
    _waiting_jobs.push_back( job );
    _spooler->_log.debug9( job->obj_name() + " ist für einen verfügbaren Prozess vorgemerkt" );
}

//----------------------------------------------------------------Process_class::remove_waiting_job

void Process_class::remove_waiting_job( Job* job )
{
    _waiting_jobs.remove( job );
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
    _name          =      e.     getAttribute( "name" );
    _max_processes = (int)e.uint_getAttribute( "max_processes", 1 );
}

//----------------------------------------------------------------------------------Spooler::as_dom
// Anderer Thread

xml::Element_ptr Process_class::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr element = document.createElement( "process_class" );
        
    THREAD_LOCK( _lock )
    {
        element.setAttribute( "name"         , _name );
        element.setAttribute( "processes"    , as_string( _process_list.size() ) );
        element.setAttribute( "max_processes", _max_processes );

        xml::Element_ptr processes_element = document.createElement( "processes" );
        element.appendChild( processes_element );

        FOR_EACH( Process_list, _process_list, it )  processes_element.appendChild( (*it)->dom( document, show ) );

        if( !_waiting_jobs.empty() )
        {
            xml::Element_ptr waiting_jobs_element = document.createElement( "waiting_jobs" );
            element.appendChild( waiting_jobs_element );

            FOR_EACH( Job_list, _waiting_jobs, j )  //waiting_jobs_element.appendChild( (*j)->dom( document, show_standard ) );
            {
                xml::Element_ptr job_element = document.createElement( "job" );
                job_element.setAttribute( "job", (*j)->name() );
                waiting_jobs_element.appendChild( job_element );
            }
        }
    }

    return element;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
