// $Id: spooler_process.cxx,v 1.1 2003/08/27 20:40:32 jz Exp $

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

    if( _temporary  &&  _module_instance_count == 0 )  _spooler->remove_process( this );
}

//-----------------------------------------------------------------------------------Process::start

void Process::start()
{
    Parameters parameters;
    parameters.push_back( Parameter( "param", "-object-server" ) );
  //parameters.push_back( Parameter( "param", "-title=" + quoted_string( _title ) ) );

    if( !log_filename().empty() )
    parameters.push_back( Parameter( "param", "-log=" + quoted_string( "+" + log_filename() ) ) );

    _session  = Z_NEW( Session( start_process( parameters ) ) );
}

//--------------------------------------------------------------------------Process::async_continue

void Process::async_continue()
{
    Async_operation* operation = _session->current_operation();

    if( operation )  
    {
        operation = operation->async_super_operation();
        if( _spooler->_debug )  _spooler->_log.debug9( "async_continue " + operation->async_state_text() );
        operation->async_continue();
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
