// $Id: spooler_thread.cxx,v 1.58 2002/11/25 23:36:23 jz Exp $
/*
    Hier sind implementiert

*/



#include "spooler.h"
#include <algorithm>
#include <sys/timeb.h>
#include "../kram/sleep.h"

namespace sos {
namespace spooler {


#define FOR_EACH_JOB( ITERATOR )  FOR_EACH( Job_list, _job_list, ITERATOR )

//-------------------------------------------------------------------Spooler_thread::Spooler_thread

Spooler_thread::Spooler_thread( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler),
    _log(spooler),
    _wait_handles(_spooler,&_log),
    _module(spooler,&_log)
{
    Z_WINDOWS_ONLY( _thread_priority = THREAD_PRIORITY_NORMAL; )

    _com_thread     = new Com_thread( this );
    _free_threading = _spooler->free_threading_default();
    _include_path   = _spooler->include_path();
}

//------------------------------------------------------------------Spooler_thread::~Spooler_thread

Spooler_thread::~Spooler_thread() 
{
    try { close(); } catch(const Xc& x ) { _log.error( x.what() ); }
    
    _event.close();
    _wait_handles.close();
}

//-----------------------------------------------------------------------------Spooler_thread::init

void Spooler_thread::init()
{
    _log.set_prefix( "Thread " + _name );

    _com_log = new Com_log( &_log );

    _event.set_name( "Thread " + _name );
    _event.add_to( &_wait_handles );

    FOR_EACH_JOB( job )  (*job)->init0();
}

//------------------------------------------------------------------------------Spooler_thread::dom

xml::Element_ptr Spooler_thread::dom( const xml::Document_ptr& document, Show_what show )
{
    xml::Element_ptr thread_element = document.createElement( "thread" );

    THREAD_LOCK( _lock )
    {
        thread_element.setAttribute( "name"           , _name );
        thread_element.setAttribute( "running_tasks"  , _running_tasks_count );

        if( _next_start_time != 0  &&  _next_start_time != latter_day )
        thread_element.setAttribute( "sleeping_until" , _next_start_time.as_string() );

        thread_element.setAttribute( "steps"          , _step_count );
        thread_element.setAttribute( "started_tasks"  , _task_count );
        thread_element.setAttribute( "os_thread_id"   , as_hex_string( (int)_thread_id ) );
        thread_element.setAttribute( "free_threading" , _free_threading? "yes" : "no" );

#       ifdef Z_WINDOWS
            thread_element.setAttribute( "priority"   , GetThreadPriority( _thread_handle ) );
#       endif

        dom_append_nl( thread_element );

        xml::Element_ptr jobs_element = document.createElement( "tasks" );
        dom_append_nl( jobs_element );

        FOR_EACH( Job_list, _job_list, it )  jobs_element.appendChild( (*it)->dom( document, show ) ), dom_append_nl( jobs_element );

        thread_element.appendChild( jobs_element );
    }

    return thread_element;
}

//---------------------------------------------------------------------------------Spooler::add_job

void Spooler_thread::add_job( const Sos_ptr<Job>& job )
{
    THREAD_LOCK( _spooler->_job_name_lock )
    {
        Job* j = _spooler->get_job_or_null( job->name() );
        if( j )  throw_xc( "SPOOLER-130", j->name(), j->thread()->name() );

        THREAD_LOCK( _lock )  _job_list.push_back( job );
    }
}

//---------------------------------------------------------------Spooler_thread::load_jobs_from_xml

void Spooler_thread::load_jobs_from_xml( const xml::Element_ptr& element, const Time& xml_mod_time, bool init )
{
    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "job" ) )
        {
            string spooler_id = e.getAttribute( "spooler_id" );

            if( _spooler->_manual? e.getAttribute("name") == _spooler->_job_name 
                                 : spooler_id.empty() || spooler_id == _spooler->id() )
            {
                string job_name = e.getAttribute("name");
                Sos_ptr<Job> job = get_job_or_null( job_name );
                if( job )
                {
                    job->set_dom( e, xml_mod_time );
                    if( init )  job->init0(),  job->init();
                }
                else
                {
                    job = SOS_NEW( Job( this ) );
                    job->set_dom( e, xml_mod_time );
                    if( init )  job->init0(),  job->init();
                    add_job( job );
                }
            }
        }
    }
}

//---------------------------------------------------------------------------Spooler_thread::close1

void Spooler_thread::close1()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->close();

        // Jobs erst bei Spooler-Ende freigeben, s. close()
        // Beim Beenden des Spooler noch laufende Threads können auf Jobs von bereits beendeten Threads zugreifen.
        // Damit's nicht knallt: Jobs schließen, aber Objekte halten.

        if( _module_instance )  _module_instance->close(), _module_instance = NULL;


        // COM-Objekte entkoppeln, falls noch jemand eine Referenz darauf hat:
        if( _com_log )  _com_log->close();
    }


    if( current_thread_id() == _thread_id )  _spooler->_java_vm.detach_thread();
}

//----------------------------------------------------------------------------Spooler_thread::close

void Spooler_thread::close()
{
    THREAD_LOCK( _lock )
    {
        close1();

        _job_list.clear();
    }
}

//-------------------------------------------------------------------------Spooler_thread::has_java

bool Spooler_thread::has_java()
{
    if( _module.kind() == Module::kind_java )  return true;

    FOR_EACH_JOB( job )  if( (*job)->_module.kind() == Module::kind_java )  return true;

    return false;
}

//----------------------------------------------------------------------------Spooler_thread::start

void Spooler_thread::start()
{
    if( has_java() )  
    {
        _spooler->_java_vm.attach_thread( _name );
    }

    if( _module.set() )
    {
        _module_instance = _module.create_instance();

        _module_instance->init();

        _module_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"        );
        _module_instance->add_obj( (IDispatch*)_com_thread           , "spooler_thread" );
        _module_instance->add_obj( (IDispatch*)_com_log              , "spooler_log"    );

        _module_instance->load();
        _module_instance->start();

        bool ok = check_result( _module_instance->call_if_exists( "spooler_init()Z" ) );
        if( !ok )  throw_xc( "SPOOLER-127" );
    }

    FOR_EACH_JOB( job )  (*job)->init();


    Z_WINDOWS_ONLY( SetThreadPriority( GetCurrentThread(), _thread_priority ); )

    THREAD_LOCK( _spooler->_thread_id_map_lock )  _spooler->_thread_id_map[ current_thread_id() ] = this;

    _nothing_done_count = 0;
    _nothing_done_max   = _job_list.size() * 2 + 3;
}

//------------------------------------------------------------------------Spooler_thread::stop_jobs

void Spooler_thread::stop_jobs()
{
    assert( current_thread_id() == _thread_id );

    FOR_EACH_JOB( it ) 
    {
        _current_job = *it;

        if( (*it)->state() != Job::s_stopped )  (*it)->stop();

        _current_job = NULL;
    }
}

//------------------------------------------------Spooler_thread::build_prioritized_order_job_array

void Spooler_thread::build_prioritized_order_job_array()
{
    _prioritized_order_job_array.clear();

    FOR_EACH_JOB( it )  if( (*it)->order_controlled() )  _prioritized_order_job_array.push_back( *it );

    sort( _prioritized_order_job_array.begin(), _prioritized_order_job_array.end(), Job::higher_job_chain_priority );

    //FOR_EACH( vector<Job*>, _prioritized_order_job_array, i )  _log.debug( "build_prioritized_order_job_array: Job " + (*i)->name() );
}

//---------------------------------------------------------------------Spooler_thread::do_something

bool Spooler_thread::do_something( Job* job )
{
    Thread_semaphore::Guard serialize_guard;
    if( !_free_threading )  serialize_guard.enter( &_spooler->_serialize_lock );

    _current_job = job;

    bool ok = job->do_something();

    _current_job = NULL;

    return ok;
}

//-----------------------------------------------------------------------------Spooler_thread::step

bool Spooler_thread::step()
{
    bool something_done = false;

    // Erst die Tasks mit höchster Priorität. Die haben absoluten Vorrang:


    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;
            if( !job->order_controlled() )
            {
                if( job->priority() >= _spooler->priority_max() )
                {
                    if( _event.signaled_then_reset() )  return true;
                    something_done |= do_something( job );
                    if( !something_done )  break;
                }
            }
        }
    }



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


        bool stepped = false;
        do
        {
            FOR_EACH( vector<Job*>, _prioritized_order_job_array, it )
            {
                Job* job = *it;

                if( _event.signaled_then_reset() )  return true;
                stepped = do_something( job );
                something_done |= stepped;
                if( stepped )  break;
            } 
        }
        while( stepped  );
    }


    // Wenn keine Task höchste Priorität hat, dann die Tasks relativ zu ihrer Priorität, außer Priorität 0:

    if( !something_done )
    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;

            if( !job->order_controlled() )
            {
                for( int i = 0; i < job->priority(); i++ )
                {
                    if( _event.signaled_then_reset() )  return true;
                    something_done |= do_something( job );
                    if( !something_done )  break;
                }
            }
        }
    }



    // Wenn immer noch keine Task ausgeführt worden ist, dann die Tasks mit Priorität 0 nehmen:

    if( !something_done )
    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;

            if( !job->order_controlled() )
            {
                if( _event.signaled_then_reset() )  return true;
                if( job->priority() == 0 )  do_something( job );
            }
        }
    }

    return something_done;
}

//------------------------------------------------------------------Spooler_thread::next_start_time

Time Spooler_thread::next_start_time()
{
    Job* next_job = next_job_to_start();
    
    _next_start_time  = next_job? next_job->_next_time : latter_day;

    return _next_start_time;
}

//----------------------------------------------------------------Spooler_thread::next_job_to_start

Job* Spooler_thread::next_job_to_start()
{
    Job* next_job = NULL;
    Time next_start_time = latter_day;

    THREAD_LOCK( _lock )
    {
        FOR_EACH_JOB( it )
        {
            Job* job = *it;
            if( job->_state == Job::s_pending
             || job->_state == Job::s_running_delayed 
             || job->_state == Job::s_running_waiting_for_order ) 
            {
                if( next_start_time > (*it)->_next_time )  next_job = *it, next_start_time = next_job->_next_time;
            }
        }
    }

    return next_job;
}

//-----------------------------------------------------------------------------Spooler_thread::wait
#ifdef Z_WINDOWS

void Spooler_thread::wait()
{
    string msg;

    THREAD_LOCK( _lock )
    {
        if( _spooler->state() == Spooler::s_paused )
        {
            _next_start_time = latter_day;
            msg = "Angehalten";
        }
        else
        {
            Job* next_job = next_job_to_start();

            if( next_job )  _next_start_time = next_job->_next_time;

            if( next_job )  msg = "Warten bis " + _next_start_time.as_string() + " für Job " + next_job->name();
                      else  msg = "Kein Job zu starten";
        }
    }


    if( _spooler->_debug )  _log.debug( msg );


    if( _next_start_time > 0 )
    {
#       ifdef SYSTEM_WIN
 
            wait_until( _next_start_time );

#        else

            double wait_time = _next_start_time - Time::now();

            while( !_wake  &&  wait_time > 0 )               
            {
                double sleep_time = 1.0;
                sos_sleep( min( sleep_time, wait_time ) );
                wait_time -= sleep_time;
            }

#       endif
    }

    { THREAD_LOCK( _lock )  _next_start_time = 0; }
}

#endif
//-----------------------------------------------------------------------Spooler_thread::wait_until
#ifdef Z_WINDOWS

int Spooler_thread::wait_until( Time until )
{
    timeb tm1, tm2;
    ftime( &tm1 );

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

        ftime( &tm2 );
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

#endif
//----------------------------------------------------------------------Spooler_thread::do_add_jobs
/*
void Spooler_thread::do_add_jobs()
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
//------------------------------------------------------------Spooler_thread::remove_temporary_jobs

void Spooler_thread::remove_temporary_jobs()
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

//------------------------------------------------------------------Spooler_thread::any_tasks_there

bool Spooler_thread::any_tasks_there()
{
    if( _running_tasks_count > 0 )  return true;

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  
        {
            if( (*it)->state() == Job::s_suspended                 )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_delayed           )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_waiting_for_order )  return true;    // Zählt nicht in _running_tasks_count
            if( (*it)->state() == Job::s_running_process           )  return true;    // Zählt nicht in _running_tasks_count
            //if( (*it)->queue_filled() )  return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------Spooler_thread::nichts_getan

void Spooler_thread::nichts_getan( double wait_time )
{
    _log.warn( "Nichts getan, running_tasks_count=" + as_string(_running_tasks_count) + " state=" + _spooler->state_name() + " _wait_handles=" + _wait_handles.as_string() );

    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  
        {
            _log.warn( "Job " + (*it)->name() + " state=" + (*it)->state_name() + " queue_filled=" + ( (*it)->queue_filled()? "ja" : "nein" ) );
        }
    }

    sos_sleep( wait_time );  // Warten, um bei Wiederholung zu bremsen
}

//-------------------------------------------------------------------------Spooler_thread::finished

bool Spooler_thread::finished()
{
    if( _running_tasks_count == 0 )
    {
        if( _spooler->state() == Spooler::s_stopping_let_run  &&  !any_tasks_there() )  return true;
        if( _spooler->_manual )  return true;   // Task ist fertig, also Thread beenden
    }

    return false;
}

//--------------------------------------------------------------------------Spooler_thread::process

bool Spooler_thread::process()
{
    bool something_done = false;

    try
    {
        something_done = step();
    
        if( something_done )  _nothing_done_count = 0;
        else 
        if( ++_nothing_done_count > _nothing_done_max )  
        {
            nichts_getan( max( 60.0, (double)_nothing_done_count / _nothing_done_max ) );
        }

        remove_temporary_jobs();
    }
    catch( const Xc&         x ) { _log.error( x.what() ); }
    catch( const exception&  x ) { _log.error( x.what() ); }
    catch( const _com_error& x ) { _log.error( as_string( x.Description() ) ); }

    return something_done;
}

//-----------------------------------------------------------------------Spooler_thread::run_thread
#ifdef SPOOLER_USE_THREADS

int Spooler_thread::run_thread()
{
    int ret = 1;

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
                process();

                if( finished() )  break;

                if( _running_tasks_count == 0 )
                {
                    wait();
                }
            }

            _event.reset();
        }

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

#endif
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

void Spooler_thread::signal_object( const string& object_set_class_name, const Level& level )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )  (*it)->signal_object( object_set_class_name, level );
    }
}

//-------------------------------------------------------------------------------------------thread
#ifdef Z_WINDOWS

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
#ifdef Z_WINDOWS

void Spooler_thread::start_thread()
{
   _thread_handle = _beginthreadex( NULL, 0, thread, this, 0, &_thread_id );
   if( !_thread_handle )  throw_mswin_error( "CreateThread" );

   _log( "thread_id=0x" + as_hex_string( (int)_thread_id ) );
}

#endif
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
//------------------------------------------------------------------Spooler_thread::get_job_or_null

Job* Spooler_thread::get_job_or_null( const string& job_name )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Job_list, _job_list, it )
        {
            Job* job = *it;
            if( stricmp( job->_name.c_str(), job_name.c_str() ) == 0 )  return job;
        }
    }

    return NULL;
}

//-------------------------------------------------------------Spooler_thread::cmd_add_jobs
// Anderer Thread

void Spooler_thread::cmd_add_jobs( const xml::Element_ptr& element )
{
    load_jobs_from_xml( element, true );

    signal();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos

