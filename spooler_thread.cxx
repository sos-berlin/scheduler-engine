// $Id: spooler_thread.cxx,v 1.2 2001/02/06 09:22:26 jz Exp $
/*
    Hier sind implementiert

*/



#include "../kram/sos.h"
#include "spooler.h"


namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------Thread::Thread

Thread::Thread( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _log(&spooler->_log),
    _wait_handles(&_log),
    _script_instance(spooler)
{
}

//----------------------------------------------------------------------------------Thread::~Thread

Thread::~Thread() 
{
    try { close(); } catch(const Xc&) {}

}

//-------------------------------------------------------------------------------------Thread::init

void Thread::init()
{
    _log.set_prefix( "Thread " + _name );
    _com_log = new Com_log( &_log );

    _event.set_name( "Thread " + _name );
    _event.add_to( &_wait_handles );
}

//------------------------------------------------------------------------------------Thread::close

void Thread::close()
{
    FOR_EACH( Job_list, _job_list, it )  (*it)->close();
    _job_list.clear();
    _script_instance.close();

    // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
    if( _com_log )  _com_log->close();
}

//------------------------------------------------------------------------------------Thread::start

void Thread::start()
{
    if( !_script.empty() )
    {
        _script_instance.init( _script._language );

        _script_instance.add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"     );
        _script_instance.add_obj( (IDispatch*)_com_log              , "spooler_log" );

        _script_instance.load( _script );

        _script_instance.call_if_exists( "spooler_init" );
    }

    FOR_EACH( Job_list, _job_list, job )  (*job)->init();
}

//-------------------------------------------------------------------------------------Thread::stop

void Thread::stop()
{
    FOR_EACH( Job_list, _job_list, it )  (*it)->close();

    _job_list.clear();
    _script_instance.close();
}

//-------------------------------------------------------------------------------------Thread::step

void Thread::step()
{
  //int  pri_sum = 0;
    bool something_done = false;

  //FOR_EACH( Task_list, _task_list, it )  pri_sum += (*it)->task_priority;


    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:

    {
        FOR_EACH( Job_list, _job_list, it )
        {
            if( _event.signaled_then_reset() )  break;
            Job* job = *it;
            if( job->_priority >= _spooler->_priority_max )  something_done |= job->do_something();
        }
    }


    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            if( _event.signaled_then_reset() )  break;
            Job* job = *it;
            for( int i = 0; i < job->_priority; i++ )  something_done |= job->do_something();
        }
    }


    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            if( _event.signaled_then_reset() )  break;
            Job* job = *it;
            if( job->_priority == 0 )  job->do_something();
        }
    }
}

//-------------------------------------------------------------------------------------Thread::wait

void Thread::wait()
{
    //tzset();

    _next_start_time = latter_day;
    Job* next_job = NULL;
    Time wait_time = latter_day;

    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;
        if( job->_state == Job::s_pending ) 
        {
            if( _next_start_time > (*it)->_next_start_time )  next_job = *it, _next_start_time = next_job->_next_start_time;
        }
    }

    if( next_job )  next_job->_log.msg( "Nächster Start " + _next_start_time.as_string() );
              else  _log.msg( "Kein Job zu starten" );



#   ifdef SYSTEM_WIN
 
        _wait_handles.wait_until( _next_start_time );

#    else

        wait_time = _next_start_time - Time::now();
        while( !_wake  &&  wait_time > 0 )
        {
            double sleep_time = 1.0;
            sos_sleep( min( sleep_time, wait_time ) );
            wait_time -= sleep_time;
        }

#   endif

    _next_start_time = 0;
    //tzset();
}

//-------------------------------------------------------------------------------Thread::run_thread

int Thread::run_thread()
{
    start();

    while( !_stop )
    {
        THREAD_SEMA( _spooler->_pause_lock );

        if( _spooler->_state == Spooler::s_running )  step();

        if( _running_tasks_count == 0 )  wait();
    }

    close();
    
    _log.msg( "Thread 0x" + as_hex_string( (int)_thread_id ) + " beendet sich" );
    _spooler->signal();
    
    return 0;
}

//-------------------------------------------------------------------------------------------thread

static uint __stdcall thread( void* param )
{
    Ole_initialize ole;
    return ((Thread*)param)->run_thread();
}

//-----------------------------------------------------------------------------Thread::start_thread

void Thread::start_thread()
{
    init();

   _thread = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
   if( !_thread )  throw_mswin_error( "CreateThread" );

   _log.msg( "thread_id=0x" + as_hex_string( (int)_thread_id ) );
}

//------------------------------------------------------------------------------Thread::stop_thread

void Thread::stop_thread()
{
    _stop = true;
    signal();
    wait_for_event( _thread, latter_day ); 
}

//--------------------------------------------------------------------------Thread::get_job_or_null

Job* Thread::get_job_or_null( const string& job_name )
{
    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;
        if( job->_name == job_name )  return job;
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

