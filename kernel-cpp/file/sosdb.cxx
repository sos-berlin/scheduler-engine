//#define MODULE_NAME "sosdb"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "(c)1996 SOS GmbH Berlin"


#include "precomp.h"
#define __USELOCALES__      // strftime()

#include "../kram/sysdep.h"
#include <ctype.h>
#include <time.h>
#include <stack>
#include <iostream>

#if defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"
#include "../kram/sosprof.h"
#include "../kram/sosclien.h"
#include "../file/sosdb.h"
#include "../kram/tabucase.h"
#include "../kram/sosdate.h"
#include "../kram/log.h"
#include "../kram/sosdate.h"
#include "../kram/sleep.h"
#include "../zschimmer/zschimmer.h"

using std::vector;
using zschimmer::ucase_;
using zschimmer::S;
using zschimmer::string_begins_with;

namespace sos {

struct Sosdb_static : Sos_self_deleting
{
    Sos_simple_array< Sos_ptr<Sos_database_static> >  _static_array;
};

DEFINE_SOS_DELETE_ARRAY( Sos_database_session* )
DEFINE_SOS_STATIC_PTR( Sos_database_session )
DEFINE_SOS_STATIC_PTR( Sosdb_static )


//----------------------------------------------------------------------------------------words

struct Dbms_word
{
    Dbms_kind                  _dbms;
    const char*                _word;
    const char*                _dbms_word;
};

const Dbms_word dbms_words[] =
{
    { dbms_oracle       , "BOOLEAN"      , "NUMERIC(1)"      },
    { dbms_oracle       , "BIT"          , "NUMERIC(1)"      },
    { dbms_oracle       , "DATETIME"     , "DATE"            },
    { dbms_oracle       , "%NOW"         , "SYSDATE"         },
    { dbms_oracle       , "%ALTER_COLUMN", "MODIFY"          },

    { dbms_oracle_thin  , "BOOLEAN"      , "NUMERIC(1)"      },
    { dbms_oracle_thin  , "BIT"          , "NUMERIC(1)"      },
    { dbms_oracle_thin  , "DATETIME"     , "DATE"            },
    { dbms_oracle_thin  , "%NOW"         , "SYSDATE"         },
    { dbms_oracle_thin  , "%ALTER_COLUMN", "MODIFY"          },

    { dbms_access       , "BOOLEAN"      , "BIT"             },
    { dbms_access       , "BLOB"         , "MEMO"            },
    { dbms_access       , "CLOB"         , "MEMO"            },
    { dbms_access       , "DATETIME"     , "DATE"            },
    { dbms_access       , "NUMERIC"      , "[double]"        },     // Wird zu double, ohne folgendes "(p,s)", siehe Code
    { dbms_access       , "%NOW"         , "(Date()+Time())" },
    { dbms_access       , "%ALTER_COLUMN", "ALTER COLUMN"    },     // ?

    { dbms_sql_server   , "BOOLEAN"      , "BIT"             },
    { dbms_sql_server   , "BLOB"         , "IMAGE"           },
    { dbms_sql_server   , "CLOB"         , "NTEXT"           },
  //{ dbms_sql_server   , "DATETIME"     , "DATE"            },
    { dbms_sql_server   , "%NOW"         , "GetDate()"       },
    { dbms_sql_server   , "%ALTER_COLUMN", "ALTER COLUMN"    },
    { dbms_sql_server   , "%UPDATE_LOCK" , "WITH(UPDLOCK)"   },     // Für jede Tabelle in der From-Klausel angebbar, siehe http://msdn.microsoft.com/library/default.asp?url=/library/en-us/tsqlref/ts_fa-fz_4ox9.asp

    { dbms_mysql        , "BOOLEAN"      , "BOOL"            },
    { dbms_mysql        , "BLOB"         , "LONGBLOB"        },
    { dbms_mysql        , "CLOB"         , "LONGTEXT"        },
    { dbms_mysql        , "%NOW"         , "now()"           },
    { dbms_mysql        , "%ALTER_COLUMN", "MODIFY"          },

    { dbms_postgresql   , "BOOLEAN"      , "NUMERIC(1)"      },   // Nicht BOOLEAN, weil hier nicht 1 und 0 gespeichert werden kann (nur true,false,'1','0')
    { dbms_postgresql   , "BLOB"         , "BYTEA"           },
    { dbms_postgresql   , "CLOB"         , "TEXT"            },
    { dbms_postgresql   , "DATETIME"     , "TIMESTAMP"       },
    { dbms_postgresql   , "%NOW"         , "NOW()"           },
  //{ dbms_postgresql   , "%ALTER_COLUMN", "MODIFY"          },
    { dbms_postgresql   , "%UCASE"       , "UPPER"           },
    { dbms_postgresql   , "%LCASE"       , "LOWER"           },

  //{ dbms_postgress    , "%NOW"         , "(timestamp 'now')"},

    { dbms_db2          , "BOOLEAN"      , "NUMERIC(1)"      },
    { dbms_db2          , "BLOB"         , "BLOB"            },
    { dbms_db2          , "CLOB"         , "CLOB"            },
    { dbms_db2          , "DATETIME"     , "TIMESTAMP"       },
    { dbms_db2          , "%NOW"         , "CURRENT_TIMESTAMP()"},
    { dbms_db2          , "%UCASE"       , "UPPER"           },
    { dbms_db2          , "%LCASE"       , "LOWER"           },

    { dbms_firebird     , "BOOLEAN"      , "NUMERIC(1)"      },   // Nicht BOOLEAN, weil hier nicht 1 und 0 gespeichert werden kann (nur true,false,'1','0')
    { dbms_firebird     , "BLOB"         , "BLOB"            },
    { dbms_firebird     , "CLOB"         , "BLOB SUB_TYPE TEXT" },
    { dbms_firebird     , "DATETIME"     , "TIMESTAMP"       },
    { dbms_firebird     , "%NOW"         , "CURRENT_TIMESTAMP"},
    { dbms_postgresql   , "%ALTER_COLUMN", "ALTER COLUMN"    },
    { dbms_firebird     , "%UCASE"       , "UPPER"           },
    { dbms_firebird     , "%LCASE"       , "LOWER"           },

    { dbms_sybase       , "BOOLEAN"      , "NUMERIC(1)"      },     // BIT: "can't specify Null values on a column of type BIT"
    { dbms_sybase       , "BLOB"         , "IMAGE"           },
    { dbms_sybase       , "CLOB"         , "TEXT"            },
  //{ dbms_sybase       , "DATETIME"     , "DATE"            },
    { dbms_sybase       , "%NOW"         , "GetDate()"       },
    { dbms_sybase       , "%ALTER_COLUMN", "ALTER COLUMN"    },
    { dbms_sybase       , "%UPDATE_LOCK" , "HOLDLOCK"        },

  //{ dbms_sossql       , "%NOW"         ,                   },

    { dbms_h2           , "%NOW"         , "NOW()"           },

    { dbms_unknown      , NULL           , NULL              },
};

//------------------------------------------------------------------------------------time_stamp
#ifdef SYSTEM_UNIX

static string time_stamp()
{
    char buffer [ 20 ];

#   if defined USE_GETTIMEOFDAY
        struct timeval t;
        strftime( buffer, 11+1, "%d %T", localtime( &t.tv_sec ) );
        sprintf( buffer + 11, ".%04d", (int)_time_stamp.tv_usec / 100 );
        buffer[ 31 + 5 ] = '\0';
#    elif defined SYSTEM_MICROSOFT
        _timeb t;
        _ftime( &t );
        strftime( buffer, 11+1, "%d %H:%M:%S", localtime( &t.time ) );
        sprintf( buffer + 11, ".%03d", (int)t.millitm );
#    else
        time_t t;
        time( &t );
        strftime( buffer, 11+1, "%d %T", localtime( &t ) );
#   endif

    return buffer;
}

#endif
//-----------------------------------------------------Sos_database_static::Sos_database_static

Sos_database_static::Sos_database_static()
{
    _session_array.obj_const_name( "Sos_database_session::_session_array" );
}

//----------------------------------------------------Sos_database_static::~Sos_database_static

Sos_database_static::~Sos_database_static()
{
    LOGI( "~Sos_database_static()  " << *this << '\n' );
}

//-------------------------------------------------------------Sos_database_static::read_profile

void Sos_database_static::read_profile()
{
    _default_db_name   = read_profile_string( "", c_str( _dbms_name ), "database" );
  //_default_qualifier = read_profile_string( "", c_str( _dbms_name ), "qualifier" );
    _default_user      = read_profile_string( "", c_str( _dbms_name ), "user"     );
    _default_password  = read_profile_string( "", c_str( _dbms_name ), "password" );
    _default_transaction_timeout = read_profile_uint( "", c_str( _dbms_name ), "transaction-timeout", 0 );
    _multiple_sessions = read_profile_bool  ( "", c_str( _dbms_name ), "multiple-sessions", true );
    _keep_sessions_open= read_profile_bool  ( "", c_str( _dbms_name ), "keep-sessions-open", false );
    _first_cmds        = read_profile_string( "", c_str( _dbms_name ), "first" );
    _debug             = log_ptr && ( read_profile_bool  ( "", c_str( _dbms_name ), "debug", _debug ) || log_category_is_set( _dbms_name ) );
}

//----------------------------------------------Sos_database_static::print_uncommited_sessions

void Sos_database_static::print_uncommited_sessions( ostream* s )
{
    for( int i = _session_array.first_index(); i <= _session_array.last_index(); i++ )
    {
        Sos_database_session* session = _session_array[ i ];
        if( session  &&  session->_write_transaction_open )  *s << "Client " << session->_client->_name << ": " << *session << '\n';
    }
}

//---------------------------------------------------Sos_database_session::Sos_database_session

Sos_database_session::Sos_database_session( Sos_database_static* st )
:
    _zero_ ( this+1 ),
    _static( st ),
  //_identifier_quote_begin ( "\"" ),
  //_identifier_quote_end   ( '"' ),
  //_date_begin_string      ( "\"" ),
  //_date_end_quote         ( '"' ),
    _date_format            ( "dd-mon-yyyy" )
{
    _concat_operator = "||";
    _array_index = -1;
    _log_stmt = true;
}

//--------------------------------------------------Sos_database_session::~Sos_database_session

Sos_database_session::~Sos_database_session()
{
    LOGI( "~Sos_database_session()  " << *this << '\n' );

    if( _static ) {
        for( int i = 0; i <= _static->_session_array.last_index(); i++ )
        {
            if( _static->_session_array[ i ] == this ) {
                _static->_session_array[ i ] = NULL;
                return;
            }
        }
    }
}

//-----------------------------------------------------------Sos_database_session::read_profile

void Sos_database_session::read_profile()
{
    if( length( _db_name ) != 0 )       // jz 23.12.00
    {
        Sos_string section = _static->_dbms_name;
        section += " -db=";
        section += _db_name;

        _qualifier          = read_profile_string( "", c_str( section ), "qualifier", c_str( _qualifier ) );
        _user               = read_profile_string( "", c_str( section ), "user"     , c_str( _user ) );
        _password           = read_profile_string( "", c_str( section ), "password" , c_str( _password ) );
        _transaction_timeout= read_profile_uint  ( "", c_str( section ), "transaction-timeout", _transaction_timeout );
      //_multiple_sessions  = read_profile_bool  ( "", c_str( section ), "multiple-sessions", _multiple_sessions );   // Wirkt das hier? jz 30.11.97
      //_keep_sessions_open = read_profile_bool  ( "", c_str( section ), "keep-sessions-open", _keep_sessions_open );

        if( _first_cmds != "" )  _first_cmds += ';';
        _first_cmds        += read_profile_string( "", c_str( section ), "first" );
        _debug              = log_ptr && read_profile_bool  ( "", c_str( _static->_dbms_name ), "debug", _debug );
    }
}

//------------------------------------------------------------------Sos_database_session::close

void Sos_database_session::close( Close_mode close_mode )
{
/*  in sosdb.cxx
    try
    {
        rollback();            // jz 15.10.00
    }
    catch( const Xc& ) {}
*/

    if( _array_index >= 0 ) {
        int index = _array_index;
        _array_index = -1;
        _client->_session_array[ index ] = 0;
    }

    _close( close_mode );
}

//---------------------------------------------------------Sos_database_session::_equal_session

Bool Sos_database_session::_equal_session( Sos_database_file* )
{
    return true;
}

//------------------------------------------------------------------Sos_database_session::open

void Sos_database_session::open( Sos_database_file* first_file )
{
    _static              = first_file->_static;
    _connection_id       = first_file->_connection_id;
    _db_name             = first_file->_db_name;
    _qualifier           = first_file->_qualifier;
    _user                = first_file->_user;
    _password            = first_file->_password;
    _open_mode           = first_file->_open_mode;
    _transaction_timeout = first_file->_transaction_timeout;
    _debug               = first_file->_debug;

    _first_cmds          = _static->_first_cmds;
    
    read_profile(); // .ini-Einstellungen aus [DRIVER -db=DB] lesen

    if( ( _open_mode & ( Any_file::in | Any_file::out ) ) == 0 )  _open_mode = File_base::Open_mode( _open_mode | Any_file::in | Any_file::out );  //throw_xc( "SOS-1357" );

    if( !first_file->_connection_wait )
    {
        _open( first_file );
    }
    else
    {
        time_t wait_until = Sos_optional_date_time::now().as_time_t() + (time_t)first_file->_connection_wait;
        int    interval   = 5;

        while(1)
        {
            time_t now = Sos_optional_date_time::now().as_time_t();

            try 
            { 
                _open( first_file ); 
                break;
            }
            catch( const exception& x )
            {
                if( first_file->_connection_wait_error_codes != "" )
                {
                    string error_codes = "," + zschimmer::join( ",", zschimmer::vector_split( " *[ ,] *", trim( first_file->_connection_wait_error_codes ) ) ) + ",";
                    if( error_codes.find( "," + string(Xc(x).code()) + "," ) == string::npos )  throw;
                }

                if( now >= wait_until )  
                {
                    Z_LOG2( "sosdb", "Wartezeit -time=" << first_file->_connection_wait << " ist abgelaufen.\n" );
                    throw;
                }

                int sleep_time = min( interval, (int)( wait_until - now ) );
                Z_LOG2( "sosdb", sleep_time << "s warten bis zum nächsten Versuch ...\n" );
                sos_sleep( sleep_time );
                interval = min( 5*60, interval + interval / 2 );
            }
        }
    }

    _is_odbc = name().substr(0,4) == "odbc"  
            || name().substr(0,4) == "jdbc";


    if( _dbms == dbms_oracle  ||  _dbms == dbms_oracle_thin )
    {
        try {
            _execute_direct( "ALTER SESSION SET NLS_NUMERIC_CHARACTERS = '.,'" );
        }
        catch( const exception& ) {}

        try {
            _execute_direct( "ALTER SESSION SET NLS_DATE_FORMAT = 'YYYY-MM-DD HH24:MI:SS'" );
        }
        catch( const exception& ) {}
    }

    if( _dbms == dbms_mysql )
    {
        try {
            _execute_direct( "SET SESSION SQL_MODE='ANSI_QUOTES'" );
        }
        catch( const exception& ) {}
    }
    else
    if( _dbms == dbms_sybase )
    {
        try {
            _execute_direct( "SET QUOTED_IDENTIFIER ON" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET CHAINED ON" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET CLOSE ON ENDTRAN ON" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET TRANSACTION ISOLATION LEVEL READ COMMITTED" );    // eMail von Andreas Liebert vom 31.10.2008
            //_execute_direct( "SET TRANSACTION ISOLATION LEVEL REPEATABLE READ" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET DATEFIRST 1" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET DATEFORMAT 'ymd'" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET LOCK WAIT 3" );
        } catch( const exception& ) {}
        try {
            _execute_direct( "SET TEXTSIZE 2048000" );
        } catch( const exception& ) {}
    }


    if( !empty( _first_cmds ) )  execute_direct( c_str( _first_cmds ) );
}

//-------------------------------------------------------------Sos_database_session::properties

Sos_database_session::Properties Sos_database_session::properties() {
    Properties result;
    result["user"] = _user;
    result["password"] = _password;
    result["path"] = _db_name;
    return result;
}

//------------------------------------------------------Sos_database_session::check_transaction

void Sos_database_session::check_transaction()
{
    if( _transaction_timed_out ) {
        _transaction_timed_out = false;
        throw_xc( "SOS-1331", _transaction_timeout );
    }
}

//---------------------------------------------------------Sos_database_session::execute_direct

void Sos_database_session::execute_direct( const Const_area& stmt )
{
    check_transaction();

    const char* p     = stmt.char_ptr();
    const char* p_end = p + stmt.length();

    {   
        string s ( stmt.char_ptr(), stmt.length() );
        if( zschimmer::string_begins_with( s, "-split- " ) )    // 2006-09-20
        {
            execute_direct_single( Const_area( stmt.char_ptr() + 8, stmt.length() - 8 ) );
            return;
        }
    }


#   if defined SYSTEM_I386
        if( !memchr( p, ';', p_end - p ) ) {
            execute_direct_single( stmt );
            return;
        }
#   endif


    char  quote       = '\0';

    while( p < p_end )     // für jede Anweisung
    {
        const char* p0 = p;

        while( p < p_end )   // für jedes Zeichen
        {
            if( *p == ';'  )  break;
            else
            if( *p == '\''  ||  *p == '"' )
            {
                quote = *p++;
                if( p >= p_end )  break;     // Quote fehlt
                while(1) {
                    const char* q = (const char*)memchr( p, quote, p_end - p );
                    if( !q )  { p = p_end; break; }   // Quote fehlt
                    p = q + 1;  //jz 8.2.98
                    if( p >= p_end )  break;
                    if( *p != quote )  break;
                    // Doppelte Quote
                    p++;
                    if( p >= p_end )  break;  // Quote fehlt
                }
            }
            else p++;
        }

        execute_direct_single( Const_area( p0, p - p0 ) );

        if( p < p_end )  p++;  // Semikolons
    }
}

//--------------------------------------------------Sos_database_session::execute_direct_single

void Sos_database_session::execute_direct_single( const Const_area& stmt_par )
{
    _need_commit_or_rollback = true;
    _single       = false;
    _row_count    = -1;        // "unbekannt"

    // Optionen herauslesen:

    const char* p1    = stmt_par.char_ptr();
    const char* p_end = p1 + length( stmt_par );

    while( p1 < p_end  &&  isspace( (unsigned char)*p1 ) )  p1++;

    if( p1 == p_end )  return;  // Leere Anweisung ignorieren

    if( *p1 == '-'  &&  strncmpi( p1, "-single ", 8 ) == 0 ) {
        _single = true;             // UPDATE und DELETE sollen genau einen Satz betreffen
        p1 += 8;
        while( p1 < p_end  &&  isspace( (unsigned char)*p1 ) )  p1++;
    }


    Dynamic_area stmt;
    convert_stmt( Const_area( p1, stmt_par.char_ptr() + stmt_par.length() - p1 ), &stmt );
    p1    = stmt.char_ptr();
    p_end = p1 + stmt.length();

    // Anweisungswort aus dem stmt holen:

    const char* p2 = p1;
    if( p_end > p1 + _cmd.size() )  p_end = p1 + _cmd.size();
    while( p2 < p_end  &&  !isspace( (unsigned char)*p2 ) )  p2++;

    _cmd.assign( p1, p2 - p1 );
    _cmd.upper_case();

    //if( _log_stmt )  LOG( "Sos_database_session::execute_direct_single: _cmd=" << _cmd << " " << stmt_par << '\n' );

    if( _cmd == "COMMIT" ) {
        if( _log_stmt )  Z_LOG2( "sosdb", __FUNCTION__ << " COMMIT\n" );
        // COMMIT WORK?
        if( p2 < p_end )  {
            _cmd += '\0';
            throw_xc( "SOS-1184", "COMMIT", _cmd.char_ptr() );
        }
        commit();
    }
    else
    if( _cmd == "ROLLBACK" ) {
        if( _log_stmt )  Z_LOG2( "sosdb", __FUNCTION__ << " ROLLBACK\n" );
        if( p2 < p_end ) {
            _cmd += '\0';
            throw_xc( "SOS-1184", "ROLLBACK", _cmd.char_ptr() );
        }
        rollback();
    }
    else
    {
        if( _log_stmt ) Z_LOG2( "sosdb", "Sos_database_session::execute_direct_single: _cmd=" << _cmd << ' ' << stmt << '\n' );


        // Um Problem bei record/tabbed zu vermeiden:
        //jz 26.6.2002 if( memchr( stmt.ptr(), '\t', stmt.length() ) )  throw_xc( "SOS-1242" );

        Bool is_write_cmd = _cmd == "INSERT"  || _cmd == "UPDATE" || _cmd == "DELETE" || _cmd == "CALL";

        if( is_write_cmd  &&  !( _open_mode & Any_file::out ) )  throw_xc( "SOS-1358" );


        _transaction_used = true;

        if( is_write_cmd  &&  !_write_transaction_open )
        {
            transaction_begun();


            try {
                _execute_direct( stmt );
            }
            catch( const Xc& ) {
                if( _row_count == 0     // -1: undefiniert, > 0: einige Sätze geändert bevor ein Fehler auftreten ist (etwa Lock_error)
                 && _cmd != "CALL" )    // Wird bei CALL _row_count korrekt geliefert?
                {
                    transaction_ends(); // Transaktion ist wegen des Fehlers und _row_count == 0 doch nicht begonnen worden.
                }
                throw;
            }
        } else {
            _execute_direct( stmt );
        }

        if( _single ) {
          //if( _row_count == -1 )  ???;
            if( _row_count == 0 )  throw_not_found_error();
            if( _row_count >  1 )  throw_xc( "SOS-1235" );
        }
    }
}

//--------------------------------------------------Sos_database_session::convert_date_or_timestamp

string Sos_database_session::convert_date_or_timestamp( const char** pp, const char* p_end )
{
    const char*& p = *pp;

    if( *p == '%' )
    {
        bool is_date      = strnicmp( p, "%date"     ,  5 ) == 0;
        bool is_timestamp = strnicmp( p, "%timestamp", 10 ) == 0;

        if( is_date | is_timestamp )
        {
            string                 result;
            Sos_optional_date_time dt;
            string                 text;

            dt.date_only( is_date );
            if( _is_odbc  &&  _dbms != dbms_sybase )  result += is_date? "{d" : "{ts";
            p++;
            while( p < p_end  &&  isalpha( (unsigned char)*p ) )  p++;
            while( p < p_end  &&  isspace( (unsigned char)*p ) )  p++;

            if( p >= p_end )  throw_xc("SOS-1413");
            if( p[0] != '(' )  throw_xc("SOS-1413");
            p++;
            while( p < p_end && isspace( (unsigned char)*p ) )  p++;
            
            if( p >= p_end || p[0] != '\'' )  throw_xc("SOS-1413");
            p++;
            while( p < p_end && p[0] != '\'' )  text += *p++;
            if( p >= p_end || p[0] != '\'' )  throw_xc("SOS-1413");
            p++;

            dt.assign( text );
            result += '\'';
            result += dt.formatted( is_date? std_date_format_iso : std_date_time_format_iso );
            result += '\'';

            while( p < p_end && isspace( (unsigned char)*p ) )  p++;
            if( p >= p_end || p[0] != ')' )  throw_xc("SOS-1413");
            p++;
            if( _is_odbc &&  _dbms != dbms_sybase )  result += '}';

            return result;
        }
    }

    return "";
}

//------------------------------------------------------Sos_database_session::convert_texttimestamp

string Sos_database_session::convert_texttimestamp( const char** pp, const char* p_end )
{
    const char*& p = *pp;

    string arg = parse_argumentlist( pp, p_end, 1 )[0];

    switch( _dbms )
    {
        case dbms_db2:
            return "(char(date(" + arg + "),iso)||' '||char(time(" + arg + "),jis))";

        case dbms_firebird:
            return "substring(Cast(" + arg + " as varchar(24)) from 1 for 19)";

        case dbms_mysql:
            return "date_format(" + arg + ",'%Y-%m-%d %H:%i:%s')";

        case dbms_oracle:
        case dbms_oracle_thin:
        case dbms_postgresql:
            return "to_char(" + arg + ",'YYYY-MM-DD HH24:MI:SS')";

        case dbms_sql_server:
            return "CONVERT(VARCHAR(19)," + arg + ",121)";

        case dbms_sybase:
            return "str_replace(CONVERT(VARCHAR(19)," + arg + ",23),'T',' ')";

        case dbms_h2:
            return "((" + arg + "))||''";

        default: 
            return "((" + arg + ")&'')";
    }
}

//--------------------------------------------------------Sos_database_session::convert_secondsdiff

string Sos_database_session::convert_secondsdiff( const char** pp, const char* p_end )
{
    const char*& p = *pp;

    vector<string> args = parse_argumentlist( pp, p_end, 2 );

    switch( _dbms )
    {
        case dbms_postgresql:
            return "extract( epoch from ((" + args[0] + ")-(" + args[1] + ")) )";

        case dbms_sybase:
            return "datediff(ss," + args[0] + "," + args[1] + ")";

        case dbms_h2:       // JS-448
            return "datediff(ss," + args[1] + "," + args[0] + ")"; // JS-448


        default: 
            return "((" + args[0] + ")-(" + args[1] + "))";
    }
}

//---------------------------------------------------------Sos_database_session::parse_argumentlist

vector<string> Sos_database_session::parse_argumentlist( const char** pp, const char* p_end, int n )
{
    vector<string> result = vector<string>( n );
    const char*& p = *pp;

    while( *p == ' ' )  p++;
    if( *p != '(' )  throw_xc( "SOS-1413" );
    p++;

    for( int i = 0; i < n; i++ ) {
        result[ i ] = parse_argument( &p, p_end );
        
        if( *p == ')' ) {
            p++;
            break;
        }
        if( *p == ',' ) {
            p++;
        }
        else 
            throw_xc("SOS-1413");
    }

    return result;
}

//-------------------------------------------------------------Sos_database_session::parse_argument

string Sos_database_session::parse_argument( const char** pp, const char* p_end )
{
    const char*& p = *pp;
    const char* start_arg = p;
    int klammern = 0;

    while( p < p_end )
    {
        bool end = false;

        switch( *p ) 
        {
            case '(':
                p++;
                klammern++;
                break;

            case ')':
                if( klammern == 0 )  end = true;
                               else  p++;
                klammern--;
                break;

            case ',':
                if( klammern == 0 )  end = true;
                               else  p++;
                break;

            //Nicht getestet und bislang nicht erforderlich:
            //case '"':
            //    p++;
            //    while( *p  &&  *p != '"' )  p++;
            //    if( p < p_end )  p++;
            //    break;

            //case '\'':
            //    p++;
            //    while( *p  &&  *p != '\'' )  p++;
            //    if( p < p_end )  p++;
            //    break;

            default:
                p++;
        }

        if( end )  break;
    }

    string arg = string( start_arg, p - start_arg );

    Dynamic_area result;
    convert_stmt( arg, &result );
    return trim( string( result.char_ptr(), result.length() ) );
}

//------------------------------------------------------------------Sos_database_session::translate

string Sos_database_session::translate( const string& word )
{
    for( const Dbms_word* w = dbms_words; w->_word; w++ )
    {
        if( _dbms == w->_dbms && stricmp( word.c_str(), w->_word ) == 0 )  return w->_dbms_word;
    }

    return word;
}

//--------------------------------------------------------Sos_database_session::translate_limit

string Sos_database_session::translate_limit( const string& stmt, int limit )
{
    switch( _dbms )
    {
        case dbms_oracle:
        case dbms_oracle_thin:
            return "select * from ( " + stmt + " ) where rownum <= " + as_string( limit );

        case dbms_mysql:
        case dbms_postgresql:
        case dbms_h2:
            return stmt + "  limit " + as_string( limit );

        case dbms_access:
        case dbms_sql_server:
        case dbms_sybase:
        {
            int pos = z::lcase( stmt ).find( "select" );
            if( pos == string::npos )  throw_xc( "SOS-1458" );

            return stmt.substr( 0, pos+6 ) + "  TOP " + as_string(limit) + " " + stmt.substr( pos+6 );
        }
        
        case dbms_sossql:
            return "-limit=" + as_string( limit ) + " " + stmt;

        case dbms_db2:
            return S() << stmt << "  fetch first " << limit << " rows only";

        case dbms_firebird:
        {
            if( zschimmer::lcase( stmt.substr( 0, 7 ) ) != "select " )  throw_xc( "SOS-1457", _dbms_name, "SELECT fehlt" );
            return stmt.substr( 0, 7 ) + "FIRST " + as_string( limit ) + stmt.substr( 6 );
            break;
        }

        default:
            throw_xc( "SOS-1457", _dbms_name );
    }
}

//-------------------------------------------------------Sos_database_session::translate_for_update

//string Sos_database_session::translate_for_update( const string& stmt )
//{
//    switch( _dbms )
//    {
//        case dbms_sql_server:
//        {
//            // Der Algorithmus fügt "WITH(UPDLOCK)" vor dem ersten WHERE ein.
//            // Bei mehreren Tabellen in der FROM-Klausel sollte der Text aber nach jeder Tabelle eingefügt werden.
//            // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/tsqlref/ts_fa-fz_4ox9.asp
//
//            const char* p0 = stmt.c_str();
//            const char* p  = p0;
//            while( *p )
//            {
//                if( *p == '"'  ||  *p == '\'' )
//                {
//                    char quote = *p++;
//                    while( *p  &&  *p != quote ) p++;
//                    if( *p )  p++;
//                }
//                else
//                if( isalpha( (unsigned char)*p ) )
//                {
//                    if( strcmpni( p, "where ", 6 ) == 0 )  break;
//                    while( isalnum( (unsigned char)*p ) )  p++;
//                }
//                else
//                   p++;
//            }
//
//            return stmt.substr( 0, p - p0 ) + " WITH(UPDLOCK) " + p;
//        }
//        
//        case dbms_oracle:
//        case dbms_oracle_thin:
//        case dbms_mysql:
//        case dbms_postgresql:
//        case dbms_access:
//      //case dbms_sossql:
//        case dbms_db2:
//        case dbms_firebird:
//        default:
//            return stmt + "  FOR UPDATE";
//    }
//}

//--------------------------------------------------------------Sos_database_session::translate_sql

string Sos_database_session::translate_sql( const string& sql_statement )
{
    Dynamic_area result;
    convert_stmt( Const_area( sql_statement ), &result );
    return string( result.char_ptr(), result.length() );
}

//-----------------------------------------------------------Sos_database_session::convert_stmt

void Sos_database_session::convert_stmt( const Const_area& stmt, Area* result_area )
{
    // Nur das erste Zeichen von _identifier_quote_begin wird berücksichtigt.
    // '"' oder _identifier_quote_begin[ 0 ] im Namen werden nicht berücksichtigt!!! IST VERBOTEN!

    const char* p           = stmt.char_ptr();
    const char* p_end       = stmt.char_ptr() + stmt.length();
    bool        append_for_update = false;

    while( p < p_end  &&  isspace( (unsigned char)*p ) )  p++;
    if( p == p_end )  return;

    if( strncmp( p, "%native ", 8 ) == 0 )  
    {
        p += 8;
        result_area->assign( p, p_end - p );
        return;
    }
    
    result_area->allocate_min( stmt.length() + 100 );
    result_area->length(0);

    int         limit = -1;
    string      result;
    char        quote_begin = length( _identifier_quote_begin ) == 0? '"' : _identifier_quote_begin[ 0 ];
    char        quote_end   = length( _identifier_quote_end   ) == 0? quote_begin  : _identifier_quote_end[ 0 ];
    Bool        in_string   = false;
    Bool        in_identifier = false;
    int         klammern      = 0;
    std::stack<bool> odbc_fn;

    result.reserve( result_area->size() );

    const char* p2 = p;
    while( p2 < p_end  &&  isalnum( (unsigned char)*p2 ) )  p2++;
    string cmd ( p, p2-p );  ucase_(&cmd);
    
    bool change_words = cmd == "CREATE" || cmd == "ALTER";


    while( p < p_end ) 
    {
        if( *p == '\'') {
            in_string = !in_string;
            result += *p++;
        } 
        else 
        if( in_string ) 
        {
            if( _dbms == dbms_mysql ||
                _dbms == dbms_postgresql )
            {
                do switch( char c = *p++ )
                {
                    case '\0': result += "\\0";  break;
                    case '\b': result += "\\b";  break;
                    case '\t': result += "\\t";  break;
                    case '\r': result += "\\r";  break;
                    case '\n': result += "\\n";  break;
                    case '\\': result += "\\\\";  break;
                    default  : result += c;
                }
                while( p < p_end  &&  *p != '\'' );
            }
            else
                result += *p++;
        }
        else
        if( *p == '"' || *p == '`' ) {   // ` für " für "select `A`, `B`"  statt "select \"A\", \"B\""
            result += in_identifier? quote_end : quote_begin; 
            in_identifier = !in_identifier; 
            p++;
        }
        else
        if( in_identifier ) {
            result += sos_toupper( *p++ );        // Für Oracle 7
        }
        else
        if( p[0] == '|'  &&  p[1] == '|' ) {
            result += _concat_operator;
            p += 2;
        }
        else
        if( p[0] == '%' )       // Erweiterung der SOS, z.b. %ucase(...)
        {
            const char* saved_p = p;
            string      word;
            
            word = *p++;
            while( p < p_end  &&  ( isalpha( (unsigned char)*p ) || *p == '_' ) )  word += toupper((unsigned char)*p++);
            while( p < p_end  &&  isspace( (unsigned char)*p) )  p++;
            
            if( *p == '(' ) {   // Leere Klammern () sind optional
                const char* q = p+1;
                while( q < p_end  &&  isspace((unsigned char)*q) )  q++;
                if( q < p_end  &&  *q == ')' )  p = q+1;
            }

            if( word == "%UPDATE_LOCK" )  _need_commit_or_rollback = true;

            string new_word = translate( word );

            if( new_word == "%NOW" )    // %NOW() nicht übersetzt?
            {
                string s = "%timestamp('" + Sos_optional_date_time::now().as_string() + "')";       // Lokale Uhr
                const char* sp = s.c_str();
                result += convert_date_or_timestamp( &sp, sp + strlen(sp) );
                assert( *sp == '\0' );
                result += ' ';
            }
            else
            if( new_word == "%TEXTTIMESTAMP" )
            {
                result += convert_texttimestamp( &p, p_end );
            }
            else
            if( new_word == "%SECONDSDIFF" )
            {
                result += convert_secondsdiff( &p, p_end );
            }
            else
            if( new_word == "%LIMIT" )  // %LIMIT nicht übersetzt? Das ist wohl immer so.
            {
                limit = -1;
                
                int lim = 0;
                if( p[0] == '(' ) 
                {
                    p++;
                    while( isspace( (unsigned char)p[0] ) )  p++;
                    while( isdigit( (unsigned char)p[0] ) )  lim *= 10,  lim += p[0] - '0',  p++;
                    while( isspace( (unsigned char)p[0] ) )  p++;
                    if( p[0] == ')' ) p++,  limit = lim;
                }

                if( limit == -1 )  throw_xc( "SOS-1459" );
            }
            else
            if( new_word == "%UPDATE_LOCK" )
            {
                append_for_update = true;
            }
            else
            if( new_word != word )      // translate() hat übersetzt?
            {
                result += new_word;
                result += ' ';
            }
            else                        // translate() hat nicht übersetzt?
            {
                p = saved_p;

                string text = convert_date_or_timestamp( &p, p_end );
                if( text != "" )
                {
                    result += text;
                }
                else
                if( _is_odbc )          // &xxx(...) --> {fn xxx(...)}
                {
                    result += "{fn ";
                    p++;
                    while( p < p_end && isalpha((unsigned char)*p) )  result += *p++;

                    if( p < p_end  &&  *p == '(' )
                    {
                        klammern++;
                        odbc_fn.push( true );
                        result += *p++;
                    }
                }
                else 
                    p = saved_p + 1;    // Sonst '%' wegwerfen
            }
        }
        else
        if( *p == '(' )
        {
            result += *p++;
            klammern++;
            odbc_fn.push( false );
        }
        else
        if( *p == ')' )  
        {
            result += *p++;
            if( klammern > 0 ) {
                if( odbc_fn.top() )   result += '}';
                --klammern, odbc_fn.pop();
            }
        }
        else
        if( isalpha( (unsigned char)*p ) )
        {
            const char* p2 = p;
            while( p2 < p_end  &&  ( isalnum( (unsigned char)*p2) || *p2 == '_' ) )  p2++;

            // Wenigstens in PostgresQL gilt limit für die gesamte Abfrage, nicht für ein einzelnes select
            if( limit >= 0  &&  !klammern  &&  ( p2 - p == 5  &&  strnicmp( p, "union"    , p2 - p ) == 0
                                              || p2 - p == 9  &&  strnicmp( p, "intersect", p2 - p ) == 0
                                              || p2 - p == 6  &&  strnicmp( p, "except"   , p2 - p ) == 0 ) )    // Die Wörter werden im Subselect nicht erkannt
            {
                result_area->append( translate_limit( result, limit ) );
                result_area->append( ' ' );
                result_area->append( p, p2 - p );
                result_area->append( ' ' );
                result = "";
                limit = -1;
            }
            else
            if( change_words )
            {
                string new_word = translate( string( p, p2-p ) );

                if( new_word == "[double]" )
                {
                    if( p2[0] == '(' ) 
                    {
                        p2++;
                        while( isspace( (unsigned char)p2[0] ) || isdigit( (unsigned char)p2[0] ) || p2[0] == ',' )  p2++;
                        if( p2[0] == ')' ) p2++;
                    }
                    result += "DOUBLE";
                }
                else
                    result += new_word;
            }
            else
                result.append( p, p2 - p );

            p = p2;
        }
        else
        {
            result += *p++;
        }
    }

    if( limit >= 0 )  result_area->append( translate_limit( result, limit ) );
                else  result_area->append( result );

    if( append_for_update )  result_area->append( "  FOR UPDATE" );
}

//-------------------------------------------------------------------------sosdb_timer_callback

int __cdecl sosdb_timer_callback( void* param )
{
    return ((Sos_database_session*)param)->timer_callback();
}

//---------------------------------------------------------Sos_database_session::timer_callback

int Sos_database_session::timer_callback()
{
    _transaction_timer = NULL;

    if( _write_transaction_open ) {
#       if defined SYSTEM_UNIX
            std::cerr << time_stamp() << "   Transaktion-Zeitlimit ist abgelaufen. Die Transaktion wird jetzt zurückgesetzt. "
                 << *this  << '\n';
#       endif

        Z_LOG2( "sosdb", "Transaktion-Zeitlimit ist abgelaufen. Die Transaktion wird jetzt zurückgesetzt. "
             << *this  << '\n' );

        _transaction_timed_out = true;  // falls rollback() Exception auswirft
        rollback();
        _transaction_timed_out = true;
    }

    return 0;
}

//------------------------------------------------------Sos_database_session::transaction_begun

void Sos_database_session::set_transaction_timer()
{
    if( _transaction_timeout ) {
        if( _transaction_timer ) {
            _transaction_timer->remove();
            _transaction_timer = NULL;
        }

        _transaction_timer = sos_static_ptr()->_timer_manager->add_timer(
                                 &sosdb_timer_callback,
                                 this,
                                 clock_as_double() + _transaction_timeout
                             );
    }
}

//-----------------------------------------------------Sos_database_session::transaction_begun

void Sos_database_session::transaction_begun()
{
    _write_transaction_open = true;
    //cerr << "Session " << _session_no  << " begin transaction " << st << '\n';

    set_transaction_timer();

/*jz 12.4.97
    if( _transaction_timeout ) {
        _transaction_timer = sos_static_ptr()->_timer_manager
                               ->add_timer( &sosdb_timer_callback,
                                            this,
                                            clock_as_double() + _transaction_timeout );
    }
*/
}

//-------------------------------------------------------Sos_database_session::transaction_ends

// Wird bei Commit und Rollback gerufen.
// Wird auch zum Rücksetzen einer wegen eines Fehlers falsch vorhergesagten Transaktion gerufen.

void Sos_database_session::transaction_ends()
{
    _write_transaction_open = false;
    // << "Session " << _session_no  << ' ' << st << '\n';

    if( _transaction_timer )  {
        _transaction_timer->remove();
        _transaction_timer = NULL;
    }
}

//-----------------------------------------------------------------Sos_database_session::commit

void Sos_database_session::commit()
{
    check_transaction();    // Transaktionszeit abgelaufen?
    transaction_ends();
    _commit();
    
    _transaction_used        = false;
    _need_commit_or_rollback = false;
}

//---------------------------------------------------------------Sos_database_session::rollback

void Sos_database_session::rollback()
{
    _transaction_timed_out = false;
    transaction_ends();
    _rollback();

    _transaction_used        = false;
    _need_commit_or_rollback = false;
}

//----------------------------------------------------------------Sos_database_session::_commit

void Sos_database_session::_commit()
{
    sos_log( "Sos_database_session::_commit\n" );
    _execute_direct( "COMMIT" );
}

//--------------------------------------------------------------Sos_database_session::_rollback

void Sos_database_session::_rollback()
{
    sos_log( "Sos_database_session::_rollback\n" );
    _execute_direct( "ROLLBACK" );
}

//-------------------------------------------------------------Sos_database_session::_obj_print

void Sos_database_session::_obj_print( ostream* s ) const
{
    if( !empty( _connection_id ) )  *s << " id=" << _connection_id;

    *s << " db=" << _db_name;

    if( !empty( _user )          )  *s << " user=" << _user;
    
    *s << " cmd=" << _cmd
       << " open_mode=" << hex << (int)_open_mode << dec;

    *s << " ref=" << _ref_count;
}

//---------------------------------------------------------Sos_database_file::Sos_database_file

Sos_database_file::Sos_database_file( Sos_database_static* st )
:
    _zero_(this+1),
    _static(st)
{
    _session = 0;
}

//--------------------------------------------------------Sos_database_file::~Sos_database_file

Sos_database_file::~Sos_database_file()
{
    LOGI2( "sosdb", "~Sos_database_file()  " << *this << '\n' );

    try
    {
        session_disconnect();
    }
    catch( exception& x )
    {
        LOG( "Fehler bei session_disconnect() ignoriert: " << x << "\n" );
    }
}

//-----------------------------------------------------------Sos_database_file::database_option

Bool Sos_database_file::database_option( Sos_option_iterator& opt )
{
    if      ( opt.with_value( "id"          ) )             _connection_id = opt.value();
    else if ( opt.with_value( "database"    )
           || opt.with_value( "db"          ) )             _db_name   = opt.value();
    else if ( opt.with_value( "qualifier"   ) )             _qualifier = opt.value();
    else if ( opt.with_value( "user"        ) )             _user      = opt.value();
    else if ( opt.with_value( "password"    ) )             _password  = opt.value();
    else if ( opt.with_value( "transaction-timeout"  ) )    _transaction_timeout  = opt.as_int();
    else if ( opt.flag      ( "read-transaction"     ) )    _read_only = opt.set();
    else if ( opt.flag      ( "keep-session"         ) )    _keep_session = opt.set();
    else if ( opt.flag      ( "debug"                ) )    _debug     = log_ptr && opt.set();
    else if ( opt.with_value( "wait"                 ) )    _connection_wait = seconds_from_hhmmss( opt.value() );
    else if ( opt.with_value( "wait-errors"          ) )    _connection_wait_error_codes = opt.value();
    else
    if( opt.with_value( "rows" ) )                          _assert_row_count = opt.value().empty()? -1 : (int)opt.as_uint();
    else
    if( opt.flag( "auto-commit" ) 
     || opt.flag( "autocommit" ) )                          _auto_commit = opt.set();
    else
    if( opt.with_value( "ignore" ) )   
    {
        _ignore_fields = opt.value();
        if( z::has_regex( _ignore_fields, "^\\(.*\\)$" ) )  _ignore_fields = _ignore_fields.substr( 1, _ignore_fields.length()-2 );
    }
    else 
        return false;

    return true;
}

//-----------------------------------------------------------Sos_database_file::bind_parameters
/*
void Sos_database_file::bind_parameters( const Record_type* type, const Byte* p )
{
    _param_type = type;
    _param_base = p;
}
*/

//----------------------------------------------------------------Sos_database_file::get_static

void Sos_database_file::get_static( const char* dbms_name )
{
    Sosdb_static* sosdb = sos_static_ptr()->_sosdb;

    if( !sosdb ) {
        Sos_ptr<Sosdb_static> o = SOS_NEW( Sosdb_static );
        sos_static_ptr()->_sosdb = +o;
        o->_static_array.obj_const_name( "Sos_database_static::_static_array" );
        sosdb = o;
    }


    int i;
    for( i = sosdb->_static_array.first_index(); i <= sosdb->_static_array.last_index(); i++ ) {
        if( sosdb->_static_array[ i ] ) {
            if( sosdb->_static_array[ i ]->_dbms_name == dbms_name )  break;
        }
    }

    if( i > sosdb->_static_array.last_index() )    // nicht gefunden?
    {
        for( i = sosdb->_static_array.first_index(); i <= sosdb->_static_array.last_index(); i++ ) {
            if( !sosdb->_static_array[ i ] )  break;
        }
        if( i > sosdb->_static_array.last_index() )  sosdb->_static_array.add_empty();

        _create_static();

        sosdb->_static_array[ i ] = _static;
        _static->_dbms_name = dbms_name;
        _static->read_profile();
    } else {
        _static = sosdb->_static_array[ i ];
    }


    // Default-Einstellungen aus der sos.ini benutzen, wenn nix gesetzt ist
    if( _db_name == empty_string )  _db_name = _static->_default_db_name;

    if( _user == empty_string ) {
        _user = _static->_default_user;
        if( _password == empty_string )  _password = _static->_default_password;
    }
}

//---------------------------------------------------------------Sos_database_file::get_session

void Sos_database_file::get_session()
{
    Sos_database_session* session = 0;
    Sos_client*           client = 0;
    int i;

    session_disconnect();   // Im Falle eines Falles


    if( _static->_multiple_sessions ) {
        client = obj_client();
    } else {
        client = +sos_static_ptr()->_std_client;
    }


    for( i = client->_session_array.first_index(); i <= client->_session_array.last_index(); i++ )
    {
        session = client->_session_array[ i ];
        if( session
       //&& dbms_name      == session->_dbms_name       // jz 17.4.97
         && _static        == session->_static          // jz 17.4.97
         && _connection_id == session->_connection_id
         && length( session->_db_name ) > 0
         && _db_name       == session->_db_name
         && _qualifier     == session->_qualifier
         && _user          == session->_user
       //&& ( !(_open_mode & Any_file::out)  ||  (session->_open_mode & Any_file::out) )
         && ( _open_mode & Any_file::out? session->_open_mode & Any_file::out : true )
         && session->_equal_session( this ) )  break;
    }

    if( i > client->_session_array.last_index() ) 
    {
        for( i = client->_session_array.first_index(); i <= client->_session_array.last_index(); i++ )  {
            if( !client->_session_array[i] )  break;
        }
        if( i > client->_session_array.last_index() )  client->_session_array.add_empty();

        _session = _create_session();
        _session->_ref_count++;


        // Sosdb_session in Sosdb_static eintragen:
        {
            int j;

            for( j = 0; j <= _static->_session_array.last_index(); j++ ) {
                if( _static->_session_array[ j ] == NULL )  break;
            }
            if( j > _static->_session_array.last_index() )  _static->_session_array.add( (Sos_database_session*)NULL );

            _static->_session_array[ j ] = _session;
        }

        _session->_client    =  client;
      //_session->_dbms_name =  dbms_name;  // jz 17.4.97

        _session->open( this );

        client->_session_array[i] = +_session;
        _session->_array_index = i;
    } 
    else 
    {
        _session = session;
        _session->_ref_count++;
        Z_LOG2( "sosdb", "Vorhandene DB-Session wird verwendet: " << *session << '\n' );
    }

    _session->_transaction_used = true;
    _session->_need_commit_or_rollback |= !_read_only;
}

//-----------------------------------------------------------Sos_database_file::session_connect

void Sos_database_file::session_connect( const char* dbms_name )
{
    get_static( dbms_name );        // Sos_database_static suchen und ggfs. anlegen:
    if( !_debug )  _debug = _static->_debug;

    _transaction_timeout = _static->_default_transaction_timeout;

    get_session();                  // Sos_database_session suchen und ggfs. anlegen:
    if( !_debug )  _debug = _session->_debug;

    _session->check_transaction();  // Transaktionszeit abgelaufen?
}

//--------------------------------------------------------Sos_database_file::session_disconnect

void Sos_database_file::session_disconnect()
{
    if( !_session )  return;

    Sos_ptr<Sos_database_session> s = _session;
    _session = 0;

    LOGI2( "sosdb", "Sos_database_file::session_disconnect  " <<  *s << '\n' );

    if( --s->_ref_count == 0 ) 
    {
        try
        {
            s->rollback();
        }
        catch( exception& x )  { LOG( "Fehler bei ROLLBACK ignoriert: " << x << "\n" ); }

        if( !_keep_session 
         && !_static->_keep_sessions_open )  s->close();
    }
}

//------------------------------------------------------------Sos_database_file::execute_direct

void Sos_database_file::execute_direct( const Const_area& stmt )
{
    _session->_current_file = this;
    _session->execute_direct( stmt );
    _session->_current_file = NULL;    // Zur Fehlererkennung
}

//----------------------------------------------------------------Sos_database_file::put_record

void Sos_database_file::put_record( const Const_area& stmt )
{
    //jz 11.4.97 _session->execute_direct( stmt );
    execute_direct( stmt );
}


} //namespace sos
