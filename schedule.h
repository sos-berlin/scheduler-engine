// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SCHEDULER_SCHEDULE_H
#define __SCHEDULER_SCHEDULE_H

namespace sos {
namespace scheduler {
namespace schedule {


struct Schedule;
struct Schedule_folder;
struct Schedule_subsystem;
struct Schedule_subsystem_interface;

//--------------------------------------------------------------------------------------------const

const int                       max_weekdays_per_month      = 5;

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

//-------------------------------------------------------------------------------------------Period

struct Period
{
                                Period                      ()                                      : _zero_(this+1) { init(); }
    explicit                    Period                      ( const xml::Element_ptr& e, const Period* deflt=NULL )  : _zero_(this+1) { init(); set_dom( e, without_date, deflt ); }
    
    void                        init                        ()                                      { _begin = _end = _repeat = _absolute_repeat = Time::never; }

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& ) const;

    bool                        empty                       () const                                { return _begin == Time::never; }
    bool                        has_start                   () const                                { return is_single_start() || repeat() != Time::never; }
    Time                        next_try                    ( const Time& );
    Period                      operator +                  ( const Time& t ) const                 { Period p = *this; p._begin += t; p._end += t; return p; }
    friend Period               operator +                  ( const Time& t, const Period& p )      { return p+t; }

    void                        set_single_start            ( const Time& );

    enum With_or_without_date { without_date, with_date };
    void                        set_dom                     ( const xml::Element_ptr&, With_or_without_date = without_date, const Period* deflt = NULL );

    bool                        operator <                  ( const Period& t ) const               { return _begin < t._begin; }  //für set<>
    bool                        operator >                  ( const Period& t ) const               { return _begin > t._begin; }  //für set<>
    bool                        is_in_time                  ( const Time& t )                       { return t >= _begin && t < _end; }
    bool                        is_comming                  ( const Time& time_of_day, With_single_start single_start ) const;

    Time                        begin                       () const                                { return _begin; }
    Time                        end                         () const                                { return _end; }
    Time                        repeat                      () const                                { return _repeat; }
    Time                        absolute_repeat             () const                                { return _absolute_repeat; }
    bool                        is_single_start             () const                                { return _single_start; }
    bool                        let_run                     () const                                { return _let_run; }
    bool                        has_repeat_or_once          () const                                { return _repeat < Time::never || _start_once; }
    Time                        next_repeated               ( const Time& ) const;
    Time                        next_repeated_allow_after_end( const Time& ) const;
    Time                        next_absolute_repeated      ( const Time&, int next ) const;

    void                        check                       ( With_or_without_date ) const;
    string                      to_xml                      () const;
    void                        print                       ( ostream& ) const;
    string                      obj_name                    () const;
    friend ostream&             operator <<                 ( ostream& s, const Period& o )         { o.print(s); return s; }


//private:
    Fill_zero                  _zero_;
    Time                       _begin;                      // Sekunden seit Mitternacht, manchmal auch mit Datum
    Time                       _end;                        // Sekunden seit Mitternacht, manchmal auch mit Datum
    Time                       _repeat;
    Time                       _absolute_repeat;
    bool                       _single_start;
    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _next_end_time beenden
    bool                       _start_once;                 // Für Joacim Zschimmer
};

//extern Period                   empty_period;               // gcc 3.2.1: Nicht const, weil gcc diese Variable nicht initialisiert. Das macht spooler.cxx.

typedef set<Period>             Period_set;

//----------------------------------------------------------------------------------------------Day

struct Day : Object
{
                                Day                         ()                                      {}
                                Day                         ( const Period_set& t )                 { _period_set = t; }
                                Day                         ( const Period& t )                     { _period_set.insert( t ); }
                                Day                         ( const xml::Element_ptr& e, const Day* deflt, const Period* p )   { set_dom_periods( e, deflt, p ); }

    void                        set_dom_periods             ( const xml::Element_ptr&, const Day* deflt, const Period* );
  //void                        set_default                 ();

                                operator bool               () const                                { return !_period_set.empty(); }

    bool                        has_time                    ( const Time& time_of_day );
    Period                      next_period                 ( const Time& time_of_day, With_single_start single_start ) const { return _period_set.empty()? Period(): next_period_(time_of_day,single_start); }
    Period                      next_period_                ( const Time& time_of_day, With_single_start single_start ) const;
    void                        add                         ( const Period& p )                     { _period_set.insert( p ); }       
    void                        merge                       ( const Day& );

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day& o )            { o.print(s); return s; }


    Period_set                 _period_set;
};

//------------------------------------------------------------------------------------------Day_set

struct Day_set
{
                                Day_set                     ( int minimum, int maximum );
    explicit                    Day_set                     ( int minimum, int maximum, const xml::Element_ptr& );

    void                        set_dom                     ( const xml::Element_ptr&, const Day* = NULL, const Period* = NULL );
    int                         get_day_number              ( const string& );
    list<int>                   get_day_numbers             ( const string& );
    int                         get_weekday_number          ( const string& );
    list<int>                   get_weekday_numbers         ( const string& );

    bool                        is_filled                   () const                                { return _is_day_set_filled; }

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Day_set& o )        { o.print(s); return s; }


  protected:
    Fill_zero                  _zero_;
    int                        _minimum;
    int                        _maximum;
    vector< ptr<Day> >         _days;
    bool                       _is_day_set_filled;
};

//--------------------------------------------------------------------------------------Weekday_set

struct Weekday_set : Day_set
{
                                Weekday_set                 ()                                      : Day_set( 0, 6 ) {}
    explicit                    Weekday_set                 ( const xml::Element_ptr& e )           : Day_set( 0, 6, e ) {}

    void                        fill_with_default           ( const Day& );
    Period                      next_period                 ( const Time&, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Weekday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Weekday_set& o )    { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------Monthday_set

struct Monthday_set : Day_set
{
                                Monthday_set                ()                                      : Day_set( 1, 31    ), _zero_(this+1) {}
    explicit                    Monthday_set                ( const xml::Element_ptr& e )           : Day_set( 1, 31, e ), _zero_(this+1) {}

    void                        set_dom                     ( const xml::Element_ptr&, const Day* = NULL, const Period* = NULL );

    bool                        is_filled                   () const                                { return Day_set::is_filled() | 
                                                                                                            _month_weekdays._is_filled | _reverse_month_weekdays._is_filled ; }
    Period                      next_period                 ( const Time&, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Monthday_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Monthday_set& o )   { o.print(s); return s; }


  private:
    Fill_zero                  _zero_;

    struct Month_weekdays
    {
                                Month_weekdays              ()                                      : _days( 7 * max_weekdays_per_month ), _is_filled(false) {}
        ptr<Day>&               day                         ( int which, int weekday_number );

        vector< ptr<Day> >     _days;
        bool                   _is_filled;
    };

    Month_weekdays             _month_weekdays;
    Month_weekdays             _reverse_month_weekdays;
};

//--------------------------------------------------------------------------------------Ultimo_set

struct Ultimo_set : Day_set
{
                                Ultimo_set                  ()                                      : Day_set( 0, 30 ) {}
    explicit                    Ultimo_set                  ( const xml::Element_ptr& e )           : Day_set( 0, 30, e ) {}

    Period                      next_period                 ( const Time&, With_single_start single_start );

    void                        print                       ( ostream& s ) const                    { s << "Ultimo_set("; Day_set::print(s); s << ")"; }
    friend ostream&             operator <<                 ( ostream& s, const Ultimo_set& o )     { o.print(s); return s; }
};

//--------------------------------------------------------------------------------------------Month

struct Month : Object
{
                                Month                       ()                                      : _zero_(this+1) {}

    void                        set_dom                     ( const xml::Element_ptr& );
  //void                        set_default                 ();
    bool                        is_filled                   () const                                { return _weekday_set .is_filled() ||
                                                                                                             _monthday_set.is_filled() ||
                                                                                                             _ultimo_set  .is_filled();   }
    Period                      next_period                 ( const Time&, With_single_start );

    Fill_zero                  _zero_;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
};

//-----------------------------------------------------------------------------------------Holidays

struct Holidays
{
                                Holidays                    ( Spooler* spooler )                    : _spooler(spooler) {}

    void                        clear                       ()                                      { _set.clear(); }
    bool                        is_filled                   () const                                { return !_set.empty(); }
    void                        set_dom                     ( File_based* source_file_based, const xml::Element_ptr&, int include_nesting = 0 );
    void                        include                     ( time_t t )                            { _set.insert( t ); }
    bool                        is_included                 ( const Time& t )                       { return _set.find( t.midnight().as_time_t() ) != _set.end(); }

    Spooler*                   _spooler;
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
    Period                      next_period                 ( const Time&, With_single_start single_start );

    bool                        is_filled                   () const                                { return !_date_set.empty(); }
    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Date_set& o )       { o.print(s); return s; }

    set<Date>                  _date_set;
};

//------------------------------------------------------------------------------------------At_set

struct At_set
{
    Period                      next_period                 ( const Time&, With_single_start single_start );

    bool                        is_filled                   () const                                { return !_at_set.empty(); }
    void                        add                         ( const Time& at )                      { _at_set.insert( at ); }
    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const At_set& o )         { o.print(s); return s; }

    set<Time>                  _at_set;
};

//-------------------------------------------------------------------------------------------------

enum Scheduler_holidays_usage                               // eMail von Andreas Liebert 2008-04-21
{ 
    with_scheduler_holidays,                                // Standard
    without_scheduler_holidays                              // Order.run_time soll nicht <config><holidays> übernehmen
};          


//-------------------------------------------------------------------------------------Schedule_use

struct Schedule_use : idispatch_implementation< Schedule_use, spooler_com::Irun_time>, 
                      spooler_com::Ihas_java_class_name,
                      Scheduler_object,
                      Dependant,
                      Non_cloneable
{
    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];


                                Schedule_use                ( Scheduler_object* using_object );
    virtual                    ~Schedule_use                ();

    // Irun_time
    STDMETHODIMP            put_Xml                         ( BSTR xml );

    // IDispatch
    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    // Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Run_time"; }

    // Scheduler_object 
    void                        close                       ();
    string                      obj_name                    () const;


    // Dependant
    bool                        on_requisite_loaded         ( File_based* );
    bool                        on_requisite_to_be_removed  ( File_based* );
  //void                        on_requisite_removed        ( File_based* );
    Prefix_log*                 log                         ()                                      { return Scheduler_object::log(); }


    void                        disconnect                  ();
    Absolute_path               schedule_path               () const                                { return _schedule_path; }
    void                    set_schedule                    ( Schedule* );
    Schedule*                   schedule                    ();
    void                    set_xml                         ( File_based* source_file_based, const string& );
    void                    set_dom                         ( File_based* source_file_based, const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                    set_scheduler_holidays_usage    ( Scheduler_holidays_usage u )          { _scheduler_holidays_usage = u; }
    void                    set_default_schedule            ( Schedule* );

    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );
    bool                     is_defined                     ()                                      { return _schedule != NULL; }
    bool                        try_load                    ();

    Period                      first_period                ()                                      { return first_period( Time::now() ); }
    Period                      first_period                ( const Time& t )                       { return next_period( t ); }

    Period                      next_period                 ( With_single_start single_start = wss_next_period )      { return next_period( Time::now(), single_start ); }
    Period                      next_period                 ( const Time&, With_single_start single_start = wss_next_period );

    bool                        period_follows              ( const Time& time )                    { return next_period(time).begin() != Time::never; }

    Time                        next_single_start           ( const Time& );
    Time                        next_any_start              ( const Time& );

    virtual void                on_schedule_loaded          ()                                      = 0;
    virtual void                on_schedule_modified        ()                                      = 0;
    virtual bool                on_schedule_to_be_removed   ()                                      = 0;
  //virtual void                on_schedule_removed         ()                                      = 0;
    virtual string              name_for_function           () const                                = 0;

  private:
    Fill_zero                  _zero_;
    Absolute_path              _schedule_path;
    ptr<Schedule>              _schedule;                   // Bennantes <schedule> oder unbenanntes, eigenes (heißt dann <run_time>)
    ptr<Schedule>              _default_schedule;           // Das Schedule, wenn sonst keins definiert ist oder benanntes Schedule nicht bekannt ist.
    Scheduler_object*          _using_object;
    Scheduler_holidays_usage   _scheduler_holidays_usage;
};

//-----------------------------------------------------------------------------------------Schedule

struct Schedule : idispatch_implementation< Schedule, spooler_com::Ischedule>, 
                  spooler_com::Ihas_java_class_name,
                  file_based< Schedule, Schedule_folder, Schedule_subsystem_interface >
{
    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];


    struct Inlay : Object
    {
        // Schedule::put_Xml() soll den Schedule inhaltlich vollständig ersetzen.
        // Die SOS bevorzugt dies vor der üblichen (und eleganten) Lösung, das Objekt zu verwerfen und ein neues anzulegen (mit Spooler.create_schedule()).
        // Deshalb unterscheiden wir hier zwischen Schedule und Inlay, damit
        // letzteres von put_Xml() entfernt und durch ein neues Inlay ersetzt werden kann, unter Beibehaltung des Schedule.
        // Siehe eMail von Andreas Liebert 2008-04-21.

                                    Inlay                       ( Schedule* );
                                   ~Inlay                       ();

        void                    set_dom                         ( File_based* source_file_based, const xml::Element_ptr& );
        xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

        Period                      next_period                 ( Schedule_use*, const Time&, With_single_start single_start );
        int                         month_index_by_name         ( const string& );
        list<int>                   month_indices_by_names      ( const string& );
        void                        check                       ();                              
        bool                        is_filled                   () const;
        Prefix_log*                 log                         ()                                  { return _schedule->log(); }

      private:
        friend struct               Schedule;

        Period                      call_function               ( Schedule_use*, const Time& beginning_time );
        Time                        next_time                   ( const Time& );

        Fill_zero                  _zero_;
        bool                       _once;
        At_set                     _at_set;
        Date_set                   _date_set;
        Weekday_set                _weekday_set;
        Monthday_set               _monthday_set;
        Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
        vector< ptr<Month> >       _months;
        Holidays                   _holidays;
        xml::Document_ptr          _dom;
        string                     _start_time_function;
        bool                       _start_time_function_error;
        Absolute_path              _covered_schedule_path;
        Time                       _covered_schedule_begin;
        Time                       _covered_schedule_end;
        Spooler*                   _spooler;
        Schedule*                  _schedule;
    };


                                Schedule                    ( Schedule_subsystem_interface*, Scheduler_holidays_usage = with_scheduler_holidays );
                               ~Schedule                    ();


    // Scheduler_object

    void                        close                       ();
  //void                        clear                       ();
  //string                      obj_name                    () const;


    // IDispatch

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID&, void** );


    // Ihas_java_class_name

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Schedule"; }


    // Irun_time

    STDMETHODIMP            put_Xml                         ( BSTR );
    STDMETHODIMP            get_Xml                         ( BSTR* );
  //STDMETHODIMP            put_Name                        ( BSTR );     
  //STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( name(), result ); }
  //STDMETHODIMP                Remove                      ();


    // Dependant
    bool                        on_requisite_loaded         ( File_based* );
    bool                        on_requisite_to_be_removed  ( File_based* );

    // file_based<>

    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

  //void                        prepare_to_remove           ();
    bool                        can_be_removed_now          ();
  //zschimmer::Xc               remove_error                ();

  //bool                        can_be_replaced_now         ();
  //void                        prepare_to_replace          ();
  //Schedule*                   on_replace_now              ();


    Schedule_folder*            schedule_folder             () const                                { return typed_folder(); }

    void                    set_dom                         ( const xml::Element_ptr& e )           { set_dom( (File_based*)NULL, e ); }
    void                    set_dom                         ( File_based* source_file_based, const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Document_ptr           dom_document                ();
  //void                        execute_xml                 ( const xml::Element_ptr&, const Show_what& );

    void                        add_use                     ( Schedule_use* );
    void                        remove_use                  ( Schedule_use* );

    void                        cover_with_schedule         ( Schedule* );
    void                        uncover_from_schedule       ( Schedule* );

    void                    set_xml                         ( File_based* source_file_based, const string& );
    Inlay*                      inlay                       ()                                      { return _inlay; }
    bool                     is_covering                    ()                                      { return _inlay->_covered_schedule_path != ""; }
    bool                     is_covering_at                 ( const Time& t )                       { return t >= _inlay->_covered_schedule_begin &&  t < _inlay->_covered_schedule_end; }
    bool                        once                        ()                                      { return _inlay->_once; }
    Period                      next_period                 ( Schedule_use*, const Time&, With_single_start );


    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Schedule& o )       { o.print(s); return s; }


  private:
    void                        set_inlay                   ( Inlay* );
    void                        initialize_inlay            ();
    bool                        try_activate_inlay          ();
    void                        assert_no_overlapped_covering( Schedule* );
    bool                        try_connect_covered_schedule();
    void                        disconnect_covered_schedule ();
    void                        disconnect_covering_schedules();


    Fill_zero                  _zero_;
    Scheduler_holidays_usage   _scheduler_holidays_usage;

    typedef stdext::hash_set<Schedule_use*>    Use_set;
    Use_set                                   _use_set;

    typedef map<double,Schedule*>  Covering_schedules;      // Geordnet!
    Covering_schedules            _covering_schedules;      // <schedule substitute="...">, die auf dieses Schedule verweisen

    ptr<Inlay>                    _inlay;
    Schedule*                     _covered_schedule;        // <schedule substitute="...">, das zu überdeckende Schedule
};

//----------------------------------------------------------------------------------Schedule_folder

struct Schedule_folder : typed_folder< Schedule >
{
                                Schedule_folder             ( Folder* );
                               ~Schedule_folder             ();

  //void                        execute_xml_schedule        ( const xml::Element_ptr& );
    void                        add_schedule                ( Schedule* schedule )                  { add_file_based( schedule ); }
    void                        remove_schedule             ( Schedule* schedule )                  { remove_file_based( schedule ); }
    Schedule*                   schedule                    ( const string& name )                  { return file_based( name ); }
    Schedule*                   schedule_or_null            ( const string& name )                  { return file_based_or_null( name ); }
  //xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "schedules" ); }
};

//---------------------------------------------------------------------Schedule_subsystem_interface

struct Schedule_subsystem_interface : Object,
                                      file_based_subsystem< Schedule >
{
                                Schedule_subsystem_interface( Scheduler*, Type_code );


    virtual ptr<Schedule_folder> new_schedule_folder        ( Folder* )                             = 0;
    Schedule*                   schedule                    ( const Absolute_path& path ) const     { return file_based( path ); }
    Schedule*                   schedule_or_null            ( const Absolute_path& path ) const     { return file_based_or_null( path ); }
    ptr<Schedule>               new_schedule                ()                                      { return new_file_based(); }
  //virtual xml::Element_ptr    execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& ) = 0;
};


ptr<Schedule_subsystem_interface> new_schedule_subsystem    ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace schedule
} //namespace scheduler
} //namespace sos

#endif
