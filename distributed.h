// $Id$

#ifndef __SCHEDULER_MEMBERS_H
#define __SCHEDULER_MEMBERS_H


namespace sos {
namespace scheduler {


struct Scheduler_member;

//----------------------------------------------------------------------------Distributed_scheduler

struct Distributed_scheduler : Async_operation, Scheduler_object
{
    enum Command
    {
        cmd_none,
        cmd_terminate,
        cmd_terminate_and_restart
    };


    static string               string_from_command         ( Command );
    static Command              command_from_string         ( const string& );



                                Distributed_scheduler       ( Spooler* );
                               ~Distributed_scheduler       ();


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );


    // Scheduler_object
    string                      obj_name                    () const;


    void                        close                       ();
    void                        shutdown                    ();                                     // Ordentliches Herunterfahren des Schedulers
  //void                        mark_begin_of_shutdown      ();

    void                        demand_exclusiveness        ( bool b = true )                       { assert( !_current_operation );  _demand_exclusiveness = b; }
    void                    set_backup                      ( bool b = true )                       { assert( !_current_operation );  _is_backup = b; }

    void                    set_my_member_id                ( const string& );
    string                      my_member_id                ()                                      { return _scheduler_member_id; }

    void                    set_backup_precedence           ( int v )                               { _backup_precedence = v; } 
    int                         backup_precedence           ()                                      { return _backup_precedence; }

    void                    set_watch_distributed_order_execution( bool b = true )                  { _watch_distributed_order_execution = b; }

    bool                        check_is_active             ( Transaction* = NULL );
    bool                        has_exclusiveness           ()                                      { return _has_exclusiveness; }
    bool                     is_active                      ()                                      { return _is_active; }
    bool                     is_scheduler_up                ();
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
    void                        set_command_for_all_inactive_schedulers_but_me( Transaction*, Command );
    void                        set_command_for_all_schedulers_but_me( Transaction*, Command );

  //string                      scheduler_up_variable_name  ();
    void                        check_member_id             ();
    bool                        check_schedulers_heart_beat ();

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
    void                        set_command_for_all_schedulers_but_me( Transaction*, const string& where, Command );
    void                        read_and_execute_command    ();
    void                        execute_command             ( Command );
    void                        make_scheduler_member_id    ();
    Scheduler_member*           exclusive_scheduler         ();                                     // NULL, wenn _exclusive_scheduler->is_empty_member()
    Scheduler_member*           empty_scheduler_record      ();

  private:
    friend struct               Scheduler_member;
    friend struct               Heart_beat;
    friend struct               Exclusive_scheduler_watchdog;
    friend struct               Active_schedulers_watchdog;

    Fill_zero                  _zero_;
    string                     _scheduler_member_id;

    bool                       _is_active;
    bool                       _has_exclusiveness;
    bool                       _is_backup;
    int                        _backup_precedence;
    bool                       _demand_exclusiveness;
    bool                       _watch_distributed_order_execution;

    time_t                     _next_heart_beat;
    time_t                     _db_last_heart_beat;
    time_t                     _db_next_heart_beat;
    time_t                     _late_heart_beat;

    bool                       _is_exclusiveness_lost;
    bool                       _is_in_error;
    bool                       _was_start_ok;
    string                     _heart_beat_command_string;
    int                        _is_in_do_a_heart_beat;
    bool                       _closed;

    Async_operation*                  _current_operation;
    ptr<Heart_beat>                   _heart_beat;
    ptr<Exclusive_scheduler_watchdog> _exclusive_scheduler_watchdog;
    ptr<Active_schedulers_watchdog>   _active_schedulers_watchdog;

    typedef stdext::hash_map< string, ptr<Scheduler_member> >    Scheduler_map;
    Scheduler_map              _scheduler_map;

    ptr<Scheduler_member>      _exclusive_scheduler;        // Kein Scheduler ist exklusiv, wenn _exclusive_scheduler->is_empty_member()
    ptr<Scheduler_member>      _my_scheduler;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
