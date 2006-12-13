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
namespace spooler {


//-------------------------------------------------------------------Spooler_thread::Spooler_thread

Spooler_thread::Spooler_thread( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _log(spooler),
  //_wait_handles(spooler,&_log),
    _lock( "Spooler_thread" )
  //_module(spooler,&_log)
{
    Z_WINDOWS_ONLY( _thread_priority = THREAD_PRIORITY_NORMAL; )

  //_prioritized_order_job_array_time = 1;  // Irgendeine Zeit, damit der Vergleich mit der spooler->job_chain_time() verschieden ausfällt

  //_com_thread     = new Com_thread( this );
  //_free_threading = _spooler->free_threading_default();
  //_include_path   = _spooler->include_path();
}

//------------------------------------------------------------------Spooler_thread::~Spooler_thread

Spooler_thread::~Spooler_thread() 
{
    try 
    { 
        close(); 
    } 
    catch(const exception& x ) { _log.error( x.what() ); }
    
  //_my_event.close();
  //_wait_handles.close();
    _task_list.clear();
}

//-----------------------------------------------------------------------------Spooler_thread::init

void Spooler_thread::init()
{
    //set_thread_name( _name );

    //_log.set_prefix( "Thread " + _name );

  //_com_log = new Com_log( &_log );

}

//----------------------------------------------------------------------Spooler_thread::dom_element
/*
xml::Element_ptr Spooler_thread::dom_element( const xml::Document_ptr& document, const Show_what& )
{
    xml::Element_ptr thread_element = document.createElement( "thread" );

    THREAD_LOCK( _lock )
    {
        thread_element.setAttribute( "name"           , _name );
        thread_element.setAttribute( "running_tasks"  , _running_tasks_count );

        if( _next_time != 0  &&  _next_time != latter_day )
        thread_element.setAttribute( "sleeping_until" , _next_time.as_string() );

        thread_element.setAttribute( "steps"          , _step_count );
        thread_element.setAttribute( "started_tasks"  , _task_count );

        if( thread_id() != _spooler->thread_id() )
        thread_element.setAttribute( "os_thread_id"   , as_hex_string( (int)thread_id() ) );

      //thread_element.setAttribute( "free_threading" , _free_threading? "yes" : "no" );

#       ifdef Z_WINDOWS
            thread_element.setAttribute( "priority"   , GetThreadPriority( thread_handle() ) );
#       endif

        dom_append_nl( thread_element );
    }

    return thread_element;
}
*/
//---------------------------------------------------------------------------Spooler_thread::close1

void Spooler_thread::close1()
{
    try
    {
        THREAD_LOCK( _lock )
        {
            //if( _module_instance )  _module_instance->close(), _module_instance = NULL;


            // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
            //if( _com_log )  _com_log->close();
        }


        if( current_thread_id() == thread_id() 
         && current_thread_id() != _spooler->thread_id()  
         && get_java_vm(false)->running()                )  get_java_vm(false)->detach_thread();

    }
    catch( const exception&  x ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }
}

//----------------------------------------------------------------------------Spooler_thread::close

void Spooler_thread::close()
{
    THREAD_LOCK( _lock )
    {
        close1();
    }
}

//----------------------------------------------------------------------------Spooler_thread::start

void Spooler_thread::start( Event* event_destination )
{
    _event = event_destination;

    if( !thread_id() )  set_thread_id( _spooler->thread_id() );

    //_log.info( "Thread startet" );

    try
    {
        //if( get_java_vm(false)->running() )  get_java_vm(false)->attach_thread( _name );

/*
        if( _module.set() )
        {
            _module_instance = _module.create_instance();
            _module_instance->set_title( "Script for Thread " + _name );

            _module_instance->init();

            _module_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"        );
            _module_instance->add_obj( (IDispatch*)_com_thread           , "spooler_thread" );
            _module_instance->add_obj( (IDispatch*)_com_log              , "spooler_log"    );

            _module_instance->load();
            _module_instance->start();

            bool ok = check_result( _module_instance->call_if_exists( "spooler_init()Z" ) );
            if( !ok )  z::throw_xc( "SCHEDULER-127" );
        }
*/

        Z_WINDOWS_ONLY( SetThreadPriority( GetCurrentThread(), _thread_priority ); )

        THREAD_LOCK( _spooler->_thread_id_map_lock )  _spooler->_thread_id_map[ current_thread_id() ] = this;

      //_nothing_done_count = 0;
      //_nothing_done_max   = _task_list.size() * 2 + 3;
    }
    catch( const exception&  x ) { _terminated = true; _log.error( x.what() ); }
    catch( const _com_error& x ) { _terminated = true; _log.error( as_string( x.Description() ) ); }
}

//-----------------------------------------------------------------------Spooler_thread::task_count

int Spooler_thread::task_count( Job* job )
{
    int result = 0;

    if( !job )  
    {
        result = _task_list.size();
    }
    else
    {
        THREAD_LOCK( _lock )
        {
            FOR_EACH_TASK( t, task )
            {
                if( task->job() == job )  result++;
            }
        }
    }

    return result;
}

//---------------------------------------------------------------------Spooler_thread::cmd_shutdown
/* Wird von Spooler gerufen

void Spooler_thread::cmd_shutdown()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH_TASK( t, task )  task->cmd_end();
    }
}
*/
//------------------------------------------------Spooler_thread::build_prioritized_order_job_array

void Spooler_thread::build_prioritized_order_job_array()
{
    if( _prioritized_order_job_array_version != _spooler->_job_chain_map_version )        // Ist eine neue Jobkette hinzugekommen?
    {
        _prioritized_order_job_array.clear();

        for( Job_list::iterator j = _spooler->_job_list.begin(); j != _spooler->_job_list.end(); j++ )
        {
            if( (*j)->order_controlled() )  _prioritized_order_job_array.push_back( *j );
        }

        sort( _prioritized_order_job_array.begin(), _prioritized_order_job_array.end(), Job::higher_job_chain_priority );

        _prioritized_order_job_array_version = _spooler->_job_chain_map_version;
        //FOR_EACH( vector<Job*>, _prioritized_order_job_array, i )  _log.debug( "build_prioritized_order_job_array: Job " + (*i)->name() );
    }
}

//-----------------------------------------------------------------Spooler_thread::get_task_or_null

ptr<Task> Spooler_thread::get_task_or_null( int task_id )
{
    ptr<Task> result = NULL;

    THREAD_LOCK( _lock )
    {
        FOR_EACH_TASK( t, task )  if( task->id() == task_id )  { result = task;  break; }
    }

    return result;
}

//-------------------------------------------------------------Spooler_thread::get_next_task_to_run
/*
Task* Spooler_thread::get_next_task_to_run()
{
    Task* next_task = NULL;
    Time  next_time = latter_day;

    THREAD_LOCK( _lock )
    {
        FOR_EACH_TASK( t, task )
        {
          //Z_DEBUG_ONLY( _log.debug9( task->name() + ".next_time=" + task->next_time().as_string() ) );
            if( next_time > task->next_time() )  next_time = task->next_time(), next_task = task;
            if( next_time == 0 )  break;
        }
    }

    return next_task;
}

//--------------------------------------------------------------------Spooler_thread::get_next_task

Task* Spooler_thread::get_next_task()
{
    Task* task = get_next_task_to_run();

    _next_time = task? task->next_time() : latter_day;

    return task;
}
*/
//-----------------------------------------------------------------------------Spooler_thread::wait
/*
void Spooler_thread::wait()
{
    string msg;

    if( _spooler->state() == Spooler::s_paused )
    {
        _next_time = latter_day;
        msg = "Angehalten";
    }
    else
    {
        Task* task = get_next_task();

        if( _spooler->_debug )  if( task )  msg = "Warten bis " + _next_time.as_string() + " für Task " + task->name();
                                      else  msg = "Keine Task aktiv";
    }


    if( _next_time > 0 )
    {
        if( _spooler->_debug )  
        {
            if( !_wait_handles.wait(0) )  _log.debug( msg ), _wait_handles.wait_until( _next_time );     // Debug-Ausgabe der Wartezeit nur, wenn kein Ergebnis vorliegt
        }
        else
        {
            _wait_handles.wait_until( _next_time );
        }
    }

    _next_time = 0;
}
*/
//------------------------------------------------------------------Spooler_thread::any_tasks_there
/*
bool Spooler_thread::any_tasks_there()
{
    if( _running_tasks_count > 0 )  return true;

    THREAD_LOCK( _lock )
    {
        FOR_EACH_TASK( t, task )
        {
            if( !job->_task_queue.empty() )  return true;
/ *
            if( (*it)->state() == Job::s_suspended                 )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_delayed           )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_waiting_for_order )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_process           )  return true;    // Zählt nicht in _running_tasks_count
            //if( (*it)->queue_filled() )  return true;
* /
        }
    }

    return false;
}
*/
//---------------------------------------------------------------------Spooler_thread::do_something

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


    if( !something_done )
    {
        Z_FOR_EACH( Job_list, _spooler->_job_list, j )
        {
            Job* job = *j;
            
            if( job->order_controlled() )
            {
                // Dieser Job ist in _prioritized_order_job_array und wird unten fortgesetzt.
            }
            else
            if( _spooler->is_exclusive() )
            {
                something_done = job->do_something();
            }

            if( something_done )  break;
        }
    }

/*
    if( Job* file_order_sink_job = _spooler->get_job_or_null( file_order_sink_job_name ) )
    {
        // Internen Job scheduler_file_order_sink nicht vernachlässigen, damit sich die Aufträge nicht stauen!
        // (Die alte Lösung, Tasks am Ende der Jobkette zu bevorzugen, wäre gut: _prioritized_order_job_array)

        Z_FOR_EACH( Job::Task_list, file_order_sink_job->_running_tasks, it )
        {
            something_done |= do_something( *it );          // Es sollte nur eine Task sein
        }
    }
*/


    // Jetzt sehen wir zu, dass die Jobs, die hinten in einer Jobkette stehen, ihre Aufträge los werden.
    // Damit sollen die fortgeschrittenen Aufträge vorrangig bearbeitet werden, um sie so schnell wie
    // möglich abzuschließen.

    if( !something_done )
    {
        build_prioritized_order_job_array();


        // ERSTMAL DIE ORDER-JOBS

        FOR_EACH( vector<Job*>, _prioritized_order_job_array, j )
        {
            something_done = (*j)->do_something();
            if( something_done )  break;
        }


        // DANN DIE TASKS

        if( !something_done )
        {
            bool stepped;
          //do
            {
                stepped = false;

                FOR_EACH( vector<Job*>, _prioritized_order_job_array, it )
                {
                    Job* job = *it;

                    FOR_EACH_TASK( it, task )
                    {
                        if( task->job() == job )
                        {
                          //if( _my_event.signaled_then_reset() )  return true;
                            if( _event  ->signaled()            )  return true;      // Das ist _event oder _spooler->_event

                            stepped = do_something( task );

                            something_done |= stepped;
                            if( stepped )  break;
                        }
                    }

                    remove_ended_tasks();
                    if( stepped )  break;
                } 
            }
          //while( stepped );
        }
    }


    if( !something_done )
    {
        FOR_EACH_TASK( it, task )
        {
          //if( _my_event.signaled_then_reset() )  return true;
            if( _event  ->signaled()            )  return true;      // Das ist _my_event oder _spooler->_event

            something_done |= do_something( task );

            if( something_done )  break;
        }

        remove_ended_tasks();
    }





    /*
    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:


    FOR_EACH_TASK( it, task )
    {
        Job* job = task->job();

        if( !job->order_controlled() )
        {
            if( job->priority() >= _spooler->priority_max() )
            {
              //if( _my_event.signaled_then_reset() )  return true;
                if( _event  ->signaled()            )  return true;      // Das ist _event oder _spooler->_event
                if( _spooler->signaled()            )  return true;

                something_done |= do_something( task );

                if( !something_done )  break;
            }
        }
    }

    remove_ended_tasks();
    */




    /*
    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    // ERSTMAL DIE NICHT-ORDER-JOBS

    if( !something_done )
    {
        for( Job_list::iterator j = _spooler->_job_list.begin(); j != _spooler->_job_list.end(); j++ )
        {
            if( !(*j)->order_controlled() )  
            {
                something_done = (*j)->do_something();
                if( something_done )  break;
            }
        }
    }


    if( !something_done )
    {
        FOR_EACH_TASK( it, task )
        {
            Job* job = task->job();

            if( !job->order_controlled() )
            {
                //for( int i = 0; i < job->priority(); i++ )
                {
                  //if( _my_event.signaled_then_reset() )  return true;
                    if( _event  ->signaled()            )  return true;      // Das ist _my_event oder _spooler->_event

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
              //if( _my_event.signaled_then_reset() )  return true;
                if( _event  ->signaled()            )  return true;      // Das ist _my_event oder _spooler->_event

                if( job->priority() == 0 )  something_done |= do_something( task );
            }
        }

        remove_ended_tasks();
    }
    */

    return something_done;
}

//---------------------------------------------------------------------Spooler_thread::nichts_getan

void Spooler_thread::nichts_getan()
{
  //_log.warn( "Nichts getan, running_tasks_count=" + as_string(_running_tasks_count) + " state=" + _spooler->state_name() + " _wait_handles=" + _wait_handles.as_string() );

  //if( _nothing_done_count == _nothing_done_max + 1 )
    {
        THREAD_LOCK( _lock )
        {
            FOR_EACH_TASK( t, task )  
            {
                _log.warn( task->name() + 
                           " state=" + task->state_name() ); 
                         //" queue_filled=" + ( (*it)->queue_filled()? "ja" : "nein" ) + 
                         //" running_tasks=" + as_string( (*it)->_running_tasks.size() ) );
            }
        }
    }

  //sos_sleep( wait_time );  // Warten, um bei Wiederholung zu bremsen
}

//---------------------------------------------------------Spooler_thread::is_ready_for_termination

bool Spooler_thread::is_ready_for_termination()
{
    THREAD_LOCK( _lock )
    {
        if( _spooler->state() == Spooler::s_stopping_let_run  &&  !_spooler->has_any_order() ) 
        {
            if( _task_list.size() == 0 )  return true;
            //? FOR_EACH_TASK( t, task )  task->cmd_end(); 
        }

        if( _task_list.size() == 0 )
        {
            if( _spooler->_manual                        )  return true;
            if( _spooler->_configuration_is_job_script   )  return true;
            if( _spooler->state() == Spooler::s_stopping )  return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------Spooler_thread::process

bool Spooler_thread::process()
{
    bool something_done = false;

  //try
    {
        something_done = step();
/*    
        if( something_done )  _nothing_done_count = 0;
        else 
        if( ++_nothing_done_count > _nothing_done_max )  
        {
            nichts_getan( min( 30.0, (double)_nothing_done_count / _nothing_done_max ) );
        }
*/
        _spooler->remove_temporary_jobs();
    }
  //catch( const exception&  x ) { _log.error( x.what() ); sos_sleep(1); }
  //catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); sos_sleep(1); }

    return something_done;
}

//----------------------------------------------------------------------Spooler_thread::thread_main
int Spooler_thread::thread_main()
{
    int             ret = 1;

/*

    Ole_initialize  ole;

    _my_event.set_name( "Thread " + _name );
    _my_event.create();
    _my_event.add_to( &_wait_handles );

    start( &_my_event );

    if( !_terminated )  
    {
        try
        {
            while( _spooler->state() != Spooler::s_stopping  
               &&  _spooler->state() != Spooler::s_stopped  )
            {
                if( _spooler->state() == Spooler::s_paused )
                {
                    wait();
                }
                else
                {
                    process();

                    if( is_ready_for_termination() )  break;

                    if( _running_tasks_count == 0 )
                    {
                        wait();
                    }
                }

                _my_event.reset();
            }

        }
        catch( const exception&  x ) { _log.error( x.what()                     );  sos_sleep(1); }
        catch( const _com_error& x ) { _log.error( as_string( x.Description() ) );  sos_sleep(1); }
    }

    close1();
    ret = 0;

    THREAD_LOCK( _spooler->_thread_id_map_lock )
    {
        Thread_id_map::iterator it = _spooler->_thread_id_map.find( current_thread_id() );
        if( it != _spooler->_thread_id_map.end() )  _spooler->_thread_id_map.erase( it );
    }

    if( ret == 1 )
    {
        _log.error( "Thread wird wegen des Fehlers beendet" );
        //close1();
    }
    
    _terminated = true;
    _spooler->signal( "Thread " + _name + " beendet sich" );
*/

    return ret;
}

//-----------------------------------------------------------------------Spooler_thread::run_thread
/*
int Spooler_thread::run_thread()
{
    int ret = 1;
    int nothing_done_count = 0;
    int nothing_done_max   = _job_list.size() * 2 + 3;

    SetThreadPriority( GetCurrentThread(), _thread_priority );

    THREAD_LOCK( _spooler->_thread_id_map_lock )
    {
        _spooler->_thread_id_map[ current_thread_id() ] = this;
    }

    try
    {
        start();

        while( _spooler->state() != Spooler::s_stopping  
           &&  _spooler->state() != Spooler::s_stopped  )
        {
            if( _spooler->state() == Spooler::s_paused )
            {
                wait();
            }
            else
            {
                bool something_done = step();
            
                if( something_done )  nothing_done_count = 0;
                else 
                if( ++nothing_done_count > nothing_done_max )  
                {
                    nichts_getan( min( 10.0, (double)nothing_done_count / nothing_done_max ) );
                }

                remove_temporary_jobs();

                if( _running_tasks_count == 0 )
                {
                    if( _spooler->state() == Spooler::s_stopping_let_run  &&  !any_tasks_there() )  break;
                    if( _spooler->_manual )  break;   // Task ist fertig, also Thread beenden

                    wait();
                }
            }

            _event.reset();
        }

        stop_jobs();
        close1();
    

        ret = 0;
    }
    catch( const Xc&         x ) { _log.error( x.what() ); }
    catch( const exception&  x ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }

    {THREAD_LOCK( _spooler->_thread_id_map_lock )
    {
        Thread_id_map::iterator it = _spooler->_thread_id_map.find( current_thread_id() );
        if( it != _spooler->_thread_id_map.end() )  _spooler->_thread_id_map.erase( it );
    }}

    if( ret == 1 )
    {
        _log.error( "Thread wird wegen des Fehlers beendet" );
        close1();
    }
    
    _terminated = true;
    _spooler->signal( "Thread " + _name + " beendet sich" );

    return ret;
}
*/
//--------------------------------------------------------------------Spooler_thread::signal_object
// Anderer Thread

void Spooler_thread::signal_object( const string& , const Level& )
{
/*
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->signal_object( object_set_class_name, level );
    }
*/
}

//-------------------------------------------------------------------------------------------thread
/*#ifdef Z_WINDOWS

static uint __stdcall thread( void* param )
{
    Ole_initialize  ole;
    Spooler_thread* thread = (Spooler_thread*)param;
    uint            ret;

    ret = thread->run_thread();

    //_endthreadex( ret );

    return ret;
}

#endif
//---------------------------------------------------------------------Spooler_thread::start_thread

void Spooler_thread::start_thread()
{
#   ifdef Z_WINDOWS
       
        _thread_handle = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
       if( !_thread_handle )  throw_mswin_error( "CreateThread" );

       _log( "thread_id=0x" + as_hex_string( (int)_thread_id ) );

#   else

       z::throw_xc( "SCHEDULER-180", "free threading" );

#   endif
}
*/
//----------------------------------------------------------------------Spooler_thread::stop_thread
/*
void Spooler_thread::stop_thread()
{
    if( _thread_handle )    // Spooler_thread überhaupt schon gestartet?
    {
        _log.msg( "stop thread" );
        _stop = true;
        signal();
    }
}
*/
//----------------------------------------------------------------Spooler_thread::interrupt_scripts
// Anderer Thread
/*
void Spooler_thread::interrupt_scripts()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->interrupt_script();
    }
}
*/
//--------------------------------------------------------Spooler_thread::wait_until_thread_stopped
/*
void Spooler_thread::wait_until_thread_stopped( Time until )
{
    if( _thread_handle )    // Thread überhaupt schon gestartet?
    {
        _log.msg( "waiting ..." );
//      bool ok = wait_for_event( _thread_handle, until );      // Warten, bis thread terminiert ist
        
//      if( !ok ) 
        {
//          _log.msg( "Skripten werden unterbrochen ..." );
            
//          try { 
//              interrupt_scripts(); 
//          } 
//          catch( const Xc& x) { _log.error(x.what()); }

            wait_for_event( _thread_handle, latter_day );      // Warten, bis thread terminiert ist
        }

        _log.msg( "... stopped" );
    }
}
*/

//--------------------------------------------------------------Spooler_thread::try_to_free_process

bool Spooler_thread::try_to_free_process( Job* for_job, Process_class* process_class, const Time& )
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
                        _spooler->_log.warn( task->obj_name() + " beendet sich jetzt schon eine Minute. Wir versuchen eine andere Task zu beenden" );
                    }
                }
            }
        }
    }
*/
    vector<Job*> prioritized_order_job_array;

    for( Job_list::iterator j = _spooler->_job_list.begin(); j != _spooler->_job_list.end(); j++ )
    {
        if( (*j)->order_controlled() )  prioritized_order_job_array.push_back( *j );
    }

    sort( prioritized_order_job_array.begin(), prioritized_order_job_array.end(), Job::higher_job_chain_priority );


    Z_FOR_EACH_REVERSE( vector<Job*>, prioritized_order_job_array, it )
    {
        Job* job = *it;
        if( job->_module->_process_class == process_class )
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

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

