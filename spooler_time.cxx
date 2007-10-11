// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
/*
    Hier sind implementiert

    Time
    Day_of_time
    Weekday_set
    Monthday_set
    Ultimo_set
    Date_set
    Month
    Run_time

    XML-Methoden sind in spooler_config.cxx
*/

#include "spooler.h"
#include "../kram/sleep.h"

#ifdef SYSTEM_HPUX
#   include <time.h>
#endif

#include <sys/types.h>
#include <sys/timeb.h>

#ifndef Z_WINDOWS
#   include <sys/time.h>
#endif



namespace sos {
namespace scheduler {
namespace time {

//-------------------------------------------------------------------------------------------static

int                             Time::static_current_difference_to_utc  = 0;

//--------------------------------------------------------------------------------------------const

const int                              max_include_nesting         = 10;
extern const int                       never_int                   = INT_MAX;
extern const Time                      latter_day                  = never_int;
       const Time                      Time::never                 = never_int;
static const char                      last_day_name[]             = "never";
static const char                      immediately_name[]          = "now";
static const int64                     base_filetime               = 116444736000000000LL;

const char* weekday_names[] = { "so"     , "mo"    , "di"      , "mi"       , "do"        , "fr"     , "sa"      ,
                                "sonntag", "montag", "dienstag", "mittwoch" , "donnerstag", "freitag", "samstag" ,
                                "sun"    , "mon"   , "tue"     , "wed"      , "thu"       , "fri"    , "sat"     ,
                                "sunday" , "monday", "tuesday" , "wednesday", "thursday"  , "friday" , "saturday",
                                NULL };

const char* month_names[] = { "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december" };

//-------------------------------------------------------------------------------------------------

Run_time::Class_descriptor              Run_time::class_descriptor ( &typelib, "sos.spooler.Run_time", Run_time::_methods );

//---------------------------------------------------------Daylight_saving_time_transition_detector

struct Daylight_saving_time_transition_detector : Daylight_saving_time_transition_detector_interface
{
                                Daylight_saving_time_transition_detector( Scheduler* );


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    void                        set_alarm                   ( time_t now );
    string                      obj_name                    () const                                { return "Daylight_saving_time_transition_detector"; }

  private:
    Fill_zero                  _zero_;
    bool                       _was_in_daylight_saving_time;
    time_t                     _next_transition_time;
    string                     _next_transition_name;
    Scheduler*                 _scheduler;
    ptr<Prefix_log>            _log;
};

//----------------------------------------------------------------------------------test_summertime

void test_summertime( const string& date_time )
{
    Time l;
    Time u;

    l.set_datetime( date_time );
    u.set_datetime_utc( date_time );

    while(1)
    {
        
        time_t now_t = ::time(NULL);
        Time::set_current_difference_to_utc( now_t );

        Time now = Time::now();
        Time now_utc;
        now_utc.set_utc( double_from_gmtime() );

        cerr << Time::current_difference_to_utc() << " " << ( now_t - now.as_time_t()  ) << " "
             << now_utc.as_string( Time::without_ms ) << ", " 
             << now.as_string( Time::without_ms ) << "    " 
             << l.as_time_t() << " " << ( ( now.as_time_t() - l.as_time_t() + 100 ) / 3600 ) << "h " << l.as_string() << ", " 
             << u.as_time_t() << " " << ( ( now.as_time_t() - u.as_time_t() + 100 ) / 3600 ) << "h " << u.as_string() << "\n";
        sos_sleep( 1 );
        if( ctrl_c_pressed )  exit(0);
    }
}

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
    if( str == "never" )
    {
        return Time::never;
    }
    else
        return as_double( str );
}

//-------------------------------------------------------------------------new_calendar_dom_element

xml::Element_ptr new_calendar_dom_element( const xml::Document_ptr& dom_document, const Time& t )
{
    xml::Element_ptr result = dom_document.createElement( "at" );

    result.setAttribute( "at", t.xml_value( Time::without_ms ) );

    return result;
}
 
//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* m, int index, const Time& time ) throw()
{
    m->insert( index, time.as_string() );
}

//----------------------------------------------------------------------------weekday_of_day_number

inline int weekday_of_day_number( int day_number )
{
    return ( day_number + 4 ) % 7;
}

//-----------------------------------------------------------------------is_in_daylight_saving_time

bool is_in_daylight_saving_time( time_t t )
{
    tm tm1;
    tm1.tm_isdst = 0;
    
    localtime_r( &t, &tm1 );
    
    return tm1.tm_isdst != 0; 
}

//--------------------------------------------------------------Time::set_current_difference_to_utc

void Time::set_current_difference_to_utc( time_t now )
{
    bool is_dst         = is_in_daylight_saving_time( now );
    int  new_difference = timezone + ( is_dst? _dstbias : 0 );

    if( static_current_difference_to_utc != new_difference )
    {
        Z_LOG2( "scheduler", Z_FUNCTION << " " << new_difference << " tz=" << timezone << " is_dst=" << is_dst << " dstbias=" << _dstbias << " (old=" << static_current_difference_to_utc << ")\n" );
        static_current_difference_to_utc = new_difference;
    }
}

//--------------------------------------------------------------------------------------Time::round

double Time::round( double t )
{
    return floor( t * 1000.0 + 0.5 ) / 1000.0;
}

//----------------------------------------------------------------------------------Time::normalize

double Time::normalize( double t )
{
    return t < 0?                 0 :
           t > Time::never._time? Time::never._time
                                : t;
}

//---------------------------------------------------------------------------------Time::operator =

void Time::operator = ( const Sos_optional_date_time& dt )
{
    if( dt.has_date() )  set( dt.as_time_t() );
                   else  set( dt.hour() * 60*60 + dt.minute() * 60 + dt.second() );
}

//------------------------------------------------------------------------------Time::time_with_now

Time Time::time_with_now( const string& time_string )
{
    Time result;

    if( Regex_submatches matches = Regex( "^ *now *(\\+ *([^ ].*))?$" ).match_subresults( time_string ) )    // " now + HH:MM"
    {
        result = now();

        if( matches.count() > 0 )
        {
            if( matches.count() != 2 )  z::throw_xc( "SCHEDULER-333", time_string );

            string time = matches[ 2 ];
            
            if( time.find( ':' ) != string::npos )
            {
                Time t ( time );
                if( t > 24*60*60 )  z::throw_xc( "SCHEDULER-333", time_string );   // Sollte nicht vorkommen
                result += t;
            }
            else
            {
                try
                {
                    result += as_int( time );
                }
                catch( exception& x ) { z::throw_xc( "SCHEDULER-333", time_string, x ); }
            }
        }
    }
    else
    {
        result.set_datetime( time_string );
    }

    return result;
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( const string& t )
{
    _is_utc = false;

    if( t == "never" )
    {
        *this = Time::never;
    }
    else
    {
        string my_t = t;

        double fraction = cut_fraction( &my_t );

        Sos_optional_date_time dt;
        dt.set_time( my_t);
        set( dt.time_as_double() + fraction );
    }
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( double t )
{
    _time = round(t);
    _is_utc = false;

    if( _time > never_int )  _time = never_int;


#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time == 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == never_int? time::last_day_name
                                                              : as_string();
#   endif
}

//----------------------------------------------------------------------------------------Time::set
#ifdef Z_WINDOWS

//void Time::set( const SYSTEMTIME& systemtime )
//{
//    set( windows::filetime_from_systemtime( systemtime ) );
//}

#endif
//----------------------------------------------------------------------------------------Time::set
#ifdef Z_WINDOWS

//void Time::set( const FILETIME& filetime )
//{
//    set( windows::double_time_t_from_filetime( filetime ) );
//    //set( (double)( *(int64*)&filetime - base_filetime ) / 10000000.0 );
//}

#endif

//-----------------------------------------------------------------------------------Time::filetime
#ifdef Z_WINDOWS

//FILETIME Time::filetime() const
//{
//    return windows::filetime_from_time_t( as_double() );
//    //FILETIME result;
//
//    //*(int64*)&result = int64_filetime();
//    //return result;
//}

//-----------------------------------------------------------------------------------Time::filetime

//int64 Time::int64_filetime() const
//{
//    return (int64)( as_double() * 10000000.0 + 0.5 ) + base_filetime;
//}

#endif

//-----------------------------------------------------------------------------------Time::month_nr

int Time::month_nr() const
{
    return Sos_optional_date_time( as_time_t() ).month();
}

//----------------------------------------------------------------------------------Time::as_double

double Time::as_double() const
{
    //Z_DEBUG_ONLY( if( _is_utc )  Z_LOG( Z_FUNCTION << " _time=" << ::sos::as_string(_time) << " - " << current_difference_to_utc() << "\n" ) );

    return _is_utc? _time - current_difference_to_utc()
                  : _time;    
}

//------------------------------------------------------------------------------Time::as_utc_double

double Time::as_utc_double() const
{
    //Z_DEBUG_ONLY( if( _is_utc )  Z_LOG( Z_FUNCTION << " _time=" << ::sos::as_string(_time) << " - " << current_difference_to_utc() << "\n" ) );

    return _is_utc? _time 
                  : _time + current_difference_to_utc();    
}

//------------------------------------------------------------------------------------Time::set_utc

Time& Time::set_utc( double t )
{
    set( t );
    if( !is_null()  &&  !is_never() )  _is_utc = true;
    return *this;
}

//-------------------------------------------------------------------------------Time::set_datetime

void Time::set_datetime( const string& t )
{
    string my_t = t;

    double fraction = cut_fraction( &my_t );
    set( Sos_optional_date_time( my_t ).as_time_t() + fraction );
}

//---------------------------------------------------------------------------Time::set_datetime_utc

void Time::set_datetime_utc( const string& t )
{
    set_datetime( t );
    if( !is_null()  &&  !is_never() )  _is_utc = true;
}

//-------------------------------------------------------------------------------Time::cut_fraction

double Time::cut_fraction( string* datetime_string )
{
    double      result = 0;
    const char* p0     = datetime_string->c_str();
    const char* p      = p0 + datetime_string->length();

    while( p > p0  &&  p[-1] == ' ' )  p--;

    int digit_count = 0;
    while( p > p0  &&  isdigit( (int)p[-1] ) )  p--, digit_count++;
    if( p > p0  &&  digit_count > 0  &&  p[-1] == '.' )
    {
        p--;
        result = ::sos::as_double( p );
        datetime_string->erase( p - p0 );
    }

    return result;
}

//----------------------------------------------------------------------------------Time::as_string

string Time::as_string( With_ms with ) const
{
    string result;

    result.reserve( 27 );  // yyyy-mm-dd hh:mm:ss.mmm UTC

    if( is_never() )
    {
        result = last_day_name;
    }
    else
    if( is_null() )
    {
        result = immediately_name;
    }
    else
    {
        char        buff [30];
        const char* bruch = with == with_ms? buff + sprintf( buff, "%0.3lf", _time ) - 4
                                           : "";

        if( _time < 100*(24*60*60) )
        {
            char hhmmss [30];
            sprintf( hhmmss, "%02d:%02d:%02d%s", (int)(_time/(60*60)), abs( (int)(_time/60) ) % 60, (int)abs( (int64)_time % 60 ), bruch );
            result = hhmmss;
        }
        else
        {
            result = Sos_optional_date_time( uint(_time) ).as_string() + bruch;
            if( _is_utc )  result += " UTC";    // xml_value() macht hieraus "Z"
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Time::jdbc_value

string Time::jdbc_value() const
{ 
    return "{ts'" + as_string( without_ms ) + "'}"; 
}

//----------------------------------------------------------------------------------Time::xml_value

string Time::xml_value( With_ms with ) const
{
    string str = as_string( with );

    if( str.length() > 10  &&  isdigit( (unsigned char)str[0] )  &&  str[10] == ' ' )
    {
        str[10] = 'T';                      // yyyy-mm-ddThh:mm:ss.mmm
    }

    if( string_ends_with( str, " UTC" ) )  str.replace( 23, 4, "Z" );

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

        return (double)tv.tv_sec + (double)tv.tv_usec / (double)1e6 - timezone - ( local_tm.tm_isdst? _dstbias : 0 );

#   else

        timeb  tm;
        ftime( &tm );
        return (double)tm.time + (double)tm.millitm / (double)1e3 - timezone - ( tm.dstflag? _dstbias : 0 );

#   endif
}

//-----------------------------------------------------new_daylight_saving_time_transition_detector

ptr<Daylight_saving_time_transition_detector_interface> new_daylight_saving_time_transition_detector( Scheduler* scheduler )  
{ 
    ptr<Daylight_saving_time_transition_detector> result = Z_NEW( Daylight_saving_time_transition_detector( scheduler ) ); 
    return +result;
}

//---------------Daylight_saving_time_transition_detector::Daylight_saving_time_transition_detector

Daylight_saving_time_transition_detector::Daylight_saving_time_transition_detector( Scheduler* scheduler )
: 
    _zero_(this+1),
    _scheduler(scheduler)
{
    _log = Z_NEW( Prefix_log( scheduler ) );
    _log->set_prefix( obj_name() );

    set_alarm( ::time(NULL) );
}

//-----------------------------------------------------------------------time_t_from_dst_systemtime
#ifdef Z_WINDOWS

static time_t time_t_from_dst_systemtime( const SYSTEMTIME& dst, const SYSTEMTIME& now )
{
    if( dst.wYear != 0  ||  dst.wMonth == 0  ||  dst.wDay == 0 )  return 0;


    SYSTEMTIME result;
    SYSTEMTIME last_day; 
    BOOL       ok;
    
    memset( &result, 0, sizeof result );
    result.wYear = now.wYear;

    while(1)
    {
        result.wMonth        = dst.wMonth;
        result.wDay          = 1;
        result.wHour         = dst.wHour;
        result.wMinute       = dst.wMinute;
        result.wMilliseconds = dst.wMilliseconds;


        // result.wDayOfWeek errechnen

        int64 filetime;  // 100 Nanosekunden
        ok = SystemTimeToFileTime( &result, (FILETIME*)&filetime );
        if( !ok )  return 0;

        ok = FileTimeToSystemTime( (FILETIME*)&filetime, &result );   // Jetzt haben wir result.wDayOfWeek
        if( !ok )  return 0;


        // Letzten Tag des Monats bestimmen

        last_day = result;
        if( ++last_day.wMonth > 12 )  last_day.wMonth = 1, last_day.wYear++;

        ok = SystemTimeToFileTime( &last_day, (FILETIME*)&filetime );
        if( !ok )  return 0;

        filetime -= 24*3600*10000000LL;

        ok = FileTimeToSystemTime( (FILETIME*)&filetime, &last_day );  
        if( !ok )  return 0;


        // Wochentag setzen

        result.wDay += ( dst.wDayOfWeek - result.wDayOfWeek + 7 ) % 7;
        result.wDayOfWeek = dst.wDayOfWeek;


        // Woche setzen (dst.wDay gibt an, der wievielte Wochentag des Monats es ist)

        result.wDay += 7 * ( dst.wDay - 1 );


        // Aber nicht über den Monat hinaus

        while( result.wDay > last_day.wDay )  result.wDay -= 7;

        if( windows::compare_systemtime( now, result ) < 0 )  break;

        result.wYear++;  // Nächstes Jahr probieren
        assert( result.wYear <= now.wYear + 1 );
    }

    return windows::time_t_from_filetime( windows::filetime_from_systemtime( result ) );
}

#endif
//----------------------------------------------Daylight_saving_time_transition_detector::set_alarm

void Daylight_saving_time_transition_detector::set_alarm( time_t now )

// Wenn die Uhr zurückgestellt wird, so dass die Zeitzone wechselt, bekommen wir das nicht mit.
// Der Scheduler bleibt in der vorherigen Zeitzone, weil set_current_difference_to_utc() nicht erneut aufgerufen wird.

{
    _next_transition_name        = "";
    _was_in_daylight_saving_time = is_in_daylight_saving_time( now );


#   ifdef Z_WINDOWS
        if( _scheduler->_zschimmer_mode )  
        {
            TIME_ZONE_INFORMATION time_zone_information;
            
            DWORD result = GetTimeZoneInformation( &time_zone_information );

            if( result != TIME_ZONE_ID_INVALID )
            {
                SYSTEMTIME now;
                GetLocalTime( &now );

                if( time_zone_information.StandardDate.wMonth )
                {
                    time_t standard_date = time_t_from_dst_systemtime( time_zone_information.StandardDate, now );
                    time_t daylight_date = time_t_from_dst_systemtime( time_zone_information.DaylightDate, now );
                    
                    if( standard_date < daylight_date )
                    {
                        _next_transition_time = standard_date;  // Kann 0 sein
                        _next_transition_name = S() << "begin of standard time: " << string_from_ole( time_zone_information.StandardName );
                    }
                    else
                    {
                        _next_transition_time = daylight_date;  // Kann 0 sein
                        _next_transition_name = S() << "begin of daylight saving time: " << string_from_ole( time_zone_information.DaylightName );
                    }
                }
                else
                {
                    _next_transition_time = time_max;
                    _next_transition_name = "no daylight saving";
                }
            }
        }
      else
#   endif
    {
        time_t local_now         = localtime_from_gmtime( now );
        time_t local_midnight    = local_now / (24*3600) * 24*3600;
        time_t local_switch_time;
        
        if( _was_in_daylight_saving_time )
        {
            local_switch_time = local_midnight + 3*3600;        // 3:00
            _next_transition_name = "possible begin of standard time";
        }
        else
        {
            local_switch_time = local_midnight + 2*3600;        // 3:00
            _next_transition_name = "possible begin of daylight saving time";
        }

        int wait_time = (int)( local_switch_time - local_now );
        if( wait_time < 0 )  wait_time += 24*3600;    // Schon vorbei? Dann +24h

        _next_transition_time = now + wait_time;
    }


    set_async_next_gmtime( _next_transition_time - 1 );    // Eine Sekunde vorher async_continue_() aufrufen


    sos::scheduler::time::Time::set_current_difference_to_utc( now );
}

//----------------------------------------Daylight_saving_time_transition_detector::async_continue_

bool Daylight_saving_time_transition_detector::async_continue_( Continue_flags )
{
    bool result = false;
    bool was_in_daylight_saving_time = _was_in_daylight_saving_time;
    time_t now = ::time(NULL);

    time_t until = _next_transition_time + 1;       // Bis eine Sekunde nach der Zeitumschaltung warten
    
    if( now < until )
    {
        if( now < until - 2 )   // Paranoid
        {
            Z_LOG( Z_FUNCTION << " Aufruf zu früh *******\n" );   // Sonst würden wir zu lange schlafen
        }
        else
        {
            while( now < until )                      // Warten von 01:59:59 bis 02:00:01
            {
                time_t t = until - now;
                Z_LOG2( "scheduler", Z_FUNCTION << "  sleep " << t << "s\n"  );
                sleep( (double)t );
                now = ::time(NULL);
            }
        }
    }

    set_alarm( now );

    if( was_in_daylight_saving_time  != _was_in_daylight_saving_time )
    {
        result = true;
        _log->info( message_string( _was_in_daylight_saving_time? "SCHEDULER-951" : "SCHEDULER-952" ) );
    }
    else
    {
        Z_DEBUG_ONLY( _log->debug9( "(no change of daylight saving time)" ) );
    }

    return result;
}

//--------------------------------------Daylight_saving_time_transition_detector::async_state_text_

string Daylight_saving_time_transition_detector::async_state_text_() const
{
    S result;
    result << obj_name() << ", " << _next_transition_name;
 
    //if( _was_in_daylight_saving_time )  result << ", waiting for end of daylight saving_time";
    //                              else  result << ", waiting for begin of daylight saving time";

    return result;
}

//------------------------------------------------------------------------------Period::set_default

void Period::set_default()
{
    _begin           = 0;             // = "00:00:00";
    _end             = 24*60*60;      // = "24:00:00";
    _repeat          = Time::never;
    _absolute_repeat = Time::never;
}

//-------------------------------------------------------------------------Period::set_single_start

void Period::set_single_start( const Time& t )
{
    _begin           = t;
    _end             = t;
    _repeat          = Time::never;
    _absolute_repeat = Time::never;
    _single_start    = true;
    _let_run         = true;
}

//----------------------------------------------------------------------------------Period::set_dom

void Period::set_dom( const xml::Element_ptr& element, Period::With_or_without_date w, const Period* deflt )
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
        set_single_start( Time( dt ) );
    }
    else
    {
        string begin = element.getAttribute( "begin", "00:00:00" );
        if( !begin.empty() )
        {
            if( w == with_date )  dt = begin;
                            else  dt.set_time( begin );
            _begin = dt;
        }

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

        if( _repeat == 0 )  _repeat = Time::never;


        string absolute_repeat = element.getAttribute( "absolute_repeat" );
        if( !absolute_repeat.empty() )
        {
            if( absolute_repeat.find( ':' ) != string::npos )
            {
                Sos_optional_date_time dt;
                dt.set_time( absolute_repeat );
                _absolute_repeat = dt.time_as_double();
            }
            else
                _absolute_repeat = as_double( absolute_repeat );

            if( _absolute_repeat == 0 )  z::throw_xc( "SCHEDULER-441", element.nodeName(), "absolute_repeat", "0" );
            if( !_absolute_repeat.is_never() )  _repeat = Time::never;
          //if( !_repeat.is_never()   )  z::throw_xc( "SCHEDULER-442", "absolute_repeat", "repeat" );
        }
    }

    string end = element.getAttribute( "end" , "24:00:00" );
    if( !end.empty() )
    {
        if( w == with_date )  dt = end;
                        else  dt.set_time( end );
        _end = dt;
    }

    _start_once = element.bool_getAttribute( "start_once", _start_once );   // Für Joacim Zschimmer
    //Wird das schon benutzt? Ist nicht berechnet.  if( _start_once  &&  !_spooler->_zschimmer_mode )  z::throw_xc( Z_FUNCTION, "Attribute start_once is not supported" );

    check( w );
}

//------------------------------------------------------------------------------------Period::check

void Period::check( With_or_without_date w ) const
{
    if( _begin < 0      )  goto FEHLER;
    if( _begin > _end   )  goto FEHLER;
    if( w == without_date )  if( _end > 24*60*60 )  goto FEHLER;
    return;

  FEHLER:
    z::throw_xc( "SCHEDULER-104", _begin.as_string(), _end.as_string() );
}

//-------------------------------------------------------------------------------Period::is_comming

bool Period::is_comming( const Time& time_of_day, With_single_start single_start ) const
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
    if( single_start & wss_next_any_start  &&  ( ( _single_start || has_repeat_or_once() ) && time_of_day <= _begin ) )  result = true;
                                                                                                       // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
    if( single_start & ( wss_next_any_start | wss_next_single_start )  &&  !_absolute_repeat.is_never()  &&  !next_absolute_repeated( time_of_day, 0 ).is_never() )  result = true;
                                                                                                                                                // ^ Falls zwei Perioden direkt aufeinander folgen
    else
        result = false;

    //Z_LOG2( "joacim", *this << ".is_comming(" << time_of_day << ',' << (int)single_start << ") ==> " << result << "\n" );

    return result;
}

//---------------------------------------------------------------------------------Period::next_try

Time Period::next_try( const Time& t )
{
    Time result = t;

    if( result >= end() )  result = Time::never;

    return result;
}

//----------------------------------------------------------------------------Period::next_repeated

Time Period::next_repeated( const Time& t ) const
{
    Time result = Time::never;
    
    if( !_repeat.is_never() )
    {
        result = t < _begin? _begin 
                           : Time( t + _repeat );
    }
    else
    if( !_absolute_repeat.is_never() )
    {
        result = next_absolute_repeated( t, 1 );
    }

    return result < _end? result : Time::never;  
}

//-------------------------------------------------------------------Period::next_absolute_repeated

Time Period::next_absolute_repeated( const Time& t, int next ) const
{
    assert( next == 0  ||  next == 1 );
    assert( !_absolute_repeat.is_never() );


    Time result = Time::never;

    if( t < _begin )
    {
        result = _begin;
    }
    else
    {
        int n = (int)( ( t - _begin ) / _absolute_repeat );
        result = _begin + ( n + 1 ) * _absolute_repeat;
        if( result == t + _absolute_repeat  &&  next == 0 )  result = t;
    }

    assert( next == 0? result >= t : result > t );

    return result < _end? result : Time::never;  
}

//------------------------------------------------------------------------------------Period::print

void Period::print( ostream& s ) const
{
    s << "Period(" << _begin << ".." << _end;
    if( _single_start )  s << " single_start";
    if( !_repeat.is_never() )  s << " repeat=" << _repeat;
    if( !_absolute_repeat.is_never() )  s << " absolute_repeat=" << _absolute_repeat;
    if( _let_run )  s << " let_run";
    s << ")";
}

//------------------------------------------------------------------------------Period::dom_element

xml::Element_ptr Period::dom_element( const xml::Document_ptr& dom_document ) const
{
    xml::Element_ptr result;

    if( _single_start )
    {
        result =  dom_document.createElement( "at" );
        result.setAttribute( "at", _begin.xml_value( Time::without_ms ) );
    }
    else
    {
        result =  dom_document.createElement( "period" );
        result.setAttribute( "begin", _begin.xml_value( Time::without_ms ) );
        result.setAttribute( "end"  , _end  .xml_value( Time::without_ms ) );
        if( _let_run )  result.setAttribute( "let_run", "yes" );

        if( _repeat          != Time::never )  result.setAttribute( "repeat"         , _repeat.as_time_t() );
        if( _absolute_repeat != Time::never )  result.setAttribute( "absolute_repeat", _absolute_repeat.as_time_t() );
    }

    return result;
}

//-----------------------------------------------------------------------------------Period::to_xml

string Period::to_xml() const
{
    S result;

    result << "<period";
    result << " begin=\"" << _begin.as_string() << "\"";
    result << " end=\""   << _end  .as_string() << "\"";
    if( !_repeat         .is_never() )  result << " repeat=\""          << _repeat         .as_string() << "\"";
    if( !_absolute_repeat.is_never() )  result << " absolute_repeat=\"" << _absolute_repeat.as_string() << "\"";
    result << "/>";

    return result;
}

//---------------------------------------------------------------------------------Period::obj_name

string Period::obj_name() const
{
    ostrstream s;
    print( s );
    s << '\0';
    return s.str();
}

//-----------------------------------------------------------------------------Day::set_dom_periods

void Day::set_dom_periods( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    if( !element )  return;

    if( default_day )  _period_set = default_day->_period_set;

    bool first = true;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( first )  first = false, _period_set.clear();
        _period_set.insert( Period( e, default_period ) );
    }
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

bool Day::has_time( const Time& time_of_day )
{
    FOR_EACH( Period_set, _period_set, it )
    {
        if( it->begin() >= time_of_day || it->end() > time_of_day )  return true;
    }

    return false;
}

//--------------------------------------------------------------------------------Day::next_period_

Period Day::next_period_( const Time& time_of_day, With_single_start single_start ) const
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

//---------------------------------------------------------------------------------Day_set::Day_set

Day_set::Day_set( int minimum, int maximum )
: 
    _zero_(this+1), 
    _minimum(minimum), 
    _maximum(maximum),
    _days(1+maximum)
{
} 

//---------------------------------------------------------------------------------Day_set::Day_set
    
Day_set::Day_set( int minimum, int maximum, const xml::Element_ptr& e )  
: 
    _zero_(this+1), 
    _minimum(minimum), 
    _maximum(maximum),
    _days(1+maximum)
{ 
    set_dom(e); 
}

//-----------------------------------------------------------------------------------Day_set::print

void Day_set::print( ostream& s ) const
{
    for( int i = 0; i < _days.size(); i++ )
    {
        if( _days[i] )  s << ' ' << i << "=" << *_days[i];
    }
}

//-------------------------------------------------------------------Weekday_set::fill_with_default

void Weekday_set::fill_with_default( const Day& default_day )
{ 
    ptr<Day> day = Z_NEW( Day( default_day ) );

    for( int i = 0; i < 7; i++ )  _days[i] = day;

    _is_day_set_filled = true;
}

//--------------------------------------------------------------------------------Weekday_set::next

Period Weekday_set::next_period( const Time& tim, With_single_start single_start )
{
    if( _is_day_set_filled )
    {
        Time time_of_day = tim.time_of_day();
        int  day_nr      = tim.day_nr();
        int  weekday     = weekday_of_day_number( day_nr );

        for( int i = weekday; i <= weekday+7; i++ )
        {
            if( const Day* day = _days[ i % 7 ] )
            {
                Period period = day->next_period( time_of_day, single_start );
                if( !period.empty() )  return day_nr*(24*60*60) + period;
            }

            day_nr++;
            time_of_day = 0;
        }
    }

    return Period();
}

//----------------------------------------------------------------------------Monthday_set::set_dom

void Monthday_set::set_dom( const xml::Element_ptr& monthdays_element, const Day* default_day, const Period* default_period )
{
    if( monthdays_element )
    {
        DOM_FOR_EACH_ELEMENT( monthdays_element, element )
        {
            if( element.nodeName_is( "day" ) )
            {
                Day       my_default_day ( element, default_day, default_period );
                list<int> day_numbers    = get_day_numbers( element.getAttribute( "day" ) );
                ptr<Day>  day            = Z_NEW( Day( element, &my_default_day, default_period ) );
                
                Z_FOR_EACH( list<int>, day_numbers, d )
                {
                    if( _days[ *d ] )  z::throw_xc( "SCHEDULER-444", *d );
                    _days[ *d ] = day;
                }

                _is_day_set_filled = true;
            }
            else
            if( element.nodeName_is( "weekday" ) )
            {
                Day       my_default_day ( element, default_day, default_period );
                list<int> weekdays       = get_weekday_numbers( element.getAttribute( "day" ) );
                int       which          = element.int_getAttribute( "which" );

                if( which == 0  ||  abs( which ) > max_weekdays_per_month )  z::throw_xc( Z_FUNCTION, S() << "Invalid value for which=" << which );    // XML-Schema hat schon geprüft

                Month_weekdays* m   = which > 0? &_month_weekdays : &_reverse_month_weekdays;
                ptr<Day>        day = Z_NEW( Day( element, &my_default_day, default_period ) );
                
                Z_FOR_EACH( list<int>, weekdays, w )
                {
                    if( m->day( abs(which), *w ) )  z::throw_xc( "SCHEDULER-446", *w, which );
                    m->day( abs(which), *w ) = day;
                }

                m->_is_filled = true;
            }
        }
    }
}

//------------------------------------------------------------------------Monthday_set::next_period

Period Monthday_set::next_period( const Time& tim, With_single_start single_start )
{
    Period result;

    if( is_filled() )
    {
        Time                    time_of_day = tim.time_of_day();
        int                     day_nr      = tim.day_nr();
        Sos_optional_date_time  date        = tim.as_time_t();
        int                     weekday     = weekday_of_day_number( day_nr );

        for( int i = 0; i < 31; i++ )
        {
            Period period;

            if( _is_day_set_filled )
            {
                if( const Day* day = _days[ date.day() ] )
                {
                    period = day->next_period( time_of_day, single_start );
                }
            }

            if( _month_weekdays._is_filled )
            {
                int which =  1 + ( date.day() - 1 ) / 7;

                if( Day* day = _month_weekdays.day( which, weekday ) )
                {
                    Period period2 = day->next_period( time_of_day, single_start );
                    if( period > period2 )  period = period2;
                }
            }

            if( _reverse_month_weekdays._is_filled )
            {
                int reverse_which = 1 + ( last_day_of_month( date ) - date.day() ) / 7;

                if( Day* day = _reverse_month_weekdays.day( reverse_which, weekday ) )
                {
                    Period period3 = day->next_period( time_of_day, single_start );
                    if( period > period3 )  period = period3;
                }
            }

            if( !period.empty() )
            {
                result = day_nr*(24*60*60) + period;
                break;
            }

            day_nr++;
            date.add_days(1);
            if( ++weekday == 7 )  weekday = 0;
            time_of_day = 0;
        }
    }

    return result;
}

//----------------------------------------------------------------Monthday_set::Month_weekdays::day

ptr<Day>& Monthday_set::Month_weekdays::day( int which, int weekday_number ) 
{ 
    assert( which != 0  && which <= max_weekdays_per_month );
    assert( weekday_number >= 0  &&  weekday_number < 7 );

    int index = 7 * ( which - 1 ) + weekday_number;

    return _days[ index ]; 
}

//--------------------------------------------------------------------------Ultimo_set::next_period

Period Ultimo_set::next_period( const Time& tim, With_single_start single_start )
{
    if( _is_day_set_filled )
    {
        Time     time_of_day = tim.time_of_day();
        int      day_nr      = tim.day_nr();
        Sos_date date        = Sos_optional_date_time( (time_t)tim );

        for( int i = 0; i < 31; i++ )
        {
            if( const Day* day = _days[ last_day_of_month( date ) - date.day() ] )
            {
                const Period& period = day->next_period( time_of_day, single_start );
                if( !period.empty() )  return day_nr*(24*60*60) + period;
            }

            day_nr++;
            date.add_days(1);
            time_of_day = 0;
        }
    }

    return Period();
}

//--------------------------------------------------------------------------------Holidays::set_dom

void Holidays::set_dom( const xml::Element_ptr& e, int include_nesting )
{
    if( e.nodeName_is( "holidays" ) )
    {
        DOM_FOR_EACH_ELEMENT( e, e2 )
        {
            if( e2.nodeName_is( "holiday" ) )
            {
                Sos_optional_date_time dt;
                dt.assign( e2.getAttribute( "date" ) );
                include( dt.as_time_t() );
            }
            else
            if( e2.nodeName_is( "include" ) )
            {
                if( include_nesting >= max_include_nesting )  z::throw_xc( "SCHEDULER-390", max_include_nesting, "<holidays>" );

                File_path file = subst_env( e2.getAttribute( "file" ) );

                try
                {
                    if( !file.is_absolute_path() )  file.prepend_directory( _spooler->_configuration_file_path.directory() );
                    
                    string xml_text = string_from_file( file );
                    Z_LOG2( "scheduler", Z_FUNCTION << "  " << xml_text << "\n" );

                    xml::Document_ptr doc;
                    doc.load_xml( xml_text );
                    if( _spooler->_validate_xml )  _spooler->_schema.validate( doc );

                    if( !doc.documentElement() )  z::throw_xc( "SCHEDULER-319", "", file );

                    set_dom( doc.documentElement(), include_nesting+1 );
                }
                catch( z::Xc& x )
                {
                    x.append_text( S() << "in <holiday><include file=\"" << file << "\"/>" );
                    throw;
                }
            }
            else
                z::throw_xc( "SCHEDULER-319", e2.nodeName(), e.nodeName() );
        }
    }
    else
    if( e.nodeName_is( "holiday" ) )
    {   
        Sos_optional_date dt;
        dt.assign( e.getAttribute( "date" ) );
        include( dt.as_time_t() );
    }
    else
        z::throw_xc( "SCHEDULER-319", e.nodeName(), Z_FUNCTION );
}

//--------------------------------------------------------------------------------------Date::print

void Date::print( ostream& s ) const
{
    Sos_optional_date dt;
    dt.set_time_t( _day_nr * (24*60*60) );

    s << "Date(" << dt << " " << _day << ')';
}

//----------------------------------------------------------------------------Date_set::next_period

Period Date_set::next_period( const Time& tim, With_single_start single_start )
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

//------------------------------------------------------------------------------At_set::next_period

Period At_set::next_period( const Time& tim, With_single_start single_start )
{
    if( single_start & wss_next_single_start )
    {
        FOR_EACH( set<Time>, _at_set, it )
        {
            const Time& time = *it;

            if( time >= tim )
            {
                Period result;
                
                result._begin           = time;
                result._end             = time;
                result._single_start    = true;
                result._let_run         = true;
                result._repeat          = Time::never;
                result._absolute_repeat = Time::never;

                return result;
            }
        }
    }

    return Period();
}

//---------------------------------------------------------------------------------Date_set::print

void At_set::print( ostream& s ) const
{
    s << "At_set(";

    FOR_EACH_CONST( set<Time>, _at_set, it )
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
            Day       my_default_day ( e, default_day, default_period );
            ptr<Day>  day            = Z_NEW( Day( e, &my_default_day, default_period ) );
            list<int> day_numbers    = get_day_numbers( e.getAttribute( "day" ) );

            Z_FOR_EACH( list<int>, day_numbers, it )
            {
                if( _days[ *it ] )  z::throw_xc( "SCHEDULER-444", *it );
                _days[ *it ] = day;
            }

            _is_day_set_filled = true;
        }
    }
}

//---------------------------------------------------------------------------------Day::set_dom_day

int Day_set::get_day_number( const string& day_string )
{
    int result;

    if( _minimum == 0  &&  _maximum == 6 )      // Es ist ein Wochentag
    {
        result = get_weekday_number( day_string );
    }
    else
    {
        result = as_int( day_string );

        if( result < _minimum  ||  result > _maximum )  z::throw_xc( "SCHEDULER-221", day_string, as_string( _minimum ), as_string( _maximum ) );
        //if( (uint)result >= NO_OF(_days) )  z::throw_xc( "SCHEDULER-INVALID-DAY", result );
    }

    return result;
}

//-------------------------------------------------------------------------Day_set::get_day_numbers

list<int> Day_set::get_day_numbers( const string& days_string )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", days_string );

    Z_FOR_EACH( vector<string>, names, it )  result.push_back( get_day_number( *it ) );

    return result;
}

//----------------------------------------------------------------------Day_set::get_weekday_number

int Day_set::get_weekday_number( const string& weekday_name )
{
    int    result     = -1;
    string day_string = lcase( weekday_name );
    
    for( const char** p = weekday_names; *p && result == -1; p++ )
        if( day_string == *p )  result = ( p - weekday_names ) % 7;

    if( result == -1 )
    {
        result = as_int( weekday_name );
        if( result == 7 )  result = 0;      // Sonntag darf auch 7 sein
    }

    if( result < 0  ||  result > 6 )  z::throw_xc( "SCHEDULER-445", weekday_name );

    return result;
}

//---------------------------------------------------------------------Day_set::get_weekday_numbers

list<int> Day_set::get_weekday_numbers( const string& weekday_names )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", weekday_names );

    Z_FOR_EACH( vector<string>, names, it )  result.push_back( get_weekday_number( *it ) );

    return result;
}

//-------------------------------------------------------------------------------Month::set_default

void Month::set_default()
{
    Day default_day;

    default_day.set_default();
    _weekday_set.fill_with_default( default_day );
}

//-----------------------------------------------------------------------------------Month::set_dom

void Month::set_dom( const xml::Element_ptr& element )
{
    Period  default_period;
    Day     default_day;
    bool    period_seen    = false;

    default_period.set_dom( element );
    default_day = default_period;

    bool a_day_is_set = false;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "period" ) )
        {
            if( !period_seen )  period_seen = true, default_day = Day();
            default_day.add( Period( e, &default_period ) );
        }
        else
        if( e.nodeName_is( "weekdays" ) )
        {
            a_day_is_set = true;
            _weekday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "monthdays" ) )
        {
            a_day_is_set = true;
            _monthday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "ultimos" ) )
        {
            a_day_is_set = true;
            _ultimo_set.set_dom( e, &default_day, &default_period );
        }
    }

    if( !a_day_is_set )  _weekday_set.fill_with_default( default_day );
}

//-------------------------------------------------------------------------------Month::next_period

Period Month::next_period( const Time& tim_, With_single_start single_start )
{
    Time   tim           = tim_;
    Period result;
    int    current_month = tim.month_nr();

    while( tim < Time::never )
    {
        int m = tim.month_nr();
        if( m != current_month )  break;

        if( _weekday_set .is_filled() )  result = min( result, _weekday_set .next_period( tim, single_start ) );
        if( _monthday_set.is_filled() )  result = min( result, _monthday_set.next_period( tim, single_start ) );
        if( _ultimo_set  .is_filled() )  result = min( result, _ultimo_set  .next_period( tim, single_start ) );
        
        if( result.begin() != Time::never )  break;

        tim = tim.midnight() + 24*60*60;                        // Keine Periode? Dann nächsten Tag probieren
    }

    if( !result.begin().is_never()  &&  result.begin().month_nr() != current_month )  result = Period();

    return result;
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

Run_time::Run_time( Scheduler_object* host_object )
:
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1),
    _spooler(host_object->_spooler),
    _host_object(host_object),
    _holidays(host_object->_spooler),
    _months(12)
{
    _log = host_object->log();  //Z_NEW( Prefix_log( host_object ) );

    if( _host_object->scheduler_type_code() == Scheduler_object::type_order )
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

//----------------------------------------------------------------------------------Run_time::close

void Run_time::close()
{
    _host_object = NULL;
}

//--------------------------------------------------------------------------------Run_time::put_Xml

STDMETHODIMP Run_time::put_Xml( BSTR xml )
{
    Z_COM_IMPLEMENT( set_xml( string_from_bstr( xml ) ) );

    // *** this ist ungültig ***
}

//----------------------------------------------------------------------------Run_time::operator ==

bool Run_time::operator == ( const Run_time& r )
{
    return dom_document().xml() == r.dom_document().xml();
}

//----------------------------------------------------------------------------Run_time::set_default

void Run_time::set_default()
{
    Day default_day;

    default_day.set_default();
    _weekday_set.fill_with_default( default_day );
    //for( int i = 0; i < 12; i++ )
    //{
    //    if( !_months[ i ] )
    //    {
    //        _months[ i ] = Z_NEW( Month() );
    //        _months[ i ]->set_default();
    //    }
    //}
}

//--------------------------------------------------------------------------------Run_time::set_xml

void Run_time::set_xml( const string& xml )
{
    //_xml = xml;

    //set_dom( _spooler->_dtd.validate_xml( xml ).documentElement() );
    xml::Document_ptr doc ( xml );
    if( _spooler->_validate_xml )  _spooler->_schema.validate( xml::Document_ptr( xml ) );

    if( !_host_object  ||  _host_object->scheduler_type_code() != Scheduler_object::type_order )  z::throw_xc( "SCHEDULER-352" );
    dynamic_cast<Order*>( _host_object ) -> set_run_time( doc.documentElement() );

    // *** this ist ungültig ***
}

//--------------------------------------------------------------------------Run_time::call_function

Period Run_time::call_function( const Time& requested_beginning )
{
    // Die Funktion sollte keine Zeit in der wiederholten Stunde nach Ende der Sommerzeit liefern.


    Period result;

    if( !_start_time_function_error )
    {
        try
        {

            string date_string = requested_beginning.as_string( Time::without_ms );
            string param2      = !_host_object? "" :
                                 _host_object->scheduler_type_code() == Scheduler_object::type_order? dynamic_cast<Order*>( +_host_object )->string_id() :
                                 _host_object->scheduler_type_code() == Scheduler_object::type_job  ? dynamic_cast<Job*  >( +_host_object )->name()
                                                                                                    : "";
            Scheduler_script* scheduler_script = _spooler->scheduler_script_subsystem()->default_scheduler_script();
            string            function_name    = _start_time_function;
            if( scheduler_script ->module()->kind() == Module::kind_java )  function_name += "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";

            Variant v = scheduler_script ->module_instance()->call( function_name, date_string, param2 );
            
            if( !v.is_null_or_empty_string() )
            {
                Time t;
                if( variant_is_numeric( v ) )  t = (time_t)v.as_int64();
                else
                if( v.vt == VT_DATE         )  t = seconds_since_1970_from_com_date( V_DATE( &v ) );
                else                           
                                               t.set_datetime( v.as_string() );

                if( t < requested_beginning )
                {
                    //if( _log )  _log->warn( message_string( "SCHEDULER-394", _start_time_function, t, requested_beginning ) );
                    z::throw_xc( "SCHEDULER-394", _start_time_function, t, requested_beginning );
                }
                else 
                if( !t.is_never() ) 
                {
                    result.set_single_start( t );
                }
            }
        }
        catch( exception& x )
        {
            _start_time_function_error = true;
            z::throw_xc( "SCHEDULER-393", _start_time_function, x );
        }
    }

    return result;
}

//--------------------------------------------------------------------Run_time::month_index_by_name

int Run_time::month_index_by_name( const string& mm )
{
    string m = lcase( mm );

    for( int i = 0; i < 12; i++ )
    {
        if( m == month_names[ i ] )  return i;
    }

    int result = as_int( m );
    if( result >= 1  &&  result <= 12 )  return result - 1;

    z::throw_xc( "INVALID-MONTH", mm );      // Sollte wegen XML-Schemas nicht passieren
}

//-----------------------------------------------------------------Run_time::month_indices_by_names

list<int> Run_time::month_indices_by_names( const string& mm )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", mm );

    Z_FOR_EACH( vector<string>, names, it )
    {
        result.push_back( month_index_by_name( *it ) );
    }

    return result;
}

//--------------------------------------------------------------------------------Run_time::set_dom

void Run_time::set_dom( const xml::Element_ptr& element )
{
    if( !element )  return;

    if( _modified_event_handler )  _modified_event_handler->on_before_modify_run_time();


    Sos_optional_date_time  dt;
    Period                  default_period;
    Day                     default_day;
    bool                    period_seen    = false;


    _dom.create();
    _dom.appendChild( _dom.clone( element ) );

    _set = true;

    _once = element.bool_getAttribute( "once", _once );
    if( _host_object  &&  _host_object->scheduler_type_code() == Scheduler_object::type_order  &&  !_once )  z::throw_xc( "SCHEDULER-220", "once='no'" );

    _start_time_function = element.getAttribute( "start_time_function" );

    default_period.set_dom( element );
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
        if( e.nodeName_is( "at" ) )
        {
            a_day_set = true;
            string at = e.getAttribute( "at" );
            if( at == "now" )  _once = true;   // "now" wirkt nicht in _at_set, weil der Zeitpunkt gleich verstrichen sein wird
            Time at_time;
            at_time.set_datetime( at );
            _at_set.add( at_time );
        }
        else
        if( e.nodeName_is( "date" ) )
        {
            a_day_set = true;
            dt.assign( e.getAttribute( "date" ) );
            if( !dt.time_is_zero() )  z::throw_xc( "SCHEDULER-208", e.getAttribute( "date" ) );
            Date date;
            date._day_nr = (int)( dt.as_time_t() / (24*60*60) );
            date._day.set_dom_periods( e, &default_day, &default_period );
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
        if( e.nodeName_is( "month" ) )
        {
            a_day_set = true;

            ptr<Month> month = Z_NEW( Month() );
            month->set_dom( e );

            list<int> months = month_indices_by_names( e.getAttribute( "month" ) );
            
            Z_FOR_EACH( list<int>, months, it )
            {
                if( _months[ *it ] )  z::throw_xc( "SCHEDULER-443", month_names[ *it ] );
                _months[ *it ] = month;
            }
        }
        if( e.nodeName_is( "holidays" ) )
        {
            _holidays.clear();
            _holidays.set_dom( e );
        }
        else
        if( e.nodeName_is( "holiday" ) )
        {
            _holidays.set_dom( e );
        }
    }


    if( !a_day_set )  _weekday_set.fill_with_default( default_day );
    //if( !a_day_is_set ) 
    //{
    //    ptr<Month> default_month = Z_NEW( Month() );
    //    default_month->_weekday_set.fill_with_default( default_day );
    //    for( int i = 0; i < 12; i++ )  if( !_months[ i ] )  _months[ i ] = default_month;
    //}

    if( _modified_event_handler )  _modified_event_handler->run_time_modified_event();
}

//----------------------------------------------------------------------------Run_time::dom_element

xml::Element_ptr Run_time::dom_element( const xml::Document_ptr& document ) const
{
    return document.clone( _dom.documentElement() );
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

Period Run_time::first_period( const Time& beginning_time )
{
    return next_period( beginning_time );
}

//------------------------------------------------------------------------------Run_time::is_filled

bool Run_time::is_filled() const
{
    bool result = _at_set  .is_filled() 
               || _date_set.is_filled();

    if( !result )
    {
        for( int i = 0; i < 12; i++ )
        {
            result = _months[ i ]  &&  _months[ i ]->is_filled();
            if( result )  break;
        }
    }

    return result;
}

//----------------------------------------------------------------------------Run_time::next_period

Period Run_time::next_period( const Time& beginning_time, With_single_start single_start )
{
    Period result;
    Time   tim   = beginning_time;
    bool   is_no_function_warning_logged = false; 
    Period last_function_result;
    
    last_function_result.set_single_start( 0 );

    while( tim < Time::never )
    {
        bool something_called = false;

        if( _start_time_function != ""  &&  single_start & ( wss_next_any_start | wss_next_single_start ) )
        {
            if( _spooler->scheduler_script_subsystem()->subsystem_state() != subsys_active  &&  _log  &&  !is_no_function_warning_logged )
            {
                _log->warn( message_string( "SCHEDULER-844", _start_time_function, Z_FUNCTION ) );
                is_no_function_warning_logged = true;
            }
            else
            if( last_function_result.begin() < tim )
            {
                try
                {
                    last_function_result = min( result, call_function( tim ) );
                    result = last_function_result;
                }
                catch( exception& x )
                {
                    _log->error( x.what() );
                    _log->error( message_string( "SCHEDULER-398", _start_time_function ) );
                }
            }

            something_called = true;
        }

        if( !tim.is_never()  ||  is_filled() )
        {
            if( _at_set      .is_filled() )  result = min( result, _at_set      .next_period( tim, single_start ) );
            if( _date_set    .is_filled() )  result = min( result, _date_set    .next_period( tim, single_start ) );

            int m = tim.month_nr() - 1;
            if( _months[ m ] )
            {
                result = min( result, _months[ m ]->next_period( tim, single_start ) );
            }
            else
            {
                if( _weekday_set .is_filled() )  result = min( result, _weekday_set .next_period( tim, single_start ) );
                if( _monthday_set.is_filled() )  result = min( result, _monthday_set.next_period( tim, single_start ) );
                if( _ultimo_set  .is_filled() )  result = min( result, _ultimo_set  .next_period( tim, single_start ) );
            }

            something_called = true;
        }

        if( !something_called )  break;                         // <run_time> ist leer

        if( result.begin() != Time::never )
        {
            if( !_holidays.is_included( result.begin() ) )  break;     // Gefundener Zeitpunkt ist kein Feiertag? Dann ok!

            // Feiertag
            tim    = result.begin().midnight() + 24*60*60;      // Nächsten Tag probieren
            result = Period();
        }
        else
        {
            tim = tim.midnight() + 24*60*60;                    // Keine Periode? Dann nächsten Tag probieren
        }

        if( tim >= beginning_time + 366*24*60*60 )  break;     // Längstens ein Jahr ab beginning_time voraussehen
    }


    return result;
}

//----------------------------------------------------------------------Run_time::next_single_start

Time Run_time::next_single_start( const Time& time )
{ 
    Period period = next_period( time, wss_next_single_start );
    
    return !period.absolute_repeat().is_never()? period.next_repeated( time )
                                               : period.begin();
}

//-------------------------------------------------------------------------Run_time::next_any_start

Time Run_time::next_any_start( const Time& time )
{ 
    Period period = next_period( time, wss_next_any_start );

    return !period.absolute_repeat().is_never()? period.next_repeated( time )
                                               : period.begin();
}

//-----------------------------------------------------------Run_time::append_calendar_dom_elements

void Run_time::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    Time t          = options->_from;
    Time last_begin;

    //bool check_for_once = _once;

    while( options->_count < options->_limit )
    {
        Period period = next_period( t, wss_next_any_start );  
        if( period.begin() >= options->_before  ||  period.begin() == Time::never )  break;

        //if( period._start_once  ||
        //    period._single_start  ||     
        //    _host_object->scheduler_type_code() == Scheduler_object::type_order ||
        //    check_for_once 
        {
            //check_for_once = false;

            if( period.begin() >= options->_from  &&  period.begin() != last_begin )
            {
                element.appendChild( period.dom_element( element.ownerDocument() ) );
                options->_count++;

                last_begin = period.begin();
            }
        }

        t = period._single_start? period.begin() + 1 : period.end();
    }
}

//----------------------------------------------------------------------------------Run_time::print

//void Run_time::print( ostream& s ) const
//{
//    s << "Run_time(\n"
//         "    date_set     =" << _date_set     << "\n"
//         "    weekday_set  =" << _weekday_set  << "\n"
//         "    monthday_set =" << _monthday_set << "\n"
//         "    ultimo_set   =" << _ultimo_set   << "\n"
//         ")\n";
//}

//-------------------------------------------------------------------------------------------------

} //namespace time


//--------------------------------------------------------------------------------------Gmtime::set
/*
void Gmtime::set( double t )
{
    _time = round(t);

    if( _time > double_time_max )  _time = double_time_max;


#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time <= 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == double_time_max? time::last_day_name
                                                                    : as_string();
#   endif
}

//---------------------------------------------------------------------------Gmtime::set_local_time

void Gmtime::set_local_time( const Time& t )
{
#   if defined Z_DEBUG
        assert( t == 0  ||  t > 30*365*3600 );
#   endif

    set( t == Time::never? double_time_max 
                        : gmtime_from_localtime( t ) );
}

//-------------------------------------------------------------------------------Gmtime::local_time

Time Gmtime::local_time() const
{
#   if defined Z_DEBUG
        assert( _time == 0  ||  _time > 30*365*3600 );
#   endif

    return _time == double_time_max? Time::never
                                   : Time( localtime_from_gmtime( _time ) );
}

//--------------------------------------------------------------------------------Gmtime::as_string

string Gmtime::as_string( With_ms with ) const
{
    string result;

    if( _time == double_time_max )
    {
        return time::last_day_name;
    }
    else
    if( _time == 0 )
    {
        return time::immediately_name;
    }
    else
    if( _time < 0 )
    {
        result = "-" + zschimmer::as_string( (int64)-_time );
    }
    else
    {
        char        buff [30];
        const char* bruch = with == with_ms? buff + sprintf( buff, "%0.3lf", _time ) - 4
                                           : "";

        if( _time < 100*(24*60*60) )
        {
            char hhmmss [30];
            sprintf( hhmmss, "%02d:%02d:%02d%s", (int)(_time/(60*60)), abs( (int)(_time/60) ) % 60, (int)abs( (int64)_time % 60 ), bruch );
            return hhmmss;
        }
        else
        {
            return string_gmt_from_time_t( (time_t)_time ) + bruch + " GMT";
        }
    }

    return result;
}

//--------------------------------------------------------------------------Gmtime::as_local_string

string Gmtime::as_local_string( With_ms w ) const
{
    return local_time().as_string( w == with_ms? time::Time::with_ms : time::Time::without_ms );
}

//------------------------------------------------------------------------------------Gmtime::round

double Gmtime::round( double t )
{ 
    return floor( t * 1000.0 + 0.5 ) / 1000.0; 
}

//--------------------------------------------------------------------------------Gmtime::normalize

double Gmtime::normalize( double t )
{ 
    return t < 0?                0 :
           t > Time::never._time? Time::never._time 
                               : t;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
