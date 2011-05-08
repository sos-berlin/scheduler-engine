
namespace sos {
namespace scheduler {
namespace order {

//--------------------------------------------------------------------------------------------const

extern const string now_database_distributed_next_time;
extern const string never_database_distributed_next_time;
extern const string blacklist_database_distributed_next_time;
extern const string replacement_database_distributed_next_time;

//----------------------------------------------------------------------------------------functions

string                          db_text_distributed_next_time();
void                            split_standing_order_name   ( const string& name, string* job_chain_name, string* order_id );

//-----------------------------------------------------------------------------------Order_id_space

struct Order_id_space : Object, Scheduler_object
{
                                Order_id_space              ( Order_subsystem_impl* );

    void                        close                       ();
  //void                        connect_job_chain           ( Job_chain* );
  //void                        disconnect_job_chain        ( Job_chain* );
    int                         number_of_job_chains        () const                                { return _job_chain_set.size(); }
    void                        check_for_unique_order_ids_of( Job_chain* ) const;
    Job_chain*                  job_chain_by_order_id_or_null( const string& order_id ) const;
    ptr<Order>                  order_or_null               ( const string& order_id ) const;
    bool                        has_order_id                ( const string& order_id ) const        { return job_chain_by_order_id_or_null( order_id ) != NULL; }
  //void                        complete_and_add            ( Job_chain* causing_job_chain );
    int                         index                       () const                                { return _index; }
    string                      name                        () const;
    string                      path                        () const                                { return name(); }
    int                         size                        () const                                { return _job_chain_set.size(); }
    bool                        is_empty                    () const                                { return _job_chain_set.empty(); }
    void                        on_order_id_space_added     ( Job_chain* causing_job_chain );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    string                      obj_name                    () const;

  private:
    friend struct               Order_id_spaces;

    void                        add_job_chain               ( Job_chain*, bool check_duplicate_order_ids = true );
    void                        remove_job_chain            ( Job_chain* );
    bool                        job_chain_is_included       ( Job_chain* job_chain ) const          { return _job_chain_set.find( job_chain->normalized_path() ) != _job_chain_set.end(); }


    Fill_zero                  _zero_;
    int                        _index;                      // this == Order_id_spaces::_array[ index ]

  public:
    String_set                 _job_chain_set;
};

//----------------------------------------------------------------------------------Order_id_spaces

struct Order_id_spaces : Order_id_spaces_interface
{
                                Order_id_spaces             ( Order_subsystem_impl* );
                               ~Order_id_spaces             ()                                     {}

    Spooler*                    spooler                     ();

    void                        add_order_id_space          ( Order_id_space*, Job_chain* causing_job_chain, int index = 0 );
    void                        remove_order_id_space       ( Order_id_space*, Job_chain* causing_job_chain, Do_log = do_log );

    void                        rebuild_order_id_space      ( Job_chain*, Job_chain* causing_job_chain, const String_set& original_job_chain_set );
    void                        disconnect_order_id_spaces  ( Job_chain* causing_job_chain, const Job_chain_set& disconnected_job_chains );
    int                         remove_job_chain_from_order_id_space( Job_chain* );
    void                        self_check                  ();
    bool                        is_empty                    () const;
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

  private:
    void                        disconnect_order_id_space   ( Job_chain*, Job_chain* causing_job_chain, const String_set& original_job_chain_set, Job_chain_set* job_chains );

    Fill_zero                      _zero_;
    Order_subsystem_impl*          _order_subsystem;
    vector< ptr<Order_id_space> >  _array;                  // [0] unbenutzt, Lücken sind NULL
};

//-----------------------------------------------------------------------------Order_subsystem_impl

struct Order_subsystem_impl : Order_subsystem
{

                                Order_subsystem_impl        ( Spooler* );


    // Subsystem

    void                        close                       ();
    string                      name                        () const                                { return "order"; }
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();
    void                        self_check                  ()                                      { _order_id_spaces.self_check(); }


    // Order_subsystem

    void                        check_exception             ();

    ptr<Job_chain_folder_interface> new_job_chain_folder    ( Folder* );

    void                        request_order               ();
    ptr<Order>                  load_order_from_database    ( Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags );
    ptr<Order>              try_load_order_from_database    ( Transaction*, const Absolute_path& job_chain_path, const Order::Id&, Load_order_flags );

    bool                        has_any_order               ();
    int                         order_count                 ( Read_transaction* ) const;
    string                      distributed_job_chains_db_where_condition() const;
    int                         processing_order_count      ( Read_transaction* ta ) const;
    xml::Element_ptr            state_statistic_element     (const xml::Document_ptr& dom_document,  const string& attribute_name, const string& attribute_value, int count) const;

    Job_chain*                  active_job_chain            ( const Absolute_path& path )           { return active_file_based( path ); }
    Job_chain*                  job_chain                   ( const Absolute_path& path )           { return file_based( path ); }
    Job_chain*                  job_chain_or_null           ( const Absolute_path& path )           { return file_based_or_null( path ); }
    void                        append_calendar_dom_elements( const xml::Element_ptr&, Show_calendar_options* );

    int                         finished_orders_count       () const                                { return _finished_orders_count; }
    Order_id_spaces_interface*  order_id_spaces_interface   ()                                      { return &_order_id_spaces; }
    Order_id_spaces*            order_id_spaces             ()                                      { return &_order_id_spaces; }
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr& dom_document, const Show_what& show_what ) const;

    // File_based_subsystem

    string                      object_type_name            () const                                { return "Job_chain"; }
    string                      filename_extension          () const                                { return ".job_chain.xml"; }
    string                      xml_element_name            () const                                { return "job_chain"; }
    string                      xml_elements_name           () const                                { return "job_chains"; }
    string                      normalized_name             ( const string& name ) const            { return lcase( name ); }
    ptr<Job_chain_folder_interface> new_job_chain_folder_interface();
    ptr<Job_chain>              new_file_based              ();
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr&, const Show_what& );



    // Privat

    bool                        orders_are_distributed      ();
    string                      db_where_condition          () const;
    string                      job_chain_db_where_condition( const Absolute_path& job_chain_path );
    string                      order_db_where_condition    ( const Absolute_path& job_chain_path, const string& order_id );
    void                        count_started_orders        ();
    void                        count_finished_orders       ();


    Fill_zero                  _zero_;
    Order_id_spaces            _order_id_spaces;

  private:
    ptr<Database_order_detector> _database_order_detector;
    int                        _started_orders_count;
    int                        _finished_orders_count;
};

}}} //namespaces
