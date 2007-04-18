// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_ORDER_H
#define __SPOOLER_ORDER_H


namespace sos {
namespace scheduler {

struct Job_chain;
struct Job_chain_node;
struct Order_queue;
struct Order;
struct Order_subsystem;
struct Database_order_detector;
struct Directory_file_order_source;

//-------------------------------------------------------------------------------------------------

extern const string             scheduler_file_path_variable_name;

//--------------------------------------------------------------------------------------------Order

struct Order : Com_order,
               Scheduler_object,
               Modified_event_handler
{
    typedef Variant             Payload;
    typedef int                 Priority;               // Höherer Wert bedeutet höhere Priorität
    typedef Variant             State;
    typedef Variant             Id;


    enum Order_state
    {
        s_none,
        s_active
    };



    Z_GNU_ONLY(                 Order                   (); )                                       // Für gcc 3.2. Nicht implementiert
                                Order                   ( Spooler* );
                                Order                   ( Spooler*, const VARIANT& );
                                Order                   ( Spooler*, const Record&, const string& job_chain_name );
                               ~Order                   ();


    // Scheduler_object:
    virtual string              obj_name                () const;
    virtual IDispatch*          idispatch               ()                                          { return this; }


    enum Close_flag { cls_dont_remove_from_job_chain, cls_remove_from_job_chain };
    void                        close                   ( Close_flag );
  //void                        close                   ()                                          { close( true ); }

    void                        init                    ();
    void                        activate                ();
    void                        load_blobs              ( Read_transaction* );
    void                        load_order_xml_blob     ( Read_transaction* );
    void                        load_run_time_blob      ( Read_transaction* );
    void                        load_payload_blob       ( Read_transaction* );

    bool                        occupy_for_task         ( Task*, const Time& now );
    void                        assert_no_task          ( const string& debug_text );
    void                        assert_task             ( const string& debug_text );

    bool                        is_immediately_processable( const Time& now );
    bool                        is_processable          ();
    void                        handle_changed_processable_state ();
    void                        signal_job_when_order_has_become_processable();

    void                        open_log                ();

    void                        print_xml_child_elements_for_event( String_stream*, Scheduler_event* );
    
    void                    set_id                      ( const Variant& );
    const Id&                   id                      ()                                          { return _id; }
    string                      string_id               ();
    static string               string_id               ( const Id& );
    void                    set_default_id              ();
    bool                        id_is_equal             ( const Id& id )                            { return _id == id; }

    void                    set_title                   ( const string& title )                     { _title = title,  _title_modified = true,  _log->set_prefix( obj_name() ); }
    string&                     title                   ()                                          { return _title; }
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                () const                                    { return _priority; }

    bool                        is_virgin               () const                                    { return _is_virgin; }
    void                    set_delay_storing_until_processing( bool b )                            { _delay_storing_until_processing = b; }

    Job_chain*                  job_chain               () const;
    string                      job_chain_name          () const                                    { return _job_chain_name; }
    Job_chain*                  job_chain_for_api       () const;
    Job_chain_node*             job_chain_node          () const                                    { return _job_chain_node; }
  //Job_chain*                  removed_from_job_chain  () const                                    { return _removed_from_job_chain; }
    Order_queue*                order_queue             ();

    bool                        finished                ();
    void                    set_end_state_reached       ()                                          { _end_state_reached = true; }
    bool                        end_state_reached       ();

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const string& );
    Job*                        job                     () const;

    void                    set_job_chain_node          ( Job_chain_node*, bool is_error_state = false );
    void                    set_state                   ( const State&, const Time& );
    void                    set_state                   ( const State& );
    void                    set_state1                  ( const State& );
    void                    set_state2                  ( const State&, bool is_error_state = false );
    State                       state                   ()                                          { return _state; }
    bool                        state_is_equal          ( const State& state )                      { return _state == state; }
    static void                 check_state             ( const State& );
    State                       initial_state           ()                                          { return _initial_state; }


    void                    set_state_text              ( const string& state_text )                { _state_text = state_text,  _state_text_modified = true; }
    string                      state_text              ()                                          { return _state_text; }

    Time                        start_time              () const                                    { return _start_time; }
    Time                        end_time                () const                                    { return _end_time; }

    void                    set_file_path               ( const File_path& );                       // Für einen Dateiauftrag (file order)
    File_path                   file_path               ();
    bool                        is_file_order           ();
    
    void                    set_payload                 ( const VARIANT& );
    const Payload&              payload                 ()                                          { return _payload; }
    string                      string_payload          () const;
    void                    set_params                  ( const xml::Element_ptr&, Variable_set_map* = NULL );
    ptr<Com_variable_set>       params_or_null          () const;
    ptr<Com_variable_set>       params                  ();
    void                    set_param                   ( const string& name, const Variant& value );
    Variant                     param                   ( const string& name );

    void                    set_xml_payload             ( const string& xml );
    void                    set_xml_payload             ( const xml::Element_ptr& );
    string                      xml_payload             () const                                    { return _xml_payload; }
    void                    set_web_service             ( const string&, bool force = false );
    void                    set_web_service             ( Web_service*, bool force = false );
    Web_service*                web_service             () const;
    Web_service*                web_service_or_null     () const                                    { return _web_service; }
    void                    set_http_operation          ( http::Operation* op )                     { _http_operation = op; }
    Web_service_operation*      web_service_operation        () const;
    Web_service_operation*      web_service_operation_or_null() const                               { return _http_operation? _http_operation->web_service_operation_or_null() : NULL; }

    Task*                       task                    () const                                    { return _task; }

    Run_time*                   run_time                ()                                          { return _run_time; }

    Com_job*                    com_job                 ();


    void                        add_to_order_queue      ( Order_queue* );
    void                        add_to_job              ( const string& job_name );

    bool                        suspended               ()                                          { return _suspended; }
    void                    set_suspended               ( bool b = true );

    void                        set_on_blacklist        ();
    void                        remove_from_blacklist   ();
    bool                        is_on_blacklist         ()                                          { return _is_on_blacklist; }

    void                        inhibit_distribution    ()                                          { _is_distribution_inhibited = true; }
    void                        assert_is_not_distributed( const string& debug_text );
    void                    set_distributed             ( bool = true );
    bool                     is_distributed             () const                                    { return _is_distributed; }

    void                        start_now               ();
    void                        setback                 ();
    void                    set_setback                 ( const Time&, bool keep_setback_count = false );
    void                        clear_setback           ( bool keep_setback_count = false );
    bool                     is_setback                 ()                                          { return _setback_count > 0; }
    void                    set_at                      ( const Time& );
    Time                        at                      ()                                          { return _setback; }
  //void                    set_run_time_xml            ( const string& );
    void                    set_replacement             ( Order* replaced_order );
    void                    set_replacement             ( bool );
    Time                        next_time               ();
    Time                        next_start_time         ( bool first_call = false );
    void                        set_next_start_time     ();

    // Auftrag in einer Jobkette:
    void                        place_in_job_chain      ( Job_chain* );
    void                        place_or_replace_in_job_chain( Job_chain* );
    bool                        try_place_in_job_chain  ( Job_chain* );
    void                        remove_from_job_chain   ();
    void                        remove_from_job         ();
    bool                        tip_own_job_for_new_distributed_order_state();
    void                        move_to_node            ( Job_chain_node* );
    void                        postprocessing          ( bool success );                           // Verarbeitung nach spooler_process()
    void                        processing_error        ();
    void                        handle_end_state        ();

    void                    set_dom                     ( const xml::Element_ptr&, Variable_set_map* = NULL );
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what&, const string* log = NULL ) const;
    xml::Document_ptr           dom                     ( const Show_what& ) const;
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    void                    set_run_time                ( const xml::Element_ptr& );
    void                        on_before_modify_run_time();
    void                        run_time_modified_event ();

    void                        db_insert               ();
    bool                        db_try_insert           ();
    bool                        db_occupy_for_processing();
    bool                        db_release_occupation   ();
    void                        db_fill_stmt            ( sql::Write_stmt* );
    string                      calculate_db_distributed_next_time();

    enum Update_option { update_anyway, update_not_occupied, update_and_release_occupation };
    bool                        db_update               ( Update_option u )                         { return db_update2( u, false ); }
    bool                        db_update2              ( Update_option, bool delet, Transaction* outer_transaction = NULL );
    bool                        db_delete               ( Update_option u, Transaction* outer_transaction = NULL )              { return db_update2( u, true, outer_transaction ); }
    bool                        db_handle_modified_order( Transaction* );

    string                      db_read_clob            ( Read_transaction*, const string& column_name );
    void                        db_update_clob          ( Transaction*, const string& column_name, const string& value );

  //void                        db_delete_order         ();
    void                        db_show_occupation      ( Log_level );
    sql::Update_stmt            db_update_stmt          ();
    sql::Where_clause           db_where_clause         ();
    void                        db_fill_where_clause    ( sql::Where_clause* );
    int                         db_get_ordering         ( Transaction* ta = NULL );
    Database*                   db                      ();
    Order_subsystem*            order_subsystem         () const;


  private:
    void                        postprocessing2         ( Job* last_job );


    friend struct               Order_queue;
    friend struct               Job_chain;


    Fill_zero                  _zero_;    

    Id                         _id;
    State                      _state;

    bool                       _id_locked;              // Einmal gesperrt, immer gesperrt
    string                     _state_text;
    bool                       _state_text_modified;
    Priority                   _priority;
    bool                       _priority_modified;
    string                     _title;
    bool                       _title_modified;
    string                     _job_chain_name;
    Payload                    _payload;
    string                     _xml_payload;
    State                      _initial_state;
    ptr<Web_service>           _web_service;

    bool                       _is_virgin;              // Noch von keiner Task berührt
    int                        _setback_count;
    bool                       _is_on_blacklist;        // assert( _job_chain )
    bool                       _suspended;

    ptr<Run_time>              _run_time;
    bool                       _run_time_modified;
    Time                       _setback;                // Bis wann der Auftrag zurückgestellt ist (bei _setback_count > 0, sonst Startzeitpunkt "at")
    bool                       _order_xml_modified;     // Datenbankspalte xml neu schreiben!
    bool                       _is_replacement;         // _replacement_for != NULL => _is_replacement

    Time                       _created;
    Time                       _start_time;             // Erster Jobschritt
    Time                       _end_time;

    bool                       _is_distributed;         // == scheduler_orders.distributed_next_time is not null


    // Flüchtige Variablen, nicht für die Datenbank:

    Order_state                _order_state;
    Job_chain*                 _job_chain;              // Nur gesetzt, wenn !_is_distributed oder in Verarbeitung (_task). Sonst wird der Auftrag nur in der Datenbank gehalten
    Job_chain_node*            _job_chain_node;         // if( _job_chain)  Nächste Stelle, falls in einer Jobkette
    Order_queue*               _order_queue;            // Auftrag ist in einer Auftragsliste, aber nicht in einer Jobkette. _job_chain == NULL, _job_chain_node == NULL!

    string                     _removed_from_job_chain_name; // Ehemaliges _job_chain->name(), nach remove_from_job_chain(), wenn _task != NULL
    ptr<Order>                 _replaced_by;            // Nur wenn _task != NULL: _replaced_by soll this in der Jobkette ersetzen
    Order*                     _replacement_for;        // _replacement_for == NULL  ||  _replacement_for->_replaced_by == this && _replacement_for->_task != NULL
    string                     _replaced_order_occupator;// Task::obj:name() oder cluster_member_id, zur Info

    Period                     _period;                 // Bei _run_time.set(): Aktuelle oder nächste Periode

    bool                       _initial_state_set;
    bool                       _in_job_queue;           // Auftrag ist in _job_chain_node->_job->order_queue() eingehängt

    Task*                      _task;                   // Auftrag wird gerade von dieser Task in spooler_process() verarbeitet 
    bool                       _moved;                  // Nur wenn _task != NULL: true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    bool                       _setback_called;

    bool                       _is_distribution_inhibited;
    bool                       _is_in_database;
    bool                       _is_db_occupied;
    State                      _occupied_state;
    bool                       _delay_storing_until_processing;  // Erst in die Datenbank schreiben, wenn die erste Task die Verarbeitung beginnt
    bool                       _end_state_reached;      // Auftrag nach spooler_process() beenden, für <file_order_sink>
    Time                       _old_next_time;
    ptr<http::Operation>       _http_operation;
};

//-------------------------------------------------------------------------------------------------

Order::State                    normalized_state        ( const Order::State& );

//-------------------------------------------------------------------------------------Order_source

struct Order_source : Scheduler_object, Event_operation
{
                                Order_source            ( Job_chain*, Scheduler_object::Type_code );


    // Scheduler_object:
    Prefix_log*                 log                     ();


    virtual void                close                   ()                                          = 0;
    virtual void                finish                  ();
    virtual void                start                   ()                                          = 0;
    virtual bool                request_order           ( const string& cause )                     = 0;
    virtual Order*              fetch_and_occupy_order  ( const Time& now, const string& cause , Task* occupying_task ) = 0;
    virtual void                withdraw_order_request  ()                                          = 0;

    virtual xml::Element_ptr    dom_element             ( const xml::Document_ptr&, const Show_what& ) = 0;

  protected:
    Fill_zero                  _zero_;
    Job_chain*                 _job_chain;
    Order::State               _next_state;
    Order_queue*               _next_order_queue;
};

//------------------------------------------------------------------------------------Order_sources

struct Order_sources 
{
    void                        close                   ();
    void                        finish                  ();
    void                        start                   ();
  //bool                        request_order           ( const string& cause );
    bool                        has_order_source        ()                                          { return !_order_source_list.empty(); }


    typedef list< ptr<Order_source> >  Order_source_list;
    Order_source_list          _order_source_list;
};

//-----------------------------------------------------------------------------------Job_chain_node

struct Job_chain_node : Com_job_chain_node 
{
    enum Action
    {
        act_process, 
        act_stop, 
        act_next_state
    };

    static Action               action_from_string      ( const string& );
    static string               string_from_action      ( Action );


                                Job_chain_node          ()                                          : _zero_(this+1) {}

    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what&, Job_chain* );
    int                         order_count             ( Read_transaction*, Job_chain* = NULL );

    void                    set_action                  ( const string& );
    string               string_action                  () const                                    { return string_from_action( _action ); }

    bool                        is_end_state            () const                                    { return _job == NULL; }
    bool                        is_file_order_sink      ()                                          { return _file_order_sink_remove || _file_order_sink_move_to != ""; }
    xml::Element_ptr            execute_xml             ( Command_processor*, const xml::Element_ptr&, const Show_what& );


    Fill_zero                  _zero_;

    Order::State               _state;                  // Bezeichnung des Zustands

    Order::State               _next_state;             // Bezeichnung des Folgezustands
    Order::State               _error_state;            // Bezeichnung des Fehlerzustands

    Job_chain_node*            _next_node;              // Folgeknoten
    Job_chain_node*            _error_node;             // Fehlerknoten
    Job_chain*                 _job_chain;

    ptr<Job>                   _job;                    // NULL: Kein Job, Auftrag endet
    string                     _job_chain_name;         // Wenn's eine untergeordnete Jobkette ist
    bool                       _file_order_sink_remove; // <file_order_sink remove="yes"/>
    File_path                  _file_order_sink_move_to;// <file_order_sink move_to="..."/>
    bool                       _suspend;                // <job_chain_node suspend="yes"/>
    int                        _delay;                  // <job_chain_node delay="..."/>  Verzögerung des Auftrags
    Action                     _action;
    int                        _priority;               // Das ist die Entfernung zum letzten Knoten + 1, negativ (also -1, -2, -3, ...)
};

//----------------------------------------------------------------------------------------Job_chain

struct Job_chain : Com_job_chain, Scheduler_object
{
    enum State
    {
        s_under_construction,   // add_job() gesperrt, add_order() frei
        s_running,              // in Betrieb
        s_stopped,              // Angehalten
        s_removing,             // Wird entfernt, aber ein Auftrag wird noch verarbeitet
        s_closed                
    };



    Z_GNU_ONLY(                 Job_chain               ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Job_chain               ( Spooler* );
                               ~Job_chain               ();

    void                        close                   ();
    void                        activate                ();
    void                        remove                  ();
    void                        check_for_removing      ();

    void                    set_name                    ( const string& name )                      { _name = name,  _log->set_prefix( obj_name() ); }
    string                      name                    () const                                    { return _name; }

    void                    set_state                   ( State state )                             { _state = state; }
    State                       state                   () const                                    { return _state; }
    static string               state_name              ( State );

    void                    set_visible                 ( bool b )                                  { _visible = b; }
    bool                        visible                 () const                                    { return _visible; }

    bool                     is_distributed             () const                                    { return _is_distributed; }

    void                    set_orders_recoverable      ( bool b )                                  { _orders_recoverable = b; }

    void                        add_orders_from_database      ( Read_transaction* );
    int                         load_orders_from_result_set   ( Read_transaction*, Any_file* result_set );
    Order*                      add_order_from_database_record( Read_transaction*, const Record& );

    bool                        tip_for_new_distributed_order       ( const Order::State& state, const Time& at );

    int                         remove_all_pending_orders( bool leave_in_database = false );

    Job_chain_node*             add_job                 ( Job*, const Order::State& input_state, 
                                                          const Order::State& next_state  = Variant(Variant::vt_missing), 
                                                          const Order::State& error_state = Variant(Variant::vt_missing) );
    void                        finish                  ();
    void                        check_job_chain_node    ( Job_chain_node* );
    bool                        contains_job            ( Job* );

  //Job*                        first_job               ();
    Job_chain_node*             first_node              ();
    Job_chain_node*             referenced_node_from_state( const Order::State& );
    Job_chain_node*             node_from_state         ( const Order::State& );
    Job_chain_node*             node_from_state_or_null ( const Order::State& );
    Job_chain_node*             node_from_job           ( Job* );
    Job*                        job_from_state          ( const Order::State& );

    void                        add_order               ( Order* );
    void                        remove_order            ( Order* );

    ptr<Order>                  order                   ( const Order::Id& id );
    ptr<Order>                  order_or_null           ( const Order::Id& id );

    bool                        has_order_id            ( Read_transaction*, const Order::Id& );
    void                        register_order          ( Order* );                                 // Um doppelte Auftragskennungen zu entdecken: Fehler SCHEDULER-186
    void                        unregister_order        ( Order* );
    void                        add_order_to_blacklist  ( Order* );
    void                        remove_order_from_blacklist( Order* );
    bool                        is_on_blacklist         ( const string& order_id );
    Order*                      blacklisted_order_or_null( const string& order_id );
    stdext::hash_set<string>    db_blacklist_id_set     ();

    int                         order_count             ( Read_transaction* );
    bool                        has_order               () const;

    string                      db_where_condition      () const;

    void                    set_dom                     ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            execute_xml             ( Command_processor*, const xml::Element_ptr&, const Show_what& );
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    Order_subsystem*            order_subsystem         () const;
    string                      obj_name                () const                                    { return "Job_chain " + _name; }


    Fill_zero                  _zero_;
    bool                       _orders_recoverable;
    bool                       _is_distributed;                 // Aufträge können vom verteilten Scheduler ausgeführt werden
    bool                       _load_orders_from_database;      // add_orders_from_database() muss noch gerufen werden.

    Order_sources              _order_sources;

    typedef stdext::hash_map< string, ptr<Order> >   Blacklist_map;
    Blacklist_map              _blacklist_map;

  private:
    friend struct               Order;
    string                     _name;
    State                      _state;
    bool                       _visible;

    typedef stdext::hash_map< string, Order* >   Order_map;
    Order_map                  _order_map;

  public:
    typedef list< ptr<Job_chain_node> >  Chain;
    Chain                      _chain;
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
    Z_GNU_ONLY(                 Order_queue             ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Order_queue             ( Job*, Prefix_log* );
                               ~Order_queue             ();

    void                        close                   ();
    void                        add_order               ( Order*, Do_log = do_log );
    void                        remove_order            ( Order*, Do_log = do_log );
    void                        reinsert_order          ( Order* );
    void                        register_order_source   ( Order_source* );
    void                        unregister_order_source ( Order_source* );
    int                         order_count             ( Read_transaction*, const Job_chain* = NULL );
    bool                        empty                   ()                                          { return _queue.empty(); }
    bool                        empty                   ( const Job_chain* job_chain )              { return order_count( (Read_transaction*)NULL, job_chain ) == 0; }  // Ohne Datenbank
    Order*                      first_order             ( const Time& now ) const;
    Order*                      fetch_order             ( const Time& now );
    Order*                      load_and_occupy_next_distributed_order_from_database( Task* occupying_task, const Time& now );
    bool                        has_order               ( const Time& now )                         { return first_order( now ) != NULL; }
    bool                        request_order           ( const Time& now, const string& cause );
    void                        withdraw_order_request  ();
    void                        withdraw_distributed_order_request();
    Order*                      fetch_and_occupy_order  ( const Time& now, const string& cause, Task* occupying_task );
    Time                        next_time               ();
    bool                        is_distributed_order_requested             ( time_t now )           { return _next_distributed_order_check_time <= now; }
    time_t                      next_distributed_order_check_time          ()              const    { return _next_distributed_order_check_time; }
    void                        calculate_next_distributed_order_check_time( time_t now );
    void                    set_next_announced_distributed_order_time( const Time&, bool is_now );
    Time                        next_announced_distributed_order_time();
    void                        tip_for_new_distributed_order       ();
  //void                        update_priorities       ();
  //ptr<Order>                  order_or_null           ( const Order::Id& );
    Job*                        job                     () const                                    { return _job; }
    bool                        is_in_any_distributed_job_chain();
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& , Job_chain* );
    string                      db_where_expression_for_distributed_orders();
    string                      db_where_expression_for_job_chain( const Job_chain* );

    Order_subsystem*            order_subsystem         () const;


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    friend struct               Directory_file_order_source;        // Darf _queue lesen
    friend struct               Job_chain_node;                     // Darf _queue lesen

    Job*                       _job;
    Prefix_log*                _log;
    ptr<Com_order_queue>       _com_order_queue;
    
    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;

  //bool                       _is_distributed_order_requested;
    time_t                     _next_distributed_order_check_time;
    int                        _next_distributed_order_check_delay;
    Time                       _next_announced_distributed_order_time;      // Gültig, wenn _is_distributed_order_requested
    bool                       _has_tip_for_new_order;

    typedef list< Order_source* >  Order_source_list;
    Order_source_list             _order_source_list;    // Muss leer sein bei ~Job!

  //int                        _lowest_priority;        // Zur Optimierung
  //int                        _highest_priority;       // Zur Optimierung
};

//------------------------------------------------------------------------Order_subsystem_interface

struct Order_subsystem_interface : Subsystem
{
    enum Load_order_flags
    {
        lo_none,
        lo_lock = 0x01,
        lo_blacklisted = 0x02,
        lo_blacklisted_lock = lo_blacklisted | lo_lock
    };

                                Order_subsystem_interface   ( Scheduler* );


    virtual void                load_job_chains_from_xml    ( const xml::Element_ptr& )             = 0;
    virtual void                add_job_chain               ( Job_chain* )                          = 0;
    virtual void                remove_job_chain            ( Job_chain* )                          = 0;
    virtual void                check_exception             ()                                      = 0;
    virtual bool                are_orders_distributed      ()                                      = 0;
    virtual bool                is_job_in_any_job_chain     ( Job* )                                = 0;
    virtual bool                is_job_in_any_distributed_job_chain( Job* )                         = 0;

    virtual void                request_order               ()                                      = 0;
    virtual ptr<Order>          load_order_from_database    ( Transaction*, const string& job_chain_name, const Order::Id&, Load_order_flags = lo_none ) = 0;
    virtual ptr<Order>      try_load_order_from_database    ( Transaction*, const string& job_chain_name, const Order::Id&, Load_order_flags = lo_none ) = 0;
    virtual string              order_db_where_condition    ( const string& job_chain_name, const string& order_id ) = 0;

    virtual Job_chain*          job_chain                   ( const string& name )                  = 0;
    virtual Job_chain*          job_chain_or_null           ( const string& name )                  = 0;
    virtual xml::Element_ptr    job_chains_dom_element      ( const xml::Document_ptr&, const Show_what& ) = 0;
    virtual void                append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) = 0;

    virtual int                 finished_orders_count       () const                                = 0;
    virtual int                 job_chain_map_version       () const                                = 0;
};


ptr<Order_subsystem_interface>  new_order_subsystem         ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
