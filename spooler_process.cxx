// $Id: spooler_process.cxx,v 1.12 2003/09/05 11:16:19 jz Exp $

#include "spooler.h"


namespace sos {
namespace spooler {

using namespace object_server;

//---------------------------------------------------------------------Spooler_thread::do_something
/*
bool Spooler_thread::do_something( Task* task )
{
    _current_task = task;

    bool ok = task->do_something();
    
    _task_closed |= task->state() == Task::s_closed;

    _current_task = NULL;

    return ok;
}

//---------------------------------------------------------------Spooler_thread::remove_ended_tasks

void Spooler_thread::remove_ended_tasks()
{
    if( _task_closed )
    {
        Task_list::iterator t = _task_list.begin();
        while( t != _task_list.end() )
        {
            Task* task = *t;
            if( task->state() == Task::s_closed )
            {
                task->job()->remove_running_task( task );
                t = _task_list.erase( t );
                continue;
            }

            t++;
        }

        _task_closed = false;
    }
}

//-----------------------------------------------------------------------------Spooler_thread::step

bool Spooler_thread::step()
{
    bool something_done = false;


    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:


    FOR_EACH_TASK( it, task )
    {
        Job* job = task->job();

        if( !job->order_controlled() )
        {
            if( job->priority() >= _spooler->priority_max() )
            {
                if( _my_event.signaled_then_reset() )  return true;
                if( _event->signaled() )  return true;      // Das ist _event oder _spooler->_event
                if( _spooler->signaled() )  return true;
                something_done |= do_something( task );
                if( !something_done )  break;
            }
        }
    }

    remove_ended_tasks();



    // Jetzt sehen wir zu, dass die Jobs, die hinten in einer Jobkette stehen, ihre Aufträge los werden.
    // Damit sollen die fortgeschrittenen Aufträge vorrangig bearbeitet werden, um sie so schnell wie
    // möglich abzuschließen.

    if( !something_done )
    {
        Time t = _spooler->job_chain_time();
        if( _prioritized_order_job_array_time != t )        // Ist eine neue Jobkette hinzugekommen?
        {
            build_prioritized_order_job_array();
            _prioritized_order_job_array_time = t;
        }


        bool stepped;
        do
        {
            stepped = false;

            FOR_EACH( vector<Job*>, _prioritized_order_job_array, it )
            {
                Job* job = *it;

                FOR_EACH_TASK( it, task )
                {
                    if( task->job() == job )
                    {
                        if( _my_event.signaled_then_reset() )  return true;
                        if( _event->signaled() )  return true;      // Das ist _event oder _spooler->_event
                        stepped = do_something( task );
                        something_done |= stepped;
                        if( stepped )  break;
                    }
                }

                remove_ended_tasks();
                if( stepped )  break;
            } 
        }
        while( stepped );
    }



    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        FOR_EACH_TASK( it, task )
        {
            Job* job = task->job();

            if( !job->order_controlled() )
            {
                for( int i = 0; i < job->priority(); i++ )
                {
                    if( _my_event.signaled_then_reset() )  return true;
                    if( _event->signaled() )  return true;      // Das ist _my_event oder _spooler->_event
                    something_done |= do_something( task );
                    if( !something_done )  break;
                }
            }
        }

        remove_ended_tasks();
    }



    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        FOR_EACH_TASK( it, task )
        {
            Job* job = task->job();

            if( !job->order_controlled() ) // ||  job->queue_filled() )     // queue_filled() bei Order-Job, falls der (unsinnigerweise?) explizit gestartet worden ist.
            {
                if( _my_event.signaled_then_reset() )  return true;
                if( _event->signaled() )  return true;      // Das ist _my_event oder _spooler->_event
                if( job->priority() == 0 )  something_done |= do_something( task );
            }
        }

        remove_ended_tasks();
    }


    return something_done;
}
*/

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
    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
  //parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !log_filename().empty() )
    parameters.push_back( Parameter( "param", "-log=" + /*quoted_string*/( "+" + log_filename() ) ) );   // -log="+xxx" funktioniert in Linux nicht, die Anführungszeichen kommen in log.cxx an

    parameters.push_back( Parameter( "program", _spooler->_my_program_filename ) );


    _connection = start_process( parameters );
    _connection->set_event( &_spooler->_event );
    _session  = Z_NEW( Session( _connection ) );
    _session->set_connection_has_only_this_session();
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

        if( _session && _session->connection() )
        process_element.setAttribute( "pid"              , _session->connection()->pid() );
      //process_element.setAttribute( "module_instances" , _module_instance_count );

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
            if( *p == process )  { process->_process_class = NULL; _process_list.erase( p ); return; }
        }
    }

    throw_xc( "Process_class::remove_process" );
}

//---------------------------------------------------------------------Process_class::start_process

Process* Process_class::start_process()
{
    ptr<Process> process;

    THREAD_LOCK( _lock )
    {
        process = Z_NEW( Process( _spooler ) );        

        process->start();
        process->set_temporary( true );      // Zunächst nach der Task beenden. (Problem mit Java, 1.9.03)

        _spooler->_log.debug( "Prozess pid=" + as_string( process->pid() ) + " gestartet" );

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

        if( !process  &&  _process_list.size() < _max_processes )  return start_process();

    }

    return process;
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
    }

    return element;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
