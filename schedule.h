// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifdef Z_SCHEDULE_DEVELEPMENT

#ifndef __SCHEDULER_SCHEDULE_H
#define __SCHEDULER_SCHEDULE_H

namespace sos {
namespace scheduler {
namespace schedule {


struct Schedule;
struct Schedule_folder;
struct Schedule_subsystem;
struct Schedule_subsystem_interface;

//-------------------------------------------------------------------------------------Schedule_use

struct Schedule_use
{
    virtual                    ~Schedule_use                ()                                      {}

    virtual bool                on_schedule_modified        ( Schedule* )                           = 0;
  //virtual bool                on_schedule_removed         ( Schedule* )                           = 0;
    virtual string              name_for_function           () const                                = 0;
    virtual string              obj_name                    () const                                = 0;
    virtual Prefix_log*         log                         ();
};

//-----------------------------------------------------------------------------------------Schedule

struct Schedule : idispatch_implementation< Schedule, spooler_com::Irun_time>, 
                  file_based< Schedule, Schedule_folder, Schedule_subsystem_interface >
{
    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];



                                //Run_time                    ( Scheduler_object* host_object, File_based* source_file_based );
                                Schedule                    ( Schedule_subsystem_interface*, const string& name = "" );
                               ~Schedule                    ();


    // Scheduler_object

    void                        close                       ();
  //string                      obj_name                    () const;


    // IDispatch

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP                QueryInterface              ( const IID&, void** );


    // Ihas_java_class_name

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Run_time"; }


    // Irun_time

    STDMETHODIMP            put_Xml                         ( BSTR xml );
  //STDMETHODIMP            put_Name                        ( BSTR );     
  //STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( name(), result ); }
  //STDMETHODIMP                Remove                      ();


    // file_based<>

    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    void                        prepare_to_remove           ();
    bool                        can_be_removed_now          ();
    zschimmer::Xc               remove_error                ();

    bool                        can_be_replaced_now         ();
    void                        prepare_to_replace          ();
    Schedule*                   on_replace_now              ();


    Schedule_folder*            schedule_folder             () const                                { return typed_folder(); }

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& ) const;
    xml::Document_ptr           dom_document                () const;
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );
    void                        execute_xml                 ( const xml::Element_ptr&, const Show_what& );

    bool                        operator ==                 ( const Run_time& );

    bool                        is_filled                   () const;
    void                    set_modified_event_handler      ( Modified_event_handler* m )           { _modified_event_handler = m; }

    void                    set_xml                         ( const string& );
  //string                      xml                         ()                                      { return _xml; }

    int                         month_index_by_name         ( const string& );
    list<int>                   month_indices_by_names      ( const string& );
    void                        check                       ();                              

    bool                        set                         () const                                { return _set; }

    void                        set_holidays                ( const Holidays& h )                   { _holidays = h; }
    void                        set_default                 ();

    bool                        once                        ()                                      { return _once; }
    void                    set_once                        ( bool b = true )                       { _once = b; }

    Period                      first_period                ()                                      { return first_period( Time::now() ); }
    Period                      first_period                ( const Time& );

    Period                      next_period                 ( With_single_start single_start = wss_next_period )      { return next_period( Time::now(), single_start ); }
    Period                      next_period                 ( const Time&, With_single_start single_start = wss_next_period );
    Period                      call_function               ( const Time& beginning_time );

    bool                        period_follows              ( const Time& time )                    { return next_period(time).begin() != Time::never; }
  //bool                        period_follows              ( const Time& time )                    { return next_period( time, wss_next_period_or_single_start ).begin() != Time::never; }

    Time                        next_single_start           ( const Time& );
    Time                        next_any_start              ( const Time& );

    void                        print                       ( ostream& ) const;
    friend ostream&             operator <<                 ( ostream& s, const Run_time& o )       { o.print(s); return s; }


  private:
    Time                        next_time                   ( const Time& );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Scheduler_object*          _host_object;
    File_based*                _source_file_based;          // == _host_object oder == NULL
  //Application                _application;
    Modified_event_handler*    _modified_event_handler;
    bool                       _set;
    bool                       _once;
    At_set                     _at_set;
    Date_set                   _date_set;
    Weekday_set                _weekday_set;
    Monthday_set               _monthday_set;
    Ultimo_set                 _ultimo_set;                 // 0: Letzter Tag, -1: Vorletzter Tag
    vector< ptr<Month> >       _months;
    Holidays                   _holidays;
  //string                     _xml;
    xml::Document_ptr          _dom;
    string                     _start_time_function;
    bool                       _start_time_function_error;
    ptr<Prefix_log>            _log;

    typedef stdext::hash_set<Schedule_use*>  Use_set;
    Use_set                                 _use_set;
};

//----------------------------------------------------------------------------------Schedule_folder

struct Schedule_folder : typed_folder< Schedule >
{
                                Schedule_folder             ( Folder* );
                               ~Schedule_folder             ();

  //void                        set_dom                     ( const xml::Element_ptr& );
  //void                        execute_xml_schedule        ( const xml::Element_ptr& );
    void                        add_schedule                ( Schedule* schedule )                          { add_file_based( schedule ); }
    void                        remove_schedule             ( Schedule* schedule )                          { remove_file_based( schedule ); }
    Schedule*                   schedule                    ( const string& name )                          { return file_based( name ); }
    Schedule*                   schedule_or_null            ( const string& name )                          { return file_based_or_null( name ); }
  //xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "schedules" ); }
};

//---------------------------------------------------------------------Schedule_subsystem_interface

struct Schedule_subsystem_interface : Object,
                                      file_based_subsystem< Schedule >
{
                                Schedule_subsystem_interface( Scheduler*, Type_code );


    ptr<Schedule_folder>        new_schedule_folder         ( Folder* );


    Schedule*                   schedule                    ( const Absolute_path& path ) const     { return file_based( path ); }
    Schedule*                   schedule_or_null            ( const Absolute_path& path ) const     { return file_based_or_null( path ); }

    virtual xml::Element_ptr    execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& ) = 0;
};

//-------------------------------------------------------------------------------------------------

} //namespace schedule
} //namespace scheduler
} //namespace sos

#endif
#endif
