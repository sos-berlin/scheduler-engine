// $Id: spooler_history.h,v 1.17 2003/06/24 15:46:28 jz Exp $

#ifndef __SPOOLER_HISTORY_H
#define __SPOOLER_HISTORY_H

#include "../file/anyfile.h"

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------Archive_switch

enum Archive_switch
{
    arc_no = 0,
    arc_yes,
    arc_gzip
};

//----------------------------------------------------------------------------------With_log_switch

typedef Archive_switch With_log_switch;

struct Transaction;

//---------------------------------------------------------------------------------------Spooler_db

struct Spooler_db : Sos_self_deleting
{
                                Spooler_db              ( Spooler* );

    void                        open                    ( const string& db_name );
    void                        close                   ();
    void                        open_history_table      ();
    bool                        opened                  ()                                          { return _db.opened(); }
    void                        commit                  ();
    void                        rollback                ();
    void                        execute                 ( const string& stmt );
    void                        create_table_when_needed( const string& tablename, const string& fields );
    void                        try_reopen_after_error  ( const exception& );
    string                      db_name                 ()                                          { return _db_name; }

    void                        spooler_start           ();
    void                        spooler_stop            ();

    int                         get_task_id             ()                                          { return get_id( "spooler_job_id" ); }
    int                         get_id                  ( const string& variable_name, Transaction* = NULL );
    int                         get_id_                 ( const string& variable_name, Transaction* );

    int                         get_order_id            ( Transaction* ta = NULL )                  { return get_id( "spooler_order_id", ta ); }
    int                         get_order_ordering      ( Transaction* ta = NULL )                  { return get_id( "spooler_order_ordering", ta ); }
    void                        insert_order            ( Order* );
    void                        delete_order            ( Order*, Transaction* );
    void                        update_order            ( Order* );

    int                         get_order_history_id    ( Transaction* ta )                         { return get_id( "spooler_order_history_id", ta ); }
    void                        write_order_history     ( Order*, Transaction* = NULL );


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;

  private:
    friend struct Spooler;
    friend struct Job_history;

    string                     _db_name;
    Any_file                   _db;
  //Any_file                   _job_id_update;
  //Any_file                   _job_id_select;
    map<string,long>           _id_counters;
    Any_file                   _history_table;
  //Any_file                   _history_update;
    vector<Dyn_obj>            _history_update_params;
    int                        _id;
    bool                       _email_sent_after_db_error;
};

//--------------------------------------------------------------------------------------Transaction

struct Transaction
{
                                Transaction             ( Spooler_db*, Transaction* outer_transaction = NULL );
                               ~Transaction             ();

    void                        commit                  ();
    void                        rollback                ();

    Spooler_db*                _db;
    Mutex_guard                _guard;
    Transaction*               _outer_transaction;
};

//--------------------------------------------------------------------------------------Job_history

struct Job_history
{
                                Job_history             ( Job* );
                               ~Job_history             ();

    void                        open                    ();
    void                        close                   ();
    void                        start                   ();
    void                        end                     ();
    void                        set_extra_field         ( const string& name, const Variant& value );
    int                         min_steps               ()                                          { return _history_yes? _on_process : INT_MAX; }

    xml::Element_ptr            read_tail               ( const xml::Document_ptr&, int id, int next, Show_what );

  private:
    void                        archive                 ( Archive_switch, const string& filename );
    void                        append_tabbed           ( int i )                                   { append_tabbed( as_string(i) ); }
    void                        append_tabbed           ( string );
    void                        write                   ( bool start );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job*                       _job;
    string                     _job_name;
    bool                       _history_yes;
    int                        _on_process;
    With_log_switch            _with_log;
    bool                       _use_db;
    bool                       _use_file;
    bool                       _error;
    bool                       _start_called;

    string                     _filename;
    string                     _type_string;            // _use_file:  -type=(...) tab ...
    zschimmer::File            _file;
    int64                      _record_pos;             // Position des Satzes, der zu Beginn des Jobs geschrieben und am Ende überschrieben oder gelöscht wird.
    string                     _tabbed_record;
    vector<string>             _extra_names;
    Record                     _extra_record;
    int                        _task_id;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
