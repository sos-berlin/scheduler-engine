/**
 * \file spooler_time.h
 * \brief Funktionalitäten für das Arbeiten mit Zeitangaben
 * \detail
 *
 * \author JZ
 * <div class="sos_branding">
 *    <p>© 2010 SOS GmbH - Berlin (<a style="color:silver" href="http://www.sos-berlin.com">http://www.sos-berlin.com</a>)</p>
 * </div>
 */

// $Id: spooler_time.cxx 13999 2010-09-02 10:53:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
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
#include "../javaproxy/com__sos__scheduler__engine__kernel__time__TimeZones.h"
typedef javaproxy::com::sos::scheduler::engine::kernel::time::TimeZones TimeZonesJ;

#ifdef SYSTEM_HPUX
#   include <time.h>
#endif

#include <sys/types.h>
#include <sys/timeb.h>

#ifndef Z_WINDOWS
#   include <sys/time.h>
#endif

//-------------------------------------------------------------------------------------------------
/*!
 * \brief Funktionalitäten für das Arbeiten mit Zeitangaben
 * \detail
 *
 * \author dev-team
 * <div class="sos_branding">
 *    <p>© 2010 SOS GmbH - Berlin (<a style="color:silver" href="http://www.sos-berlin.com">http://www.sos-berlin.com</a>)</p>
 * </div>
 */
//-------------------------------------------------------------------------------------------------

namespace sos {
namespace scheduler {
namespace time {

//--------------------------------------------------------------------------------------------const

extern const time_t                    never_int                   = INT_MAX;
extern const double                    never_double                = (double)never_int;
       const Time                      Time::never                 = Time(never_double, is_utc);
       const Duration                  Duration::eternal           = Duration(never_double);
       const Duration                  Duration::epsilon           = Duration(0.001);
       const Duration                  Duration::day               = Duration(24*60*60);
static const char                      never_name[]                = "never";
static const char                      immediately_name[]          = "now";
static const int64                     base_filetime               = 116444736000000000LL;

//-------------------------------------------------------------------------new_calendar_dom_element

xml::Element_ptr new_calendar_dom_element( const xml::Document_ptr& dom_document, const Time& t )
{
    xml::Element_ptr result = dom_document.createElement( "at" );

    result.setAttribute( "at", t.xml_value( without_ms ) );

    return result;
}
 
//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* m, int index, const Time& time ) throw()
{
    m->insert( index, time.as_string() );
}

//------------------------------------------------------------------------------insert_into_message

void insert_into_message( Message_string* m, int index, const Duration& o) throw()
{
    m->insert( index, o.as_string() ); 
}

//---------------------------------------------------------------------------------------localToUtc

static double localToUtc(const string& timeZone, double t) {
    return t == 0 || t == never_double? t : 
        TimeZonesJ::localToUtc(timeZone, (int64)(t * 1000.0 + 0.5)) / 1000.0;
}

//---------------------------------------------------------------------------------------utcToLocal

static double utcToLocal(const string& timeZone, double t) {
    return t == 0 || t == never_double? t : 
        TimeZonesJ::utcToLocal(timeZone, (int64)(t * 1000.0 + 0.5)) / 1000.0;
}

//-------------------------------------------------------------------------------------Duration::of

Duration Duration::of(const string& s) 
{
    if (s.find( ':' ) != string::npos)
    {
        Sos_optional_date_time dt;
        dt.set_time(s);
        return Duration(dt.time_as_double());
    }
    else
    if (s == never_name)
    {
        return Duration::eternal;
    }
    else
        return Duration(::zschimmer::as_double(s));
}

//------------------------------------------------------------------------------Duration::as_string

string Duration::as_string(With_ms w) const 
{
    return Time(as_double()).as_string(w);
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

//------------------------------------------------------------------------------Time::set_date_time

void Time::set_date_time(const Sos_optional_date_time& dt)
{
    if( dt.has_date() )  set( dt.as_time_t() );
                   else  set( dt.hour() * 60*60 + dt.minute() * 60 + dt.second() );
}

//--------------------------------------------------------------------------------Time::operator +=

void Time::operator += (const Duration& d)
{ 
    if (!is_never()) {
        set(d.is_eternal()? never_double : as_double() + d.as_double());
    }
}

//--------------------------------------------------------------------------------Time::operator -=

void Time::operator -= (const Duration& d)
{ 
    if (!is_never()) {
        set(d.is_eternal()? never_double : as_double() - d.as_double());
    }
}

//---------------------------------------------------------------------------------Time::operator +

Time Time::operator + ( const Duration& d ) const
{ 
    return is_never() || d.is_eternal()? never
                                       : Time( as_double() + d.as_double() );
}

//---------------------------------------------------------------------------------Time::operator +

//Time Time::operator + ( double t ) const
//{ 
//    assert( !is_never() );
//    assert( t < never_double );
//
//    return is_never()? never
//                     : Time( as_double() + t );
//}

//---------------------------------------------------------------------------------Time::operator -

Duration Time::operator - ( const Time& t ) const
{ 
    //assert( !is_never() );
    assert( !t.is_never() );

    return Duration( as_double() - t.as_double() );
}

//---------------------------------------------------------------------------------Time::operator -

//Time Time::operator - ( double t ) const
//{ 
//    //assert( !is_never() );
//    assert( t < never_double );
//
//    return Time( as_double() - t );
//}

//----------------------------------------------------------------------Time::of_date_time_with_now

/*!
* \brief Jobstartzeit ermitteln
* \details
* Folgende Angaben für den Start eines Jobs sind zugelassen:
* - yyyy-mm-dd HH:MM[:SS]
* - now+SS
* - now+MM:SS
* - now+HH:MM:SS
* \version 2.0.224
* \return Time-Objekt
*/
Time Time::of_date_time_with_now( const string& time_string )
{
    Time result;

    // Startzeit mit "now+HH:MM:SS" oder "now+MM:SS" oder "now+SS" vorgegeben?
    if( Regex_submatches matches = Regex( "^ *now *(\\+ *([^ ].*))?$" ).match_subresults( time_string ) )    // " now + HH:MM"
    {
        result = now();

        if( matches.count() > 0 )
        {
            if( matches.count() != 2 )  z::throw_xc( "SCHEDULER-333", time_string );

            string time = matches[ 2 ];

            if( time.find( ':' ) != string::npos )
            {
                result += Duration::of(time);
            }
            else
            {
                try
                {
                    result += Duration(as_int(time));
                }
                catch( exception& x ) { z::throw_xc( "SCHEDULER-333", time_string, x ); }
            }
        }
    }
    else
    {
        // nimmt eine Datumsangabe im ISO-Format entgegen
        result = of_date_time( time_string );
    }

    return result;
}

//----------------------------------------------------------------------------------------Time::set

void Time::set( const string& t )
{
    _is_utc = true;

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
    _is_utc = true; //t == 0 || t == never_double;

    if( _time > never_double )  assert( !"time > never" ),  _time = never_double;


#   if defined Z_DEBUG && defined Z_WINDOWS
        if( _time == 0 )  _time_as_string.clear();   // Für static empty_period sollte in gcc as_string() nicht gerufen werden! (Sonst Absturz)
                    else  _time_as_string = _time == never_double? time::never_name
                                                                 : as_string();
#   endif
}

//-----------------------------------------------------------------------------------Time::month_nr

int Time::month_nr() const
{
    return Sos_optional_date_time( as_time_t() ).month();
}

//------------------------------------------------------------------------------------Time::compare

int Time::compare(const Time& o) const 
{
    if (_is_utc == o._is_utc || 
        is_zero() || 
        is_never() ||
        o.is_zero() || 
        o.is_never()) {
            return _time < o._time? -1 : _time > o._time? +1 : 0;
    } else {
        assert(!"INVALID-TIME-COMPARISON");
        throw_xc("INVALID-TIME-COMPARISON", as_string(with_ms), o.as_string(with_ms));
    }
}

//----------------------------------------------------------------------------------Time::as_double

double Time::as_double() const
{
    return as_double_or_never();
}

//-------------------------------------------------------------------------Time::as_double_or_never

double Time::as_double_or_never() const
{
    if (!_is_utc && !is_zero() && !is_never()) throw_xc("NO-UTC-TIME", as_string());
    return _time;
}

//------------------------------------------------------------------------------Time::as_utc_double

double Time::as_utc_double() const
{
    if (!_is_utc && !is_zero() && !is_never()) throw_xc("NO-UTC-TIME", as_string());
    return _time;
}

//------------------------------------------------------------------------------------Time::set_utc

Time& Time::set_utc( double t )
{
    set( t );
    //if( !is_zero()  &&  !is_never() )  
    _is_utc = true;
    return *this;
}

//-------------------------------------------------------------------------------Time::of_date_time

Time Time::of_date_time( const string& t, bool utc_is_default)
{
    if( t == never_name )
    {
        return Time::never;
    }
    else
    {
        bool   utc = utc_is_default;
        string my_t   = t;

        if( string_ends_with( my_t, "Z"     ) )  my_t.erase( my_t.length() - 1 ),  utc = true;
        else
        if( string_ends_with( my_t, "+0000" ) )  my_t.erase( my_t.length() - 5 ),  utc = true;

        double fraction = cut_fraction( &my_t );
        double dt = Sos_optional_date_time( my_t ).as_time_t() + fraction;
        return utc? Time(dt, is_utc) : Time(localToUtc("", dt), is_utc);
    }
}

//-------------------------------------------------------------------------Time::of_local_date_time

Time Time::of_local_date_time( const string& t)
{
    if( t == never_name ) {
        return Time::never;
    } else {
        string my_t = t;
        double fraction = cut_fraction( &my_t );
        double dt = Sos_optional_date_time( my_t ).as_time_t() + fraction;
        return Time(dt);
    }
}

//-------------------------------------------------------------------------Time::utc_from_time_zone

Time Time::utc_from_time_zone(const string& time_zone) const
{
    return is_zero() || is_never()? Time(*this) 
        : Time(localToUtc(time_zone, as_double()), is_utc);
}

//-------------------------------------------------------------------------Time::utc_from_time_zone

Time Time::local_time(const string& time_zone) const
{
    return is_zero() || is_never()? Time(*this) 
        : Time(utcToLocal(time_zone, as_double()), is_utc);
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

    result.reserve( 23 );  // yyyy-mm-dd hh:mm:ss.mmm

    if( is_never() )
    {
        result = never_name;
    }
    else
    if( is_zero() )
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
            //if( _is_utc )  result += " UTC";    // xml_value() macht hieraus "Z"
        }
    }

    return result;
}

//----------------------------------------------------------------------------------Time::xml_value

string Time::xml_value( With_ms with ) const
{
    string str = as_string( with ) + "Z";   // Z: UTC-Zeit

    if( str.length() > 10  &&  isdigit( (unsigned char)str[0] )  &&  str[10] == ' ' )
    {
        str[10] = 'T';                      // yyyy-mm-ddThh:mm:ss.mmm
    }

    //if( string_ends_with( str, " UTC" ) )  str.replace( 23, 4, "Z" );

    return str;
}

//---------------------------------------------------------------------------------------Time::now

Time Time::now()
{
#   if !defined Z_WINDOWS
        timeval  tv;
        gettimeofday( &tv, NULL );
        return Time((double)tv.tv_sec + (double)tv.tv_usec / (double)1e6, is_utc);
#   else
        timeb  tm;
        ftime( &tm );
        return Time((double)tm.time + (double)tm.millitm / (double)1e3, is_utc);
#   endif
}

//---------------------------------------------------------------------------------Time::local_now

//Time Time::local_now()
//{
//#   if defined SYSTEM_LINUX
//
//        // Linux füllt nicht time_b::dstflag
//
//        timeval  tv;
//        tm       local_tm;
//
//        gettimeofday( &tv, NULL );
//        localtime_r( &tv.tv_sec, &local_tm ;
//        return Time(timegm( &local_tm ) + (double)tv.tv_usec / 1e6);
//
//#   elif defined SYSTEM_HPUX || defined SYSTEM_SOLARIS || defined Z_AIX
//
//        timeval  tv;
//        tm       local_tm;
//
//        gettimeofday( &tv, NULL );
//        localtime_r( &tv.tv_sec, &local_tm );
//
//        return Time((double)tv.tv_sec + (double)tv.tv_usec / (double)1e6 - timezone - ( local_tm.tm_isdst? _dstbias : 0 ));
//
//#   else
//
//        timeb  tm;
//        ftime( &tm );
//        return Time((double)tm.time + (double)tm.millitm / (double)1e3 - timezone - ( tm.dstflag? _dstbias : 0 ));
//
//#   endif
//}

//-------------------------------------------------------------------------------------------------

} //namespace time
} //namespace scheduler
} //namespace sos
