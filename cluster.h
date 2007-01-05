// $Id$

#ifndef __SCHEDULER_CLUSTER_H
#define __SCHEDULER_CLUSTER_H


namespace sos {
namespace scheduler {
namespace cluster {

struct Cluster_member;

//------------------------------------------------------------------------------------------Cluster

struct Cluster : Async_operation, Scheduler_object
{
                                Cluster                     ( Spooler* );
                               ~Cluster                     ();


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    // Scheduler_object
    string                      obj_name                    () const;


    void                        close                       ();
    void                    set_continue_exclusive_operation( bool b = true )                       { _continue_exclusive_operation = b; }
  //void                        mark_begin_of_shutdown      ();

    void                        demand_exclusiveness        ( bool b = true )                       { _demand_exclusiveness = b; }
    void                    set_backup                      ( bool b = true )                       { _is_backup = b; }

    void                    set_my_member_id                ( const string& );
    string                      my_member_id                ()                                      { return _cluster_member_id; }

    void                    set_backup_precedence           ( int v )                               { _backup_precedence = v; _is_backup_precedence_set = true; } 
    int                         backup_precedence           ()                                      { return _backup_precedence; }

    void                    set_orders_distributed          ( bool b = true )                       { _are_orders_distributed = b; }

    bool                        check_is_active             ( Transaction* = NULL );
    bool                        has_exclusiveness           ()                                      { return _has_exclusiveness; }
    bool                     is_active                      ()                                      { return _is_active; }
    bool                     is_member_allowed_to_start     ();
    bool                     is_backup_member_allowed_to_start();
    bool                     is_backup                      ()                                      { return _is_backup; }
    bool                     is_exclusiveness_lost          ()                                      { return _is_exclusiveness_lost; }
  //Command                     heart_beat_command          ()                                      { return _heart_beat_command; }
    string                      exclusive_member_id         ();
    string                      empty_member_id             ();
  //time_t                      last_heart_beat             ()                                      { return _db_last_heart_beat; }

    bool                        start                       ();
    //bool                        wait_until_is_scheduler_up  ();
    //bool                        wait_until_is_active        ();
    //bool                        wait_until_has_exclusiveness();
    bool                        do_a_heart_beat             ();
    void                        show_exclusive_scheduler    ( Transaction* ta )                     { show_active_schedulers( ta, true ); }
    void                        show_active_schedulers      ( Transaction*, bool exclusive_only = false );
  //void                        set_command_for_all_inactive_schedulers_but_me( Transaction*, Command );
  //void                        set_command_for_all_active_schedulers_but_me  ( Transaction*, const string& command );
    bool                        set_command_for_all_schedulers_but_me         ( Transaction*, const string& command );
    bool                        set_command_for_scheduler   ( Transaction*, const string& command, const string& member_id );
    bool                        delete_dead_scheduler_record( const string& cluster_member_id );

    string                      http_url_of_member_id       ( const string& cluster_member_id );
    Cluster_member*             cluster_member_or_null    ( const string& cluster_member_id );

  //string                      scheduler_up_variable_name  ();
  //void                        check_member_id             ();
    bool                        check_schedulers_heart_beat ();

    xml::Document_ptr           my_member_dom_document      ();
    xml::Element_ptr            my_member_dom_element       ( const xml::Document_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

  protected:
    void                        create_table_when_needed    ();
  //void                        delete_my_member_record     ( Transaction* ta );
    void                        delete_old_member_records   ( Transaction* ta );
    void                        start_operations            ();
    void                        close_operations            ();

    void                        check_empty_member_record   ();
    void                        insert_member_record        ();

    void                        assert_database_integrity   ( const string& message_text );
    bool                        mark_as_exclusive           ();
  //void                        mark_as_active              ();
    void                        calculate_next_heart_beat   ( time_t now );
    bool                        check_my_member_record      ( Transaction*, bool force_error = false );
    bool                        check_heart_beat_is_in_time ( time_t expected_next_heart_beat );
    void                        lock_member_records         ( Transaction*, const string& member1_id, const string& member2_id );
  //bool                        check_database_integrity    ();
    bool                        heartbeat_member_record     ();
    bool                        set_command_for_all_schedulers_but_me( Transaction*, const string& command, const string& where_condition );
    void                        read_and_execute_command    ();
    void                        execute_command             ( const string& command );
    void                        make_cluster_member_id      ();
    Cluster_member*             exclusive_scheduler         ();                                     // NULL, wenn _exclusive_scheduler->is_empty_member()
    Cluster_member*             empty_member_record         ();
    void                        recommend_next_deadline_check_time( time_t );

  private:
    friend struct               Cluster_member;
    friend struct               Heart_beat_watchdog_thread;
    friend struct               Heart_beat;
    friend struct               Exclusive_scheduler_watchdog;
    friend struct               Active_schedulers_watchdog;

    Fill_zero                  _zero_;
    string                     _cluster_member_id;

    bool                       _is_active;
    bool                       _has_exclusiveness;
    bool                       _is_backup;
    int                        _backup_precedence;
    bool                       _is_backup_precedence_set;
    bool                       _demand_exclusiveness;
    bool                       _are_orders_distributed;
    bool                       _continue_exclusive_operation;

    time_t                     _next_heart_beat;
    time_t                     _db_last_heart_beat;
    time_t                     _db_next_heart_beat;
    time_t                     _late_heart_beat;
    time_t                     _recommended_next_deadline_check_time;

    bool                       _is_exclusiveness_lost;
    bool                       _is_in_error;
    bool                       _was_start_ok;
    bool                       _closed;

    ptr<Heart_beat_watchdog_thread>   _heart_beat_watchdog_thread;
    ptr<Heart_beat>                   _heart_beat;
    ptr<Exclusive_scheduler_watchdog> _exclusive_scheduler_watchdog;
    ptr<Active_schedulers_watchdog>   _active_schedulers_watchdog;

    typedef stdext::hash_map< string, ptr<Cluster_member> >    Scheduler_map;
    Scheduler_map              _scheduler_map;

    ptr<Cluster_member>        _exclusive_scheduler;        // Kein Scheduler ist exklusiv, wenn _exclusive_scheduler->is_empty_member()
    ptr<Cluster_member>        _my_scheduler;


  public:
    const static int            default_backup_precedence;
    const static int            default_non_backup_precedence;
};

//-------------------------------------------------------------------------------------------------

} //namespace cluster
} //namespace scheduler
} //namespace sos

#endif
