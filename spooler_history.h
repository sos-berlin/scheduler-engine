// $Id: spooler_history.h,v 1.7 2002/04/10 20:34:14 jz Exp $

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

//---------------------------------------------------------------------------------------Spooler_db

struct Spooler_db
{
                                Spooler_db              ( Spooler* );

    void                        open                    ( const string& db_name );
    void                        close                   ();
    void                        open_history_table      ();
    bool                        opened                  ()                                          { return _db.opened(); }
    int                         get_id                  ();
    void                        execute                 ( const string& stmt );
    void                        commit                  ();
    void                        rollback                ();
    void                        create_table_when_needed( const string& tablename, const string& fields );
    string                      db_name                 ()                                          { return _db_name; }

    void                        spooler_start           ();
    void                        spooler_stop            ();


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
    int                        _id;
};

//--------------------------------------------------------------------------------------Transaction

struct Transaction
{
                                Transaction             ( Spooler_db* );
                               ~Transaction             ();

    void                        commit                  ();
    void                        rollback                ();

    Spooler_db*                _db;
    Thread_semaphore::Guard    _guard;
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
    void                        set_extra_field         ( const string& name, const CComVariant& value );

    xml::Element_ptr            read_tail               ( xml::Document_ptr, int id, int next, bool with_log );

  private:
    void                        archive                 ( Archive_switch, const string& filename );
    void                        append_tabbed           ( int i )                                   { append_tabbed( as_string(i) ); }
    void                        append_tabbed           ( string );
    void                        write                   ( bool start );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Job*                       _job;
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
  //vector<CComVariant>        _extra_values;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
