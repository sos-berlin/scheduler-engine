// $Id: spooler_time.cxx,v 1.4 2001/01/22 11:04:12 jz Exp $
/*
    Hier sind implementiert

    now()
    Weekday_set
    Monthday_set
    Ultimo_set
    Run_time
*/

#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/log.h"

//#if !defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
//#endif


namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------Time::operator =

void Time::operator = ( const Sos_optional_date_time& dt )
{
    _time = dt.hour() * 60*60 + dt.minute() * 60 + dt.second();
}

//---------------------------------------------------------------------------------Time::as_string

string Time::as_string() const
{
    char buff [30];
    int blen = sprintf( buff, "%0.3lf", _time );

    return Sos_optional_date_time( uint(_time) ).as_string() + ( buff + (blen-4) );
}

//---------------------------------------------------------------------------------------Time::now

Time Time::now() 
{
#   if 1 //defined SYSTEM_WIN

        _timeb  tm;
        _ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3 - _timezone;

#    elif define SYSTEM_LINUX

        struct timeval tm;
        gettimeofday( &tm, NULL );
        return (double)tm.tv_sec + (double)tm.tv_usec / (double)1e6 - _timezone;

#   else

        return time(NULL) - _timezone;

#   endif
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
    int                     day_no = tim / (24*60*60);
    Sos_date                date   = Sos_optional_date_time( tim );

    for( int i = 0; i < 31; i++ )
    {
        if( _days[ last_day_of_month( date ) - date.day() ] )  return day_no * (24*60*60);
        day_no++;
        date.add_days(1);
    }

    return latter_day;
}

//----------------------------------------------------------------------------------Run_time::check

void Run_time::check()
{
    if( _begin_time_of_day < 0                )  throw_xc( "SPOOLER-104" );
    if( _begin_time_of_day > _end_time_of_day )  throw_xc( "SPOOLER-104" );
    if( _end_time_of_day > 24*60*60           )  throw_xc( "SPOOLER-104" );
}

//----------------------------------------------------------------------------------Run_time::first

Time Run_time::first( Time tim_par )
{
    return next_time( tim_par );
}

//-----------------------------------------------------------------------------------Run_time::next

Time Run_time::next( Time tim_par )
{
    if( _single_start )  return latter_day;
    return next_time( tim_par );
}

//------------------------------------------------------------------------------Run_time::next_time

Time Run_time::next_time( Time tim_par )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    if( _next_start_time == latter_day )  return latter_day;

    Time tim = tim_par;

    time_t time_only = (time_t)tim % (24*60*60);

    if( _single_start  &&  time_only > _begin_time_of_day
     || time_only > _end_time_of_day                      )  tim += 24*60*60;

    tim -= time_only;


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

    _next_start_time = next + _begin_time_of_day;
    _next_end_time = _next_start_time + ( _end_time_of_day - _begin_time_of_day );

    if( _next_start_time < tim_par )
    {
        //_next_start_time += int( (tim_par-_next_start_time) / _retry_period + _retry_period - 0.01 ) * _retry_period;
        _next_start_time = tim_par;
    }

    return _next_start_time;
}

//-------------------------------------------------------------------------------Run_time::next_try

Time Run_time::next_try( Time t )
{ 
    Time result = min( Time( t + _retry_period ), latter_day ); 
    if( result >= _next_end_time )  result = next();
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
