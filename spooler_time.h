// $Id$

#ifndef __SPOOLER_TIME_H
#define __SPOOLER_TIME_H

#include <math.h>

namespace sos {
namespace scheduler {
namespace time {

//-------------------------------------------------------------------------------------------------

extern const int                never_int;

//---------------------------------------------------------------------------------------------Time

struct Time
{
    enum With_ms
    {
        without_ms,
        with_ms
    };


    enum Is_utc
    {
        is_utc
    };



    static Time                 time_with_now               ( const string& );              // Datum mit Zeit oder "now+zeit"
    static void             set_current_difference_to_utc   ( time_t now );
    static int                  current_difference_to_utc   ()                              { return static_current_difference_to_utc; }


                                Time                        ( double t = 0.0 )              { set(t); }
                                Time                        ( double t, Is_utc )            { set_utc( t ); }
#if !defined Z_AIX
                                Time                        ( time_t t )                    { set((double)t); }
#endif
                                Time                        ( time_t t, Is_utc )            { set_utc((double)t); }
                                Time                        ( int t )                       { set((double)t); }
                                Time                        ( uint t )                      { set((double)t); }
                                Time                        ( const string& t )             { set(t); }
                                Time                        ( const char* t   )             { set(t); }
                                Time                        ( const Sos_optional_date_time& dt ) { *this = dt; }
  //Z_WINDOWS_ONLY(             Time                        ( const FILETIME& t )           { set(t); } )
  //Z_WINDOWS_ONLY(             Time                        ( const SYSTEMTIME& t )         { set(t); } )

    void                        operator =                  ( double t )                    { set(t); }
    void                        operator =                  ( time_t t )                    { set((double)t); }
#if !defined Z_AIX
    void                        operator =                  ( int t )                       { set((double)t); }
#endif
    void                        operator =                  ( const string& t )             { set(t); }
    void                        operator =                  ( const char* t )               { set(t); }
    void                        operator =                  ( const Sos_optional_date_time& );

    void                        operator +=                 ( double );
    void                        operator -=                 ( double );

    Time                        operator +                  ( const Time& );
    Time                        operator +                  ( double );
    Time                        operator +                  ( int t )                       { return *this + (double)t; }
    Time                        operator -                  ( const Time& );
    Time                        operator -                  ( double );
    Time                        operator -                  ( int t )                       { return *this - (double)t; }

    bool                        operator <                  ( const Time& t ) const         { return as_double_or_never() <  t.as_double_or_never(); }
    bool                        operator <=                 ( const Time& t ) const         { return as_double_or_never() <= t.as_double_or_never(); }
    bool                        operator ==                 ( const Time& t ) const         { return as_double_or_never() == t.as_double_or_never(); }
    bool                        operator !=                 ( const Time& t ) const         { return as_double_or_never() != t.as_double_or_never(); }
    bool                        operator >=                 ( const Time& t ) const         { return as_double_or_never() >= t.as_double_or_never(); }
    bool                        operator >                  ( const Time& t ) const         { return as_double_or_never() >  t.as_double_or_never(); }

    bool                        operator <                  ( double t ) const              { return as_double_or_never() <  round(t); }
    bool                        operator <=                 ( double t ) const              { return as_double_or_never() <= round(t); }
    bool                        operator ==                 ( double t ) const              { return as_double_or_never() == round(t); }
    bool                        operator !=                 ( double t ) const              { return as_double_or_never() != round(t); }
    bool                        operator >=                 ( double t ) const              { return as_double_or_never() >= round(t); }
    bool                        operator >                  ( double t ) const              { return as_double_or_never() >  round(t); }

    bool                        operator <                  ( int t ) const                 { return as_double_or_never() <  round(t); }
    bool                        operator <=                 ( int t ) const                 { return as_double_or_never() <= round(t); }
    bool                        operator ==                 ( int t ) const                 { return as_double_or_never() == round(t); }
    bool                        operator !=                 ( int t ) const                 { return as_double_or_never() != round(t); }
    bool                        operator >=                 ( int t ) const                 { return as_double_or_never() >= round(t); }
    bool                        operator >                  ( int t ) const                 { return as_double_or_never() >  round(t); }

                                operator double             () const                        { return as_double_or_never(); }
    bool                        operator !                  () const                        { return is_null(); }

    void                    set_null                        ()                              { set( 0 ); }
    bool                     is_null                        () const                        { return _time == 0; }
    void                    set_never                       ()                              { set( never_int ); } 
    bool                     is_never                       () const                        { return _time == never_int; } 
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
    Time&                       set_utc                     ( time_t t )                    { set( (double)t );  return *this; }
    double                      as_double                   () const;
    double                      as_double_or_never          () const;
    double                      as_utc_double               () const;

#ifdef Z_WINDOWS
    //void                        set                         ( const FILETIME& ); 
    //void                        set                         ( const SYSTEMTIME& );
    //FILETIME                    filetime                    () const;
#endif

    Time&                       set_datetime                ( const string& );
    void                        set_datetime_utc            ( const string& );
    Time                        time_of_day                 () const                        { return as_double() - midnight(); }
    Time                        midnight                    () const                        { return day_nr() * 24*60*60; }
    int                         day_nr                      () const                        { return uint(as_double()) / (24*60*60); }
    int                         month_nr                    () const;
    time_t                      as_time_t                   () const                        { return (time_t)( as_double    () + 0.0001 ); }
    time_t                      as_utc_time_t               () const                        { return (time_t)( as_utc_double() + 0.0001 ); }
    DATE                        as_local_com_date           () const                        { return com_date_from_seconds_since_1970( round( as_double() ) ); }
  //int64                       int64_filetime              () const;
    double                      cut_fraction                ( string* datetime_string );

    string                      as_string                   ( With_ms = with_ms ) const;                        
    string                      xml_value                   ( With_ms = with_ms ) const;                        
    void                        print                       ( ostream& s ) const            { s << as_string(); }
    friend ostream&             operator <<                 ( ostream& s, const Time& o )   { o.print(s); return s; }

    static Time                 now                         ();



#   if defined Z_DEBUG && defined Z_WINDOWS                 // Time in statischer Variablen führt mit gcc 3.3 zum Absturz in string::string
        string                 _time_as_string;
#   endif    

  private:
    double                     _time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970 oder seit Mitternacht
    bool                       _is_utc;

  public:
    static int                  static_current_difference_to_utc;
    static const Time           never;
};      

void                            insert_into_message         ( Message_string*, int index, const Time& ) throw();


extern const Time               latter_day;

//-----------------------------------------------Daylight_saving_time_transition_detector_interface

struct Daylight_saving_time_transition_detector_interface : Async_operation
{
};


ptr<Daylight_saving_time_transition_detector_interface> new_daylight_saving_time_transition_detector( Scheduler* );

//-------------------------------------------------------------------------------------------------

Time                            time_from_string            ( const string& );
void                            test_summertime             ( const string& date_time );
xml::Element_ptr                new_calendar_dom_element    ( const xml::Document_ptr&, const Time& );

//-------------------------------------------------------------------------------------------------

} //namespace time

//-------------------------------------------------------------------------------------------Gmtime
/* Hübsche Klasse, berücksichtigt aber bei der Umrechnug die Sommerzeit nicht.
// Greenwich Time (UTC)

struct Gmtime
{
    enum With_ms
    {
        without_ms,
        with_ms
    };

    static Gmtime               now                         ()                                      { return Gmtime( double_from_gmtime() ); }
    static double               round                       ( double t );
    static double               normalize                   ( double t );


                                Gmtime                      ()                                      : _time(0) {}
    explicit                    Gmtime                      ( const time::Time& t )                 { set_local_time( t ); }
    explicit                    Gmtime                      ( double t )                            { set( t ); }

                              //operator double             () const                                { return _time; }

  //Gmtime&                     operator =                  ( const Time& t )                       { set_local_time( t );  return *this; }
  //Gmtime&                     operator =                  ( time_t t )                            { _time = (double)t; }

    void                        operator +=                 ( double t )                            { set( _time + t ); }
    void                        operator -=                 ( double t )                            { set( _time - t ); }

    Gmtime                      operator +                  ( double t ) const                      { return Gmtime( _time + t ); }
    Gmtime                      operator +                  ( int    t ) const                      { return Gmtime( _time + (double)t ); }
    Gmtime                      operator -                  ( double t ) const                      { return Gmtime( _time - t ); }
    Gmtime                      operator -                  ( int    t ) const                      { return Gmtime( _time - (double)t ); }

    Gmtime                      operator -                  ( const Gmtime& t ) const               { return Gmtime( _time - t._time ); }

    bool                        operator <                  ( const Gmtime& t ) const               { return _time <  t._time; }
    bool                        operator <=                 ( const Gmtime& t ) const               { return _time <= t._time; }
    bool                        operator ==                 ( const Gmtime& t ) const               { return _time == t._time; }
    bool                        operator !=                 ( const Gmtime& t ) const               { return _time != t._time; }
    bool                        operator >=                 ( const Gmtime& t ) const               { return _time >= t._time; }
    bool                        operator >                  ( const Gmtime& t ) const               { return _time >  t._time; }

    bool                        operator <                  ( int t ) const                         { return _time <  t; }
    bool                        operator <=                 ( int t ) const                         { return _time <= t; }
    bool                        operator ==                 ( int t ) const                         { return _time == t; }
    bool                        operator !=                 ( int t ) const                         { return _time != t; }
    bool                        operator >=                 ( int t ) const                         { return _time >= t; }
    bool                        operator >                  ( int t ) const                         { return _time >  t; }

    void                    set_local_time                  ( const time::Time& t );
    time::Time                  local_time                  () const;
    
    bool                        is_null                     () const                                { return _time == 0; }
    double                      as_double                   () const                                { return _time; }
    string                      as_string                   ( With_ms = with_ms ) const;
    string                      as_local_string             ( With_ms = with_ms ) const;

    void                        set_null                    ()                                      { set( 0.0 ); }
    void                        set_time_t                  ( time_t t )                            { set( (double)t ); }
    void                        set                         ( double t );

    Gmtime                      time_of_day                 () const                                { return Gmtime( _time - midnight().as_double() ); }
    Gmtime                      midnight                    () const                                { return Gmtime( day_nr() * 24*60*60 ); }
    int                         day_nr                      () const                                { return (int)( int64(_time) / (24*60*60) ); }
    time_t                      as_time_t                   () const                                { return (time_t)( _time + 0.0001 ); }

  private:
#   if defined Z_DEBUG && defined Z_WINDOWS                 // Time in statischer Variablen führt mit gcc 3.3 zum Absturz in string::string
        string                 _time_as_string;
#   endif    

    double                     _time;
};

extern const Gmtime             doomsday;
*/
//-------------------------------------------------------------------------------------------------

//inline void insert_into_message( Message_string* m, int index, const time::Time& t ) throw()        { m->insert_string( index, t.as_string() ); }

//-------------------------------------------------------------------------------------------------

using time::Time;

} //namespace scheduler
} //namespace sos

#endif
