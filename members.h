// $Id$

#ifndef __SCHEDULER_MEMBERS_H
#define __SCHEDULER_MEMBERS_H


namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Scheduler_member

struct Scheduler_member : Async_operation, Scheduler_object
{
    enum Command
    {
        cmd_none,
        cmd_terminate,
        cmd_terminate_and_restart
    };

    static const int            max_precedence;

    static string               string_from_command         ( Command );
    static Command              command_from_string         ( const string& );



                                Scheduler_member            ( Spooler* );
                               ~Scheduler_member            ();

    void                        close                       ();
    void                        shutdown                    ();                                     // Ordentliches Herunterfahren des Schedulers

    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );

    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const;


    void                        demand_exclusiveness        ( bool b = true )                       { assert( !_current_operation );  _demand_exclusiveness = b; }
    void                    set_backup                      ( bool b = true )                       { assert( !_current_operation );  _is_backup = b; }

    void                    set_member_id                   ( const string& );
    string                      member_id                   ()                                      { return _scheduler_member_id; }

    bool                     is_active                      ()                                      { return _is_active; }
    bool                        has_exclusiveness           ()                                      { return _has_exclusiveness; }
    bool                     is_exclusiveness_stolen        ()                                      { return _is_exclusiveness_stolen; }
    bool                     is_scheduler_up                ();
    bool                     is_backup                      ()                                      { return _is_backup; }
    Command                     heart_beat_command          ()                                      { return _heart_beat_command; }
    string                      exclusive_member_id         ();
    string                      empty_member_id             ();
    time_t                      last_heart_beat             ()                                      { return _db_last_heart_beat; }

    bool                        start                       ();
    bool                        wait_until_is_scheduler_up  ();
    bool                        wait_until_is_active        ();
    bool                        wait_until_has_exclusiveness();
    bool                        do_a_heart_beat             ( Transaction* );
    bool                        do_a_heart_beat             ( Transaction*, bool db_record_marked_active );
    bool                        do_exclusive_heart_beat     ( Transaction* );
    void                        mark_as_inactive            ( Transaction*, bool delete_inactive_record = false, bool delete_new_active_record = false );
    void                        show_active_schedulers      ( Transaction* );
    void                        set_command_for_all_inactive_schedulers_but_me( Transaction*, Command );
    void                        set_command_for_all_schedulers_but_me( Transaction*, Command );

  //string                      scheduler_up_variable_name  ();
    void                        check_member_id             ();

  protected:
    void                        create_table_when_needed    ();
    void                        delete_member_record        ( Transaction* ta );
    void                        delete_old_member_records   ( Transaction* ta );
    void                        start_operations            ();
    void                        close_operations            ();

    void                        calculate_next_heart_beat   ();
    void                        insert_scheduler_id_record  ( Transaction* );
    bool                        check_empty_member_record   ();
    void                        insert_member_record        ( Transaction* );
    void                        lock_member_records         ( Transaction*, const string& member1_id, const string& member2_id );
    void                        assert_database_integrity   ( const string& message_text );
    bool                        check_database_integrity    ( Transaction* );
    bool                        try_to_heartbeat_member_record( Transaction*, bool db_record_marked_active );
    void                        set_command_for_all_schedulers_but_me( Transaction*, const string& where, Command );
  //void                        set_command_for_members     ( Transaction*, const string& where, Command );
    void                        execute_command             ( Command );
    void                        db_execute                  ( const string& stmt );
    void                        make_scheduler_member_id    ();

  private:
    friend struct               Heart_beat;
    friend struct               Exclusive_scheduler_watchdog;

    Fill_zero                  _zero_;
    string                     _scheduler_member_id;
    bool                       _is_active;
    bool                       _demand_exclusiveness;
    bool                       _has_exclusiveness;
    bool                       _is_backup;
    time_t                     _next_heart_beat;
    time_t                     _db_last_heart_beat;
    time_t                     _db_next_heart_beat;
    bool                       _is_exclusiveness_stolen;
    Command                    _heart_beat_command;
    string                     _heart_beat_command_string;
    Async_operation*           _current_operation;
    ptr<Heart_beat>            _heart_beat;
    ptr<Exclusive_scheduler_watchdog> _exclusive_scheduler_watchdog;
    ptr<Prefix_log>            _log;
    bool                       _closed;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
