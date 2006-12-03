// $Id$

#ifndef __SPOOLER_HISTORY_H
#define __SPOOLER_HISTORY_H

#include "../file/anyfile.h"



#ifdef Z_HPUX
#   define GZIP_AUTO ""   // gzip -auto liefert ZLIB_STREAM_ERROR mit gcc 3.1, jz 7.5.2003
#   define GZIP      ""
#else
#   define GZIP_AUTO "gzip -auto | "
#   define GZIP      "gzip | "
#endif



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

struct Spooler_db : Object, Scheduler_object
{
                                Spooler_db              ( Spooler* );

    void                        open                    ( const string& db_name );
    void                        close                   ();
    bool                        opened                  ()                                          { return _db.opened(); }
    string                      db_name                 ()                                          { return _db_name; }
    string                      error                   ()                                          { THREAD_LOCK_RETURN( _error_lock, string, _error ); }
    bool                        is_waiting              () const                                    { return _waiting; }
    int                         order_id_length_max     ()                                          { return opened()? _order_id_length_max : const_order_id_length_max; }

    void                        spooler_start           ();
    void                        spooler_stop            ();

    Prefix_log*                 log                     ()                                          { return _log; }
    int                         get_task_id             ()                                          { return get_id( "spooler_job_id" ); }
    int                         get_order_id            ( Transaction* ta = NULL )                  { return get_id( "spooler_order_id", ta ); }
    int                         get_order_ordering      ( Transaction* ta = NULL )                  { return get_id( "spooler_order_ordering", ta ); }
    int                         get_order_history_id    ( Transaction* ta )                         { return get_id( "spooler_order_history_id", ta ); }

    xml::Element_ptr            read_task               ( const xml::Document_ptr&, int task_id, const Show_what& );

    void                        insert_order            ( Order* );
    void                        update_order            ( Order* );
    void                        update_orders_clob      ( Order*, const string& column_name, const string& value );
    void                        update_orders_clob      ( const string& job_chain_name, const string& order_id, const string& column_name, const string& value );
    string                      read_orders_clob        ( Order*, const string& column_name );
    string                      read_orders_clob        ( const string& job_chain_name, const string& order_id, const string& column_name );

    void                        update_clob             ( const string& table_name, const string& column_name, const string& key_name, int           key_value, const string& value );
    void                        update_clob             ( const string& table_name, const string& column_name, const string& key_name, const string& key_value, const string& value );
    string                      read_clob               ( const string& table_name, const string& column_name, const string& key_name, const string& key_value );

    void                        write_order_history     ( Order*, Transaction* = NULL );
    void                        finish_order            ( Order*, Transaction* = NULL );

    void                        execute                 ( const string& stmt );
    void                        commit                  ();
    void                        rollback                ();
    void                        try_reopen_after_error  ( exception&, bool wait_endless = false );

    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Thread_semaphore           _error_lock;
    z::sql::Database_descriptor _db_descr;
  private:
    friend struct Spooler;
    friend struct Job_history;
    friend struct Task_history;
    friend struct Transaction;

    void                        open2                   ( const string& db_name );
    void                        open_history_table      ();
    void                        get_history_table_table ();
    void                        create_table_when_needed( const string& tablename, const string& fields );
    void                        add_column              ( const string& table_name, const string& column_name, const string add_clause );
    void                        handle_order_id_columns ();
    int                         expand_varchar_column   ( const string& table_name, const string& column_name, int minimum_width, int new_width );
    int                         column_width            ( const string& table_name, const string& column_name );
    int                         get_id                  ( const string& variable_name, Transaction* = NULL );
    int                         get_id_                 ( const string& variable_name, Transaction* );
    void                        delete_order            ( Order*, Transaction* );

    string                     _db_name;
    Any_file                   _db;
    string                     _error;
  //Any_file                   _job_id_update;
  //Any_file                   _job_id_select;
    map<string,long32>         _id_counters;
    Any_file                   _history_table;
  //Any_file                   _history_update;
    vector<Dyn_obj>            _history_update_params;
    int                        _id;
    bool                       _email_sent_after_db_error;
    int                        _error_count;
    bool                       _waiting;
    ptr<Prefix_log>            _log;
    int                        _order_id_length_max;
};

//--------------------------------------------------------------------------------------Transaction

struct Transaction
{
                                Transaction             ( Spooler_db*, Transaction* outer_transaction = NULL );
                               ~Transaction             ();

    void                        commit                  ();
    void                        rollback                ();
  //void                        try_reopen_after_error  ();

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
    int                         min_steps               ()                                          { return _history_yes? _on_process : INT_MAX; }

    xml::Element_ptr            read_tail               ( const xml::Document_ptr&, int id, int next, const Show_what&, bool use_task_schema = false );

  private:
    friend struct               Task_history;

    void                        archive                 ( Archive_switch, const string& filename );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job*                       _job;
    Task*                      _last_task;              // Wem gehört der zuletzt geschriebene Satz?
    string                     _job_name;
    bool                       _history_yes;
    int                        _on_process;             // Beim soundsovieltem _on_process Historiensazt schreiben
    With_log_switch            _with_log;
    bool                       _use_db;
    bool                       _use_file;
    bool                       _error;
    bool                       _start_called;

    string                     _filename;
    string                     _type_string;            // _use_file:  -type=(...) tab ...
    zschimmer::File            _file;
    string                     _tabbed_record;
    vector<string>             _extra_names;
    Sos_ptr<Record_type>       _extra_type;
};

//-------------------------------------------------------------------------------------Task_history

struct Task_history
{
                                Task_history            ( Job_history*, Task* );
                               ~Task_history            ();

    void                        start                   ();
    void                        end                     ();
    void                        set_extra_field         ( const string& name, const Variant& value );

    xml::Element_ptr            read_tail               ( const xml::Document_ptr&, int id, int next, const Show_what& );

  private:
    void                        append_tabbed           ( int i )                                   { append_tabbed( as_string(i) ); }
    void                        append_tabbed           ( string );
    void                        write                   ( bool start );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job_history*               _job_history;
    Task*                      _task;
    bool                       _start_called;

    int64                      _record_pos;             // Position des Satzes, der zu Beginn des Jobs geschrieben und am Ende überschrieben oder gelöscht wird.
    string                     _tabbed_record;
    Record                     _extra_record;
    int                        _task_id;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
