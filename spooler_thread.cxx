// $Id$
/*
    Hier sind implementiert

*/


#include "spooler.h"
#include <algorithm>
#include <sys/timeb.h>
#include "../kram/sleep.h"
#include "../kram/sos_java.h"

namespace sos {
namespace scheduler {


//-------------------------------------------------------------------Task_subsystem::Task_subsystem

Task_subsystem::Task_subsystem( Spooler* spooler )
:
    Subsystem( spooler, this, type_task_subsystem ),
    _zero_(this+1)
{
}

//------------------------------------------------------------------Task_subsystem::~Task_subsystem

Task_subsystem::~Task_subsystem() 
{
    _task_list.clear();
}

//----------------------------------------------------------------------------Task_subsystem::close

void Task_subsystem::close() 
{
    Z_FOR_EACH( Task_list, _task_list, t )  (*t)->job_close();

    _prioritized_order_job_array.clear();
    _task_list.clear();
    _event = NULL;
}

//------------------------------------------------Task_subsystem::build_prioritized_order_job_array

void Task_subsystem::build_prioritized_order_job_array()
{
    if( _prioritized_order_job_array_job_chain_map_version != _spooler->order_subsystem()->file_based_map_version() 
     || _prioritized_order_job_array_job_map_version       != _spooler->job_subsystem  ()->file_based_map_version() )
    {
        // Ist eine neue Jobkette oder Job hinzu- oder weggekommen?

        _prioritized_order_job_array.clear();

        FOR_EACH_JOB( job )
        {
            // is_order_controlled() nicht einfach durch is_in_job_chain() ersetzen, denn wechselt erst beim Aktivieren des Jobs auf true, also nach file_based_map_version()
            if( job->is_order_controlled() )  _prioritized_order_job_array.push_back( job );
        }

        sort( _prioritized_order_job_array.begin(), _prioritized_order_job_array.end(), Job::higher_job_chain_priority );

        _prioritized_order_job_array_job_chain_map_version = _spooler->order_subsystem()->file_based_map_version();
        _prioritized_order_job_array_job_map_version       = _spooler->job_subsystem  ()->file_based_map_version();
        //FOR_EACH( vector<Job*>, _prioritized_order_job_array, i )  _log.debug( "build_prioritized_order_job_array: Job " + (*i)->name() );
    }
}

//-----------------------------------------------------------------Task_subsystem::get_task_or_null

ptr<Task> Task_subsystem::get_task_or_null( int task_id )
{
    ptr<Task> result = NULL;

    FOR_EACH_TASK( t, task )  if( task->id() == task_id )  { result = task;  break; }

    return result;
}

//---------------------------------------------------------------------Task_subsystem::do_something

bool Task_subsystem::do_something( Task* task, const Time& now )
{
    bool something_done = false;

    if( !_spooler->_zschimmer_mode || task->next_time() <= now )
    {
        something_done = task->do_something();
        
        _task_closed |= task->state() == Task::s_closed;
    }

    return something_done;
}

//---------------------------------------------------------------Task_subsystem::remove_ended_tasks

void Task_subsystem::remove_ended_tasks()
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


//#       if defined Z_DEBUG && defined Z_WINDOWS
//        // Könnte eine hübsche Klasse werden.
//        if( _spooler->_check_memory_leak )
//        {
//            static bool         first = true;
//            static _CrtMemState memory_state;
//
//            if( !_spooler->has_any_task()  &&  !_spooler->has_any_order() )     // Scheduler im Leerlauf?
//            {
//                if( first ) 
//                {
//                    first = false;
//                    _CrtMemCheckpoint( &memory_state );
//                }
//                else
//                {
//                    Z_LOGI2( "scheduler", "Checking for memory leak\n" );
//                    _CrtMemState new_memory_state;
//                    _CrtMemState memory_state_difference;
//
//                    _CrtMemCheckpoint( &new_memory_state );
//                    int are_significantly_different = _CrtMemDifference( &memory_state_difference, &memory_state, &new_memory_state );
//                    if( are_significantly_different )
//                    {
//                        _CrtMemDumpStatistics( &memory_state_difference );
//                        _CrtMemDumpAllObjectsSince( &memory_state );
//                        _CrtMemCheckpoint( &memory_state );
//                    }
//                }
//            }
//        }
//#       endif
    }
}

//-----------------------------------------------------------------------------Task_subsystem::step

bool Task_subsystem::step( const Time& now )
{
    bool something_done = false;


    if( !something_done )
    {
        FOR_EACH_JOB( job )
        {
            if( job->is_order_controlled() )
            {
                // Dieser Job ist in _prioritized_order_job_array und wird unten fortgesetzt.
            }
            else
            //2007-01-27 War der Versuch einer Optimierung, funktioniert nicht mit start_when_directory_changed:   if( job->next_time() <= now )
            {
                something_done = job->do_something();
            }

            if( something_done )  break;
        }
    }


    // Jetzt sehen wir zu, dass die Jobs, die hinten in einer Jobkette stehen, ihre Aufträge los werden.
    // Damit sollen die fortgeschrittenen Aufträge vorrangig bearbeitet werden, um sie so schnell wie
    // möglich abzuschließen.

    if( !something_done )
    {
        build_prioritized_order_job_array();


        // ERSTMAL DIE ORDER-JOBS

        FOR_EACH( vector<Job*>, _prioritized_order_job_array, j )
        {
            Job* job = *j;
            if( job->next_time() <= now )
            {
                something_done = job->do_something();
                if( something_done )  break;
            }
        }


        // DANN DIE TASKS

        if( !something_done )
        {
            bool stepped;
            {
                stepped = false;

                FOR_EACH( vector<Job*>, _prioritized_order_job_array, it )
                {
                    Job* job = *it;

                    FOR_EACH_TASK( it, task )
                    {
                        if( task->job() == job )
                        {
                            if( _spooler->_event.signaled() )  return true;  

                            stepped = do_something( task, now );

                            something_done |= stepped;
                            if( stepped )  break;
                        }
                    }

                    remove_ended_tasks();
                    if( stepped )  break;
                } 
            }
        }
    }


    if( !something_done )
    {
        FOR_EACH_TASK( it, task )
        {
            if( _spooler->_event.signaled() )  return true;      // Das ist _my_event oder _spooler->_event

            something_done |= do_something( task, now );
        }

        remove_ended_tasks();
    }


    return something_done;
}

//---------------------------------------------------------Task_subsystem::is_ready_for_termination

bool Task_subsystem::is_ready_for_termination()
{
    if( _spooler->state() == Spooler::s_stopping_let_run  &&  
        ( !_spooler->order_subsystem()  ||  _spooler->order_subsystem()->has_any_order() ) )
    {
        if( _task_list.size() == 0 )  return true;
    }

    if( _task_list.size() == 0 )
    {
        if( _spooler->_manual                        )  return true;
        if( _spooler->_configuration_is_job_script   )  return true;
        if( _spooler->state() == Spooler::s_stopping )  return true;
    }

    return false;
}

//--------------------------------------------------------------------------Task_subsystem::process

bool Task_subsystem::process( const Time& now )
{
    bool something_done = step( now );
    _spooler->job_subsystem()->remove_temporary_jobs();

    return something_done;
}

//--------------------------------------------------------------Task_subsystem::try_to_free_process

bool Task_subsystem::try_to_free_process( Job* for_job, Process_class* process_class, const Time& )
{
/*  Was passiert, wenn ein zweiter Job try_to_free_process() rufen?
    Der beendet doch keine zweite Task, weil die vom ersten Job beendete findet!
    Die Task müsste markiert werden. 
    Aber das wird zu komplizert, wir lassen das erstmal.

    Z_FOR_EACH_REVERSE( vector<Job*>, _prioritized_order_job_array, it )
    {
        Job* job = *it;
        if( job->_module._process_class == process_class )
        {
            FOR_EACH_TASK( it, task )
            {
                if( task->job() == job )
                {
                    if( task->ending() )
                    {
                        if( task->ending_since() + 60 >= now )  return;   // Task beendet sich (seit weniger als eine Minute)
                        _spooler->log()->warn( task->obj_name() + " beendet sich jetzt schon eine Minute. Wir versuchen eine andere Task zu beenden" );
                    }
                }
            }
        }
    }
*/
    vector<Job*> prioritized_order_job_array;

    FOR_EACH_JOB( job )
    {
        if( job->is_order_controlled() )  prioritized_order_job_array.push_back( job );
    }

    sort( prioritized_order_job_array.begin(), prioritized_order_job_array.end(), Job::higher_job_chain_priority );


    Z_FOR_EACH_REVERSE( vector<Job*>, prioritized_order_job_array, it )
    {
        Job* job = *it;
        if( job->_module->_process_class_path == process_class->path() )
        {
            FOR_EACH_TASK( it, task )
            {
                if( task->job() == job )
                {
                    if( task->is_idle() )
                    {
                        task->cmd_nice_end( for_job );
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

//-------------------------------------------------------------------Task_subsystem::kill_all_tasks

void Task_subsystem::end_all_tasks( Task::End_mode end_mode )
{
    FOR_EACH_TASK( it, task )
    {
        task->cmd_end( end_mode );
    }
}

//----------------------------------------------------------Task_subsystem::increment_running_tasks

void Task_subsystem::increment_running_tasks()
{ 
    InterlockedIncrement( &_running_tasks_count ); 
    //_spooler->update_console_title();
}

//----------------------------------------------------------Task_subsystem::decrement_running_tasks

void Task_subsystem::decrement_running_tasks()
{ 
    InterlockedDecrement( &_running_tasks_count ); 
    //_spooler->update_console_title();
}

//--------------------------------------------------------------Task_subsystem::count_started_tasks

void Task_subsystem::count_started_tasks()
{
    _finished_tasks_count++;
    //_spooler->update_console_title();
}

//-------------------------------------------------------------Task_subsystem::count_finished_tasks

void Task_subsystem::count_finished_tasks()
{
    _started_tasks_count++;
    _spooler->update_console_title( 2 );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

