// $Id: spooler_history.h,v 1.1 2002/04/05 13:21:47 jz Exp $

#ifndef __SPOOLER_HISTORY_H
#define __SPOOLER_HISTORY_H

#include "../file/anyfile.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------------Spooler_db

struct Spooler_db
{
                                Spooler_db              ( Spooler* );

    void                        open                    ( const string& db_name );
    void                        close                   ();
    void                        open_history_table      ();
    bool                        opened                  ()                                          { return _db.opened(); }
    int                         get_id                  ();
    void                        execute                 ( const string& stmt )                      { _db.put( stmt ); }
    void                        commit                  ();
    void                        rollback                ();
    void                        create_table_when_needed( const string& tablename, const string& fields );
    string                      dbname                  ()                                          { return _db_name; }


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;

  private:
    friend struct Spooler;
    friend struct Job_history;

    string                     _db_name;
    Any_file                   _db;
    Any_file                   _job_id_update;
    Any_file                   _job_id_select;
    int                        _next_free_job_id;
    Any_file                   _history_table;
    Any_file                   _history_update;
    vector<Dyn_obj>            _history_update_params;
};

//--------------------------------------------------------------------------------------Transaction

struct Transaction
{
                                Transaction             ( Spooler_db* db ) : _db(db), _guard(&db->_lock), _ok(false)  {}
                               ~Transaction             ();

    void                        commit                  ();
    void                        rollback                ();

    Thread_semaphore::Guard    _guard;
    Spooler_db*                _db;
    bool                       _ok;
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
    void                        append_tabbed           ( int i )                                   { append_tabbed( as_string(i) ); }
    void                        append_tabbed           ( string );
    void                        write                   ( bool start );

    Any_file                    read_last               ( int );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job*                       _job;
    string                     _filename;
    bool                       _with_log;
    int                        _on_process;
    bool                       _use_db;
    bool                       _use_file;
    zschimmer::File            _file;
    int64                      _record_pos;             // Position des Satzes, der zu Beginn des Jobs geschrieben und am Ende überschrieben oder gelöscht wird.
    string                     _tabbed_record;
    vector<string>             _extra_columns_names;
    map<string,string>         _extra_fields;
    bool                       _error;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
