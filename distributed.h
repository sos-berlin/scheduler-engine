// $Id$

#ifndef __SCHEDULER_MEMBERS_H
#define __SCHEDULER_MEMBERS_H


namespace sos {
namespace scheduler {


struct Other_scheduler;

//----------------------------------------------------------------------------Distributed_scheduler

struct Distributed_scheduler : Async_operation, Scheduler_object
{
    enum Command
    {
        cmd_none,
        cmd_terminate,
        cmd_terminate_and_restart
    };

    //static const int            min_precedence;
    //static const int            max_precedence;


    static string               string_from_command         ( Command );
    static Command              command_from_string         ( const string& );



                                Distributed_scheduler       ( Spooler* );
                               ~Distributed_scheduler       ();

    void                        close                       ();
    void                        shutdown                    ();                                     // Ordentliches Herunterfahren des Schedulers

    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );

    // Scheduler_object
    string                      obj_name                    () const;


    void                        demand_exclusiveness        ( bool b = true )                       { assert( !_current_operation );  _demand_exclusiveness = b; }
    void                    set_backup                      ( bool b = true )                       { assert( !_current_operation );  _is_backup = b; }

    void                    set_member_id                   ( const string& );
    string                      member_id                   ()                                      { return _scheduler_member_id; }

    void                    set_backup_precedence           ( int v )                               { _backup_precedence = v; } 
    int                         backup_precedence           ()                                      { return _backup_precedence; }

    bool                        check_is_active             ();
    bool                        has_exclusiveness           ()                                      { return _has_exclusiveness; }
    bool                     is_active                      ()                                      { return _is_active; }
    bool                     is_exclusiveness_stolen        ()                                      { return _is_exclusiveness_stolen; }
    bool                     is_scheduler_up                ();
    bool                     is_backup                      ()                                      { return _is_backup; }
  //Command                     heart_beat_command          ()                                      { return _heart_beat_command; }
    string                      exclusive_member_id         ();
    string                      empty_member_id             ();
    time_t                      last_heart_beat             ()                                      { return _db_last_heart_beat; }

    bool                        start                       ();
    bool                        wait_until_is_scheduler_up  ();
    bool                        wait_until_is_active        ();
    bool                        wait_until_has_exclusiveness();
    bool                        do_a_heart_beat             ();
    void                        mark_as_inactive            ( bool delete_inactive_record = false, bool delete_new_active_record = false );
    void                        show_exclusive_scheduler    ( Transaction* ta )                     { show_active_schedulers( ta, true ); }
    void                        show_active_schedulers      ( Transaction*, bool exclusive_only = false );
    void                        set_command_for_all_inactive_schedulers_but_me( Transaction*, Command );
    void                        set_command_for_all_schedulers_but_me( Transaction*, Command );

  //string                      scheduler_up_variable_name  ();
    void                        check_member_id             ();
    void                        check_active_schedulers_heart_beat( Transaction* );

  protected:
    void                        create_table_when_needed    ();
    void                        delete_member_record        ( Transaction* ta );
    void                        delete_old_member_records   ( Transaction* ta );
    void                        start_operations            ();
    void                        close_operations            ();

    bool                        mark_as_exclusive           ();
  //void                        mark_as_active              ();
    void                        calculate_next_heart_beat   ( time_t now );
    bool                        check_heart_beat_is_in_time ( time_t expected_next_heart_beat );
    void                        insert_scheduler_id_record  ( Transaction* );
    void                        check_empty_member_record   ();
    void                        insert_member_record        ( Transaction*, bool set_active );
    void                        lock_member_records         ( Transaction*, const string& member1_id, const string& member2_id );
    void                        assert_database_integrity   ( const string& message_text );
  //bool                        check_database_integrity    ();
    bool                        heartbeat_member_record     ();
    void                        set_command_for_all_schedulers_but_me( Transaction*, const string& where, Command );
  //void                        set_command_for_members     ( Transaction*, const string& where, Command );
    void                        read_and_execute_command    ();
    void                        execute_command             ( Command );
    void                        db_execute                  ( const string& stmt );
    void                        make_scheduler_member_id    ();
    Other_scheduler*            exclusive_scheduler         ();     // NULL, wenn _exclusive_scheduler->is_empty_member()
    Other_scheduler*            empty_scheduler_record      ();

  private:
    friend struct               Heart_beat;
    friend struct               Exclusive_scheduler_watchdog;

    Fill_zero                  _zero_;
    string                     _scheduler_member_id;
    bool                       _is_active;
    bool                       _demand_exclusiveness;
    bool                       _has_exclusiveness;
    bool                       _is_backup;
    int                        _backup_precedence;
    time_t                     _next_heart_beat;
    time_t                     _db_last_heart_beat;
    time_t                     _db_next_heart_beat;
    time_t                     _late_heart_beat;
    bool                       _is_exclusiveness_stolen;
    bool                       _is_in_error;
  //Command                    _heart_beat_command;
    string                     _heart_beat_command_string;
    Async_operation*           _current_operation;
    ptr<Heart_beat>            _heart_beat;
    ptr<Exclusive_scheduler_watchdog> _exclusive_scheduler_watchdog;
    bool                       _closed;
    int                        _is_in_do_a_heart_beat;

    typedef stdext::hash_map< string, ptr<Other_scheduler> >    Scheduler_map;
    Scheduler_map              _active_scheduler_map;

    ptr<Other_scheduler>       _exclusive_scheduler;        // Kein Scheduler ist exklusiv, wenn _exclusive_scheduler->is_empty_member()
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
