// $Id: spooler.cxx,v 1.7 2001/01/04 18:17:26 jz Exp $




/*
    WAS FEHLT?

    Listen etc. sperren bei execute()

*/

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "../file/anyfile.h"
#include "spooler.h"


namespace sos {

extern const Bool _dll = false;

namespace spooler {

using namespace std;

//---------------------------------------------------------------------------------------------now

Time now() 
{
    return time(NULL) - _timezone;
}

//----------------------------------------------------------------------------Spooler_object::level

Level Spooler_object::level()
{
    CComVariant level = com_invoke( _dispatch, "level" );
    level.ChangeType( VT_INT );

    return level.intVal;
}

//--------------------------------------------------------------------------Spooler_object::process

void Spooler_object::process( Level output_level )
{
    com_invoke( _dispatch, "process()", output_level );
}

//---------------------------------------------------------------------------------Object_set::open

void Object_set::open()
{
    _script_site = new Script_site;
    _script_site->_engine_name = _object_set_descr->_class->_script_language;
    _script_site->init_engine();

    _script_site->parse( _object_set_descr->_class->_script );

    CComVariant object_set_vt = _script_site->call( "spooler_make_object_set()" );
    if( object_set_vt.vt != VT_DISPATCH )  throw_xc( "SPOOLER-103", _object_set_descr->_class_name );
    _dispatch = object_set_vt.pdispVal;

    com_invoke( _dispatch, "spooler_open()" );
}

//--------------------------------------------------------------------------------Object_set::close

void Object_set::close()
{
    com_invoke( _dispatch, "spooler_close()" );

    _script_site->close_engine();
    _script_site = NULL;
}

//----------------------------------------------------------------------------------Object_set::get

Spooler_object Object_set::get()
{
    Spooler_object object;

    while(1)
    {
        CComVariant obj = com_invoke( _dispatch, "spooler_get()" );

        if( obj.vt == VT_EMPTY    )  return Spooler_object(NULL);
        if( obj.vt != VT_DISPATCH )  throw_xc( "SPOOLER-102", _object_set_descr->_class_name );
        
        object = obj.pdispVal;

        if( obj.pdispVal == NULL )  break;  // EOF
        if( _object_set_descr->_level_interval.is_in_interval( object.level() ) )  break;
    }

    return object;
}

//---------------------------------------------------------------------------Weekday_set::next_date

Time Weekday_set::next_date( Time tim )
{
    int day_no = tim / (24*60*60);

    int weekday = ( day_no + 4 ) % 7;

    for( int i = weekday; i < weekday+7; i++ )
    {
        if( _days[ i % 7 ] )  return day_no * (24*60*60);
        day_no++;
    }

    return latter_day;
}

//--------------------------------------------------------------------------Monthday_set::next_date

Time Monthday_set::next_date( Time tim )
{
    int                     day_no = tim / (24*60*60);
    Sos_optional_date_time  date   = tim;

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

    return latter_day;
}

//----------------------------------------------------------------------------Ultimo_set::next_date

Time Ultimo_set::next_date( Time tim )
{
/*
    int         day_no = tim / (24*60*60);
    struct tm*  tm = localtime(tim);
    Sos_date    date;

    date.assign_date( 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }
*/
    return latter_day;
}

//---------------------------------------------------------------------------------Start_time::next

Time Start_time::next( Time tim_par )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    Time tim = tim_par;

    time_t time_only = (time_t)tim % (24*60*60);
    if( time_only > (time_t)_time_of_day + _duration )  tim += 24*60*60 ;

    tim -= time_only;  //( tim + 24*60*60-1 ) / (24*60*60) * 24*60*60;


    Time next;
 
    while(1)
    {
        next = latter_day;

        FOR_EACH( set<time_t>, _date_set, it )
        {
            if( *it < next  &&  *it >= tim )  next = *it;
        }

        next = min( next, _weekday_set .next_date( tim ) );
        next = min( next, _monthday_set.next_date( tim ) );
        next = min( next, _ultimo_set  .next_date( tim ) );

        if( _holiday_set.find( next ) == _holiday_set.end() )  break;

        tim += (24*60*60);
    }

    _next_start_time = next + _time_of_day;

    if( _period )
    {
        _next_start_time += int( tim - _next_start_time ) / (int)_period * _period;
        if( _next_start_time < tim )  tim += _period;
    }
    else
    {
        if( _next_start_time + _duration < tim )  _next_start_time = latter_day;
    }

    return _next_start_time;
}

//-----------------------------------------------------------------------------------------Task::Task

Task::Task( Spooler* spooler, const Sos_ptr<Job>& descr )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job(descr) 
{
    set_new_start_time();
}

//--------------------------------------------------------------------------Task::set_new_start_time

void Task::set_new_start_time()
{
    _next_start_time = _job->_start_time.next();
    _next_end_time   = _next_start_time + _job->_start_time._duration;
}

//---------------------------------------------------------------------------------------Task::start

void Task::start()
{
    _running_since = now();

    _object_set = SOS_NEW( Object_set( &_job->_object_set_descr ) );
    _object_set->open();

    _next_start_time = max( _next_start_time + _spooler->_try_start_job_period, now() );
    if( now() >= _next_end_time )  set_new_start_time();

    _running = true;
}

//-----------------------------------------------------------------------------------------Task::end

void Task::end()
{
    _object_set->close();

    _running = false;
}

//----------------------------------------------------------------------------------------Task::step

bool Task::step()
{
    Spooler_object object = _object_set->get();

    if( object.is_null() )  return false;
    
    object.process( _job->_output_level );

    _step_count++;

    return true;
}

//------------------------------------------------------------------------------------Spooler::step

bool Spooler::step()
{
    Thread_semaphore::Guard guard = &_semaphore;

    bool something_done = false;

    FOR_EACH( Task_list, _task_list, it )
    {
        Task* task = *it;

        if( !task->_running )
        {
            if( now() >= task->_next_start_time )
            {
                task->start();

                _running_jobs_count++;
                something_done = true;
            }
        }
        else
        {
            bool ok = task->step();
            if( ok ) 
            {
                something_done = true;
            }
            else
            {
                task->end();
                _running_jobs_count--;
            }
        }
    }

    return something_done;
}

//------------------------------------------------------------------------------------Spooler::load

void Spooler::load()
{
    tzset();

    _try_start_job_period = 10;

    {
        Thread_semaphore::Guard guard = &_semaphore;

        load_xml();
    }
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    Thread_semaphore::Guard guard = &_semaphore;

    FOR_EACH( Job_list, _job_list, it )
    {
        Sos_ptr<Task> task = SOS_NEW( Task( this, *it ) );

        _task_list.push_back( task );
    }

    _spooler_start_time = now();

    _comm_channel.start_thread();
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    tzset();

    if( _running_jobs_count == 0 )
    {
        Time next_start_time;

        {
            Thread_semaphore::Guard guard = &_semaphore;

            next_start_time = latter_day;
            FOR_EACH( Task_list, _task_list, it )  if( next_start_time > (*it)->_next_start_time )  next_start_time = (*it)->_next_start_time;
        }

        Time diff = next_start_time - now();
        if( diff > 0 ) 
        {
            cerr << "Nächster Start: " << Sos_optional_date_time( next_start_time ) << '\n';
            sos_sleep( diff );
        }

        tzset();
    }
}

//-------------------------------------------------------------------------------------Spooler::run

void Spooler::run()
{
    while(1)
    {
        step();
        wait();
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler

//-----------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    HRESULT hr = CoInitialize(NULL);
    if( FAILED(hr) )  throw_ole( hr, "CoInitialize" );

    {
        spooler::Spooler spooler;

        spooler.load();
        spooler.start();
        spooler.run();
    }

    CoUninitialize();
    return 0;
}


} //namespace sos

