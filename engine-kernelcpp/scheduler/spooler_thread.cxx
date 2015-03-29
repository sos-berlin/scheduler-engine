// $Id: spooler_thread.cxx 13071 2007-10-05 12:07:43Z jz $
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
    _task_set.clear();
}

//----------------------------------------------------------------------------Task_subsystem::close

void Task_subsystem::close() 
{
    Z_FOR_EACH(Task_set, _task_set, t)  (*t)->job_close();

    _prioritized_order_job_array.clear();
    _task_set.clear();
    _event = NULL;
}

//---------------------------------------------------------------------Task_subsystem::do_something

bool Task_subsystem::do_something() 
{
    bool something_done = false;
    FOR_EACH_TASK(it, task) {
        something_done |= task->do_something();
    }
    return something_done;
}

//------------------------------------------------Task_subsystem::build_prioritized_order_job_array

void Task_subsystem::build_prioritized_order_job_array()
{
    if( _prioritized_order_job_array_job_chain_version != _spooler->order_subsystem()->file_based_version() 
     || _prioritized_order_job_array_job_version       != _spooler->job_subsystem  ()->file_based_version() )
    {
        // Ist eine neue Jobkette oder Job hinzu- oder weggekommen?

        _prioritized_order_job_array.clear();

        FOR_EACH_JOB( job ) {
            if( job->is_in_job_chain() )  _prioritized_order_job_array.push_back( job );
        }

        sort( _prioritized_order_job_array.begin(), _prioritized_order_job_array.end(), Job::higher_job_chain_priority );

        _prioritized_order_job_array_job_chain_version = _spooler->order_subsystem()->file_based_version();
        _prioritized_order_job_array_job_version       = _spooler->job_subsystem  ()->file_based_version();
        //FOR_EACH( vector<Job*>, _prioritized_order_job_array, i )  _log.debug( "build_prioritized_order_job_array: Job " + (*i)->name() );
    }
}

//-------------------------------------------------------------------------Task_subsystem::task_log

string Task_subsystem::task_log(int task_id) const
{
    if (Task* task = get_task_or_null(task_id))
        return task->log_string();
    else {
        return db()->read_task_log(task_id);
    }
}

//-----------------------------------------------------------------Task_subsystem::get_task_or_null

ptr<Task> Task_subsystem::get_task_or_null( int task_id ) const
{
    ptr<Task> result = NULL;

    FOR_EACH_TASK_CONST( t, task )  if( task->id() == task_id )  { result = task;  break; }

    return result;
}

//---------------------------------------------------------------Task_subsystem::remove_ended_tasks

void Task_subsystem::remove_task(Task* task) {
    _task_set.erase(task);
    task->job()->remove_running_task(task);
    if (is_ready_for_termination())
        _spooler->signal();
}

//---------------------------------------------------------Task_subsystem::is_ready_for_termination

bool Task_subsystem::is_ready_for_termination()
{
    if( _spooler->state() == Spooler::s_stopping_let_run  &&  
        ( !_spooler->order_subsystem()  ||  _spooler->order_subsystem()->has_any_order() ) )
    {
        if (_task_set.empty())  return true;
    }

    if (_task_set.empty()) {
        if( _spooler->_manual                        )  return true;
        if( _spooler->_configuration_is_job_script   )  return true;
        if( _spooler->state() == Spooler::s_stopping )  return true;
    }

    return false;
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
    build_prioritized_order_job_array();
    Z_FOR_EACH_REVERSE( vector<Job*>, _prioritized_order_job_array, it ) {
        Job* job = *it;
        bool ok = job->try_to_end_task(for_job, process_class);
        if (ok) break;
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

