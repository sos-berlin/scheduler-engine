// $Id: spooler_thread.cxx,v 1.30 2002/04/11 13:31:00 jz Exp $
/*
    Hier sind implementiert

*/



#include "../kram/sos.h"
#include <sys/timeb.h>
#include "spooler.h"
#include "../kram/sleep.h"

namespace sos {
namespace spooler {


#define FOR_EACH_JOB( ITERATOR )  FOR_EACH( Job_list, _job_list, ITERATOR )

//-----------------------------------------------------------------------------------Thread::Thread

Thread::Thread( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _log(spooler),
    _wait_handles(_spooler,&_log),
    _script(spooler),
    _script_instance(&_log),
    _thread_priority( THREAD_PRIORITY_NORMAL )   // Windows
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
  //_log.set_profile_section( "Thread " + _name );

    _com_log = new Com_log( &_log );

    _event.set_name( "Thread " + _name );
    _event.add_to( &_wait_handles );
}

//--------------------------------------------------------------------------------------Thread::xml

xml::Element_ptr Thread::xml( xml::Document_ptr document, bool show_all )
{
    xml::Element_ptr thread_element = document->createElement( "thread" );

    THREAD_LOCK( _lock )
    {
        thread_element->setAttribute( "name"           , as_dom_string( _name ) );
        thread_element->setAttribute( "running_tasks"  , as_dom_string( _running_tasks_count ) );

        if( _next_start_time != 0  &&  _next_start_time != latter_day )
        thread_element->setAttribute( "sleeping_until" , as_dom_string( _next_start_time.as_string() ) );

        thread_element->setAttribute( "steps"          , as_dom_string( _step_count ) );
        thread_element->setAttribute( "started_tasks"  , as_dom_string( _task_count ) );
        thread_element->setAttribute( "os_thread_id"   , as_dom_string( as_hex_string( (int)_thread_id ) ) );
        thread_element->setAttribute( "priority"       , as_dom_string( GetThreadPriority( _thread_handle ) ) );
        thread_element->setAttribute( "free_threading" , as_dom_string( _free_threading? "yes" : "no" ) );

        dom_append_nl( thread_element );

        xml::Element_ptr jobs_element = document->createElement( "tasks" );
        dom_append_nl( jobs_element );

        FOR_EACH( Job_list, _job_list, it )  jobs_element->appendChild( (*it)->xml( document, show_all ) ), dom_append_nl( jobs_element );

        thread_element->appendChild( jobs_element );
    }

    return thread_element;
}

//---------------------------------------------------------------------------------Spooler::add_job

void Thread::add_job( const Sos_ptr<Job>& job )
{
    THREAD_LOCK( _spooler->_job_name_lock )
    {
        Job* j = _spooler->get_job_or_null( job->name() );
        if( j )  throw_xc( "SPOOLER-130", j->name(), j->thread()->name() );

        THREAD_LOCK( _lock )  _job_list.push_back( job );
    }
}

//-----------------------------------------------------------------------Thread::load_jobs_from_xml

void Thread::load_jobs_from_xml( const xml::Element_ptr& element, bool init )
{
    for( xml::Element_ptr e = element->firstChild; e; e = e->nextSibling )
    {
        if( e->tagName == "job" ) 
        {
            string spooler_id = as_string( e->getAttribute( "spooler_id" ) );
            if( spooler_id.empty()  ||  spooler_id == _spooler->id() )
            {
                Sos_ptr<Job> job = SOS_NEW( Job( this ) );
                job->set_xml( e );

                if( init )  job->init();

                add_job( job );
            }
        }
    }
}

//------------------------------------------------------------------------------------Thread::close

void Thread::close()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->close();
        _job_list.clear();
        _script_instance.close();

        // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
        if( _com_log )  _com_log->close();
    }
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
        _script_instance.start();

        bool ok = check_result( _script_instance.call_if_exists( "spooler_init" ) );
        if( !ok )  throw_xc( "SPOOLER-127" );
    }

    FOR_EACH_JOB( job )  (*job)->init();
}

//--------------------------------------------------------------------------------Thread::stop_jobs

void Thread::stop_jobs()
{
    assert( GetCurrentThreadId() == _thread_id );

    FOR_EACH_JOB( it ) 
    {
        _current_job = *it;

        if( (*it)->state() != Job::s_stopped )  (*it)->stop();

        _current_job = NULL;
    }
}

//-----------------------------------------------------------------------------Thread::do_something

bool Thread::do_something( Job* job )
{
    Thread_semaphore::Guard serialize_guard;
    if( !_free_threading )  serialize_guard.enter( &_spooler->_serialize_lock );

    _current_job = job;

    bool ok = job->do_something();

    _current_job = NULL;

    return ok;
}

//-------------------------------------------------------------------------------------Thread::step

bool Thread::step()
{
  //int  pri_sum = 0;
    bool something_done = false;

  //FOR_EACH( Task_list, _task_list, it )  pri_sum += (*it)->task_priority;


    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:

    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;
            if( job->priority() >= _spooler->priority_max() )
            {
                if( _event.signaled_then_reset() )  return true;
                something_done |= do_something( job );
                if( !something_done )  break;
            }
        }
    }


    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;
            for( int i = 0; i < job->priority(); i++ )
            {
                if( _event.signaled_then_reset() )  return true;
                something_done |= do_something( job );
                if( !something_done )  break;
            }
        }
    }


    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        FOR_EACH_JOB( it )
        {
            if( _event.signaled_then_reset() )  return true;
            Job* job = *it;
            if( job->priority() == 0 )  do_something( job );
        }
    }

    return something_done;
}

//-------------------------------------------------------------------------------------Thread::wait

void Thread::wait()
{
    //tzset();
    string msg;

    THREAD_LOCK( _lock )
    {
        _next_start_time = latter_day;

        if( _spooler->state() == Spooler::s_paused )
        {
            msg = "Angehalten";
        }
        else
        {
            Job* next_job = NULL;

            Time wait_time = latter_day;

            FOR_EACH_JOB( it )
            {
                Job* job = *it;
                if( job->_state == Job::s_pending ) 
                {
                    if( _next_start_time > (*it)->_next_time )  next_job = *it, _next_start_time = next_job->_next_time;
                }
            }

            if( next_job )  msg = "Warten bis " + _next_start_time.as_string() + " für Job " + next_job->name();
                      else  msg = "Kein Job zu starten";
        }
    }

    //if( !_wait_handles.empty() )  msg += " oder " + _wait_handles.as_string();
    if( _spooler->_debug )  _log.debug( msg );


#   ifdef SYSTEM_WIN
 
        //_wait_handles.wait_until( _next_start_time );
        wait_until( _next_start_time );

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

//-------------------------------------------------------------------------------Thread::wait_until

int Thread::wait_until( Time until )
{
    _timeb tm1, tm2;
    _ftime( &tm1 );

    while(1)
    {
        Time now       = Time::now();
        Time today2    = now.midnight() + 2*3600;           // Heute 2:00 Uhr (für Sommerzeitbeginn: Uhr springt von 2 Uhr auf 3 Uhr)
        Time tomorrow2 = now.midnight() + 2*3600 + 24*3600;
        Time today3    = now.midnight() + 3*3600;           // Heute 3:00 Uhr (für Winterzeitbeginn: Uhr springt von 3 Uhr auf 2 Uhr)
        int  ret       = -1;

        if( now < today2  &&  until >= today2 )     ret = _wait_handles.wait_until( today2 + 0.01 );
        else
        if( now < today3  &&  until >= today3 )     ret = _wait_handles.wait_until( today3 + 0.01 );
        else 
        if( until >= tomorrow2 )                    ret = _wait_handles.wait_until( tomorrow2 + 0.01 );
        else
            break;

        if( ret != -1 )  return ret;

        _ftime( &tm2 );
        if( tm1.dstflag != tm2.dstflag )  
            _log.info( tm2.dstflag? "Sommerzeit" : "Winterzeit" );
        else {
#           ifdef _DEBUG
                _log.debug9( "Keine Sommerzeitumschaltung" );
#           endif
        }
    }

    return _wait_handles.wait_until(  until );
}

//------------------------------------------------------------------------------Thread::do_add_jobs
/*
void Thread::do_add_jobs()
{
    try
    {
        load_jobs_from_xml( _add_jobs_element, true );
    }
    catch( const Xc& x         ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }

    _add_jobs_element = NULL;  
    _add_jobs_document = NULL;
}
*/
//--------------------------------------------------------------------Thread::remove_temporary_jobs

void Thread::remove_temporary_jobs()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  
        {
            if( (*it)->should_removed() )    
            {
                if( _spooler->_debug )  (*it)->_log.debug( "Temporärer Job wird entfernt" );
                (*it)->close(); 
                it = _job_list.erase( it );
            }
        }
    }
}

//--------------------------------------------------------------------------Thread::any_tasks_there

bool Thread::any_tasks_there()
{
    if( _running_tasks_count > 0 )  return true;

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  
        {
            if( (*it)->queue_filled() )  return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------Thread::run_thread

int Thread::run_thread()
{
    int ret = 1;
    int nothing_done_count = 0;
    int nothing_done_max   = _job_list.size() * 2 + 3;

    SetThreadPriority( GetCurrentThread(), _thread_priority );

    THREAD_LOCK( _spooler->_thread_id_map_lock )
    {
        _spooler->_thread_id_map[ GetCurrentThreadId() ] = this;
    }

    try
    {
        start();

        while(   _spooler->state() != Spooler::s_stopping  
           &&    _spooler->state() != Spooler::s_stopped  )
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
                    _log.warn( "Nichts getan, running_tasks_count=" + as_string(_running_tasks_count)  );
                    sos_sleep(1);  // Warten, um bei Wiederholung zu bremsen
                }

                remove_temporary_jobs();

                if( _running_tasks_count == 0 )
                {
                    if( _spooler->state() == Spooler::s_stopping_let_run  &&  !any_tasks_there() )  break;

                    wait();
                }
            }

            _event.reset();
        }

        stop_jobs();
        close();
    
        _spooler->signal( "Thread " + _name + " beendet sich" );

        ret = 0;
    }
    catch( const Xc&         x ) { _log.error( x.what() ); }
    catch( const exception&  x ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }

    {THREAD_LOCK( _spooler->_thread_id_map_lock )
    {
        Thread_id_map::iterator it = _spooler->_thread_id_map.find( GetCurrentThreadId() );
        if( it != _spooler->_thread_id_map.end() )  _spooler->_thread_id_map.erase( it );
    }}

    if( ret == 1 )
    {
        _log.error( "Thread wird wegen des Fehlers beendet" );
        close();
        _spooler->signal( "thread error" );
    }
    
    return ret;
}

//----------------------------------------------------------------------------Thread::signal_object
// Anderer Thread

void Thread::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->signal_object( object_set_class_name, level );
    }
}

//-------------------------------------------------------------------------------------------thread

static uint __stdcall thread( void* param )
{
    Ole_initialize ole;
    
    uint ret = ((Thread*)param)->run_thread();
    //_endthreadex( ret );

    return ret;
}

//-----------------------------------------------------------------------------Thread::start_thread

void Thread::start_thread()
{
    init();

   _thread_handle = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
   if( !_thread_handle )  throw_mswin_error( "CreateThread" );

   _log( "thread_id=0x" + as_hex_string( (int)_thread_id ) );
}

//------------------------------------------------------------------------------Thread::stop_thread
/*
void Thread::stop_thread()
{
    if( _thread_handle )    // Thread überhaupt schon gestartet?
    {
        _log.msg( "stop thread" );
        _stop = true;
        signal();
    }
}
*/
//------------------------------------------------------------------------Thread::interrupt_scripts
// Anderer Thread
/*
void Thread::interrupt_scripts()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->interrupt_script();
    }
}
*/
//----------------------------------------------------------------Thread::wait_until_thread_stopped
/*
void Thread::wait_until_thread_stopped( Time until )
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
//--------------------------------------------------------------------------Thread::get_job_or_null

Job* Thread::get_job_or_null( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            Job* job = *it;
            if( job->_state  &&  job->_name == job_name )  return job;
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------Thread::cmd_add_jobs
// Anderer Thread

void Thread::cmd_add_jobs( const xml::Element_ptr& element )
{
    load_jobs_from_xml( element, true );

    signal();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

