// $Id: spooler_thread.cxx,v 1.4 2001/02/08 11:21:16 jz Exp $
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
    _com_thread = new Com_thread( this );
}

//----------------------------------------------------------------------------------Thread::~Thread

Thread::~Thread() 
{
    try { close(); } catch(const Xc& x ) { _log.error( x.what() ); }
    
    _event.close();
    _wait_handles.close();
}

//-------------------------------------------------------------------------------------Thread::init

void Thread::init()
{
    _log.set_prefix( "Thread " + _name );
    _com_log = new Com_log( &_log );

    _event.set_name( "Thread " + _name );
    _event.add_to( &_wait_handles );
}

//-----------------------------------------------------------Command_processor::execute_show_thread

xml::Element_ptr Thread::xml( xml::Document_ptr document )
{
    xml::Element_ptr thread_element = document->createElement( "thread" );

    THREAD_LOCK( _lock )
    {
        thread_element->setAttribute( "name"         , as_dom_string( _name ) );
        thread_element->setAttribute( "running_tasks", as_dom_string( _running_tasks_count ) );

        if( _next_start_time != 0  &&  _next_start_time != latter_day )
        thread_element->setAttribute( "sleeping_until", as_dom_string( _next_start_time.as_string() ) );

        thread_element->setAttribute( "steps"        , as_dom_string( _step_count ) );
        thread_element->setAttribute( "started_tasks", as_dom_string( _task_count ) );

        dom_append_nl( thread_element );

        xml::Element_ptr jobs_element = document->createElement( "tasks" );
        dom_append_nl( jobs_element );

        FOR_EACH( Job_list, _job_list, it )  jobs_element->appendChild( (*it)->xml(document) ), dom_append_nl( jobs_element );

        thread_element->appendChild( jobs_element );

    }

    return thread_element;
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

        _script_instance.add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"        );
        _script_instance.add_obj( (IDispatch*)_com_thread           , "spooler_thread" );
        _script_instance.add_obj( (IDispatch*)_com_log              , "spooler_log"    );

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

bool Thread::step()
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
            if( job->priority() >= _spooler->_priority_max )  something_done |= job->do_something();
        }
    }


    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            if( _event.signaled_then_reset() )  break;
            Job* job = *it;
            for( int i = 0; i < job->priority(); i++ )  something_done |= job->do_something();
        }
    }


    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            if( _event.signaled_then_reset() )  break;
            Job* job = *it;
            if( job->priority() == 0 )  job->do_something();
        }
    }

    return something_done;
}

//-------------------------------------------------------------------------------------Thread::wait

void Thread::wait()
{
    //tzset();
    Job* next_job = NULL;

    THREAD_LOCK( _lock )
    {
        _next_start_time = latter_day;
        Time wait_time = latter_day;

        FOR_EACH( Job_list, _job_list, it )
        {
            Job* job = *it;
            if( job->_state == Job::s_pending ) 
            {
                if( _next_start_time > (*it)->_next_start_time )  next_job = *it, _next_start_time = next_job->_next_start_time;
            }
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

    { THREAD_LOCK( _lock )  _next_start_time = 0; }
    //tzset();
}

//-------------------------------------------------------------------------------Thread::run_thread

int Thread::run_thread()
{
    int ret = 1;

    try
    {
        start();

        while( !_stop )
        {
            if( _running_tasks_count == 0 )  wait();

            THREAD_LOCK( _spooler->_pause_lock );
        
            if( _spooler->_state == Spooler::s_running ) 
            {
                if( _stop )  break;
                bool something_done = step();
                if( !something_done )  _log.warn( "Nichts getan" );
            }
        }

        close();
    
        _log.msg( "Thread 0x" + as_hex_string( (int)_thread_id ) + " beendet sich" );
        _spooler->signal();

        ret = 0;
    }
    catch( const Xc&         x ) { _log.error( x.what() ); }
    catch( const exception&  x ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }

    if( ret == 1 )
    {
        _log.error( "Thread wird wegen des Fehlers beendet" );
        close();
        _spooler->signal();
    }
    
    return ret;
}

//----------------------------------------------------------------------------Thread::signal_object
// Anderer Thread

void Thread::signal_object( const string& object_set_class_name, const Level& level )
{
    FOR_EACH( Job_list, _job_list, it )  (*it)->signal_object( object_set_class_name, level );
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

   _thread_handle = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
   if( !_thread_handle )  throw_mswin_error( "CreateThread" );

   _log.msg( "thread_id=0x" + as_hex_string( (int)_thread_id ) );
}

//------------------------------------------------------------------------------Thread::stop_thread

void Thread::stop_thread()
{
    if( _thread_handle )    // Thread überhaupt schon gestartet?
    {
        _log.msg( "stop thread" );
        _stop = true;
        signal();
    }
}

//----------------------------------------------------------------Thread::wait_until_thread_stopped

void Thread::wait_until_thread_stopped()
{
    if( _thread_handle )    // Thread überhaupt schon gestartet?
    {
        _log.msg( "waiting ..." );
        wait_for_event( _thread_handle, latter_day );      // Warten, bis thread terminiert ist
        _log.msg( "... stopped" );
    }
}

//--------------------------------------------------------------------------Thread::get_job_or_null

Job* Thread::get_job_or_null( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            Job* job = *it;
            if( job->_name == job_name )  return job;
        }
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

