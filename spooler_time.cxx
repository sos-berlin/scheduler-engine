// $Id: spooler_time.cxx,v 1.8 2001/03/27 08:02:46 jz Exp $
/*
    Hier sind implementiert

    Time
    Day_of_time
    Weekday_set
    Monthday_set
    Ultimo_set
    Date_set
    Run_time

    XML-Methoden sind in spooler_config.cxx
*/

#include "../kram/sos.h"
#include "spooler.h"

#include <sys/types.h>
#include <sys/timeb.h>


namespace sos {
namespace spooler {
namespace time {


Period empty_period;

//--------------------------------------------------------------------------------Time::operator =

void Time::operator = ( const Sos_optional_date_time& dt )
{
    _time = dt.hour() * 60*60 + dt.minute() * 60 + dt.second();
}

//---------------------------------------------------------------------------------Time::as_string

string Time::as_string() const
{
    char buff [30];
    char* bruch = buff + sprintf( buff, "%0.3lf", _time ) - 4;

    if( _time < 100*(24*60*60) )
    {
        char hhmmss [30];
        int blen = sprintf( hhmmss, "%02d:%02d:%02d", (int)(_time/(60*60)), (int)(_time/60) % 60, (int)_time % 60 );
        return sos::as_string(hhmmss) + bruch;
    }
    else
    {
        return Sos_optional_date_time( uint(_time) ).as_string() + bruch;
    }
}

//---------------------------------------------------------------------------------------Time::now

Time Time::now() 
{
#   if 1 //defined SYSTEM_WIN

        _timeb  tm;
        _ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3 - _timezone - _dstbias;

#   elif define SYSTEM_LINUX

        struct timeval tm;
        gettimeofday( &tm, NULL );
        return (double)tm.tv_sec + (double)tm.tv_usec / (double)1e6 - _timezone - _dstbias;

#   else

        return time(NULL) - _timezone - _dstbias;

#   endif
}

//------------------------------------------------------------------------------------Period::check

void Period::check() const
{
    if( _begin < 0      )  goto FEHLER;
    if( _begin > _end   )  goto FEHLER;
    if( _end > 24*60*60 )  goto FEHLER;
    return;

  FEHLER:
    throw_xc( "SPOOLER-104", _begin.as_string(), _end.as_string() );
}

//-------------------------------------------------------------------------------Period::is_comming

bool Period::is_comming( Time time_of_day )
{
    if( time_of_day < _begin
     || time_of_day < _end   && !_single_start )  return true;
                 // ^-- Falls time_of_day == previous_period.end(), sonst Schleife!

    return false;
}

//---------------------------------------------------------------------------------Period::next_try

Time Period::next_try( Time t )
{ 
    Time result = min( Time( t + _repeat ), latter_day ); 
    if( result >= end() )  result = latter_day;
    return result;
}

//----------------------------------------------------------------------Period::set_next_start_time
/*
void Period::set_next_start_time( const Time& time )
{
    if( time > _end )  _begin = time;
}
*/
//------------------------------------------------------------------------------------Period::print

void Period::print( ostream& s ) const
{
    s << "Period(" << _begin << ".." << _end;
    if( _single_start )  s << " single_start";
                   else  s << " repeat=" << _repeat;
    if( _let_run )  s << " let_run";
    s << ")";
}

//------------------------------------------------------------------------------------Day::has_time

bool Day::has_time( Time time_of_day ) 
{
    FOR_EACH( Period_set, _period_set, it )
    {
        if( it->begin() >= time_of_day || it->end() > time_of_day )  return true;
    }

    return false;
}

//--------------------------------------------------------------------------------Day::next_period_

const Period& Day::next_period_( Time time_of_day )
{
    FOR_EACH( Period_set, _period_set, it )
    {
        if( it->is_comming( time_of_day ) )  return *it;
    }

    return empty_period;
}

//---------------------------------------------------------------------------------------Day::print

void Day::print( ostream& s ) const
{ 
    s << "Day(";
    
    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        s << *it << ' ';
    }
}

//-----------------------------------------------------------------------------------Day_set::print

void Day_set::print( ostream& s ) const
{
    for( int i = 0; i < NO_OF(_days); i++ )
    {
        if( _days[i] )  s << ' ' << i << "=" << _days[i];
    }
}

//--------------------------------------------------------------------------------Weekday_set::next

Period Weekday_set::next_period( Time tim ) 
{
    Time time_of_day = tim.time_of_day();
    int  day_nr      = tim.day_nr();
    int  weekday     = ( day_nr + 4 ) % 7;
    
    for( int i = weekday; i < weekday+7; i++ )
    {
        const Period& period = _days[ i % 7 ].next_period( time_of_day );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        time_of_day = 0;
    }

    return empty_period;
}

//------------------------------------------------------------------------Monthday_set::next_period

Period Monthday_set::next_period( Time tim )
{
    Time                    time_of_day = tim.time_of_day();
    int                     day_nr      = tim.day_nr();
    Sos_optional_date_time  date        = tim;

    for( int i = 0; i < 31; i++ )
    {
        const Period& period = _days[ date.day() ].next_period( time_of_day );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        date.add_days(1);
        time_of_day = 0;
    }

    return empty_period;
}

//--------------------------------------------------------------------------Ultimo_set::next_period

Period Ultimo_set::next_period( Time tim )
{
    Time     time_of_day = tim.time_of_day();
    int      day_nr      = tim.day_nr();
    Sos_date date        = Sos_optional_date_time( tim );

    for( int i = 0; i < 31; i++ )
    {
        const Period& period = _days[ last_day_of_month( date ) - date.day() ].next_period( time_of_day );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        date.add_days(1);
        time_of_day = 0;
    }

    return empty_period;
}

//--------------------------------------------------------------------------------------Date::print

void Date::print( ostream& s ) const
{   
    Sos_optional_date dt;
    dt.set_time_t( _day_nr * (24*60*60) );

    s << "Date(" << dt << " " << _day << ')';
}

//----------------------------------------------------------------------------Date_set::next_period

Period Date_set::next_period( Time tim )
{
    Time     time_of_day = tim.time_of_day();
    int      day_nr      = tim.day_nr();

    FOR_EACH( set<Date>, _date_set, it )
    {
        Date& date = *it;
        
        if( date._day_nr >= day_nr )
        {
            const Period& period = date._day.next_period( date._day_nr == day_nr? time_of_day : 0 );
            if( !period.empty() )  return date._day_nr*(24*60*60) + period;
        }
    }

    return empty_period;
}

//---------------------------------------------------------------------------------Date_set::print

void  Date_set::print( ostream& s ) const
{
    s << "Date_set(";

    FOR_EACH_CONST( set<Date>, _date_set, it )
    {
        s << *it << " ";
    }

    s << ")";
}

//---------------------------------------------------------------------------Run_time::first_period

Period Run_time::first_period( Time tim_par )
{
    return next_period( tim_par );
}

//----------------------------------------------------------------------------Run_time::next_period

Period Run_time::next_period( Time tim_par )
{
    //if( _single_start )  return latter_day;
    return next_period_( tim_par );
}

//---------------------------------------------------------------------------Run_time::next_period_

Period Run_time::next_period_( Time tim_par )
{
    // Bei der Umschaltung von Winter- auf Sommerzeit fehlt eine Stunde!

    Time tim = tim_par;
    Period next;
 
    while(1)
    {
        next = empty_period;

        next = min( next, _date_set    .next_period( tim ) );
        next = min( next, _weekday_set .next_period( tim ) );
        next = min( next, _monthday_set.next_period( tim ) );
        next = min( next, _ultimo_set  .next_period( tim ) );

        if( _holiday_set.find( next.begin().midnight() ) == _holiday_set.end() )  break;

        tim = next.begin().midnight() + 24*60*60;   // Feiertag? Dann nächsten Tag probieren
    }

    //if( _next.begin() < tim_par )  _next_start_time = tim_par;

    return next;
}

//----------------------------------------------------------------------------------Run_time::print

void Run_time::print( ostream& s ) const
{
    s << "Run_time(\n"
         "    date_set     =" << _date_set     << "\n"
         "    weekday_set  =" << _weekday_set << "\n"
         "    monthday_set =" << _monthday_set     << "\n"
         "    ultimo_set   =" << _ultimo_set     << "\n"
         ")\n";
}

//-------------------------------------------------------------------------------------------------

} //namespace time
} //namespace spooler
} //namespace sos
