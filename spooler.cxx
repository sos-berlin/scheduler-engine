// $Id: spooler.cxx,v 1.4 2001/01/02 19:07:45 jz Exp $

//#include <precomp.h>

#include "../kram/sos.h"
#include "../kram/sleep.h"
#include "spooler.h"


/*

  gmtime()  ist nicht thread-sicher!
  gmtime()  ist nicht thread-sicher!

*/


namespace sos {

extern const Bool _dll = false;

namespace spooler {


double now() 
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

double Weekday_set::next_date( double tim )
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

double Monthday_set::next_date( double tim )
{
    int         day_no = tim / (24*60*60);
    time_t      t = tim;
    struct tm*  tm = gmtime(&t);
    Sos_date    date;

    date.assign_date( 1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

    return latter_day;
}

//----------------------------------------------------------------------------Ultimo_set::next_date

double Ultimo_set::next_date( double tim )
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

double Start_time::next( double tim )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    time_t     t = tim;
    struct tm* tm = gmtime( &t );

    double next;
    
    next =            _weekday_set .next_date( tim );
    next = min( next, _monthday_set.next_date( tim ) );
    next = min( next, _ultimo_set  .next_date( tim ) );

    _next_start_time = next + _time_of_day;

    if( _repeat_time )
    {
        _next_start_time += ( tim - _next_start_time ) / _repeat_time * _repeat_time;

        if( _next_start_time < tim )  tim += _repeat_time;
    }

    if( _next_start_time + _duration < tim )  _next_start_time = latter_day;
    return _next_start_time;
}

//-----------------------------------------------------------------------------------------Job::Job

Job::Job( Spooler* spooler, const Sos_ptr<Job_descr>& descr )    
: 
    _zero_(this+1), 
    _spooler(spooler), 
    _job_descr(descr) 
{
    set_new_start_time();
}

//--------------------------------------------------------------------------Job::set_new_start_time

void Job::set_new_start_time()
{
    _next_start_time = _job_descr->_start_time.next();
    _next_end_time   = _next_start_time + _job_descr->_start_time._duration;
}

//---------------------------------------------------------------------------------------Job::start

void Job::start()
{
    _object_set = SOS_NEW( Object_set( &_job_descr->_object_set_descr ) );
    _object_set->open();

    _next_start_time = max( _next_start_time + _spooler->_try_start_job_period, now() );
    if( now() >= _next_end_time )  set_new_start_time();

    _running = true;
}

//-----------------------------------------------------------------------------------------Job::end

void Job::end()
{
    _object_set->close();

    _running = false;
}

//----------------------------------------------------------------------------------------Job::step

bool Job::step()
{
    Spooler_object object = _object_set->get();

    if( object.is_null() )  return false;
    
    object.process( _job_descr->_output_level );

    return true;
}

//------------------------------------------------------------------------------------Spooler::step

bool Spooler::step()
{
    bool something_done = false;

    FOR_EACH( Job_list, _job_list, it )
    {
        Job* job = *it;

        if( !job->_running )
        {
            if( now() >= job->_next_start_time )
            {
                job->start();

                _running_jobs_count++;
                something_done = true;
            }
        }
        else
        {
            bool ok = job->step();
            if( ok ) 
            {
                something_done = true;
            }
            else
            {
                job->end();
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

    load_xml();
}

//-----------------------------------------------------------------------------------Spooler::start

void Spooler::start()
{
    FOR_EACH( Job_descr_list, _job_descr_list, it )
    {
        Sos_ptr<Job> job = SOS_NEW( Job( this, *it ) );

        _job_list.push_back( job );
    }
}

//------------------------------------------------------------------------------------Spooler::wait

void Spooler::wait()
{
    if( _running_jobs_count == 0 )
    {
        double next_start_time = latter_day;
        FOR_EACH( Job_list, _job_list, it )  if( next_start_time > (*it)->_next_start_time )  next_start_time = (*it)->_next_start_time;

        double diff = next_start_time - now();
        if( diff > 0 )  sos_sleep( diff );
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


}// namespace sos