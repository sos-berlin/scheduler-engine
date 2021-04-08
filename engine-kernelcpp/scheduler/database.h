#ifndef __SCHEDULER_DATABASE_H
#define __SCHEDULER_DATABASE_H

#include "../file/anyfile.h"

#if defined Z_HPUX_PARISC
#   define GZIP_AUTO ""   // gzip -auto liefert ZLIB_STREAM_ERROR mit gcc 3.1, jz 7.5.2003
#   define GZIP      ""
#else
#   define GZIP_AUTO "gzip -auto | "
#   define GZIP      "gzip | "
#endif

namespace sos {
namespace scheduler {
namespace database {

//--------------------------------------------------------------------------------------------const

extern const int                max_column_length;
extern const int                order_title_column_size;
extern const int order_state_text_column_size;

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
struct Read_transaction;

//-----------------------------------------------------------------------------Database_lock_syntax

enum Database_lock_syntax
{ 
    db_lock_none,
    db_lock_for_update,         // Sätze mit "select for update" sperren
    db_lock_with_updlock,       // SQL-Server: Sätze mit "select with(updlock)" sperren, für SQL Server
};

//-----------------------------------------------------------------------------------------Database

struct Database : Object, javabridge::has_proxy<Database>, Abstract_scheduler_object
{
                                Database                ( Spooler* );

    void                        close                   ();
    void                        open                    ();
    bool                        opened                  ()                                          { return _db.opened(); }
    string                      db_name                 ()                                          { return _db_name; }
    void                    set_db_name                 (const string& o)                           { _db_name = o; }
    sql::Database_descriptor*   database_descriptor     ()                                          { return &_database_descriptor; }
    string                      error                   ()                                          { return _error; }
    bool                        is_waiting              () const                                    { return _waiting; }
    int                         order_id_length_max     ()                                          { return opened()? _order_id_length_max : const_order_id_length_max; }

    void                        spooler_start           ();
    void                        spooler_stop            ();

    int                         get_task_id             (const string& debug)                       { return get_id("spooler_job_id", (Transaction*)NULL, debug); }
    int                         get_order_id            ( Transaction* ta = NULL )                  { return get_id( "spooler_order_id", ta ); }
    int                         get_order_history_id    ( Transaction* ta )                         { return get_id( "spooler_order_history_id", ta ); }
    int                         get_id                  (const string& variable_name, Transaction* = NULL, const string& debug = "");

    xml::Element_ptr            read_task               ( const xml::Document_ptr&, int task_id, const Show_what& );
    string                      read_task_log           (int task_id);

    Transaction*                transaction             ();
    Transaction*                transaction_or_null     ()                                          { return _transaction; }
    bool                        is_in_transaction       ()                                          { return _transaction != NULL; }
    int                         record_count            ()                                          { return _db.record_count(); }
    Dbms_kind                   dbms_kind               ()                                          { return _db.dbms_kind(); }
    string                      dbms_name               ()                                          { return _db.dbms_name(); }
    ptr<Com_variable_set>       properties              ();                                         // Mit "password"
    string                      transform_sql           (const string& statement);
    Database_lock_syntax        lock_syntax             ();
    void                        open_or_wait_until_opened();
    bool                        try_reopen_after_error  ( const exception&, const string& function);
    void                        check_database          ();
    void                        create_tables_when_needed();
    bool                        create_table_when_needed( Transaction*, const string& table_name, const string& fields );
    void                        create_table            (Transaction* ta, const string& tablename, const string& fields);
    void                        rename_column           ( Transaction*, const string& table_name, const string& column_name, const string& new_column_name, const string& type );

    time_t                      reopen_time             () const                                    { return _reopen_time; }
    void                        require_database        () const;
    
    bool last_scheduler_run_failed() const {
        return _last_scheduler_run_failed;
    }

    javabridge::Lightweight_jobject jdbc_connection();

    Fill_zero                  _zero_;
    
    string                     _variables_tablename;
    string                     _orders_tablename;
    string                     _order_history_tablename;
    string                     _order_step_history_tablename;
    string                     _clusters_tablename;
    string                     _tasks_tablename;
    string                     _job_history_tablename;

    sql::Database_descriptor   _database_descriptor;
    sql::Table_descriptor      _jobs_table;
    sql::Table_descriptor      _job_chains_table;
    sql::Table_descriptor      _job_chain_nodes_table;

  private:
    friend struct Spooler;
    friend struct Job_history;
    friend struct Task_history;
    friend struct Read_transaction;
    friend struct Transaction;

    void                        open2                   ();
    void                        open_history_table      ( Read_transaction* );
    bool                        add_column              ( Transaction*, const string& table_name, const string& column_name, const string add_clause );
    bool                        alter_column_allow_null ( Transaction*, const string& table_name, const string& column_name, const string& type );
    void                        handle_order_id_columns ( Transaction* );
    int                         expand_varchar_column   ( Transaction*, const string& table_name, const string& column_name, int minimum_width, int new_width );
    int                         column_width            ( Transaction*, const string& table_name, const string& column_name );
    int                         get_id_                 (const string& variable_name, Transaction*, const string& debug);
    void                        delete_order            ( Order*, Transaction* );

    string                     _db_name;
    Any_file                   _db;
    ptr<Com_variable_set>      _properties;
    string                     _error;
    Any_file                   _history_table;
    vector<Dyn_obj>            _history_update_params;
    int                        _id;
    bool                       _email_sent_after_db_error;
    int                        _error_count;
    bool                       _waiting;
    int                        _order_id_length_max;
    Transaction*               _transaction;
    time_t                     _reopen_time;
    bool                       _is_reopening_database_after_error; // Rekursionsschutz; true: database.cxx wartet nicht auf Datenbank (damit Scheduler nicht blockiert)
    bool _last_scheduler_run_failed;

  public:
    static const int            seconds_before_reopen;
};

//---------------------------------------------------------------------------------Read_transaction

struct Read_transaction
{
                                Read_transaction        ( Database* );
                               ~Read_transaction        ();

    virtual bool                is_read_only            () const                                    { return true; }
    virtual void                begin_transaction       ( Database* );

    void                        assert_is_commitable    ( const string& debug_text ) const;

    void                    set_log_sql                 ( bool b )                                  { _log_sql = b; }

    void                        set_transaction_read    ()                                          {} //{ _transaction_read = true; } 

    Record                      read_single_record      ( const string& sql, const string& debug_text ) { return read_single_record( sql, debug_text, false ); }
    Any_file                    open_result_set         ( const string& sql, const string& debug_text ) { return open_result_set( sql, debug_text, false ); }
    Any_file                    open_file               ( const string& db_prefix, const string& sql, const string& debug_text = "" ) { return open_file( db_prefix, sql, debug_text, false ); }
    string                      read_clob_or_empty      ( const string& table_name, const string& column_name, const string& where );
    string                      read_clob               ( const string& table_name, const string& column_name, const string& where );
    string                      read_clob               ( const string& table_name, const string& column_name, const string& key_name, const string& key_value );
    string                      file_as_string          ( const string& hostware_filename );

    Database*                   db                      ()                                          { assert( _db ); return _db; }
    sql::Database_descriptor*   database_descriptor     ()                                          { return db()->database_descriptor(); }


  protected:
    Record                      read_single_record      ( const string& sql, const string& debug_text, bool need_commit_or_rollback );
    Any_file                    open_result_set         ( const string& sql, const string& debug_text, bool writes_transaction );
    Any_file                    open_file               ( const string& db_prefix, const string& sql, const string& debug_text, bool need_commit_or_rollback );
    Any_file                    open_file_2             ( const string& db_prefix, const string& execution_sql, const string& debug_text, bool need_commit_or_rollback, const string& logging_sql );


    Fill_zero                  _zero_;
    Database*                  _db;
    bool                       _log_sql;
    Prefix_log*                _log;
    Spooler*                   _spooler;
};

//--------------------------------------------------------------------------------------Transaction

struct Transaction : Read_transaction
{
    enum Execute_flags { ex_none, ex_force, ex_native };


                                Transaction             ( Database* );
                                Transaction             ( Database*, Transaction* outer_transaction );
             
                               ~Transaction             ();

    virtual bool                is_read_only            () const                                    { return false; }
    virtual void                begin_transaction       ( Database* );

    Record                      read_commitable_single_record( const string& sql, const string& debug_text )                        { return read_single_record( sql, debug_text, true ); }
    Any_file                    open_commitable_result_set( const string& sql, const string& debug_text )                           { return open_result_set( sql, debug_text, true ); }
    Any_file                    open_commitable_file      ( const string& db_prefix, const string& sql, const string& debug_text )  { return open_file( db_prefix, sql, debug_text, true ); }

    void                        commit                  ( const string& debug_text );
    void                        intermediate_commit     ( const string& debug_text );
    void                        rollback                ( const string& debug_text, Execute_flags = ex_none );
    void                        force_rollback          ( const string& debug_text )                { rollback( debug_text, ex_force ); }
    void                        set_transaction_written ()                                          {}
    void                        suppress_heart_beat_timeout_check()                                 { _suppress_heart_beat_timeout_check = true; }
    bool                        is_transaction_used     ()                                          { return db()->_db.is_transaction_used(); }
    bool                        need_commit_or_rollback ()                                          { return db()->_db.need_commit_or_rollback(); }
    Transaction*                outer_transaction       ()                                          { return _outer_transaction; }

    void                        execute                 ( const string& sql, const string& debug_text, Execute_flags = ex_none );
    void                        execute_single          ( const string& sql, const string& debug_text );
    bool                        try_execute_single      ( const string& sql, const string& debug_text );
    void                        store                   ( sql::Update_stmt&, const string& debug_text );
    string                      get_variable_text       ( const string& name, bool* record_exists = NULL );
    void                        set_variable            ( const string& name, const string& value );
    void                        insert_variable         ( const string& name, const string& value );
    void                        update_variable         ( const string& name, const string& value );
    bool                        try_update_variable     ( const string& name, const string& value );
    void                        update_clob             ( const string& table_name, const string& column_name, const string& value, const string& where );
    void                        update_clob             ( const string& table_name, const string& column_name, const string& key_name, int           key_value, const string& value );
    void                        update_clob             ( const string& table_name, const string& column_name, const string& key_name, const string& key_value, const string& value );

    int                         record_count            ()                                          { return _db->record_count(); }

    bool                        create_index            ( const string& table_name, const string& index_name, const string& short_index_name, const string& column_list, 
                                                          const string& debug_text );



    Fill_zero                  _zero_;
    Transaction* const         _outer_transaction;
    bool                       _suppress_heart_beat_timeout_check;
};

//-----------------------------------------------------------------------------------Database_retry
// for( Database_retry db_retry ( _spooler->_db ); retry_db; retry_db++ )
// try
// {
//      Transaction ta;
//      ...
//      ta.commit();
// } 
// catch( exception& x )
// {
//      db_retry.reopen_after_error( x );
// }

struct Database_retry
{
                                Database_retry          ( Database* db )                            : _db(db), _enter_loop(1), _immediately_reopened_count(0) {}

                                operator bool           ()                                          { return enter_loop(); }
    bool                        enter_loop              ()                                          { return _enter_loop > 0; }
    void                        repeat_loop             ()                                          { _enter_loop = 2; }
    void                        reopen_database_after_error( const exception&, const string& function );
    void                        operator ++             (int)                                       { _enter_loop--; }

    Database*                  _db;
    int                        _enter_loop;
    int                        _immediately_reopened_count;
};

//-------------------------------------------------------------------------Retry_nested_transaction
// for (Retry_nested_transaction ta(_spooler->_db, outer_transaction); ta.enter_loop(); ta++) try {
//      ...
//      ta.commit();
// } 
// catch (exception& x) { ta.reopen_database_after_error(x, Z_FUNCTION); }

struct Retry_nested_transaction : Transaction
{
                                Retry_nested_transaction(Database*, Transaction*);
  

    bool                        enter_loop              ()                                          { return _database_retry.enter_loop(); }
    void                        reopen_database_after_error( const exception&, const string& function );
    void                        operator ++             (int);

    Database_retry             _database_retry;
};

//--------------------------------------------------------------------------------Retry_transaction

struct Retry_transaction : Retry_nested_transaction
{
                                Retry_transaction       ( Database* db )                            : Retry_nested_transaction( db, (Transaction*)NULL ) {}

    void                        intermediate_rollback   ( const string& debug_text );
};

//--------------------------------------------------------------------------------------Job_history

struct Job_history
{
                                Job_history             ( Job* );
                               ~Job_history             ();

    void                        read_profile_settings   ();
    void                        set_dom_settings        ( xml::Element_ptr settings_element );
    void                        open                    (  Transaction* );
    int                         min_steps               ()                                          { return _history_yes? _on_process : INT_MAX; }

    xml::Element_ptr            read_tail               ( const xml::Document_ptr&, int id, int next, const Show_what&, bool use_task_schema = false );

  private:
    friend struct               Task_history;

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job*                       _job;
    Task*                      _last_task;              // Wem gehört der zuletzt geschriebene Satz?
    Absolute_path              _job_path;
    bool                       _history_yes;
    int                        _on_process;             // Beim soundsovieltem _on_process Historiensatz schreiben
    With_log_switch            _with_log;
    bool                       _history_enabled;
    bool                       _start_called;

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
    void                        write                   ( bool start );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job_history*               _job_history;
    Task*                      _task;
    bool                       _start_called;

    int64                      _record_pos;             // Position des Satzes, der zu Beginn des Jobs geschrieben und am Ende überschrieben oder gelöscht wird.
    Record                     _extra_record;
    stdext::hash_map<string, string> _extra_clobs;
    int                        _task_id;
};

//-------------------------------------------------------------------------------------------------

} //namespace database
} //namespace scheduler
} //namespace sos

#endif
