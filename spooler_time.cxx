// $Id$
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

//-------------------------------------------------------------------------------------------------

extern const int                       latter_day_int              = INT_MAX;
extern const Time                      latter_day                  = latter_day_int;
static const char                      last_day_name[]             = "never";
static const char                      immediately_name[]          = "now";

const char* weekday_names[] = { "so"     , "mo"    , "di"      , "mi"       , "do"        , "fr"     , "sa"      ,
                                "sonntag", "montag", "dienstag", "mittwoch" , "donnerstag", "freitag", "samstag" ,
                                "sun"    , "mon"   , "tue"     , "wed"      , "thu"       , "fri"    , "sat"     ,
                                "sunday" , "monday", "tuesday" , "wednesday", "thursday"  , "friday" , "saturday",
                                NULL };


//-------------------------------------------------------------------------------------------------

Run_time::Class_descriptor              Run_time::class_descriptor ( &typelib, "sos.spooler.Run_time", Run_time::_methods );

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
    if( t == "never" )
    {
        *this = latter_day;
    }
    else
    {
        Sos_optional_date_time dt;
        dt.set_time( t );
        set( dt.time_as_double() );
    }
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( double t )
{
    _time = round(t);

    if( _time > latter_day_int )  _time = latter_day_int;


#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time == 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == latter_day_int? last_day_name
                                                                   : as_string();
#   endif
}

//-------------------------------------------------------------------------------Time::set_datetime

void Time::set_datetime( const string& t )
{
    set( Sos_optional_date_time(t).as_time_t() );
}

//----------------------------------------------------------------------------------Time::as_string

string Time::as_string( With_ms with ) const
{
    if( _time == latter_day_int )
    {
        return last_day_name;
    }
    else
    if( _time == 0 )
    {
        return immediately_name;
    }
    else
    {
        char        buff [30];
        const char* bruch = with == with_ms? buff + sprintf( buff, "%0.3lf", _time ) - 4
                                           : "";

        if( _time < 100*(24*60*60) )
        {
            char hhmmss [30];
            sprintf( hhmmss, "%02d:%02d:%02d%s", (int)(_time/(60*60)), (int)(_time/60) % 60, (int)_time % 60, bruch );
            return hhmmss;
        }
        else
        {
            return Sos_optional_date_time( uint(_time) ).as_string() + bruch;
        }
    }
}

//----------------------------------------------------------------------------------Time::xml_value

string Time::xml_value( With_ms with ) const
{
    string str = as_string( with );
    assert( str[10] == ' ' );
    str[10] = 'T';                      // yyyy-mm-ddThh:mm:ss.mmm
    return str;
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

#   elif defined SYSTEM_HPUX || defined SYSTEM_SOLARIS

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
    if( !element )  return;

    Sos_optional_date_time dt;

    if( deflt )  *this = *deflt;

    //if( _application == application_order  &&  element.getAttribute( "let_run" ) != "" )  z::throw_xc( "SCHEDULER-220", "let_run='yes'" );
    _let_run = element.bool_getAttribute( "let_run", _let_run );

    string single_start = element.getAttribute( "single_start" );
    if( !single_start.empty() )
    {
        dt.set_time( single_start );

        _begin        = dt;
        _end          = dt;
        _repeat       = latter_day;
        _single_start = true;
        _let_run      = true;
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
    z::throw_xc( "SCHEDULER-104", _begin.as_string(), _end.as_string() );
}

//-------------------------------------------------------------------------------Period::is_comming

bool Period::is_comming( Time time_of_day, With_single_start single_start ) const
{
    bool result;

    if( single_start & wss_next_period  &&  !_single_start  &&  time_of_day < _end )  result = true;
                                                                         // ^-- Falls time_of_day == previous_period.end(), sonst Schleife!
    else
    if( single_start & wss_next_begin  &&  !_single_start  &&  time_of_day <= _begin )  result = true;
                                                                        // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
    if( single_start & wss_next_single_start  &&  _single_start  &&  time_of_day <= _begin )  result = true;
                                                                              // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
        result = false;

    //Z_LOG2( "joacim", *this << ".is_comming(" << time_of_day << ',' << (int)single_start << ") ==> " << result << "\n" );

    return result;
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
    if( !element )  return;

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

Period Day::next_period_( Time time_of_day, With_single_start single_start ) const
{
    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        if( it->is_comming( time_of_day, single_start ) )  return *it;
    }

    return Period();
}

//---------------------------------------------------------------------------------------Day::print

void Day::print( ostream& s ) const
{
    s << "Day(";

    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        s << *it << ' ';
    }

    s << ")";
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

    for( int i = weekday; i <= weekday+7; i++ )
    {
        Period period = _days[ i % 7 ].next_period( time_of_day, single_start );
        if( !period.empty() )  return day_nr*(24*60*60) + period;
        day_nr++;
        time_of_day = 0;
    }

    return Period();
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

    return Period();
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

    return Period();
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

    return Period();
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
    if( !element )  return;

    //Period my_default_period ( element, default_period );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "day" ) )
        {
            Day my_default_day ( e, default_day, default_period );
            int day            = -1;

            if( _minimum == 0  &&  _maximum == 6 )      // Es ist ein Wochentag
            {
                string day_string = lcase( e.getAttribute( "day" ) );
                for( const char** p = weekday_names; *p && day == -1; p++ )
                    if( day_string == *p )  day = ( p - weekday_names ) % 7;

                if( day == -1 )
                {
                    day = e.int_getAttribute( "day" );
                    if( day == 7 )  day = 0;      // Sonntag darf auch 7 sein
                }
            }
            else
                day = e.int_getAttribute( "day" );


            if( day < _minimum  ||  day > _maximum )  z::throw_xc( "SCHEDULER-221", as_string( day ), as_string( _minimum ), as_string( _maximum ) );
            if( (uint)day >= NO_OF(_days) )  z::throw_xc( "SCHEDULER-INVALID-DAY", day );

            _days[day].set_dom( e, &my_default_day, default_period );
        }
    }
}

//-------------------------------------------------------------------------------Run_time::_methods

const Com_method Run_time::_methods[] =
{
#ifdef COM_METHOD
    COM_PROPERTY_GET( Run_time,  1, Java_class_name, VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Run_time,  2, Xml            ,              0, VT_BSTR ),
#endif
    {}
};

//-------------------------------------------------------------------------------Run_time::Run_time

Run_time::Run_time( Spooler* spooler, Application a )
:
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1),
    _spooler(spooler),
    _application(a)
{
    if( _application == application_order )
    {
        _once = true;
    }

    _dom.create();
    _dom.appendChild( _dom.createElement( "run_time" ) );       // <run_time/>
}

//-------------------------------------------------------------------------Run_time::QueryInterface

STDMETHODIMP Run_time::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Irun_time           , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDispatch                        , result );

    return Idispatch_implementation::QueryInterface( iid, result );
}

//--------------------------------------------------------------------------------Run_time::put_Xml

STDMETHODIMP Run_time::put_Xml( BSTR xml )
{
    Z_COM_IMPLEMENT( set_xml( string_from_bstr( xml ) ) );
}

//----------------------------------------------------------------------------Run_time::operator ==

bool Run_time::operator == ( const Run_time& r )
{
    return dom_document().xml() == r.dom_document().xml();
}

//----------------------------------------------------------------------------Run_time::set_default

void Run_time::set_default()
{
    set_default_days();
}

//-----------------------------------------------------------------------Run_time::set_default_days

void Run_time::set_default_days()
{
    Day default_day;

    default_day.set_default();

    for( int i = 0; i < 7; i++ )  _weekday_set._days[i] = default_day;
}

//--------------------------------------------------------------------------------Run_time::set_xml

void Run_time::set_xml( const string& xml )
{
    //_xml = xml;

    //set_dom( _spooler->_dtd.validate_xml( xml ).documentElement() );
    xml::Document_ptr doc ( xml );
    if( _spooler->_validate_xml )  _spooler->_schema.validate( xml::Document_ptr( xml ) );
    set_dom( doc.documentElement() );
}

//--------------------------------------------------------------------------------Run_time::set_dom

void Run_time::set_dom( const xml::Element_ptr& element )
{
    if( !element )  return;

    if( _modified_event_handler )  _modified_event_handler->before_modify_event();


    Sos_optional_date_time  dt;
    Period                  default_period;
    Day                     default_day;
    bool                    period_seen = false;


    _dom.create();
    _dom.appendChild( _dom.clone( element ) );

    _set = true;

    _once = element.bool_getAttribute( "once", _once );
    if( _application == application_order  &&  !_once )  z::throw_xc( "SCHEDULER-220", "once='yes'" );

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
            if( !dt.time_is_zero() )  z::throw_xc( "SCHEDULER-208", e.getAttribute( "date" ) );
            Date date;
            date._day_nr = (int)( dt.as_time_t() / (24*60*60) );
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


    if( _modified_event_handler )  _modified_event_handler->modified_event();
}

//----------------------------------------------------------------------------Run_time::dom_element

xml::Element_ptr Run_time::dom_element( const xml::Document_ptr& document ) const
{
    return document.clone( _dom.documentElement() );

/*
    xml::Element_ptr run_time_element = document.createElement( "run_time" );
    return run_time_element;
*/
}

//---------------------------------------------------------------------------Run_time::dom_document

xml::Document_ptr Run_time::dom_document() const
{
    xml::Document_ptr document;

    document.create();
    document.appendChild( dom_element( document ) );

    return document;
}

//---------------------------------------------------------------------------Run_time::first_period

Period Run_time::first_period( Time tim_par )
{
    return next_period( tim_par );
}

//----------------------------------------------------------------------------Run_time::next_period

Period Run_time::next_period( Time tim_par, With_single_start single_start )
{
    Time    tim = tim_par;
    Period  next;

    //while(1)
    while( tim < tim_par + 366*24*60*60 )
    {
        next = Period();

        next = min( next, _date_set    .next_period( tim, single_start ) );
        next = min( next, _weekday_set .next_period( tim, single_start ) );
        next = min( next, _monthday_set.next_period( tim, single_start ) );
        next = min( next, _ultimo_set  .next_period( tim, single_start ) );

        if( next.begin() != latter_day )
        {
            if( _holiday_set.find( (uint)next.begin().midnight() ) == _holiday_set.end() )  return next;  // Gefundener Zeitpunkt ist kein Feiertag? Dann ok!
            tim = next.begin().midnight() + 24*60*60;   // Feiertag? Dann nächsten Tag probieren
        }
        else
        {
            tim = tim.midnight() + 24*60*60;   // Keine Periode? Dann nächsten Tag probieren
        }
    }

    return Period();
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
