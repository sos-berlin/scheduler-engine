// $Id: spooler_time.cxx,v 1.46 2003/09/22 07:54:56 jz Exp $
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

#include "spooler.h"

#ifdef SYSTEM_HPUX
#   include <time.h>
#endif

#include <sys/types.h>
#include <sys/timeb.h>

#ifndef Z_WINDOWS
#   include <sys/time.h>
#endif


namespace sos {
namespace spooler {
namespace time {

extern const int                       latter_day_int              = INT_MAX;
extern const Time                      latter_day                  = latter_day_int;
static const char                      last_day_name[]             = "never";

Period empty_period;

//---------------------------------------------------------------------------------time_from_string

Time time_from_string( const string& str )
{
    if( str.find( ':' ) != string::npos )
    {
        Sos_optional_date_time dt;
        dt.set_time( str );
        return dt.time_as_double();
    }
    else
        return as_double( str );
}

//---------------------------------------------------------------------------------Time::operator =

void Time::operator = ( const Sos_optional_date_time& dt )
{
    if( dt.has_date() )  set( dt.as_time_t() );
                   else  set( dt.hour() * 60*60 + dt.minute() * 60 + dt.second() );
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( const string& t )
{
    Sos_optional_date_time dt;
    dt.set_time( t );
    set( dt.time_as_double() );
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( double t )
{ 
    _time = round(t); 

#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time == 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == latter_day_int? last_day_name 
                                                                   : as_string();
#   endif                                                           
}

//-------------------------------------------------------------------------------Time::set_datetime

void Time::set_datetime( const string& t )
{
    set( Sos_optional_date_time(t).as_double() );
}

//----------------------------------------------------------------------------------Time::as_string

string Time::as_string( With_ms with ) const
{
    if( _time == latter_day_int )
    {
        return last_day_name;
    }
    else
    {
        char        buff [30];

        //char* old_locale = setlocale( LC_NUMERIC, "C" );

        const char* bruch = with == with_ms? buff + sprintf( buff, "%0.3lf", _time ) - 4
                                        : "";

        //setlocale( LC_NUMERIC, old_locale );

        if( _time < 100*(24*60*60) )
        {
            char hhmmss [30];
            sprintf( hhmmss, "%02d:%02d:%02d", (int)(_time/(60*60)), (int)(_time/60) % 60, (int)_time % 60 );
            return sos::as_string(hhmmss) + bruch;
        }
        else
        {
            return Sos_optional_date_time( uint(_time) ).as_string() + bruch;
        }
    }
}

//---------------------------------------------------------------------------------------Time::now

Time Time::now()
{
#   if defined SYSTEM_LINUX

        // Linux füllt nicht time_b::dstflag

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );
        return timegm( &local_tm ) + (double)tv.tv_usec / 1e6;

#   elif defined SYSTEM_HPUX

        timeval  tv;
        tm       local_tm;

        gettimeofday( &tv, NULL );
        localtime_r( &tv.tv_sec, &local_tm );

        return (double)tv.tv_sec + (double)tv.tv_usec / (double)1e6 - timezone + ( local_tm.tm_isdst? _dstbias : 0 ); // Das ist heuristisch. (Warum nicht -_dstbias?) Im Winter 2003 erneut testen!

        //gettimeofday( &tv, &tz );
        //return (double)tv.tv_sec + (double)tv.tv_usec / (double)1e6 - timezone - ( daylight? _dstbias : 0 );  // dsttime ist im Winter gesetzt? Das ist doch falsch.   - ( tz.tz_dsttime?_dstbias : 0 );

#   else

        timeb  tm;
        ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3 - timezone - ( tm.dstflag? _dstbias : 0 );

#   endif
}

//------------------------------------------------------------------------------Period::set_default

void Period::set_default()
{
    _begin  = "00:00:00";
    _end    = "24:00:00";
    _repeat = latter_day;
}

//----------------------------------------------------------------------------------Period::set_dom

void Period::set_dom( const xml::Element_ptr& element, const Period* deflt )
{
    Sos_optional_date_time dt;

    if( deflt )  *this = *deflt;

    string let_run = element.getAttribute( "let_run" );
    if( !let_run.empty() )  _let_run = as_bool( let_run );

    string single_start = element.getAttribute( "single_start" );
    if( !single_start.empty() ) 
    {
        dt.set_time( single_start );
        _begin = dt;
        _repeat = latter_day;
        _single_start = true;
        _let_run = true;
        _end = _begin;
    }
    else
    {
        string begin = element.getAttribute( "begin", "00:00:00" );
        if( !begin.empty() )  dt.set_time( begin ), _begin = dt;

        string repeat = element.getAttribute( "repeat" );
        if( !repeat.empty() )
        {
            if( repeat.find( ':' ) != string::npos )
            {
                Sos_optional_date_time dt;
                dt.set_time( repeat );
                _repeat = dt.time_as_double();
            }
            else
                _repeat = as_double( repeat );
        }

        if( _repeat == 0 )  _repeat = latter_day;
    }

    string end = element.getAttribute( "end" , "24:00:00" );
    if( !end.empty() )  dt.set_time( end ), _end = dt;

    check();
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

bool Period::is_comming( Time time_of_day, With_single_start single_start ) const
{
    if( single_start & wss_next_period )
    {
        if( !_single_start  &&  time_of_day < _end )  return true;
                                         // ^-- Falls time_of_day == previous_period.end(), sonst Schleife!
    }

    if( single_start & wss_next_single_start )
    {
        if( _single_start  &&  time_of_day < _begin )  return true;
    }

    return false;
}

//---------------------------------------------------------------------------------Period::next_try

Time Period::next_try( Time t )
{ 
/* 30.5.03
    Time result = latter_day;

    if( _repeat )
    {
        result = min( Time( t + _repeat ), latter_day ); 
        if( result >= end() )  result = latter_day;
    }
*/
    Time result = t;

    if( result >= end() )  result = latter_day;

    return result;
}

//------------------------------------------------------------------------------------Period::print

void Period::print( ostream& s ) const
{
    s << "Period(" << _begin << ".." << _end;
    if( _single_start )  s << " single_start";
                   else  s << " repeat=" << _repeat;
    if( _let_run )  s << " let_run";
    s << ")";
}

//-------------------------------------------------------------------------------------Day::set_dom

void Day::set_dom( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    if( default_day )  _period_set = default_day->_period_set;

  //Period my_default_period ( element, default_period );
    bool   first = true;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( first )  first = false, _period_set.clear();
        _period_set.insert( Period( e, default_period ) );
    }

  //if( _period_set.empty() )  _period_set.insert( my_default_period );
}

//---------------------------------------------------------------------------------Day::set_default

void Day::set_default()
{
    Period period;
    period.set_default();

    _period_set.clear();
    _period_set.insert( period );
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

const Period& Day::next_period_( Time time_of_day, With_single_start single_start ) const
{
    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        if( it->is_comming( time_of_day, single_start ) )  return *it;
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

Period Weekday_set::next_period( Time tim, With_single_start single_start ) 
{
    Time time_of_day = tim.time_of_day();
    int  day_nr      = tim.day_nr();
    int  weekday     = ( day_nr + 4 ) % 7;
    
    for( int i = weekday; i < weekday+7; i++ )
    {
        const Period& period = _days[ i % 7 ].next_period( time_of_day, single_start );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        time_of_day = 0;
    }

    return empty_period;
}

//------------------------------------------------------------------------Monthday_set::next_period

Period Monthday_set::next_period( Time tim, With_single_start single_start )
{
    Time                    time_of_day = tim.time_of_day();
    int                     day_nr      = tim.day_nr();
    Sos_optional_date_time  date        = tim.as_time_t();

    for( int i = 0; i < 31; i++ )
    {
        const Period& period = _days[ date.day() ].next_period( time_of_day, single_start );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        date.add_days(1);
        time_of_day = 0;
    }

    return empty_period;
}

//--------------------------------------------------------------------------Ultimo_set::next_period

Period Ultimo_set::next_period( Time tim, With_single_start single_start ) 
{
    Time     time_of_day = tim.time_of_day();
    int      day_nr      = tim.day_nr();
    Sos_date date        = Sos_optional_date_time( (time_t)tim );

    for( int i = 0; i < 31; i++ )
    {
        const Period& period = _days[ last_day_of_month( date ) - date.day() ].next_period( time_of_day, single_start );
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

Period Date_set::next_period( Time tim, With_single_start single_start )
{
    Time     time_of_day = tim.time_of_day();
    int      day_nr      = tim.day_nr();

    FOR_EACH( set<Date>, _date_set, it )
    {
        const Date& date = *it;
        
        if( date._day_nr >= day_nr )
        {
            const Period& period = date._day.next_period( date._day_nr == day_nr? time_of_day : Time(0), single_start );
            if( !period.empty() )  return date._day_nr*(24*60*60) + period;
        }
    }

    return empty_period;
}

//---------------------------------------------------------------------------------Date_set::print

void Date_set::print( ostream& s ) const
{
    s << "Date_set(";

    FOR_EACH_CONST( set<Date>, _date_set, it )
    {
        s << *it << " ";
    }

    s << ")";
}

//---------------------------------------------------------------------------------Day_set::set_dom

void Day_set::set_dom( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    //Period my_default_period ( element, default_period );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "day" ) )
        {
            Day my_default_day ( e, default_day, default_period );

            int day = int_from_variant( e.getAttribute( "day" ) );
            if( (uint)day >= NO_OF(_days) )  throw_xc( "SPOOLER-INVALID-DAY", day );
            _days[day].set_dom( e, &my_default_day, default_period );
        }
    }
}

//----------------------------------------------------------------------------Run_time::set_default

void Run_time::set_default()
{
}

//-----------------------------------------------------------------------Run_time::set_default_days

void Run_time::set_default_days()
{
    Day default_day;

    default_day.set_default();

    for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = default_day;
}

//--------------------------------------------------------------------------------Run_time::set_dom

void Run_time::set_dom( const xml::Element_ptr& element )
{
    Sos_optional_date_time  dt;
    Period                  default_period;
    Day                     default_day;
    bool                    period_seen = false;
    

    _set = true;
    _once = element.bool_getAttribute( "once", _once );

    default_period.set_dom( element, NULL );
    default_day = default_period;

    bool a_day_set = false;


    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "period" ) )
        {
            if( !period_seen )  period_seen = true, default_day = Day();
            default_day.add( Period( e, &default_period ) );
        }
        else
        if( e.nodeName_is( "date" ) )
        {
            a_day_set = true;
            dt.assign( e.getAttribute( "date" ) );
            Date date;
            date._day_nr = dt.as_time_t() / (24*60*60);
            date._day.set_dom( e, &default_day, &default_period );
            _date_set._date_set.insert( date );
        }
        else
        if( e.nodeName_is( "weekdays" ) )
        {
            a_day_set = true;
            _weekday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "monthdays" ) )
        {
            a_day_set = true;
            _monthday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "ultimos" ) )
        {
            a_day_set = true;
            _ultimo_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "holidays" ) )
        {
            _holiday_set.clear();

            DOM_FOR_EACH_ELEMENT( e, e2 )
            {
                if( e2.nodeName_is( "holiday" ) )
                {
                    Sos_optional_date_time dt;
                    dt.assign( e2.getAttribute( "date" ) );
                    _holiday_set.insert( dt.as_time_t() );
                }
            }
        }
        if( e.nodeName_is( "holiday" ) )
        {
            dt.assign( e.getAttribute( "date" ) );
            _holiday_set.insert( dt.as_time_t() );
        }
    }

    if( !a_day_set )  for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = default_day;
}

//---------------------------------------------------------------------------Run_time::first_period

Period Run_time::first_period( Time tim_par )
{
    return next_period( tim_par );
}

//----------------------------------------------------------------------------Run_time::next_period

Period Run_time::next_period( Time tim_par, With_single_start single_start )
{
    Time tim = tim_par;
    Period next;
 
    while(1)
    {
        next = empty_period;

        next = min( next, _date_set    .next_period( tim, single_start ) );
        next = min( next, _weekday_set .next_period( tim, single_start ) );
        next = min( next, _monthday_set.next_period( tim, single_start ) );
        next = min( next, _ultimo_set  .next_period( tim, single_start ) );

        if( _holiday_set.find( (uint)next.begin().midnight() ) == _holiday_set.end() )  break;

        tim = next.begin().midnight() + 24*60*60;   // Feiertag? Dann nächsten Tag probieren
    }

    return next;
}

//----------------------------------------------------------------------------------Run_time::print

void Run_time::print( ostream& s ) const
{
    s << "Run_time(\n"
         "    date_set     =" << _date_set     << "\n"
         "    weekday_set  =" << _weekday_set  << "\n"
         "    monthday_set =" << _monthday_set << "\n"
         "    ultimo_set   =" << _ultimo_set   << "\n"
         ")\n";
}

//-------------------------------------------------------------------------------------------------

} //namespace time
} //namespace spooler
} //namespace sos
