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
    void                        shutdown                    ();                                     // Entfernt auch Datenbanksätze

    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const                                { return obj_name(); }
    bool                        async_continue_             ( Continue_flags );

    // Scheduler_object
    Prefix_log*                 log                         ()                                      { return _log; }
    string                      obj_name                    () const                                { return "Scheduler_member"; } // + _scheduler_member_id; }


    void                    set_backup                      ( bool b = true )                       { assert( !_operation );  _is_backup = b; }

    void                    set_member_id                   ( const string& );
    string                      member_id                   ()                                      { return _scheduler_member_id; }

    bool                     is_active                      ()                                      { return _is_active; }
    bool                     is_scheduler_terminated        ()                                      { return _is_scheduler_terminated; }
    bool                     is_backup                      ()                                      { return _is_backup; }
    time_t                      last_heart_beat             ()                                      { return _last_heart_beat; }

    void                        start                       ();
    void                        do_heart_beat               ();
    void                        do_heart_beat               ( Transaction* );
    void                        show_active_members         ( Transaction* );
    void                        try_to_become_active        ( Transaction* = NULL );

    string                      active_member_variable_name();
    Spooler_db*                 db                          ();
    void                        check_member_id             ();

  protected:
    void                        delete_member_record        ( Transaction* ta );
    void                        delete_scheduler_id_record  ( Transaction* ta );
    void                        delete_old_member_records   ( Transaction* ta );
    void                        start_operation             ();
    void                        close_operation             ();

  //void                        write_member_record         ( Transaction* );
  //void                        be_active                   ( Transaction*, const string& previous_active_member_id );
    void                        become_active               ();
    bool                        insert_scheduler_id_record  ( Transaction* );
    void                        try_to_become_active2       ( Transaction* );
    bool                        try_to_heartbeat_member_record( Transaction* );
    void                        insert_member_record        ( Transaction* );
    void                        make_scheduler_member_id    ();

  private:
    Fill_zero                  _zero_;
    string                     _scheduler_member_id;
    bool                       _is_active;
    bool                       _is_backup;
    bool                       _is_scheduler_terminated;        // Scheduler ist ordentlich beendet worden
    time_t                     _last_heart_beat;
    time_t                     _next_heart_beat;
    string                     _last_active_member_id;
    ptr<Async_operation>       _operation;
    ptr<Prefix_log>            _log;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
