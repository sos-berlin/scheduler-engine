// $Id$

#ifndef __SPOOLER_TIME_H
#define __SPOOLER_TIME_H

#include <math.h>

namespace sos {
namespace scheduler {
namespace time {

//-------------------------------------------------------------------------------------------------

extern const int                never_int;

//--------------------------------------------------------------------------------With_single_start

enum With_single_start
{
    wss_next_period                 = 0x01,                 // Nächste <period> mit end vor einem Zeitpunkt, begin ist egal
    wss_next_single_start           = 0x02,
    wss_next_period_or_single_start = 0x03,
    wss_next_begin                  = 0x04,                 // Nächste <period> mit begin ab einem Zeitpunkt
    wss_next_begin_or_single_start  = 0x06,
    wss_next_any_start              = 0x08                  // Nächster Start: single_start, start_once, repeat (nur erster Start in der Periode)
};

//---------------------------------------------------------------------------------------------Time

struct Time
{
    enum With_ms
    {
        without_ms,
        with_ms
    };


    static Time                 time_with_now               ( const string& );              // Datum mit Zeit oder "now+zeit"
    static void             set_current_difference_to_utc   ();
    static int                  current_difference_to_utc   ()                              { set_current_difference_to_utc(); return static_current_difference_to_utc; }


                                Time                        ( double t = 0.0 )              { set(t); }
                                Time                        ( time_t t )                    { set((double)t); }
                                Time                        ( int t )                       { set((double)t); }
                                Time                        ( uint t )                      { set((double)t); }
                                Time                        ( const string& t )             { set(t); }
                                Time                        ( const char* t   )             { set(t); }
                                Time                        ( const Sos_optional_date_time& dt ) { *this = dt; }
    Z_WINDOWS_ONLY(             Time                        ( const FILETIME& t )           { set(t); } )
    Z_WINDOWS_ONLY(             Time                        ( const SYSTEMTIME& t )         { set(t); } )

    void                        operator =                  ( double t )                    { set(t); }
    void                        operator =                  ( time_t t )                    { set((double)t); }
    void                        operator =                  ( int t )                       { set((double)t); }
    void                        operator =                  ( const string& t )             { set(t); }
    void                        operator =                  ( const char* t )               { set(t); }
    void                        operator =                  ( const Sos_optional_date_time& );

    void                        operator +=                 ( double t )                    { set( as_double() + t ); }
    void                        operator -=                 ( double t )                    { set( as_double() - t ); }

    Time                        operator +                  ( const Time& t )               { return Time( as_double() + t.as_double() ); }
    Time                        operator +                  ( double t )                    { return Time( as_double() + t             ); }
    Time                        operator +                  ( int t )                       { return Time( as_double() + t             ); }
    Time                        operator -                  ( const Time& t )               { return Time( as_double() - t.as_double() ); }
    Time                        operator -                  ( double t )                    { return Time( as_double() - t             ); }
    Time                        operator -                  ( int t )                       { return Time( as_double() - t             ); }

    bool                        operator <                  ( const Time& t ) const         { return as_double() <  t.as_double(); }
    bool                        operator <=                 ( const Time& t ) const         { return as_double() <= t.as_double(); }
    bool                        operator ==                 ( const Time& t ) const         { return as_double() == t.as_double(); }
    bool                        operator !=                 ( const Time& t ) const         { return as_double() != t.as_double(); }
    bool                        operator >=                 ( const Time& t ) const         { return as_double() >= t.as_double(); }
    bool                        operator >                  ( const Time& t ) const         { return as_double() >  t.as_double(); }

    bool                        operator <                  ( double t ) const              { return as_double() <  round(t); }
    bool                        operator <=                 ( double t ) const              { return as_double() <= round(t); }
    bool                        operator ==                 ( double t ) const              { return as_double() == round(t); }
    bool                        operator !=                 ( double t ) const              { return as_double() != round(t); }
    bool                        operator >=                 ( double t ) const              { return as_double() >= round(t); }
    bool                        operator >                  ( double t ) const              { return as_double() >  round(t); }

    bool                        operator <                  ( int t ) const                 { return as_double() <  round(t); }
    bool                        operator <=                 ( int t ) const                 { return as_double() <= round(t); }
    bool                        operator ==                 ( int t ) const                 { return as_double() == round(t); }
    bool                        operator !=                 ( int t ) const                 { return as_double() != round(t); }
    bool                        operator >=                 ( int t ) const                 { return as_double() >= round(t); }
    bool                        operator >                  ( int t ) const                 { return as_double() >  round(t); }

                                operator double             () const                        { return as_double(); }
    bool                        operator !                  () const                        { return is_null(); }

    void                    set_null                        ()                              { set( 0 ); }
    bool                     is_null                        () const                        { return _time == 0; }
    void                    set_never                       ()                              { set( never_int ); } 
    bool                     is_never                       () const                        { return _time == never_int; } 
    Time                        rounded_to_next_second      () const                        { return Time( floor( _time + 0.9995 ) ); }


    static double               round                       ( double t );
    static double               normalize                   ( double t );
    void                        set                         ( int t )                       { set( (double)t ); }
    void                        set                         ( time_t t )                    { set( (double)t ); }
    void                        set                         ( double );
    void                        set                         ( const string& );
    void                        set_utc                     ( double );
    double                      as_double                   () const;

#ifdef Z_WINDOWS
    void                        set                         ( const FILETIME& ); 
    void                        set                         ( const SYSTEMTIME& );
    FILETIME                    filetime                    () const;
#endif

    void                        set_datetime                ( const string& );
    void                        set_datetime_utc            ( const string& );
    Time                        time_of_day                 () const                        { return as_double() - midnight(); }
    Time                        midnight                    () const                        { return day_nr() * 24*60*60; }
    int                         day_nr                      () const                        { return uint(as_double()) / (24*60*60); }
    time_t                      as_time_t                   () const                        { return (time_t)( as_double() + 0.0001 ); }
    DATE                        as_local_com_date           () const                        { return com_date_from_seconds_since_1970( round( as_double() ) ); }
    int64                       int64_filetime              () const;
    double                      cut_fraction                ( string* datetime_string );

    string                      as_string                   ( With_ms = with_ms ) const;                        
    string                      jdbc_value                  () const;
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

//-------------------------------------------------------------------------------------------Period

struct Period
{
                                Period                      ()                                      : _zero_(this+1) { init(); }
    explicit                    Period                      ( const xml::Element_ptr& e, const Period* deflt=NULL )  : _zero_(this+1) { init(); set_dom( e, deflt ); }
    
    void                        init                        ()                                      { _begin = _end = _repeat = Time::never; }

    bool                        empty                       () const                                { return _begin == Time::never; }
    bool                        has_start                   () const                                { return is_single_start() || repeat() != Time::never; }
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
    bool                        has_repeat_or_once          () const                                { return _repeat < Time::never || _start_once; }

  //void                        set_next_start_time         ( const Time& );

    void                        check                       () const;
    void                        print                       ( ostream& ) const;
    string                      obj_name                    () const;
    friend ostream&             operator <<                 ( ostream& s, const Period& o )         { o.print(s); return s; }


//private:
    Fill_zero                  _zero_;
    Time                       _begin;                      // Sekunden seit Mitternacht, manchmal auch mit Datum
    Time                       _end;                        // Sekunden seit Mitternacht, manchmal auch mit Datum
    Time                       _repeat;
    bool                       _single_start;
    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _next_end_time beenden
    bool                       _start_once;                 // Für Joacim Zschimmer
};

//extern Period                   empty_period;               // gcc 3.2.1: Nicht const, weil gcc diese Variable nicht initialisiert. Das macht spooler.cxx.

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
    Period                      next_period                 ( Time time_of_day, With_single_start single_start ) const { return _period_set.empty()? Period(): next_period_(time_of_day,single_start); }
    Period                      next_period_                ( Time time_of_day, With_single_start single_start ) const;
    void                        add                         ( const Period& p )                     { _period_set.insert( p ); }       

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day& o )            { o.print(s); return s; }


    Period_set                 _period_set;
};

//------------------------------------------------------------------------------------------Day_set

struct Day_set
{
                                Day_set                     ( int minimum, int maximum )                 : _minimum(minimum), _maximum(maximum) {} 
    explicit                    Day_set                     ( int minimum, int maximum, const xml::Element_ptr& e )  : _minimum(minimum), _maximum(maximum) { set_dom(e); }

    void                        set_dom                     ( const xml::Element_ptr&, const Day* = NULL, const Period* = NULL );

    bool                        is_empty                    ();
    char                        operator []                 ( int i )                               { return _days[i]; }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day_set& o )        { o.print(s); return s; }


    int                        _minimum;
    int                        _maximum;
    Day                        _days                        [1+31];
};

//--------------------------------------------------------------------------------------Weekday_set

struct Weekday_set : Day_set
{
                                Weekday_set                 ()                                      : Day_set( 0, 6 ) {}
    explicit                    Weekday_set                 ( const xml::Element_ptr& e )           : Day_set( 0, 6, e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Weekday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Weekday_set& o )    { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------Monthday_set

struct Monthday_set : Day_set
{
                                Monthday_set                ()                                      : Day_set( 1, 31 ) {}
    explicit                    Monthday_set                ( const xml::Element_ptr& e )           : Day_set( 1, 31, e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Monthday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Monthday_set& o )   { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------Ultimo_set

struct Ultimo_set : Day_set
{
                                Ultimo_set                  ()                                      : Day_set( 0, 30 ) {}
    explicit                    Ultimo_set                  ( const xml::Element_ptr& e )           : Day_set( 0, 30, e ) {}

    Period                      next_period                 ( Time, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Ultimo_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Ultimo_set& o )     { o.print(s); return s; }
};

//-----------------------------------------------------------------------------------------Holidays

struct Holidays
{
    void                        clear                       ()                                      { _set.clear(); }
    void                        set_dom                     ( const xml::Element_ptr&, const File_path& include_path, int include_nesting = 0 );
    void                        include                     ( time_t t )                            { _set.insert( t ); }
    bool                        is_included                 ( time_t t )                            { return _set.find( t ) != _set.end(); }

    typedef stdext::hash_set<time_t> Set;
    Set                        _set;
};

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

//------------------------------------------------------------------------------------------At_set

struct At_set
{
    Period                      next_period                 ( Time, With_single_start single_start );

    void                        add                         ( const Time& at )                      { _at_set.insert( at ); }
    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const At_set& o )         { o.print(s); return s; }

    set<Time>                  _at_set;
};

//----------------------------------------------------------------------------------------Run_time

struct Run_time : idispatch_implementation< Run_time, spooler_com::Irun_time >, 
                  spooler_com::Ihas_java_class_name
{
  //enum Application { application_job, application_order };


    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Run_time"; }

    STDMETHODIMP            put_Xml                         ( BSTR xml );


    bool                        operator ==                 ( const Run_time& );

                                Run_time                    ( Spooler*, Order* = NULL );

    void                    set_modified_event_handler      ( Modified_event_handler* m )           { _modified_event_handler = m; }

    void                    set_xml                         ( const string& );
  //string                      xml                         ()                                      { return _xml; }

    void                    set_dom                         ( const xml::Element_ptr& );            // Setzt nicht _xml!
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& ) const;
    xml::Document_ptr           dom_document                () const;

    void                        check                       ();                              

    bool                        set                         () const                                { return _set; }

    void                        set_holidays                ( const Holidays& h )                   { _holidays = h; }
    void                        set_default                 ();
    void                        set_default_days            ();

    bool                        once                        ()                                      { return _once; }
    void                    set_once                        ( bool b = true )                       { _once = b; }

    Period                      first_period                ()                                      { return first_period( Time::now() ); }
    Period                      first_period                ( Time );

    Period                      next_period                 ( With_single_start single_start = wss_next_period )      { return next_period( Time::now(), single_start ); }
    Period                      next_period                 ( Time, With_single_start single_start = wss_next_period );

    bool                        period_follows              ( Time time )                           { return next_period(time).begin() != Time::never; }

    Time                        next_single_start           ( Time time )                           { return next_period(time,wss_next_single_start).begin(); }
    Time                        next_any_start              ( Time time )                           { return next_period(time,wss_next_any_start).begin(); }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Run_time& o )       { o.print(s); return s; }



  private:
    Time                        next_time                   ( Time );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Order*                     _order;
  //Application                _application;
    Modified_event_handler*    _modified_event_handler;
    bool                       _set;
    bool                       _once;
    At_set                     _at_set;
    Date_set                   _date_set;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
    Holidays                   _holidays;
  //string                     _xml;
    xml::Document_ptr          _dom;
};

//-------------------------------------------------------------------------------------------------

Time                            time_from_string            ( const string& );
void                            test_summertime             ( const string& date_time );

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

inline void insert_into_message( Message_string* m, int index, const time::Time& t ) throw()        { m->insert_string( index, t.as_string() ); }

//-------------------------------------------------------------------------------------------------

using time::Time;
using time::Period;
using time::Run_time;

} //namespace scheduler
} //namespace sos

#endif
