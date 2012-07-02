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
    bool                        operator !                  () const                                { return is_null(); }


    double                      as_double                   () const                                { return _seconds; }
    Duration                    rounded_to_next_second      () const                                { return Duration( floor( _seconds + 0.9995 ) ); }
    bool                        is_defined                  () const                                { return _seconds != 0.0; }
    bool                        is_null                     () const                                { return _seconds == 0.0; }
    bool                        is_eternal                  () const                                { return _seconds == never_double; } 
    friend ostream&             operator <<                 ( ostream& s, const Duration& o )       { o.print(s); return s; }
    void                        print                       ( ostream& s ) const                    { s << as_string(); }
    string                      as_string                   ( With_ms = with_ms ) const;                        
    long                        seconds                     () const                                { return (long)(_seconds + 0.0005); }

    static const Duration       epsilon;
    static const Duration       eternal;

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



    static Time                 time_with_now               ( const string& );              // Datum mit Zeit oder "now+zeit"


    explicit                    Time                        ( double t = 0.0 )              { set(t); }
                                Time                        ( double t, Is_utc )            { set_utc( t ); }
#if !defined Z_AIX
    explicit                    Time                        ( time_t t )                    { set((double)t); }
#endif
                                Time                        ( time_t t, Is_utc )            { set_utc((double)t); }
                              //Time                        ( int t )                       { set((double)t); }
                              //Time                        ( uint t )                      { set((double)t); }
    explicit                    Time                        ( const string& t )             { set(t); }
    explicit                    Time                        ( const char* t   )             { set(t); }
    explicit                    Time                        ( const Sos_optional_date_time& dt ) { *this = dt; }

  //void                        operator =                  ( double t )                    { set(t); }
  //void                        operator =                  ( time_t t )                    { set((double)t); }
#if !defined Z_AIX
  //void                        operator =                  ( int t )                       { set((double)t); }
#endif
  //void                        operator =                  ( const string& t )             { set(t); }
  //void                        operator =                  ( const char* t )               { set(t); }
  //void                        operator =                  ( const Sos_optional_date_time& );

    void                        operator +=                 (const Duration&);
    void                        operator -=                 (const Duration&);

    Time                        operator +                  ( const Duration& ) const;
  //Time                        operator +                  ( double ) const;
  //Time                        operator +                  ( int t ) const                 { return *this + (double)t; }
    Duration                    operator -                  ( const Time& ) const;
  //Time                        operator -                  ( double ) const;
  //Time                        operator -                  ( int t ) const                 { return *this - (double)t; }

    bool                        operator <                  ( const Time& t ) const         { return compare(t) < 0; }
    bool                        operator <=                 ( const Time& t ) const         { return compare(t) <= 0; }
    bool                        operator ==                 ( const Time& t ) const         { return compare(t) == 0; }
    bool                        operator !=                 ( const Time& t ) const         { return compare(t) != 0; }
    bool                        operator >=                 ( const Time& t ) const         { return compare(t) >= 0; }
    bool                        operator >                  ( const Time& t ) const         { return compare(t) > 0; }

private:
    bool                        operator <                  ( double t ) const;
    bool                        operator <=                 ( double t ) const;
    bool                        operator ==                 ( double t ) const;
    bool                        operator !=                 ( double t ) const;
    bool                        operator >=                 ( double t ) const;
    bool                        operator >                  ( double t ) const;

    bool                        operator <                  ( int t ) const;
    bool                        operator <=                 ( int t ) const;
    bool                        operator ==                 ( int t ) const;
    bool                        operator !=                 ( int t ) const;
    bool                        operator >=                 ( int t ) const;
    bool                        operator >                  ( int t ) const;
public:

                              //operator double             () const                        { return as_double_or_never(); }
                                operator bool               () const                        { return is_defined(); }
    bool                        operator !                  () const                        { return is_null(); }
    int                         compare                        (const Time& t) const;

    void                    set_null                        ()                              { set( 0 ); }
    bool                     is_null                        () const                        { return _time == 0; }
    bool                        is_defined                  () const                        { return _time != 0; }
    void                    set_never                       ()                              { set( never_int ); } 
    bool                     is_never                       () const                        { return _time == never_double; } 
    Time                        rounded_to_next_second      () const                        { return Time( floor( _time + 0.9995 ) ); }


    static double               round                       ( double t );
    static double               normalize                   ( double t );
#if !defined Z_AIX
    void                        set                         ( int t )                       { set( (double)t ); }
#endif
    void                        set                         ( time_t t )                    { set( (double)t ); }
    void                        set                         ( double );
    void                        set                         ( const string& );
    Time&                       set_utc                     ( double );
    Time&                       set_utc                     ( time_t t )                    { set_utc( (double)t );  return *this; } // JS-457
    double                      as_double                   () const;
    double                      as_double_or_never          () const;
    double                      as_utc_double               () const;
    Time&                       set_datetime                ( const string& );
    void                        set_datetime_utc            ( const string& );
    Time                        time_of_day                 () const                        { return Time((*this - midnight()).as_double()); }
    Time                        midnight                    () const                        { return day_nr() * 24*60*60; }
    int                         day_nr                      () const                        { return uint(as_double()) / (24*60*60); }
    int                         month_nr                    () const;
    time_t                      as_time_t                   () const                        { return (time_t)( as_double    () + 0.0001 ); }
    time_t                      as_utc_time_t               () const                        { return (time_t)( as_utc_double() + 0.0001 ); }
    DATE                        as_local_com_date           () const                        { return com_date_from_seconds_since_1970( round( as_double() ) ); }
    double                      cut_fraction                ( string* datetime_string );
    long                        ms                          () const                        { return (long)(_time * 1000 + 0.5); }
    Time                        utc_from_time_zone          (const string&);

    string                      as_string                   ( With_ms = with_ms ) const;                        
    string                      xml_value                   ( With_ms = with_ms ) const;                        
    void                        print                       ( ostream& s ) const            { s << as_string(); }
    friend ostream&             operator <<                 ( ostream& s, const Time& o )   { o.print(s); return s; }

    static Time                 now                         ();
    static Time                 local_now                   ();

#   if defined Z_DEBUG && defined Z_WINDOWS                 // Time in statischer Variablen führt mit gcc 3.3 zum Absturz in string::string
        string                 _time_as_string;
#   endif    

  private:
    double                     _time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970 oder seit Mitternacht
    bool                       _is_utc;

  public:
    static const Time           never;
};      

void                            insert_into_message         ( Message_string*, int index, const Time& ) throw();

//-------------------------------------------------------------------------------------------------

Duration                        duration_from_string        ( const string& );
Time                            time_from_string            ( const string& );
xml::Element_ptr                new_calendar_dom_element    ( const xml::Document_ptr&, const Time& );

//-------------------------------------------------------------------------------------------------

inline string xml_of_time_t(time_t t) {
    return Time( t, Time::is_utc ).xml_value();
}

inline string string_of_time_t(time_t t) {
    return Time( t, Time::is_utc ).as_string();
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
