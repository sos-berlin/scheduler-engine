#ifndef __SOSDB_H
#define __SOSDB_H

#ifndef __SOSLIMTX_H
#   include "../kram/soslimtx.h"
#endif

#ifndef __SOSCLIEN_H
#   include "../kram/sosclien.h"
#endif

#ifndef __ABSFILE_H
#   include "../file/absfile.h"
#endif

#ifndef __SOSTIMER_H
#   include "../kram/sostimer.h"
#endif

namespace sos
{

struct Sos_database_session;
struct Sos_database_file;
struct Sos_option_iterator;


//--------------------------------------------------------------------------Sos_database_static
// Oberklasse für die statischen Daten der Datenbankdateitypen.
// Für jeden Datenbankdateityp gibt es genau ein Exemplar (z.B: Odbc_static).

struct Sos_database_static : Sos_self_deleting
{
                                Sos_database_static     ();
                               ~Sos_database_static     ();

    void                        read_profile            ();
    void                        print_uncommited_sessions( ostream* );      // Sessions mit offener Transaktion

    Sos_string                 _dbms_name;

    Sos_string                 _default_db_name;             // Für Sos_database_session
    Sos_string                 _default_user;                // Für Sos_database_session
    Sos_string                 _default_password;            // Für Sos_database_session
    int                        _default_transaction_timeout; // Für Sos_database_session

    Bool                       _debug;                  // Protokoll und vielleicht zusätzliche Prüfungen
    Bool                       _multiple_sessions;      // Für jeden Client eine eigene Session
    Bool                       _keep_sessions_open;
    Sos_string                 _first_cmds;             // Erste auszuführende Kommandos
    Sos_simple_array<Sos_database_session*> _session_array;
};

//-------------------------------------------------------------------------Sos_database_session
// Verbindung zu einer Datenbank mit eigener, unabhängiger Transaktionsverwaltung.

struct Sos_database_session : Sos_self_deleting
{
    BASE_CLASS( Sos_self_deleting )

                                Sos_database_session    ( Sos_database_static* = NULL );
                               ~Sos_database_session    ();

    void                        read_profile            ();

    void                        open                    ( Sos_database_file* );
    void                        close                   ( Close_mode mode = close_normal );
    void                        remove_file             ();
    void                        execute_direct          ( const Const_area& );
    void                        execute_direct_single   ( const Const_area& );
  //void                        check_write_transaction ( const Const_area& stmt );
    void                        commit                  ();
    void                        rollback                ();
    bool                     is_transaction_used        ()                                      { return _transaction_used; }
    bool                        need_commit_or_rollback ()                                      { return _need_commit_or_rollback; }

    string                      translate_sql           ( const string& sql_statement );
    void                        convert_stmt            ( const Const_area& stmt_par, Area* stmt );
    string                      convert_date_or_timestamp( const char**, const char* end );
    string                      convert_texttimestamp   ( const char**, const char* end );
    string                      convert_secondsdiff     ( const char**, const char* p_end );
    virtual string              translate               ( const string& word );
    virtual string              translate_limit         ( const string& stmt, int limit );
    typedef ::stdext::hash_map<string,string> Properties;
    virtual Properties          properties();

  protected:
    void                        transaction_begun       ();
    void                        transaction_ends        ();
    void                        set_transaction_timer   ();
    int                         timer_callback          ();
    std::vector<string>         parse_argumentlist      ( const char** pp, const char* p_end, int n );
    string                      parse_argument          ( const char** pp, const char* p_end );

    friend Sos_timer_callback   sosdb_timer_callback;

  public:

    void                        check_transaction       ();   // Exception, wenn Transaktion ungültig geworden ist (timeout)

    virtual Bool               _equal_session           ( Sos_database_file* );
    virtual void               _open                    ( Sos_database_file* ) = 0;
    virtual void               _close                   ( Close_mode = close_normal ) = 0;
    virtual void               _execute_direct          ( const Const_area& ) = 0;
    virtual void               _commit                  ();
    virtual void               _rollback                ();

    virtual Sos_string          name                    ()                              { return ""; } // z.B. "odcb", "oracle"
  //virtual Sos_string          translate_name          ( const Sos_string& nam )       { return nam; }

    void                       _obj_print               ( ostream* ) const;


    Fill_zero                  _zero_;
    Sos_client*                _client;
    int                        _array_index;            // für Sos_client::_session_array[]
    Sos_database_static*       _static;
    Sos_database_file*         _current_file;           // evtl. 0
    Sos_limited_text<30>       _cmd;                    // SQL-Anweisungswort bei execute_direct()
    long                       _row_count;
    Bool                       _single;                 // "-single" Genau ein Satz soll betroffen sein
    Bool                       _write_transaction_open;
    int                        _transaction_timeout;
    Sos_timer_handle           _transaction_timer;
    int                        _ref_count;
    bool                       _auto_commit;

    // Verbindungsdaten
  //jz 17.4.97 Sos_string                 _dbms_name;              // Typ, z.B. "oracle", "odbc"
    Sos_string                 _first_cmds;
    string                     _connection_id;          // Name, um getrennte Verbindungen zur selben Datenbank zu unterscheiden
    Sos_string                 _db_name;                // Datenbankname (x:orasrv, DSN SAB etc.)
    Sos_string                 _qualifier;              // Von ODBC
    Sos_string                 _user;                   // Benutzername
    Sos_string                 _password;               // Passwort
    Any_file::Open_mode        _open_mode;              // INOUT || IN

    Sos_limited_text<10>       _identifier_quote_begin; // "
    Sos_limited_text<10>       _identifier_quote_end;   // "
    Sos_limited_text<20>       _date_format;            // dd-mon-yyyy
    Sos_limited_text<30>       _date_time_format;       // dd-mon-yyyy HH:MM:SS
    string                     _concat_operator;        // ||

    Bool                       _transaction_timed_out;
    bool                       _transaction_used;
    bool                       _need_commit_or_rollback;
    Bool                       _log_stmt;               // Wird von Ingres_file false gesetzt
    bool                       _is_odbc;
    Bool                       _debug;
    string                     _dbms_name;
    Dbms_kind                  _dbms;
};


//-----------
// Oberklasse für einen Datenbankdateityp.

struct Sos_database_file : Abs_file
{
    BASE_CLASS( Abs_file )
    DEFINE_OBJ_IS_TYPE( Sos_database_file )

                                Sos_database_file       ( Sos_database_static* = NULL );
                               ~Sos_database_file       ();

  //void                        bind_parameters         ( const Record_type*, const Byte* );

    void                        session_connect         ( const char* type_name );
    void                        session_disconnect      ();
    void                        execute_direct          ( const Const_area& );

    const char*                 identifier_quote_begin  ()                                      { return c_str( _session->_identifier_quote_begin ); } 
    const char*                 identifier_quote_end    ()                                      { return c_str( _session->_identifier_quote_end ); }
    const char*                 date_format             ()                                      { return c_str( _session->_date_format ); }
    const char*                 date_time_format        ()                                      { return c_str( _session->_date_time_format ); }
    const char*                 concat_operator         ()                                      { return c_str( _session->_concat_operator ); }
                                 
    virtual void                put_record              ( const Const_area&  );


    Fill_zero                  _zero_;

    Sos_ptr<Sos_database_session> _session;             // aktuelle Verbindung
    Sos_ptr<Sos_database_static>  _static;

    // Zwischenpuffer für Verbindungsdaten:
    string                     _connection_id;          // Name, um getrennte Verbindungen zur selben Datenbank zu unterscheiden
    Sos_string                 _db_name;                // Datenbankname (x:orasrv, DSN SAB etc.)
    Sos_string                 _qualifier;              // Von ODBC
    Sos_string                 _user;                   // Benutzername
    Sos_string                 _password;               // Passwort
    int                        _transaction_timeout;
    bool                       _read_only;
    double                     _connection_wait;
    string                     _connection_wait_error_codes;
    Any_file::Open_mode        _open_mode;              // INOUT || IN
    string                     _ignore_fields;
    int                        _assert_row_count;
    bool                       _auto_commit;

    long                       _row_count;
    Bool                       _debug;

  protected:
    virtual void               _create_static           () = 0;

    virtual Sos_ptr<Sos_database_session> 
                               _create_session          () = 0;

    Bool                        database_option         ( Sos_option_iterator& opt );
    void                        get_static              ( const char* dbms_name );

  private:
    void                        get_session             ();

    Bool                       _keep_session;           // Session offenlassen
  //Sos_ptr<Record_type>       _param_type;
  //const Byte*                _param_base;
};

} //namespace sos

#endif

