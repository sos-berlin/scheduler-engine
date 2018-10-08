// $Id: spooler_order.h 14884 2011-07-22 13:35:24Z ss $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_ORDER_H
#define __SPOOLER_ORDER_H

#include "../zschimmer/reference.h"

namespace sos {
namespace scheduler {
namespace order {

struct Database_order_detector;
struct Directory_file_order_source;
struct Job_chain;
struct Job_chain_folder;
struct Order;
struct Order_id_space;
struct Order_id_spaces;
struct Order_queue;
struct Order_schedule_use;
struct Order_subsystem_impl;
struct Standing_order_folder;
struct Standing_order_subsystem;
struct File_exists_call;

namespace job_chain
{
    struct End_node;
    struct Node;
    struct Job_node;
    struct Nested_job_chain_node;
    struct Order_queue_node;
    struct Sink_node;
};

//-------------------------------------------------------------------------------FOR_EACH_JOB_CHAIN

#define FOR_EACH_JOB_CHAIN( JOB_CHAIN )  \
    Z_FOR_EACH( Order_subsystem::File_based_map, spooler()->order_subsystem()->_file_based_map, __job_chain_iterator__ )  \
        if( Job_chain* JOB_CHAIN = __job_chain_iterator__->second )

//-------------------------------------------------------------------------------------------------

typedef stdext::hash_set<Job_chain*>   Job_chain_set;

//-----------------------------------------------------------------------------Untouched_is_allowed

enum Untouched_is_allowed {
    untouched_not_allowed = false,
    untouched_allowed = true
};


extern const string scheduler_file_order_path_variable_name;

//---------------------------------------------------------------------------Order_state_transition

struct Order_state_transition { 
    public: static const Order_state_transition standard_error;
    public: static const Order_state_transition success;
    public: static const Order_state_transition keep;

    public: static Order_state_transition of_exit_code(int i) {
        return Order_state_transition(i);
    }
    
    public: static Order_state_transition of_bool(bool b) {
        return b? success : standard_error;
    }
    
    public: Order_state_transition(int return_code) :
        _internal_value(return_code)
    {}

    private: Order_state_transition() :
        _internal_value(INT64_MAX)  // Out of range of Unix and Windows exit codes (max. 32 bits)
    {}

    public: bool operator ==(const Order_state_transition& o) const { 
        return internal_value() == o.internal_value();
    }

    public: bool operator !=(const Order_state_transition& o) const { 
        return !(*this == o);
    }

    public: bool is_success() const {
        return *this == success;
    }

    public: bool is_error() const {
        return *this != success && *this != keep;
    }

    public: int return_code() const {
        assert((_internal_value & ~0xFFFFFFFF) == 0);
        return (int)_internal_value;
    }

    public: int64 internal_value() const {
        return _internal_value;
    }

    private: int64 _internal_value;
};


//--------------------------------------------------------------------------------------------Order

struct Order : Com_order,
               file_based< Order, Standing_order_folder, Standing_order_subsystem >,
               javabridge::has_proxy<Order>
{
    typedef Variant             Payload;
    typedef int                 Priority;               // Höherer Wert bedeutet höhere Priorität
    typedef Variant             State;
    typedef Variant             Id;



    Z_GNU_ONLY(                 Order                   (); )                                       // Für gcc 3.2. Nicht implementiert
                                Order                   ( Standing_order_subsystem* );
                               ~Order                   ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Order>::java_sister(); }
    OrderJ&                     typed_java_sister           ()                                      { return _typed_java_sister; }


    // Abstract_scheduler_object

    void                        close                       ();
    virtual string              obj_name                    () const;


    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Com_order::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Com_order::Release(); }

    void                    set_name                        ( const string& );
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    bool                        can_be_removed_now          ();
    bool                        on_remove_now               ();
    Order*                      on_replace_now              ();
    bool                        try_delete_distributed      (ptr<Order>* removed_order = NULL);


    // Dependant
    void                        on_requisite_removed        ( File_based* );


    //

    Standing_order_folder*      standing_order_folder       () const                                { return typed_folder(); }
    
    void                        load_record                 ( const Absolute_path&, const Record& );
    void                        load_blobs                  ( Read_transaction* );
    void                        load_order_xml_blob         ( Read_transaction* );
    void                        load_run_time_blob          ( Read_transaction* );
    void                        load_payload_blob           ( Read_transaction* );

    void                        occupy_for_task             ( Task*, const Time& now );
    void                        assert_no_task              ( const string& debug_text );
    void                        assert_task                 ( const string& debug_text );

    bool                        is_immediately_processable  ( const Time& now );
    bool                        is_processable              ();
    bool                        is_self_processable         ();
    void                        handle_changed_processable_state();
    void                        signal_job_when_order_has_become_processable();

    void                        open_log                ();

    void                        print_xml_child_elements_for_event( String_stream*, Scheduler_event* );
    
    void                    set_id                      ( const Variant& );
    const Id&                   id                      ()                                          { return _id; }
    string                      string_id               ();
    static string               string_id               ( const Id& );
    void                    set_default_id              ();
    bool                        id_is_equal             ( const Id& id )                            { return _id == id; }
    
    bool id_locked() const {
        return _id_locked;
    }

    void                    set_title                   ( const string& title )                     { _title = title,  _title_modified = true,  log()->set_prefix( obj_name() ); }
    string&                     title                   ()                                          { return _title; }
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                () const                                    { return _priority; }

    void                        touch                   (Task*);
    bool                        is_touched              () const                                    { return _is_touched; }

    bool is_touched_in_current_job_chain() const { 
        return _outer_job_chain_path.empty()? _is_touched : _is_nested_touched; 
    }

    const Absolute_path& outer_job_chain_path() const {
        return _outer_job_chain_path;
    }

    void                    set_delay_storing_until_processing( bool b )                            { _delay_storing_until_processing = b; }

    Job_chain*                  job_chain               () const;
    Absolute_path               job_chain_path          () const                                    { return _job_chain_path; }
    string                      job_chain_path_string   () const                                    { return _job_chain_path.with_slash(); }
    bool                        is_in_job_chain         () const                                    { return !_job_chain_path.empty(); }
    Job_chain*                  job_chain_for_api       () const;
    job_chain::Node*            job_chain_node          () const                                    { return _job_chain_node; }
    javabridge::Lightweight_jobject java_job_chain_node() const;
    Order_queue*                order_queue             ();

    bool                        finished                ();
    void                    set_end_state_reached       ()                                          { _end_state_reached = true; }
    bool                        end_state_reached       ();

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const Absolute_path& );
    Job*                        job                     () const;

    void                    set_job_chain_node          ( job_chain::Node*, bool is_error_state = false );
    void                    set_job_chain_node_raw      (job_chain::Node* node)                     { _job_chain_node = node; }
    void                    set_state                   ( const State&, const Time& );
    void                    set_state                   ( const State& );
    void                    set_state1                  ( const State& );
    void                    set_state2                  ( const State&, bool is_error_state = false );
    const State&                state                   () const                                    { return _state; }
    string                      string_state            () const                                    { return _state.as_string(); }
    bool                        state_is_equal          ( const State& state )                      { return _state == state; }
    static void                 check_state             ( const State& );
    const State&                initial_state           () const                                    { return _initial_state; }
    string                      initial_state_string    () const                                    { return _initial_state.as_string(); }
    void                        reset                   ();

    void                    set_end_state               (const State&, bool with_original = false);
    const State&                end_state               ()                                          { return _end_state; }
    string                      end_state_string        ()                                          { return _end_state.as_string(); }

    void                    set_state_text              ( const string& state_text )                { _state_text = state_text,  _state_text_modified = true; }
    const string&               state_text              () const                                    { return _state_text; }

    Time                        start_time              () const                                    { return _start_time; }
    Time                        end_time                () const                                    { return _end_time; }

    void                    set_file_path               (const File_path&, const string& agent_address = "");                       // Für einen Dateiauftrag (file order)
    File_path                   file_path               () const;
    bool                        is_file_order           () const;
    bool                        is_agent_file_order     () const;
    string                      file_agent_address      () const;
    void on_blacklisted_file_removed();
    
    void                    set_payload                 ( const VARIANT& );
    const Payload&              payload                 ()                                          { return _payload; }
    string                      string_payload          () const;
    string                      remote_scheduler        ()                                          { return params()->get_string("scheduler.remote_scheduler"); }
    void                    set_params                  ( const xml::Element_ptr&, Variable_set_map* = NULL );
    ptr<Com_variable_set>       params_or_null          () const;
    ptr<Com_variable_set>       params                  ();
    void                    set_param                   ( const string& name, const Variant& value );
    Variant                     param                   ( const string& name ) const;

    void                    set_payload_xml_string      ( const string& xml );
    void                    set_payload_xml             ( const xml::Element_ptr& );
    string                      xml_payload_string      () const                                    { return _payload_xml_string; }
    void                    set_web_service             ( const string&, bool force = false );
    void                    set_web_service             ( Web_service*, bool force = false );
    Web_service*                web_service             () const;
    Web_service*                web_service_or_null     () const                                    { return _web_service; }
    void                    set_http_operation          ( http::Operation* op )                     { _http_operation = op; }
    Web_service_operation*      web_service_operation        () const;
    Web_service_operation*      web_service_operation_or_null() const                               { return _http_operation? _http_operation->web_service_operation_or_null() : NULL; }

    int task_id() const;
    Task*                       task                    () const                                    { return _task; }


    Com_job*                    com_job                 ();

    bool                        suspended               () const                                    { return _suspended; }
    void                    set_suspended               ( bool b = true );

    void                    set_ignore_max_orders       (bool b)                                    { _ignore_max_orders = b; }
    bool                     is_ignore_max_orders       ()                                          { return _ignore_max_orders; }

    void                        set_on_blacklist        ();
    void                        remove_from_blacklist   ();
    bool                        is_on_blacklist         () const                                    { return _is_on_blacklist; }
    jlong                       java_fast_flags         () const;

    void                        inhibit_distribution    ()                                          { _is_distribution_inhibited = true; }
    void                        assert_is_not_distributed( const string& debug_text );
    void                    set_distributed             ( bool = true );
    bool                     is_distributed             () const                                    { return _is_distributed; }

    void                        start_now               ();
    bool                        setback                 ();
    void                    set_setback                 ( const Time&, bool keep_setback_count = false );
    bool                        setback_called          () const                                    { return _setback_called; }
    void                        clear_setback           ( bool keep_setback_count = false );
    bool                     is_setback                 () const                                    { return _setback_count > 0; }
    int                         setback_count           ()                                          { return _setback_count; }
    void                    set_at                      ( const Time& );
    void                    set_at_after_delay          ( const Time& );
    Time                        at                      () const                                    { return _setback; }
    void                    set_replacement             ( Order* replaced_order );
    void                    set_replacement             ( bool );
    void                        activate_schedule       ();
    void                        handle_changed_schedule ();
    int64                       next_time_millis        ()                                          { return next_time().millis(); }
    Time                        next_time               ();
    void                        set_next_start_time     ();
    void                    set_task_error              ( const Xc& x )                             { _task_error = x; }
    
    const string& last_error() const {
        return _last_error;
    }

    // Auftrag in einer Jobkette:
    enum Job_chain_stack_option { jc_remove_from_job_chain_stack, jc_leave_in_job_chain_stack };
    void                        place_in_job_chain      ( Job_chain*, Job_chain_stack_option = jc_remove_from_job_chain_stack );
    void                        place_or_replace_in_job_chain( Job_chain* );
    bool                        try_place_in_job_chain  ( Job_chain*, Job_chain_stack_option = jc_remove_from_job_chain_stack, bool exists_exception = false );
    void                        set_job_chain_path      (const Absolute_path& path);
    void                        clear_job_chain_path    ();
    void                        java_remove             ()                                          { remove(rm_standard); }
    void                        remove                  ( File_based::Remove_flag );
    void                        remove_from_job_chain   ( Job_chain_stack_option = jc_remove_from_job_chain_stack, Transaction* = NULL );
    void                        remove_from_job_chain_stack();
    void                        tip_next_node_for_new_distributed_order_state();
    void                        move_to_node            ( job_chain::Node* );

    void                        postprocessing          (const Order_state_transition&, const Xc*);                    // Verarbeitung nach spooler_process()
    void                        processing_error        ();
    void                        handle_end_state        ();
    bool                        handle_end_state_of_nested_job_chain();
    void                        handle_end_state_repeat_order( const Time& );
    bool                        move_to_nested_job_chain(const Order::State& next_outer_job_chain_state, bool success = true);
    void                        move_to_nested_job_chain(Job_chain* nested_job_chain);
    Job_chain*                  nested_job_chain(const Order::State& outer_state, bool success = true);

    void                        on_carried_out          ();
    void                        prepare_for_next_roundtrip();
    void                        restore_initial_settings();
    void                        check_for_replacing_or_removing_with_distributed(When_to_act = act_later);

    void                    set_dom                     ( const xml::Element_ptr&, Variable_set_map* );
    void                        set_identification_attributes( const xml::Element_ptr& );
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what&, const string* log );
    xml::Element_ptr            why_dom_element         (const xml::Document_ptr&, const Time& now);
    xml::Document_ptr           dom                     ( const Show_what& );
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    void                    set_schedule                ( File_based* source_file_based, const xml::Element_ptr& );
    Schedule_use*               schedule_use            ();
    void                        on_schedule_loaded      ();
    void                        on_schedule_modified    ();
    bool                        on_schedule_to_be_removed();
  //void                        on_schedule_removed     ();

    void                        persist                 ();
    void                        db_insert               ();
    bool                        db_try_insert           ( bool throw_exists_exception = false );
    void                        db_insert_order_history_record( Transaction* );
    void                        db_update_order_history_record_and_begin_new_history( Transaction* );
    void                        db_update_order_history_record( Transaction* );
    void                        db_update_order_history_state( Transaction* );
    void                        db_insert_order_step_history_record( Transaction* );
    void                        db_update_order_step_history_record( Transaction* );
    bool                        db_occupy_for_processing();
    bool                        db_release_occupation   ();
    void                        db_fill_stmt            ( sql::Write_stmt* );
    void                        close_log_and_write_history();
    string                      calculate_db_distributed_next_time();
    string                      database_runtime_xml    ();
    string                      database_xml            ();

    enum Update_option { update_anyway, update_not_occupied, update_and_release_occupation };
    bool                        db_update               ( Update_option u, Transaction* outer_transaction = NULL )              { return db_update2( u, false, outer_transaction ); }
    bool                        db_update2              ( Update_option, bool delet, Transaction* outer_transaction = NULL );
    bool                        db_delete               ( Update_option u, Transaction* outer_transaction = NULL )              { return db_update2( u, true, outer_transaction ); }
    bool                        db_handle_modified_order( Transaction* );

    string                      db_read_clob            ( Read_transaction*, const string& column_name );
    void                        db_update_clob          ( Transaction*, const string& column_name, const string& value );

    void                        db_show_occupation      ( Log_level );
    sql::Update_stmt            db_update_stmt          ();
    sql::Where_clause           db_where_clause         ();
    void                        db_fill_where_clause    ( sql::Where_clause* );
    int                         db_get_ordering         ( Transaction* ta = NULL );
    Database*                   db                      ();
    Order_subsystem_impl*       order_subsystem         () const;
    Com_log*                    com_log                 () const                                    { return _com_log; }

    void db_start_order_history();
    void on_occupied();
    void on_call(const File_exists_call&);

    int history_id() const {
        return _history_id;
    }

    int64 next_step_at_millis() const {
        return _setback.millis();
    }

    int64 setback_millis() const {
        return _setback_count ? _setback.millis() : 0;
    }

  private:
    Time                        first_start_time();
    Time                        next_start_time();
    void                        postprocessing2(Job* last_job);
    xml::Element_ptr            append_calendar_dom_element_for_setback(  const xml::Element_ptr& element, Show_calendar_options* options );
    void                        set_attributes_and_remove_duplicates( const xml::Element_ptr& element, xml::Simple_node_ptr node, xml::Element_ptr setback_element );
    bool                        order_is_removable_or_replaceable();
    
    friend struct               job_chain::Order_queue_node;
    friend struct               Order_queue;
    friend struct               Job_chain;


    Fill_zero                  _zero_;    

    string                     _file_based_job_chain_name;
    Absolute_path              _file_based_job_chain_path;
    string                     _file_based_id;

    Id                         _id;
    State                      _state;
    bool                       _its_me_removing;

    string                     _last_error;
    bool                       _is_success_state;       // Rückgabe des letzten Prozessschritts
    State                      _end_state;
    State                      _original_end_state;

    bool                       _id_locked;              // Einmal gesperrt, immer gesperrt
    string                     _state_text;
    bool                       _state_text_modified;
    Priority                   _priority;
    bool                       _priority_modified;
    string                     _title;
    bool                       _title_modified;
    Absolute_path              _job_chain_path;
    string                     _normalized_job_chain_path;
    Absolute_path              _outer_job_chain_path;
    State                      _outer_job_chain_state;
    Payload                    _payload;
    ptr<Com_variable_set>      _original_params;
    string                     _payload_xml_string;
    State                      _initial_state;          // Für Wiederholung mit <run_time> oder <schedule>. Bei verschachtelten Jobkette in der übergeordneten Jobkette
    ptr<Web_service>           _web_service;

    bool                       _is_touched;             // Von einer Task berührt
    bool                       _is_nested_touched;      // Falls nested job chain: Gestartet innerhalb einer inneren job chain. Sonst false
    int                        _setback_count;
    bool                       _is_on_blacklist;        // assert( _job_chain )
    bool                       _suspended;

    ptr<Order_schedule_use> const _schedule_use;
    bool                       _schedule_modified;
    Time                       _setback;                // Bis wann der Auftrag zurückgestellt ist (bei _setback_count > 0, sonst Startzeitpunkt "at")
    bool                       _order_xml_modified;     // Datenbankspalte xml neu schreiben!
    bool                       _is_replacement;         // _replacement_for != NULL => _is_replacement

    Time                       _created;
    Time                       _start_time;             // Erster Jobschritt
    Time                       _end_time;

    bool                       _is_distributed;         // == scheduler_orders.distributed_next_time is not null
    bool mutable               _is_file_order_cached;
    bool mutable               _is_file_order_cached_value;

    // Flüchtige Variablen, nicht für die Datenbank:

    Job_chain*                 _job_chain;              // Nur gesetzt, wenn !_is_distributed oder in Verarbeitung (_task). Sonst wird der Auftrag nur in der Datenbank gehalten
    job_chain::Node*           _job_chain_node;         // if( _job_chain)  Nächste Stelle, falls in einer Jobkette

    Absolute_path              _removed_from_job_chain_path; // Ehemaliges _job_chain->name(), nach remove_from_job_chain(), wenn _task != NULL
    ptr<Order>                 _replaced_by;            // Nur wenn _task != NULL: _replaced_by soll this in der Jobkette ersetzen
    Order*                     _replacement_for;        // _replacement_for == NULL  ||  _replacement_for->_replaced_by == this && _replacement_for->_task != NULL
    string                     _replaced_order_occupator;// Task::obj:name() oder cluster_member_id, zur Info

    Period                     _period;                 // Bei _schedule.set(): Aktuelle oder nächste Periode

    bool                       _initial_state_set;
    bool                       _is_in_order_queue;      // Auftrag ist in _job_chain_node->order_queue() eingehängt

    int                        _history_id;
    int                        _step_number;
    Task*                      _task;                   // Auftrag wird gerade von dieser Task in spooler_process() verarbeitet 
    bool                       _moved;                  // Nur wenn _task != NULL: true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    bool                       _setback_called;
    Xc_copy                    _task_error;

    bool                       _is_distribution_inhibited;
    bool                       _is_in_database;
    bool                       _is_db_occupied;
    State                      _occupied_state;
    bool                       _delay_storing_until_processing;  // Erst in die Datenbank schreiben, wenn die erste Task die Verarbeitung beginnt
    bool                       _end_state_reached;      // Auftrag nach spooler_process() beenden, für <file_order_sink>
  //Time                       _signaled_next_time;
    ptr<http::Operation>       _http_operation;
    ptr<Com_log>               _com_log;                // COM-Hülle für Log
    bool                       _ignore_max_orders;
    OrderJ                     _typed_java_sister;
    ptr<File_exists_call>      _file_exists_call;

    public: string _java_occupying_cluster_member_id;  // Only temporarly used for java_for_each_distributed_order
};

//-------------------------------------------------------------------------------------------------

Order::State                    normalized_state        ( const Order::State& );

//-------------------------------------------------------------------------------------Order_source

struct Order_source : Abstract_scheduler_object, Event_operation
{
                                Order_source            ( Job_chain*, Scheduler_object::Type_code );


    // Abstract_scheduler_object:
    Prefix_log*                 log                     () const;


    virtual void                close                   ()                                          = 0;
    virtual void                initialize              ();
    virtual void                activate                ()                                          = 0;
    virtual bool                request_order           ( const string& cause )                     = 0;
    virtual Order*              fetch_and_occupy_order  (const Order::State&, Task* occupying_task, const Time& now, const string& cause) = 0;
    virtual void                withdraw_order_request  ()                                          = 0;

    virtual xml::Element_ptr    dom_element             ( const xml::Document_ptr&, const Show_what& ) = 0;

  protected:
    Fill_zero                  _zero_;
    Job_chain*                 _job_chain;
    Order::State               _next_state;
    job_chain::Order_queue_node* _next_node;
};

//------------------------------------------------------------------------------------Order_sources

struct Order_sources 
{
    void                        close                   ();
    void                        initialize              ();
    void                        activate                ();
  //bool                        request_order           ( const string& cause );
    bool                        has_order_source        ()                                          { return !_order_source_list.empty(); }


    typedef list< ptr<Order_source> >  Order_source_list;
    Order_source_list          _order_source_list;
};

//-------------------------------------------------------------------------------------------------

namespace job_chain {

//----------------------------------------------------------------------------------job_chain::Node

struct Node : Com_job_chain_node,
              Abstract_scheduler_object
{
    //---------------------------------------------------------------------------------------------

#   define DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( MY_CLASS, TYPE_CODE )                              \
                                                                                                    \
        bool is_type( Type type ) const                                                             \
        {                                                                                           \
            return type == TYPE_CODE || Base_class::is_type( type );                                \
        }                                                                                           \
                                                                                                    \
        static MY_CLASS* cast( Node* node )                                                         \
        {                                                                                           \
            MY_CLASS* result = NULL;                                                                \
            if( node )                                                                              \
            {                                                                                       \
                result = try_cast( node );                                                          \
                if( !result )  assert(0), z::throw_xc( Z_FUNCTION, node? node->obj_name() : "(Node*)NULL" );  \
            }                                                                                       \
            return result;                                                                          \
        }                                                                                           \
                                                                                                    \
        static MY_CLASS* try_cast( Node* node )                                                     \
        {                                                                                           \
            if( node  &&  node->is_type( TYPE_CODE ) )                                              \
            {                                                                                       \
                assert( dynamic_cast<MY_CLASS*>( node ) );                                          \
                return static_cast<MY_CLASS*>( node );                                              \
            }                                                                                       \
            else                                                                                    \
                return NULL;                                                                        \
        }

    //---------------------------------------------------------------------------------------------

    enum Type
    {
        n_none,
        n_order_queue,          // Nur zum Casten
        n_job,
        n_job_chain,
        n_end,
        n_file_order_sink
    };

    enum State
    {
        s_none,
        s_initialized,
        s_active,
        s_closed
    };

    enum Action
    {
        act_process, 
        act_stop, 
        act_next_state
    };


    static Action               action_from_string          ( const string& );
    static string               string_from_action          ( Action );
    static string               string_from_state           ( State );
    static State                state_from_string           ( const string& );


                                Node                        ( Job_chain*, const Order::State& state, Type );

    virtual jobject             java_sister                 () = 0;
    virtual NodeJ&              typed_java_sister           () = 0;

    virtual void                close                       ();
    string                      obj_name                    () const;
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& );
    State                       state                       () const                                { return _state; }
    string                      state_name                  () const                                { return string_from_state( _state ); }


    virtual bool                initialize                  ();
    virtual void                activate                    ();
  //virtual void                replace                     ( Node* )                               {}

    const Order::State&         order_state                 () const                                { return _order_state; }
    string                      string_order_state          () const                                { return _order_state.as_string(); }
    void                    set_next_state                  ( const Order::State& );
    const Order::State&         next_state                  () const                                { return _next_state; }
    string                      string_next_state           () const                                { return _next_state.as_string(); }
    void                    set_error_state                 ( const Order::State& );
    const Order::State&         error_state                 () const                                { return _error_state; }
    string                      string_error_state          () const                                { return _error_state.as_string(); }

    Node*                       next_node                   () const                                { return _next_node; }
    Node*                       error_node                  () const                                { return _error_node; }
    Job_chain*                  job_chain                   () const                                { return _job_chain; }
    Absolute_path               job_chain_path              () const;

    Type                        type                        () const                                { return _type; }
    
    void                    set_suspending_order            ( bool b )                              { _is_suspending_order = b; }
    bool                     is_suspending_order            () const                                { return _is_suspending_order; }
    void                    set_delay                       (const Duration& d)                     { _delay = d; }
    Duration                    delay                       () const                                { return _delay; }
    Action                      action                      () const                                { return _action; }
    void                        set_action_string           (const string& o)                       { set_action(action_from_string(o)); }
    virtual bool                set_action                  (Action);
    void                        set_action_complete         (const string&);
    void                        recalculate_skipped_nodes   ();
    string                      string_action               () const                                { return string_from_action(_action); }
    int                         priority                    () const                                { return _priority; }
    bool                        is_ready_for_order_processing() const;
    virtual bool                is_type                     ( Type ) const                          { return false; }

    virtual void                wake_orders                 ()                                      {}


    xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );



    Fill_zero                  _zero_;
    vector<job_chain::Order_queue_node*> _skipped_nodes;

  protected:
    friend struct               ::sos::scheduler::order::Job_chain;

    void                        set_state                   ( State state )                         { _state = state; }

    void                        database_record_store       ();

    Order::State               _order_state;                // Bezeichnung des Zustands
    Order::State               _next_state;                 // Bezeichnung des Folgezustands
    Order::State               _error_state;                // Bezeichnung des Fehlerzustands

    Type                       _type;
    State                      _state;
    bool                       _is_suspending_order;        // <job_chain_node suspend="yes"/>
    Duration                   _delay;                      // <job_chain_node delay="..."/>  Verzögerung des Auftrags
    Action                     _action;
    int                        _node_index;
    int                        _priority;                   // Das ist die Entfernung zum letzten Knoten + 1, negativ (also -1, -2, -3, ...)

    ptr<Node>                  _next_node;                  // Folgeknoten
    ptr<Node>                  _error_node;                 // Fehlerknoten
    ptr<Job_chain>             _job_chain;

    Action                     _db_action;
};

//------------------------------------------------------------------------------job_chain::End_node

struct End_node : Node, javabridge::has_proxy<End_node>
{
    typedef Node                Base_class;
    DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( End_node, n_end )

                                End_node                    ( Job_chain* job_chain, const Order::State& state );

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<End_node>::java_sister(); }
    
    EndNodeJ& typed_java_sister() { 
        return _typed_java_sister; 
    }

    private:
    EndNodeJ _typed_java_sister;
};

//----------------------------------------------------------------------job_chain::Order_queue_node

struct Order_queue_node : Node
{
    typedef Node                Base_class;
    DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( Order_queue_node, n_order_queue )


                                Order_queue_node            ( Job_chain*, const Order::State&, Type );
    void                        close                       ();
  //void                        replace                     ( Node* old_node );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        register_order_source       (Order_source*);
    void                        unregister_order_source     (Order_source*);
    Order_queue*                order_queue                 () const                                { return _order_queue; }  // 1:1-Beziehung
    bool                    set_action                      (Action);
    virtual void                wake_orders                 ();
    bool                        request_order               (const Time& now, const string& cause);
    void                        withdraw_order_request      ();
    Order*                      fetch_and_occupy_order      (Task* occupying_task, const Time& now, const string& cause, const Process_class*);
    bool                        is_ready_for_order_processing ();
    xml::Element_ptr            why_dom_element             (const xml::Document_ptr&, const Time&) const;

  private:
    typedef list< Order_source* >  Order_source_list;
    Order_source_list             _order_source_list;                   // Muss leer sein bei ~Order_queue_node!
    ptr<Order_queue>           _order_queue;
    bool                       _order_queue_is_loaded;
};

//------------------------------------------------------------------------------job_chain::Job_node

struct Job_node : Order_queue_node,
                  Dependant,
                  javabridge::has_proxy<Job_node>
{
    typedef Order_queue_node    Base_class;
    DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( Job_node, n_job )


                                Job_node                    ( Job_chain*, const Order::State&, const Absolute_path& job_path );
                               ~Job_node                    ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Job_node>::java_sister(); }
    JobNodeJ&                   typed_java_sister           ()                                      { return _typed_java_sister; }

    void                        close                       ();
    bool                        initialize                  ();
    void                        activate                    ();

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    // Dependant
    bool                        on_requisite_loaded         ( folder::File_based* found_job );
    string                      obj_name                    () const                                { return Order_queue_node::obj_name(); }
    Prefix_log*                 log                         () const                                { return Order_queue_node::log(); }

    string                      job_path                    () const                                { return _job_path; }
    string                      normalized_job_path         () const;
    Job*                        job                         () const;
    Job*                        job_or_null                 () const;
    bool                    set_action                      (Action);
    bool                     is_on_error_setback            () const                                { return _on_error_setback; }
    bool                     is_on_error_suspend            () const                                { return _on_error_suspend; }

    void                        connect_job                 ( Job* );
    void                        disconnect_job              ();
    void wake_orders();
    string next_order_state_string(const Order_state_transition &);

  private:
    friend struct               order::Job_chain;           // add_job_node()

    Fill_zero                  _zero_;
    Absolute_path const        _job_path;
    Job*                       _job;
    bool                       _on_error_setback;
    bool                       _on_error_suspend;
    JobNodeJ                   _typed_java_sister;
};

//-----------------------------------------------------------------job_chain::Nested_job_chain_node

struct Nested_job_chain_node : Node, javabridge::has_proxy<Nested_job_chain_node>
{
    typedef Node                Base_class;
    DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( Nested_job_chain_node, n_job_chain )


                                Nested_job_chain_node       ( Job_chain*, const Order::State&, const Absolute_path& job_chain_path );
                               ~Nested_job_chain_node       ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Nested_job_chain_node>::java_sister(); }
    NestedJobChainNodeJ&        typed_java_sister           ()                                      { return _typed_java_sister; }

    void                        close                       ();
    bool                        initialize                  ();
  //void                        replace                     ( Node* old_node );

    Absolute_path               nested_job_chain_path       () const                                { return _nested_job_chain_path; }
    Job_chain*                  nested_job_chain            () const                                { return _nested_job_chain; }
    void                        on_releasing_referenced_object( const reference< Nested_job_chain_node, Job_chain >& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


  private:
    friend struct ::sos::scheduler::Job_chain;

    Absolute_path              _nested_job_chain_path; 
    reference< Nested_job_chain_node, Job_chain >  _nested_job_chain;
  //Job_chain_set              _using_job_chains_set;
    NestedJobChainNodeJ        _typed_java_sister;
};

//-----------------------------------------------------------------------------job_chain::Sink_node

struct Sink_node : Job_node, javabridge::has_proxy<Sink_node>
{
    typedef Job_node            Base_class;
    DEFINE_JOB_CHAIN_NODE_CAST_FUNCTIONS( Sink_node, n_file_order_sink )


                                Sink_node                   ( Job_chain*, const Order::State&, const Absolute_path& job_path, const string& move_to, bool remove );

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Sink_node>::java_sister(); }
    SinkNodeJ&                  typed_java_sister           ()                                      { return _typed_java_sister; }

    bool                        file_order_sink_remove      () const                                { return _file_order_sink_remove; }
    File_path                   file_order_sink_move_to     () const                                { return _file_order_sink_move_to; }

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

  private:
    bool                       _file_order_sink_remove;     // <file_order_sink remove="yes"/>
    File_path                  _file_order_sink_move_to;    // <file_order_sink move_to="..."/>
    SinkNodeJ _typed_java_sister;
};

//-------------------------------------------------------------------------------------------------

} //namespace job_chain

//----------------------------------------------------------------------------------------Job_chain

struct Job_chain : Com_job_chain, 
                   file_based< Job_chain, Job_chain_folder_interface, Order_subsystem >,
                   is_referenced_by<job_chain::Nested_job_chain_node,Job_chain>,
                   javabridge::has_proxy<Job_chain>
{
    enum State      // Kann wegfallen, denn file_based_state() hat dieselbe Funktion
    {
        jc_under_construction,   // add_node() gesperrt, add_order() frei
        jc_initialized,          
        jc_loaded,               // Aus Datenbank geladen
        jc_running,
        jc_stopped,
        jc_closed                
    };

    //---------------------------------------------------------------------------------------------

    Z_GNU_ONLY(                 Job_chain                   ();  )                                  // Für gcc 3.2. Nicht implementiert
                                Job_chain                   ( Scheduler* );
                               ~Job_chain                   ();

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Com_job_chain::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Com_job_chain::Release(); }

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Job_chain>::java_sister(); }
    
    const JobChainJ& typed_java_sister() const { 
        return _typed_java_sister; 
    }

    // Abstract_scheduler_object:
    string                      obj_name                    () const                                { return "Job_chain " + path(); }
    void                        close                       ();



    // file_based<>

    bool                        on_initialize               ();
    bool                        on_load                     ();
    bool                        on_activate                 ();

    void                        assert_is_not_distributed   ( const string&) const;

    File_based*                 new_base_file               ( const Base_file_info& );

    void                        on_prepare_to_remove        ();
    bool                        can_be_removed_now          ();
    bool                        on_remove_now               ();
    zschimmer::Xc               remove_error                ();

    void                        prepare_to_replace          ();
    bool                        can_be_replaced_now         ();
    Job_chain*                  on_replace_now              ();


    Job_chain_folder_interface* job_chain_folder            () const                                { return typed_folder(); }

    void                        wake_orders                 ();
    void                    set_state                       ( const State& );
    State                       state                       () const                                { return _state; }
    string                      state_name                  () const;
    
    bool is_loaded_or_active() const { 
        return _state == jc_loaded || is_active();
    }

    bool is_active() const { 
        return _state == jc_running || _state == jc_stopped; 
    }

    void                    set_title                       ( const string& title )                 { _title = title; }
    string                      title                       () const                                { return _title; }

    void                    set_visible                     ()                                      { if( _visible == visible_no )  _visible = visible_yes; }
    void                    set_visible                     ( Visibility v )                        { _visible = v; }
    bool                     is_visible                     () const                                { return _visible == visible_yes; }

    bool                     is_distributed                 () const                                { return _is_distributed; }
    string                      db_distributed_member_id    () const;

    void                    set_orders_are_recoverable      ( bool b )                              { _orders_are_recoverable = b; }
    bool                        orders_are_recoverable      () const                                { return _orders_are_recoverable; }


    list<Requisite_path>        missing_requisites          ();
    void                        fill_holes                  ();
  //bool                        initialize_nested_job_chains();
    bool                        check_nested_job_chains     ();
    void                        check_job_chain_node        ( job_chain::Node* );
    void                        complete_nested_job_chains  ();
    bool contains_nested_job_chains() const;


    job_chain::Node*            add_job_node                ( const Path& job_path, const Order::State& input_state, 
                                                              const Order::State& next_state, 
                                                              const Order::State& error_state,
                                                              const xml::Element_ptr& = xml::Element_ptr() );
    
    job_chain::Node*            add_end_node                ( const Order::State& input_state );


    job_chain::Node*            first_node                  () const;
    job_chain::Node*            referenced_node_from_state  ( const Order::State& );
    job_chain::Node*            node_from_state             ( const Order::State& ) const;
    job_chain::Node*            node_from_state_or_null     ( const Order::State& ) const;
    job_chain::Job_node*        node_from_job               ( Job* );
    vector<javabridge::Has_proxy*> java_nodes();
    vector<javabridge::Has_proxy*> java_orders();

    int                         remove_all_pending_orders   ( bool leave_in_database = false );
    void                        add_order                   ( Order* );
    void                        remove_order                ( Order* );
    ptr<Order>                  order                       ( const Order::Id& );
    ptr<Order>                  order_or_null               ( const Order::Id& );
    bool                        has_order_id                ( Read_transaction*, const Order::Id& );
    int                         order_count                 ( Read_transaction* ) const;
    bool                        has_order                   () const;
    bool                        has_order_in_task           () const;
    void                        register_order              ( Order* );                                 // Um doppelte Auftragskennungen zu entdecken: Fehler SCHEDULER-186
    void                        unregister_order            ( Order* );

    bool                        tip_for_new_distributed_order( const Order::State& state, const Time& at );
    void                        tip_for_new_order      (const Order::State&);

    void                        add_order_to_blacklist      ( Order* );
    void                        remove_order_from_blacklist ( Order* );
    Order*                      blacklisted_order_or_null   ( const string& order_id );
    stdext::hash_set<string>    db_get_blacklisted_order_id_set( const File_path& directory, const Regex& );

    void                        database_read_record        (const Record&);
    void                        database_read_node_record   (job_chain::Node*, const Record&);
    void                        db_try_delete_non_distributed_order(Transaction* outer_transaction, const string& order_id);
    string                      db_where_condition          () const;

    // Für verschachtelte Jobketten, deren Auftragskennungsräume verbunden sind:
    void                        disconnect_nested_job_chains_and_rebuild_order_id_space();
    Order_id_space*             order_id_space              () const                                    { return _order_id_space; }
    string                      order_id_space_name         () const;
    void                    set_order_id_space              ( Order_id_space* );
    bool                        order_id_space_contains_order_id(const string& order_id);

    String_set                  connected_job_chains        ();
    void                    get_connected_job_chains        ( String_set* );


    void                    set_dom                         ( const xml::Element_ptr& );

    xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );

    bool                        is_visible_in_xml_folder    ( const Show_what& ) const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    Order_subsystem_impl*       order_subsystem             () const;

    void                        recalculate_skipped_nodes   ();
    vector<job_chain::Order_queue_node*> skipped_order_queue_nodes(const Order::State&) const;
    vector<Order::State>        skipped_states              (const Order::State&) const;
    int                         number_of_touched_orders    () const;
    int                         number_of_touched_orders_obeying_max_orders(bool count_touched_in_current_job_chain) const;
    bool                 number_of_touched_orders_available () const                                { return !is_distributed(); }
    Untouched_is_allowed        untouched_is_allowed        () const                                { return is_max_orders_reached()? untouched_not_allowed : untouched_allowed; }
    bool                        is_max_orders_set           () const                                { return _max_orders < INT_MAX; }
    bool                        is_max_orders_reached       () const;
    bool                        is_below_this_outer_job_chain_max_orders() const;
    int                         max_orders                  () const                                { return _max_orders; }
    bool                        is_ready_for_order_processing() const;
    xml::Element_ptr            why_dom_element             (const xml::Document_ptr&) const;
    xml::Element_ptr            WriterFilter_ptr            () const;
    bool                        is_stopped                  () const                                { return _state == jc_stopped; }

    public: const Absolute_path& default_process_class_path() const {
        return _default_process_class_path;
    }

    public: const Absolute_path& file_watching_process_class_path() const {
        return _file_watching_process_class_path;
    }

  private:
    void                        check_for_removing          ();
    void                        database_record_store       ();
    void                        database_record_remove      ();
    void                        database_record_load        ( Read_transaction* );
    int                         load_orders_from_result_set ( Transaction*, Any_file* result_set );
    Order*                      add_order_from_database_record( Transaction*, const Record& );
    xml::Element_ptr            order_xml_file_based_node_or_null(Read_transaction*, const Record&) const;


    friend struct               Order;

    Fill_zero                  _zero_;
    const JobChainJ            _typed_java_sister;
    State                      _state;
    bool                       _is_stopped;
    string                     _title;
    int                        _max_orders;
    Order_id_space*            _order_id_space;
    Visibility                 _visible;
    bool                       _orders_are_recoverable;
    bool                       _is_distributed;                 // Aufträge können vom verteilten Scheduler ausgeführt werden

    bool                       _db_stopped;
    Absolute_path _default_process_class_path;
    Absolute_path _file_watching_process_class_path;

  public:
    typedef stdext::hash_map< string, Order* >   Order_map;
    Order_map                  _order_map;

    typedef list< ptr<job_chain::Node> >  Node_list;
    Node_list                  _node_list;

    Order_sources              _order_sources;

    typedef stdext::hash_map< string, ptr<Order> >   Blacklist_map;
    Blacklist_map              _blacklist_map;
};

//------------------------------------------------------------------------Order_id_spaces_interface

struct Order_id_spaces_interface
{
    virtual                    ~Order_id_spaces_interface   ()                                             {}

    virtual bool                is_empty                    () const                                       = 0;
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
};

//--------------------------------------------------------------------------------------Order_queue
// 1:1-Beziehung mit Order_queue_node

struct Order_queue : Com_order_queue,
                     Abstract_scheduler_object,
                     javabridge::has_proxy<Order_queue>
{
    Z_GNU_ONLY(                 Order_queue                 ();  )                                  // Für gcc 3.2. Nicht implementiert
                                Order_queue                 ( job_chain::Order_queue_node* );
                               ~Order_queue                 ();

    // Abstract_scheduler_object

    void                        close                       ();
    string                      obj_name                    () const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            why_dom_element             (const xml::Document_ptr&, const Time& now);
      
    job_chain::Order_queue_node* order_queue_node           () const                                { return _order_queue_node; }
    Job_chain*                  job_chain                   () const                                { return _job_chain; }

    void                        add_order                   ( Order*, Do_log = do_log );
    void                        remove_order                ( Order*, Do_log = do_log );
    void                        reinsert_order              ( Order* );
    int                         order_count                 ( Read_transaction* ) const;
    int                         java_order_count            () const { return order_count((Read_transaction*)NULL); }  // Provisorisch, solange Java Read_transaction nicht kennt
    int                         touched_order_count         ();
    int                         number_of_touched_orders_obeying_max_orders(bool count_touched_in_current_job_chain) const;
    bool                        empty                       ()                                      { return _queue.empty(); }
    Order*                      first_processable_order     () const;
    Order*                      first_immediately_processable_order(Untouched_is_allowed, const Time& now ) const;
    bool                        is_below_outer_chain_max_orders(const Absolute_path& outer_job_chain_path) const;
    Order*                      fetch_order                 ( const Time& now );
    Order*                      load_and_occupy_next_distributed_order_from_database(Task* occupying_task, Untouched_is_allowed, const Time& now);
    bool                        has_immediately_processable_order( const Time& now = Time(0) );
    bool                        request_order               ( const Time& now, const string& cause );
    void                        withdraw_order_request      ();
    void                        withdraw_distributed_order_request();
    Order*                      fetch_and_occupy_order      ( Task* occupying_task, Untouched_is_allowed, const Time& now, const string& cause );
    void unoccupy_order(Order*);
    Time                        next_time                   ();
    bool                        is_distributed_order_requested( time_t now )                        { return _next_distributed_order_check_time <= now; }
    time_t                      next_distributed_order_check_time() const                           { return _next_distributed_order_check_time; }
    void                        calculate_next_distributed_order_check_time( time_t now );
    void                    set_next_announced_distributed_order_time( const Time&, bool is_now );
    Time                        next_announced_distributed_order_time();
    void                        tip_for_new_distributed_order();
    bool                        is_in_any_distributed_job_chain();
    string                      db_where_expression         () const;

    void                        on_node_replaced            ( job_chain::Order_queue_node* n )      { _order_queue_node = n;  _job_chain = n->job_chain(); }


    Fill_zero                  _zero_;

    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;

    bool                       _is_loaded;

  private:
    void check_orders_for_replacing_or_removing(File_based::When_to_act);

    ptr<Com_order_queue>       _com_order_queue;
    Job_chain*                 _job_chain;
    job_chain::Order_queue_node* _order_queue_node;         // 1:1-Beziehung, _order_queue_node->order_queue() == this
    
    time_t                     _next_distributed_order_check_time;
    int                        _next_distributed_order_check_delay;
    Time                       _next_announced_distributed_order_time;  // Gültig, wenn _is_distributed_order_requested
    bool                       _has_tip_for_new_order;
};

//-----------------------------------------------------------------------Job_chain_folder_interface

struct Job_chain_folder_interface : typed_folder< Job_chain >
{
                                Job_chain_folder_interface  ( Folder* );


    Job_chain*                  job_chain                   ( const string& name ) const            { return file_based( name ); }
    Job_chain*                  job_chain_or_null           ( const string& name ) const            { return file_based_or_null( name ); }

  //virtual void                set_dom                     ( const xml::Element_ptr& )             = 0;
    virtual void                add_job_chain               ( Job_chain* )                          = 0;
    virtual void                remove_job_chain            ( Job_chain* )                          = 0;
  //virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "job_chains" ); }
};

//----------------------------------------------------------------------------------Order_subsystem

struct Order_subsystem: Object, 
                        file_based_subsystem< Job_chain >,
                        javabridge::has_proxy<Order_subsystem>
{
    enum Load_order_flags
    {
        lo_none,
        lo_lock = 0x01,
        lo_blacklisted = 0x02,
        lo_blacklisted_lock = lo_blacklisted | lo_lock,
        lo_allow_occupied = 0x04
    };

                                Order_subsystem             ( Scheduler* );

    virtual OrderSubsystemJ&    typed_java_sister           ()                                      = 0;

    virtual ptr<Job_chain_folder_interface> new_job_chain_folder( Folder* )                         = 0;
    virtual void                check_exception             ()                                      = 0;
    virtual bool                orders_are_distributed      ()                                      = 0;

    virtual void                reread_distributed_job_chain_from_database(Job_chain* which = NULL) = 0;
    virtual void                reread_distributed_job_chain_nodes_from_database(Job_chain* which_job_chain = NULL, job_chain::Node* which_node = NULL) = 0;

    virtual void                request_order               ()                                      = 0;
    virtual ptr<Order>          load_distributed_order_from_database(Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags = lo_none, string* occupying_cluster_member_id = NULL) = 0;
    virtual ptr<Order>      try_load_distributed_order_from_database(Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags = lo_none, string* occupying_cluster_member_id = NULL) = 0;
    virtual string              order_db_where_condition    ( const Absolute_path& job_chain_path, const string& order_id ) = 0;

    virtual bool                has_any_order               ()                                      = 0;
    virtual int                 order_count                 ( Read_transaction* )  const            = 0;

    virtual Job_chain*          job_chain                   ( const Absolute_path& )                = 0;
    virtual Job_chain*          job_chain_or_null           ( const Absolute_path& ) const          = 0;
    virtual void                append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* ) = 0;

    virtual int                 finished_orders_count       () const                                = 0;
    virtual Order_id_spaces_interface* order_id_spaces_interface()                                  = 0;
    virtual Order_id_spaces*    order_id_spaces             ()                                      = 0;
    virtual const Bstr& scheduler_file_order_path_variable_name_Bstr() const = 0;

    virtual void java_for_each_distributed_order(const ArrayListJ& job_chain_paths, const ArrayListJ& order_ids_j, int per_order_limit, OrderCallbackJ) = 0;
};


ptr<Order_subsystem>            new_order_subsystem         ( Scheduler* );

//----------------------------------------------------------------------------Standing_order_folder

struct Standing_order_folder : typed_folder< Order >
{
                                Standing_order_folder       ( Folder* );
                               ~Standing_order_folder       ();


  //void                        set_dom                     ( const xml::Element_ptr& );
  //void                        execute_xml_standing_order  ( const xml::Element_ptr& );
  //xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "orders" ); }

    void                        add_standing_order          ( Order* standing_order )               { add_file_based( standing_order ); }
    void                        remove_standing_order       ( Order* standing_order )               { remove_file_based( standing_order ); }
    Order*                      standing_order              ( const string& name )                  { return file_based( name ); }
    Order*                      standing_order_or_null      ( const string& name )                  { return file_based_or_null( name ); }
};

//-------------------------------------------------------------------------Standing_order_subsystem

struct Standing_order_subsystem : file_based_subsystem< Order >,
                                  javabridge::has_proxy<Standing_order_subsystem>,
                                  Object
{
                                Standing_order_subsystem    ( Scheduler* );

    // Subsystem

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Standing_order_subsystem>::java_sister(); }


    // File_based_subsystem

    string                      object_type_name            () const                                { return "Standing_order"; }
    string                      filename_extension          () const                                { return ".order.xml"; }
    void                        assert_xml_element_name     ( const xml::Element_ptr& ) const;
    string                      xml_element_name            () const                                { return "order"; }
    string                      xml_elements_name           () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
    string                      normalized_name             ( const string& ) const;
    ptr<Order>                  new_file_based              (const string& source);
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "orders" ); }
    string                      name_attributes             () const                                { return "job_chain id"; }


    ptr<Standing_order_folder>  new_standing_order_folder   ( Folder* folder )                      { return Z_NEW( Standing_order_folder( folder ) ); }
    ptr<Order>                  new_order                   ()                                      { return new_file_based(""); }


    Order*                      order                       ( const Absolute_path& job_chain_path, const string& order_id ) const  { return file_based        ( make_path( job_chain_path, order_id ) ); }
    Order*                      order_or_null               ( const Absolute_path& job_chain_path, const string& order_id ) const  { return file_based_or_null( make_path( job_chain_path, order_id ) ); }

  //string                      make_name                   ( const string&        job_chain_name, const string& order_id ) const;
    Path                        make_path                   ( const Path&          job_chain_path, const string& order_id ) const;
    Absolute_path               make_path                   ( const Absolute_path& job_chain_path, const string& order_id ) const;

  //xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );
    
    bool is_activating() const {
        return _is_activating;
    }

    private:
    Fill_zero _zero_;
    bool _is_activating;
};

inline ptr<Standing_order_subsystem> new_standing_order_subsystem( Scheduler* scheduler )           { return Z_NEW( Standing_order_subsystem( scheduler ) ); }

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace scheduler
} //namespace sos

#endif
