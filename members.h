// $Id$

#ifndef __SCHEDULER_MEMBERS_H
#define __SCHEDULER_MEMBERS_H


namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Scheduler_member

struct Scheduler_member : Async_operation, Scheduler_object
{
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


    void                    set_backup                      ( bool b = true )                       { assert( !_current_operation );  _is_backup = b; }

    void                    set_member_id                   ( const string& );
    string                      member_id                   ()                                      { return _scheduler_member_id; }

    bool                     is_active                      ()                                      { return _is_active; }
    bool                     is_scheduler_terminated        ();
    bool                     is_backup                      ()                                      { return _is_backup; }
    time_t                      last_heart_beat             ()                                      { return _last_heart_beat; }

    void                        start                       ();
    bool                        do_heart_beat               ();
    bool                        do_heart_beat               ( Transaction* );

    string                      active_member_variable_name();
    Spooler_db*                 db                          ();
    void                        check_member_id             ();

  protected:
    void                        create_table_when_needed    ();
    void                        delete_member_record        ( Transaction* ta );
    void                        delete_scheduler_id_record  ( Transaction* ta );
    void                        delete_old_member_records   ( Transaction* ta );
    void                        start_operation             ();
    void                        close_operation             ();

    bool                        insert_scheduler_id_record  ( Transaction* );
    bool                        try_to_heartbeat_member_record( Transaction* );
    bool                        insert_member_record        ( Transaction* );
    void                        make_scheduler_member_id    ();
    void                        db_execute                  ( const string& stmt );

  private:
    friend struct               Active_scheduler_heart_beat;
    friend struct               Inactive_scheduler_watchdog;

    Fill_zero                  _zero_;
    string                     _scheduler_member_id;
    bool                       _is_active;
    bool                       _is_backup;
    time_t                     _last_heart_beat;
    time_t                     _next_heart_beat;
    Async_operation*           _current_operation;
    ptr<Active_scheduler_heart_beat> _active_scheduler_heart_beat;
    ptr<Inactive_scheduler_watchdog> _inactive_scheduler_watchdog;
    ptr<Prefix_log>            _log;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
