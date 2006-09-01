// $Id$

#ifndef __SPOOLER_ORDER_H
#define __SPOOLER_ORDER_H


namespace sos {
namespace spooler {

struct Job_chain;
struct Job_chain_node;
struct Order_queue;
struct Order;

//-------------------------------------------------------------------------------------------------

extern const string             scheduler_file_path_variable_name;

//--------------------------------------------------------------------------------------------Order

struct Order : Com_order,
               Scheduler_object,
               Modified_event_handler
{
    Fill_zero                  _zero_;    

    typedef Variant             Payload;
    typedef int                 Priority;               // Höherer Wert bedeutet höhere Priorität
    typedef Variant             State;
    typedef Variant             Id;


    Z_GNU_ONLY(                 Order                   (); )                                       // Für gcc 3.2. Nicht implementiert
                                Order                   ( Spooler* );
                                Order                   ( Spooler*, const VARIANT& );
                                Order                   ( Spooler*, const Record&, const string& payload, const string& run_time, const string& xml );
                               ~Order                   ();

    void                        init                    ();
    void                        attach_task             ( Task* );
    void                        assert_no_task          ();
    bool                        is_immediately_processable( const Time& now = Time() );
    void                        open_log                ();
    void                        close                   ();

    Prefix_log*                 log                     ()                                          { return _log; }
    
    void                    set_id                      ( const Variant& );
    Id                          id                      ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _id ); }
    string                      string_id               ();
    static string               string_id               ( const Id& );
    void                    set_default_id              ();
    bool                        id_is_equal             ( const Id& id )                            { if( _id_locked ) return _id == id; else THREAD_LOCK_RETURN( _lock, bool, _id == id ); }

    void                    set_title                   ( const string& title )                     { THREAD_LOCK(_lock)  _title = title,  _title_modified = true,  _log->set_prefix( obj_name() ); }
    string&                     title                   ()                                          { THREAD_LOCK_RETURN( _lock, string, _title ); }
    string                      obj_name                () const;
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                () const                                    { return _priority; }

    bool                        is_virgin               () const                                    { return _is_virgin; }
    void                    set_delay_storing_until_processing( bool b )                            { _delay_storing_until_processing = b; }

    Job_chain*                  job_chain               () const                                    { return _job_chain? _job_chain : _removed_from_job_chain; }
    Job_chain_node*             job_chain_node          () const                                    { return _job_chain_node; }
  //Job_chain*                  removed_from_job_chain  () const                                    { return _removed_from_job_chain; }
    Order_queue*                order_queue             ();

    bool                        finished                ();

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const string& );
    Job*                        job                     () const;

    void                    set_state                   ( const State& );
    void                    set_state                   ( const State&, const Time& );
    void                    set_state2                  ( const State&, bool is_error_state = false );
    State                       state                   ()                                          { THREAD_LOCK_RETURN( _lock, State, _state ); }
    bool                        state_is_equal          ( const State& state )                      { THREAD_LOCK_RETURN( _lock, bool, _state == state ); }
    static void                 check_state             ( const State& );
    State                       initial_state           ()                                          { THREAD_LOCK_RETURN( _lock, State, _initial_state ); }


    void                    set_state_text              ( const string& state_text )                { THREAD_LOCK( _lock )  _state_text = state_text,  _state_text_modified = true; }
    string                      state_text              ()                                          { THREAD_LOCK_RETURN( _lock, string, _state_text ); }

    Time                        start_time              () const                                    { return _start_time; }
    Time                        end_time                () const                                    { return _end_time; }

    void                    set_file_path               ( const File_path& );                       // Für einen Dateiauftrag (file order)
    File_path                   file_path               () const;
    bool                        is_file_order           () const;
    
    void                    set_payload                 ( const VARIANT& );
    Payload                     payload                 ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _payload ); }
    string                      string_payload          () const;
    ptr<spooler_com::Ivariable_set> params_or_null      () const;
    ptr<spooler_com::Ivariable_set> params              () const;
    void                    set_param                   ( const string& name, const Variant& value );
    Variant                     param                   ( const string& name ) const;

    void                    set_xml_payload             ( const string& xml );
    string                      xml_payload             () const                                    { return _xml_payload; }

    void                    set_web_service             ( const string& );
    void                    set_web_service             ( Web_service* );
    Web_service*                web_service             () const;
    Web_service*                web_service_or_null     () const                                    { return _web_service; }
    void                    set_http_operation          ( http::Operation* op )                     { _http_operation = op; }
    Web_service_operation*      web_service_operation          () const;
    Web_service_operation*      web_service_operation_or_null  () const                             { return _http_operation? _http_operation->web_service_operation_or_null() : NULL; }

    Run_time*                   run_time                ()                                          { return _run_time; }

    Com_job*                    com_job                 ();


    void                        add_to_order_queue      ( Order_queue* );
    void                        add_to_job              ( const string& job_name );

    void                        setback_                ();
    void                        setback                 ( const Time& );
    void                    set_at                      ( const Time& );
    Time                        at                      ()                                          { return _setback; }
  //void                    set_run_time_xml            ( const string& );
    Time                        next_start_time         ( bool first_call = false );

    // Auftrag in einer Jobkette:
    void                        add_to_job_chain        ( Job_chain* );
    void                        add_to_or_replace_in_job_chain( Job_chain* );
    bool                        try_add_to_job_chain    ( Job_chain* );
    void                        remove_from_job_chain   ( bool leave_in_database = false );
    void                        move_to_node            ( Job_chain_node* );
    void                        postprocessing          ( bool success );                           // Verarbeitung nach spooler_process()
    void                        processing_error        ();

    void                    set_dom                     ( const xml::Element_ptr&, Variable_set_map* = NULL );
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what&, const string* log = NULL ) const;
    xml::Document_ptr           dom                     ( const Show_what& ) const;

    void                    set_run_time                ( const xml::Element_ptr& );
    void                        before_modify_run_time_event();
    void                        run_time_modified_event ();

    ptr<Prefix_log>            _log;


  private:
    void                        postprocessing2         ( Job* last_job );


    friend struct               Order_queue;
    friend struct               Job_chain;
    friend void                 Spooler_db::insert_order( Order* );
    friend void                 Spooler_db::update_order( Order* );
    friend void                 Spooler_db::finish_order( Order*, Transaction* );


    Thread_semaphore           _lock;

    Id                         _id;
    bool                       _id_locked;              // Einmal gesperrt, immer gesperrt
    bool                       _is_users_id;            // Id ist nicht vom Spooler generiert, also nicht sicher eindeutig.
  //bool                       _remove_from_job_chain;  // Nur wenn _task != NULL: Nach spooler_process() remove_form_job_chain() rufen!
    Priority                   _priority;
    bool                       _priority_modified;
    State                      _state;
    State                      _initial_state;
    bool                       _initial_state_set;
    string                     _state_text;
    bool                       _state_text_modified;
    string                     _title;
    bool                       _title_modified;
    Job_chain*                 _job_chain;              
    Job_chain_node*            _job_chain_node;         // Nächster Stelle, falls in einer Jobkette
    Job_chain*                 _removed_from_job_chain; // Ehemaliges _job_chain, nach remove_from_job_chain(), wenn _task != NULL
  //bool                       _dont_close_log;
    Order_queue*               _order_queue;            // Auftrag ist in einer Auftragsliste, aber nicht in einer Jobkette. _job_chain == NULL, _job_chain_node == NULL!
    Payload                    _payload;
  //bool                       _payload_modified;       // (Bei einem Objekt wird nur bemerkt, dass die Referenz geändert wurde, nicht das Objekt selbst)

    string                     _xml_payload;
    bool                       _order_xml_modified;     // Datenbankspalte xml neu schreiben!

    ptr<Order>                 _replaced_by;            // Nur wenn _task != NULL: _replaced_by soll this in der Jobkette ersetzen
    Order*                     _replacement_for;        // _replacement_for == NULL  ||  _replacement_for->_replaced_by == this && _replacement_for->_task != NULL

    Time                       _created;
    Time                       _start_time;             // Erster Jobschritt
    Time                       _end_time;

    bool                       _in_job_queue;           // Auftrag ist in _job_chain_node->_job->order_queue() eingehängt
    Task*                      _task;                   // Auftrag wird gerade von dieser Task in spooler_process() verarbeitet 
    bool                       _moved;                  // Nur wenn _task != NULL: true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    ptr<Run_time>              _run_time;
    Period                     _period;                 // Bei _run_time.set(): Aktuelle oder nächste Periode
  //bool                       _period_once;
    Time                       _setback;                // Bis wann der Auftrag zurückgestellt ist (bei _setback_count > 0, sonst Startzeitpunkt "at")
    int                        _setback_count;
    bool                       _suspended;
  //bool                       _recoverable;            // In Datenbank halten
    bool                       _is_in_database;
    bool                       _delay_storing_until_processing;  // Erst in die Datenbank schreiben, wenn die erste Task die Verarbeitung beginnt
    bool                       _is_virgin;              // Noch von keiner Task berührt

    ptr<Web_service>           _web_service;
    ptr<http::Operation>       _http_operation;
};

//----------------------------------------------------------------------Directory_file_order_source

struct Directory_file_order_source : //idispatch_implementation< Directory_file_order_source, spooler_com::Idirectory_file_order_source >,
                                     Async_operation, Scheduler_object
{
    enum State
    {
        s_none,
        s_order_requested
    };


    explicit                    Directory_file_order_source( Job_chain*, const xml::Element_ptr& );
                               ~Directory_file_order_source();

    virtual Prefix_log*         log                     ();
    void                        start                   ();
    Order*                      request_order           ();

    // Async_operation:
    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return false; }
    virtual string              async_state_text_       () const                                    { return "Directory_file_order_source"; }

  private:
    Fill_zero                  _zero_;
    File_path                  _path;
    string                     _regex_string;
    Regex                      _regex;
    ptr<Directory_watcher>     _directory_watcher;
    Job_chain*                 _job_chain;
    bool                       _order_requested;
};

//------------------------------------------------------------------------------------Order_sources

struct Order_sources //: Async_operation
{
    void                        start                   ();
    Order*                      request_order           ();

    // Async_operation:
  //virtual bool                async_continue_         ( Continue_flags );
  //virtual bool                async_finished_         () const                                    { return false; }
  //virtual string              async_state_text_       () const                                    { return "Order_sources"; }


    typedef list< ptr<Directory_file_order_source> >  Order_source_list;
    Order_source_list          _order_source_list;
};

//-----------------------------------------------------------------------------------Job_chain_node

struct Job_chain_node : Com_job_chain_node 
{
                                Job_chain_node          ()                                          : _zero_(this+1) {}

    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what&, Job_chain* );
    int                         order_count             ( Job_chain* = NULL );
    bool                        is_file_order_sink      ()                                          { return _file_order_sink_remove || _file_order_sink_move_to != ""; }


    Fill_zero                  _zero_;

    ptr<Job>                   _job;                    // NULL: Kein Job, Auftrag endet
    bool                       _file_order_sink_remove;  // <file_order_sink remove="yes"/>
    string                     _file_order_sink_move_to; // <file_order_sink move_to="..."/>

    Order::State               _state;                  // Bezeichnung des Zustands

    Order::State               _next_state;             // Bezeichnung des Folgezustands
    Job_chain_node*            _next_node;              // Folgeknoten
    
    Order::State               _error_state;            // Bezeichnung des Fehlerzustands
    Job_chain_node*            _error_node;             // Fehlerknoten

    int                        _priority;               // Das ist die Entfernung zum letzten Knoten + 1, negativ (also -1, -2, -3, ...)
    bool                       _suspended;
};

//----------------------------------------------------------------------------------------Job_chain

struct Job_chain : Com_job_chain, Scheduler_object
{
    //typedef Variant             State;

    enum State
    {
        s_under_construction,   // add_job() gesperrt, add_order() frei
        s_ready,                // in Betrieb
        s_removing,             // Wird entfernt, aber ein Auftrag wird noch verarbeitet
        s_closed                
    };



    Z_GNU_ONLY(                 Job_chain               ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Job_chain               ( Spooler* );
                               ~Job_chain               ();

    void                        close                   ();
    void                        remove                  ();
    void                        check_for_removing      ();
    Prefix_log*                 log                     ()                                          { return &_log; }

    void                    set_name                    ( const string& name )                      { THREAD_LOCK( _lock )  _name = name,  _log.set_prefix( "Job_chain " + _name ); }
    string                      name                    ()                                          { THREAD_LOCK_RETURN( _lock, string, _name ); }

    void                    set_state                   ( State state )                             { _state = state; }
    State                       state                   () const                                    { return _state; }
  //bool                        finished                () const                                    { return _state == s_finished; }
    static string               state_name              ( State );

    void                    set_visible                 ( bool b )                                  { _visible = b; }
    bool                        visible                 () const                                    { return _visible; }

    void                    set_orders_recoverable      ( bool b )                                  { _orders_recoverable = b; }
    void                        load_orders_from_database();
    int                         remove_all_pending_orders( bool leave_in_database = false );

    void                        add_job                 ( Job*, const Order::State& input_state, const Order::State& output_state = error_variant, const Order::State& error_state = error_variant );
    void                        finish                  ();

    Job_chain_node*             first_node             ();
    Job_chain_node*             node_from_state         ( const Order::State& );
    Job_chain_node*             node_from_state_or_null ( const Order::State& );
    Job_chain_node*             node_from_job           ( Job* );

  //Order*                      add_order               ( VARIANT* order_or_payload, VARIANT* job_or_state );
  //void                        remove_order            ( Order* );

    ptr<Order>                  order                   ( const Order::Id& id );
    ptr<Order>                  order_or_null           ( const Order::Id& id );

    bool                        has_order_id            ( const Order::Id& );
    void                        register_order          ( Order* );                                 // Um doppelte Auftragskennungen zu entdecken: Fehler SCHEDULER-186
    void                        unregister_order        ( Order* );

    int                         order_count             ();
    bool                        has_order               () const;

    void                    set_dom                     ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& );

    Order*                      request_order           ()                                          { return _order_sources.request_order(); }

    // Async_operation:
    //virtual bool                async_continue_         ( Continue_flags );
    //virtual bool                async_finished_         () const                                    { return false; }   // nie
    //virtual string              async_state_text_       () const                                    { return "Job_chain " + _name; }


    Fill_zero                  _zero_;
    bool                       _orders_recoverable;
    bool                       _load_orders_from_database;      // load_orders_from_database() muss noch gerufen werden.

  private:
    friend struct               Order;
    Thread_semaphore           _lock;
    Prefix_log                 _log;
    string                     _name;
    State                      _state;
    bool                       _visible;

    Order_sources              _order_sources;

    typedef list< ptr<Job_chain_node> >  Chain;
    Chain                      _chain;

    typedef stdext::hash_map< string, Order* >   Order_map;
    Order_map                  _order_map;
};

//--------------------------------------------------------------------------------Internal_priority
/*
struct Internal_priority
{
                                Internal_priority       ( Priority p = 0 )                          : _ext(p), _sub(0) {}

    bool                        operator <              ( const Internal_priority& p ) const        { return _ext < p._ext? -1
                                                                                                             _ext > p._ext? +1
                                                                                                             _sub < p._sub? -1
                                                                                                             _sub > p._sub? +1
                                                                                                                          :  0; }
    
    Priority                   _ext;
    int                        _sub;
};
*/
//--------------------------------------------------------------------------------------Order_queue

struct Order_queue : Com_order_queue
{
    enum Do_log
    {
        dont_log = false,
        do_log   = true
    };


    Z_GNU_ONLY(                 Order_queue             ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Order_queue             ( Job*, Prefix_log* );
                               ~Order_queue             ();

    void                        close                   ();
    void                        add_order               ( Order*, Do_log = do_log );
  //Order*                      add_order               ( const Order::Payload& );
    void                        remove_order            ( Order* );
    int                         order_count             ( const Job_chain* = NULL );
    bool                        empty                   ()                                          { return _queue.empty(); }
    bool                        empty                   ( const Job_chain* job_chain )              { return order_count( job_chain ) == 0; }
    Order*                      first_order             ( const Time& now );
    bool                        has_order               ( const Time& now )                         { return first_order(now) != NULL; }
    ptr<Order>                  get_order_for_processing( const Time& now );
    Time                        next_time               ();
    void                        update_priorities       ();
    ptr<Order>                  order_or_null           ( const Order::Id& );
    Job*                        job                     () const                                    { return _job; }
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& , Job_chain* );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Thread_semaphore           _lock;
    Job*                       _job;
    Prefix_log*                _log;
    ptr<Com_order_queue>       _com_order_queue;
    
    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;                  // Order._setback == 0
    Queue                      _setback_queue ;         // Order._setback > 0

    int                        _lowest_priority;        // Zur Optimierung
    int                        _highest_priority;       // Zur Optimierung
    bool                       _has_users_id;           // D.h. id auf Eindeutigkeit prüfen. Bei selbst generierten Ids überflüssig. Zur Optimierung.


  //typedef map<Order::Id, Queue::iterator >   Id_map;
  //Id_map                                    _id_map;

};

//-------------------------------------------------------------------------------------------------

//string                          string_from_state       ( Order::State );

} //namespace spooler
} //namespace sos

#endif
