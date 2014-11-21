// $Id: spooler_time.h 14013 2010-09-14 07:21:33Z ss $

#ifndef __SPOOLER_TIME_H
#define __SPOOLER_TIME_H

#include <math.h>

namespace sos {
namespace scheduler {
namespace time {

//-------------------------------------------------------------------------------------------------

extern const time_t             never_int;
extern const double             never_double;

//------------------------------------------------------------------------------------------With_ms

enum With_ms {
    without_ms,
    with_ms
};

//-----------------------------------------------------------------------------------------Duration

struct Duration {
    static Duration             of                          (const string& s);

    explicit                    Duration                    (double o = 0.0)                        : _seconds(o) {}

    Duration&                   operator =                  (const Duration& o)                     { _seconds = o._seconds; return *this; }

    bool                        operator <                  (const Duration& o) const               { return _seconds < o._seconds; }
    bool                        operator <=                 (const Duration& o) const               { return _seconds <= o._seconds; }
    bool                        operator ==                 (const Duration& o) const               { return _seconds == o._seconds; }
    bool                        operator !=                 (const Duration& o) const               { return _seconds != o._seconds; }
    bool                        operator >=                 (const Duration& o) const               { return _seconds >= o._seconds; }
    bool                        operator >                  (const Duration& o) const               { return _seconds > o._seconds; }
    bool                        operator !                  () const                                { return is_zero(); }
    Duration                    operator -                  () const                                { return Duration(-_seconds); }
    Duration&                   operator +=                 (const Duration& o)                     { _seconds += o._seconds; return *this; }
    Duration&                   operator -=                 (const Duration& o)                     { _seconds -= o._seconds; return *this; }

    double                      as_double                   () const                                { return _seconds; }
    int64                       millis                      () const                                { return (int64)(floor(_seconds * 1000 + 0.5) + 0.5); }
    Duration                    rounded_to_next_second      () const                                { return Duration( floor( _seconds + 0.9995 ) ); }
    bool                        not_zero                    () const                                { return _seconds != 0.0; }
    bool                        is_zero                     () const                                { return _seconds == 0.0; }
    bool                        is_eternal                  () const                                { return _seconds == never_double; } 
    friend ostream&             operator <<                 ( ostream& s, const Duration& o )       { o.print(s); return s; }
    void                        print                       ( ostream& s ) const                    { s << as_string(); }
    string                      as_string                   ( With_ms = with_ms ) const;                        
    int64                       seconds                     () const                                { return (int64)(_seconds + 0.0005); }

    static const Duration       eternal;
    static const Duration       epsilon;
    static const Duration       day;

private:
    double                     _seconds;
};

void                            insert_into_message         ( Message_string*, int index, const Duration&) throw();

//---------------------------------------------------------------------------------------------Time

struct Time
{

    enum Is_utc
    {
        is_utc
    };



    static Time                 of_date_time_with_now       (const string&, const string& time_zone_name);  // Datum mit Zeit oder "now+zeit"
    static Time                 of_utc_date_time            (const string& s)               { return of_date_time(s, "UTC"); }
    static Time                 of_local_date_time          (const string&);
    static Time                 of_date_time                (const string& date_time, const string& time_zone_name);
    static Time                 of_time_t                   (time_t t)                      { return Time(t); }
    static Time                 of_millis                   (int64 millis)                  { return Time(millis / 1000.0); }


                                Time                        ()                              : _time(0.0) {}
    explicit                    Time                        ( double t)                     { set(t); }
                                Time                        ( double t, Is_utc )            { set_utc( t ); }
#if !defined Z_AIX_32
    explicit                    Time                        ( time_t t )                    { set((double)t); }
#endif
                                Time                        ( time_t t, Is_utc )            { set_utc((double)t); }
    explicit                    Time                        ( int t )                       { set((double)t); }
    explicit                    Time                        ( const Sos_optional_date_time& dt ) { set_date_time(dt); }

    void                        operator +=                 (const Duration&);
    void                        operator -=                 (const Duration&);

    Time                        operator +                  ( const Duration& ) const;
    Duration                    operator -                  ( const Time& ) const;

    bool                        operator <                  ( const Time& t ) const         { return _time <  t._time; }
    bool                        operator <=                 ( const Time& t ) const         { return _time <= t._time; }
    bool                        operator ==                 ( const Time& t ) const         { return _time == t._time; }
    bool                        operator !=                 ( const Time& t ) const         { return _time != t._time; }
    bool                        operator >=                 ( const Time& t ) const         { return _time >= t._time; }
    bool                        operator >                  ( const Time& t ) const         { return _time >  t._time; }
    bool                        operator !                  () const                        { return is_zero(); }
    int                         compare                     (const Time& t) const;

    void                        set_date_time               (const Sos_optional_date_time&);
    bool                     is_zero                        () const                        { return _time == 0; }
    bool                        not_zero                    () const                        { return _time != 0; }
    bool                     is_never                       () const                        { return _time == never_double; } 
    bool                     is_valid_time                  () const                        { return !is_zero() && !is_never(); }
    bool                        has_date                    () const                        { return _time > 366*24*60*60; }   // Eigentlich nur bis ein Tag
    Time                        rounded_to_next_second      () const                        { return Time( floor( _time + 0.9995 ) ); }


    static double               round                       ( double t );
    static double               normalize                   ( double t );
#if !defined Z_AIX_32
    void                        set                         ( int t )                       { set( (double)t ); }
#endif
    void                        set                         ( time_t t )                    { set( (double)t ); }
    void                        set                         ( double );
    Time&                       set_utc                     ( double );
    Time&                       set_utc                     ( time_t t )                    { set_utc( (double)t );  return *this; } // JS-457
    double                      as_double                   () const;
    double                      as_double_or_never          () const;
    double                      as_utc_double               () const;
    int64                       millis                      () const                        { return (int64)(as_double() * 1000); }
    Time                        time_of_day                 () const                        { return Time((*this - midnight()).as_double()); }
    Time                        midnight                    () const                        { return Time(day_nr() * 24*60*60); }
    int                         day_nr                      () const                        { return uint(as_double()) / (24*60*60); }
    int                         month_nr                    () const;
    time_t                      as_time_t                   () const                        { return (time_t)( as_double    () + 0.0001 ); }
    time_t                      as_utc_time_t               () const                        { return (time_t)( as_utc_double() + 0.0001 ); }
    DATE                        as_local_com_date           () const                        { return com_date_from_seconds_since_1970( round( as_double() ) ); }
    static double               cut_fraction                ( string* datetime_string );
    int64                       ms                          () const                        { return (int64)(_time * 1000 + 0.5); }
    Time                        utc_from_time_zone          (const string&) const;
    Time                        local_time                  (const string& time_zone) const;

    string                      db_string                   ( With_ms = with_ms ) const;    // Für Datenbank
    string                      utc_string                  ( With_ms = with_ms ) const;
    string                      without_timezone_string     ( With_ms = with_ms ) const;
    string                      as_string                   (const string& time_zone_name, With_ms = with_ms ) const;                        
    string                      xml_value                   ( With_ms = with_ms ) const;                        
    friend ostream&             operator <<                 ( ostream& s, const Time& o );

    static Time                 now                         ();
    static Time                 local_now                   ();

#   if defined Z_DEBUG && defined Z_WINDOWS                 // Time in statischer Variablen führt mit gcc 3.3 zum Absturz in string::string
        string                 _time_as_string;
#   endif    

  private:
    double                     _time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970 oder seit Mitternacht

  public:
    static const Time           never;
};      

void                            insert_into_message         ( Message_string*, int index, const Time& ) throw();

//-------------------------------------------------------------------------------------------------

xml::Element_ptr                new_calendar_dom_element    ( const xml::Document_ptr&, const Time& );

//-------------------------------------------------------------------------------------------------

inline string xml_of_time_t(time_t t) {
    return Time( t, Time::is_utc ).xml_value(without_ms);
}

inline string string_of_time_t(time_t t) {
    return Time( t, Time::is_utc ).utc_string(without_ms);
}

} //namespace time

//-------------------------------------------------------------------------------------------------

using time::Time;
using time::Duration;
using time::xml_of_time_t;
using time::string_of_time_t;

} //namespace scheduler
} //namespace sos

#endif
