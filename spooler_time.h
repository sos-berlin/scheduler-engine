// $Id: spooler_time.h,v 1.17 2002/11/28 11:18:23 jz Exp $

#ifndef __SPOOLER_TIME_H
#define __SPOOLER_TIME_H

#include <math.h>

namespace sos {
namespace spooler {
namespace time {

//--------------------------------------------------------------------------------With_single_start

enum With_single_start
{
    wss_next_period                 = 0x01,
    wss_next_single_start           = 0x02,
    wss_next_period_or_single_start = 0x03
};

//---------------------------------------------------------------------------------------------Time

struct Time
{
    enum With_ms
    {
        without_ms,
        with_ms
    };

                                Time                        ( double t = 0.0 )              { set(t); }
                                Time                        ( time_t t )                    { set(t); }
                                Time                        ( int t )                       { set(t); }
                                Time                        ( uint t )                      { set(t); }
                                Time                        ( const string& t )             { set(t); }
                                Time                        ( const char* t   )             { set(t); }
                                Time                        ( const Sos_optional_date_time& dt ) { *this = dt; }

    void                        operator =                  ( double t )                    { set(t); }
    void                        operator =                  ( int t )                       { set(t); }
    void                        operator =                  ( const string& t )             { set(t); }
    void                        operator =                  ( const char* t )               { set(t); }
    void                        operator =                  ( const Sos_optional_date_time& );

    void                        operator +=                 ( double t )                    { set( _time + t ); }
    void                        operator -=                 ( double t )                    { set( _time - t ); }

    Time                        operator -                  ( const Time& t )               { return Time( _time - t ); }
  //Time                        operator +                  ( const Time& t )               { return Time( _time + t ); }

    bool                        operator <                  ( const Time& t ) const         { return _time <  t._time; }
    bool                        operator <=                 ( const Time& t ) const         { return _time <= t._time; }
    bool                        operator ==                 ( const Time& t ) const         { return _time == t._time; }
    bool                        operator !=                 ( const Time& t ) const         { return _time != t._time; }
    bool                        operator >=                 ( const Time& t ) const         { return _time >= t._time; }
    bool                        operator >                  ( const Time& t ) const         { return _time >  t._time; }

    bool                        operator <                  ( double t ) const              { return _time <  round(t); }
    bool                        operator <=                 ( double t ) const              { return _time <= round(t); }
    bool                        operator ==                 ( double t ) const              { return _time == round(t); }
    bool                        operator !=                 ( double t ) const              { return _time != round(t); }
    bool                        operator >=                 ( double t ) const              { return _time >= round(t); }
    bool                        operator >                  ( double t ) const              { return _time >  round(t); }

    bool                        operator <                  ( int t ) const                 { return _time <  round(t); }
    bool                        operator <=                 ( int t ) const                 { return _time <= round(t); }
    bool                        operator ==                 ( int t ) const                 { return _time == round(t); }
    bool                        operator !=                 ( int t ) const                 { return _time != round(t); }
    bool                        operator >=                 ( int t ) const                 { return _time >= round(t); }
    bool                        operator >                  ( int t ) const                 { return _time >  round(t); }

                                operator double             () const                        { return _time; }

    static double               round                       ( double t )                    { return floor( t * 1000.0 + 0.5 ) / 1000.0; }
    void                        set                         ( double t )                    { _time = round(t); }
    void                        set                         ( const string& );
    Time                        time_of_day                 () const                        { return _time - midnight(); }
    Time                        midnight                    () const                        { return day_nr() * 24*60*60; }
    int                         day_nr                      () const                        { return uint(_time) / (24*60*60); }
    time_t                      as_time_t                   () const                        { return (time_t)( _time + 0.0001 ); }

    string                      as_string                   ( With_ms = with_ms ) const;                        
    void                        print                       ( ostream& s ) const            { s << as_string(); }
    friend ostream&             operator <<                 ( ostream& s, const Time& o )   { o.print(s); return s; }

    static Time                 now                         ();

    double                     _time;                       // wie time_t: Anzahl Sekunden seit 1.1.1970 oder seit Mitternacht
};      

const Time                      latter_day                  = INT_MAX;

//-------------------------------------------------------------------------------------------Period

struct Period
{
                                Period                      ()                                      : _zero_(this+1) { init(); }
    explicit                    Period                      ( const xml::Element_ptr& e, const Period* deflt=NULL )  : _zero_(this+1) { init(); set_dom( e, deflt ); }
    
    void                        init                        ()                                      { _begin=_end=_repeat=latter_day; }

    bool                        empty                       () const                                { return _begin == latter_day; }
    bool                        has_start                   () const                                { return is_single_start() || repeat() != latter_day; }
    Time                        next_try                    ( Time );
    Period                      operator +                  ( const Time& t ) const                 { Period p = *this; p._begin += t; p._end += t; return p; }
    friend Period               operator +                  ( const Time& t, const Period& p )      { return p+t; }

    void                        set_default                 ();
    void                        set_dom                     ( const xml::Element_ptr&, const Period* deflt );

    bool                        operator <                  ( const Period& t ) const               { return _begin < t._begin; }  //für set<>
    bool                        is_in_time                  ( Time t )                              { return t >= _begin && t < _end; }
    bool                        is_comming                  ( Time time_of_day, With_single_start single_start ) const;

    Time                        begin                       () const                                { return _begin; }
    Time                        end                         () const                                { return _end; }
    Time                        repeat                      () const                                { return _repeat; }
    bool                        is_single_start             () const                                { return _single_start; }
    bool                        let_run                     () const                                { return _let_run; }

  //void                        set_next_start_time         ( const Time& );

    void                        check                       () const;
    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Period& o )         { o.print(s); return s; }


//private:
    Fill_zero                  _zero_;
    Time                       _repeat;
    Time                       _begin;                      // Sekunden seit Mitternacht
    Time                       _end;                        // Sekunden seit Mitternacht
    bool                       _single_start;
    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _next_end_time beenden
};

extern Period                   empty_period;               // gcc 3.2.1: Nicht const, weil gcc diese Variable nicht initialisiert. Das macht spooler.cxx.

typedef set<Period>             Period_set;

//----------------------------------------------------------------------------------------------Day

struct Day
{
                                Day                         ()                                      {}
                                Day                         ( const Period_set& t )                 { _period_set = t; }
                                Day                         ( const Period& t )                     { _period_set.insert( t ); }
                                Day                         ( const xml::Element_ptr& e, const Day* deflt, const Period* p )   { set_dom( e, deflt, p ); }

    void                        set_dom                     ( const xml::Element_ptr&, const Day* deflt, const Period* );
    void                        set_default                 ();

                                operator bool               () const                                { return !_period_set.empty(); }

    bool                        has_time                    ( Time time_of_day );
    const Period&               next_period                 ( Time time_of_day, With_single_start single_start ) const { return _period_set.empty()? empty_period: next_period_(time_of_day,single_start); }
    const Period&               next_period_                ( Time time_of_day, With_single_start single_start ) const;
    void                        add                         ( const Period& p )                     { _period_set.insert( p ); }       

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day& o )            { o.print(s); return s; }


    Period_set                 _period_set;
};

//------------------------------------------------------------------------------------------Day_set

struct Day_set
{
                                Day_set                     ()                                      {} 
    explicit                    Day_set                     ( const xml::Element_ptr& e )           { set_dom(e); }

    void                        set_dom                     ( const xml::Element_ptr&, const Day* = NULL, const Period* = NULL );

    bool                        is_empty                    ();
    char                        operator []                 ( int i )                               { return _days[i]; }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day_set& o )        { o.print(s); return s; }


    Day                        _days                        [31];
};

//--------------------------------------------------------------------------------------Weekday_set

struct Weekday_set : Day_set
{
                                Weekday_set                 ()                                      {}
    explicit                    Weekday_set                 ( const xml::Element_ptr& e )           : Day_set( e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Weekday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Weekday_set& o )    { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------Monthday_set

struct Monthday_set : Day_set
{
                                Monthday_set                ()                                      {}
    explicit                    Monthday_set                ( const xml::Element_ptr& e )           : Day_set( e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Monthday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Monthday_set& o )   { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------Ultimo_set

struct Ultimo_set : Day_set
{
                                Ultimo_set                  ()                                      {}
    explicit                    Ultimo_set                  ( const xml::Element_ptr& e )           : Day_set( e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Ultimo_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Ultimo_set& o )     { o.print(s); return s; }
};

//-------------------------------------------------------------------------------------Holiday_set

typedef set<uint>               Holiday_set;

/*
inline Holiday_set& operator += ( Holiday_set& a, const Holiday_set& b )
{
    for( Holiday_set::iterator it = b.begin(); it != b.end(); it++ )
    {
        a.insert( *it );
    }

    return a;
}
*/
//--------------------------------------------------------------------------------------------Date

struct Date
{
    bool                        operator <                  ( const Date& date ) const              { return _day_nr < date._day_nr; }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Date& o )           { o.print(s); return s; }

    uint                       _day_nr;
    Day                        _day;
};

//----------------------------------------------------------------------------------------Date_set

struct Date_set
{
    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Date_set& o )       { o.print(s); return s; }

    set<Date>                  _date_set;
};

//----------------------------------------------------------------------------------------Run_time

struct Run_time
{
                                Run_time                    ()                                      : _zero_(this+1) {}

    void                        set_dom                     ( const xml::Element_ptr& ) ;

    void                        check                       ();                              

    bool                        set                         () const                                { return _set; }

    void                        set_holidays                ( const Holiday_set& h )                { _holiday_set = h; }
    void                        set_default                 ();
    void                        set_default_days            ();

    bool                        once                        ()                                      { return _once; }
    void                    set_once                        ( bool b = true )                       { _once = b; }

    Period                      first_period                ()                                      { return first_period( Time::now() ); }
    Period                      first_period                ( Time );

    Period                      next_period                 ( With_single_start single_start = wss_next_period )      { return next_period( Time::now(), single_start ); }
    Period                      next_period                 ( Time, With_single_start single_start = wss_next_period );

    bool                        period_follows              ( Time time )                           { return next_period(time).begin() != latter_day; }

    Time                        next_single_start           ( Time time )                           { return next_period(time,wss_next_single_start).begin(); }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Run_time& o )       { o.print(s); return s; }



  private:
    Time                        next_time                   ( Time );


    Fill_zero                  _zero_;

    bool                       _set;
    bool                       _once;
    Date_set                   _date_set;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
    Holiday_set                _holiday_set;
};

//-------------------------------------------------------------------------------------------------

} //namespace time

using time::Time;
using time::Period;
using time::Run_time;
using time::latter_day;

} //namespace spooler
} //namespace sos

#endif
