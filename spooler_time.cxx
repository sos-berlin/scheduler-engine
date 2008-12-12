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

extern const int                       never_int                   = INT_MAX;
extern const Time                      latter_day                  = never_int;
       const Time                      Time::never                 = never_int;
static const char                      never_name[]                = "never";
static const char                      immediately_name[]          = "now";
static const int64                     base_filetime               = 116444736000000000LL;

//-------------------------------------------------------------------------------------------------

//Run_time::Class_descriptor              Run_time::class_descriptor ( &typelib, "sos.spooler.Run_time", Run_time::_methods );

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
    if( str == never_name )
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

//--------------------------------------------------------------------------------Time::operator +=

void Time::operator += ( double t )
{ 
    assert( !is_never() );
    assert( t < never_int );

    if( !is_never() )  set( as_double() + t ); 
}

//--------------------------------------------------------------------------------Time::operator -=

void Time::operator -= ( double t )
{ 
    //assert( !is_never() );
    assert( t < never_int );

    set( as_double() - t );
}

//---------------------------------------------------------------------------------Time::operator +

Time Time::operator + ( const Time& t )
{ 
    assert( !is_never() );
    assert( !t.is_never() );

    return is_never() || t.is_never()? never
                                     : Time( as_double() + t.as_double() ); 
}
                                                                                                  
//---------------------------------------------------------------------------------Time::operator +

Time Time::operator + ( double t )
{ 
    assert( !is_never() );
    assert( t < never_int );

    return is_never()? never
                     : Time( as_double() + t ); 
}

//---------------------------------------------------------------------------------Time::operator -

Time Time::operator - ( const Time& t )
{ 
    //assert( !is_never() );
    assert( !t.is_never() );

    return Time( as_double() - t.as_double() ); 
}

//---------------------------------------------------------------------------------Time::operator -

Time Time::operator - ( double t )
{ 
    //assert( !is_never() );
    assert( t < never_int );

    return Time( as_double() - t ); 
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

    if( t == never_name )
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

    if( _time > never_int )  assert( !"time > never" ),  _time = never_int;


#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time == 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == never_int? time::never_name
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
    assert( !is_never() );
    //Z_DEBUG_ONLY( if( _is_utc )  Z_LOG( Z_FUNCTION << " _time=" << ::sos::as_string(_time) << " - " << current_difference_to_utc() << "\n" ) );

    return as_double_or_never();
}

//-------------------------------------------------------------------------Time::as_double_or_never

double Time::as_double_or_never() const
{
    //Z_DEBUG_ONLY( if( _is_utc )  Z_LOG( Z_FUNCTION << " _time=" << ::sos::as_string(_time) << " - " << current_difference_to_utc() << "\n" ) );

    return _is_utc? _time - current_difference_to_utc()
                  : _time;    
}

//------------------------------------------------------------------------------Time::as_utc_double

double Time::as_utc_double() const
{
    //Z_DEBUG_ONLY( if( _is_utc )  Z_LOG( Z_FUNCTION << " _time=" << ::sos::as_string(_time) << " - " << current_difference_to_utc() << "\n" ) );

    return is_never()? never_int :
           _is_utc   ? _time 
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

Time& Time::set_datetime( const string& t )
{
    if( t == never_name )
    {
        set_never();
    }
    else
    {
        bool   is_utc = false;
        string my_t   = t;

        if( string_ends_with( my_t, "Z"     ) )  my_t.erase( my_t.length() - 1 ),  is_utc = true;
        else
        if( string_ends_with( my_t, "+0000" ) )  my_t.erase( my_t.length() - 5 ),  is_utc = true;

        double fraction = cut_fraction( &my_t );

        if( is_utc )  set_utc( Sos_optional_date_time( my_t ).as_time_t() + fraction );
                else  set    ( Sos_optional_date_time( my_t ).as_time_t() + fraction );
    }

    return *this;
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
    while( p > p0  &&  isdigit( (uchar)p[-1] ) )  p--, digit_count++;
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
        result = never_name;
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

#   elif defined SYSTEM_HPUX || defined SYSTEM_SOLARIS || defined Z_AIX

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
                    else  _time_as_string = _time == double_time_max? time::never_name
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
        return time::never_name;
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
