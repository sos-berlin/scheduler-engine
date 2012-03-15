//#define MODULE_NAME "sqlfile"
//#define AUTHOR      "Jörg Schwiemann"
//#define COPYRIGHT   "1995 (C) Sos GmbH"

// SQL-File-Zugriffe

// Datum: 31.07.1995
// Stand: 01.08.1995


#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sysxcept.h"
#include "../kram/sosfield.h"
#include "../kram/sosdate.h" // wg. std_date_format_ingres
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../kram/sosctype.h"
#include "../file/absfile.h"
#include "../kram/tabucase.h"

#include "../kram/log.h"

#include <ctype.h>      // wg. Speicherproblem
#include "../kram/sosalloc.h"   // wg. Speicherproblem

#define LOGBLOCK(e) //Log_block qq(e)

namespace sos {

const int max_stmt_length           = 8192;
const int max_update_prefix_length  = 1024;    // die erste Allokation

//-------------------------------------------------------------------------------------Sql_file

struct Sql_file : Abs_file
{
                                    Sql_file            ();
                                   ~Sql_file            ();

    void                            prepare_open        ( const char*, Open_mode, const File_spec& );
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            insert              ( const Const_area& area );
    void                            update              ( const Const_area& area )  { update_direct( area ); }
    void                            update_direct       ( const Const_area& area );
    void                            store               ( const Const_area& area );

    void                            del                 ( const Key& key );

 protected:
    void                            get_record          ( Area& area );
    void                            get_record_key      ( Area& area, const Key& key );
    void                            put_record          ( const Const_area& area );

 private:
    void                            veraltetes_open     ();
    void                            set_key_descr_ptr   ( Field_descr* field_descr );
    void                           _modify_fields       ( const Sos_string& );
    void                            append_name         ( Area* buffer, const char* name );

    void                            rollback            ();
    void                            commit              ();

    void                            append_value        ( const Field_descr& f, const Byte* p,
                                                          Area* buffer, Area* hilfspuffer );
    void                            append_field_names  ( Area*, const Record_type& );
    void                            append_where_clause ( Area*, const Const_area& key );
    void                            execute_stmt        ( const Const_area& );

    void                            create_table_stmt   ();
    void                            create_index_stmt   ();
    void                            append_table_name   ( Area*, const char* name );
    void                            append_table_name   ( Area* area, const Sos_string& name )      { append_table_name( area, c_str( name ) ); }
    void                            append_quoted_name  ( Area*, const char* name );
    void                            append_quoted_name  ( Area* area, const Sos_string& name )      { append_quoted_name( area, c_str( name ) ); }

    Fill_zero                      _zero_;

    Sos_string                     _db_filename;
    Sos_string                     _sql_command;
    Sos_string                     _seq_filename;           // Dateiname für sequentiellen Zugriffs (select ohne where)
    Sos_string                     _tablename;
    Any_file                       _session;
    Any_file                       _file;
    Dynamic_area                   _stmt;
    Open_mode                      _open_mode;

    int                            _required_length;        // minimale Satzlänge bis zum Schlüssel

    Sos_ptr<Record_type>           _type;
  //Sos_ptr<Record_type>           _write_record_type_ptr;   // null-optimierung und readonly für insert
    Sos_ptr<Field_descr>           _key_descr_ptr;
    Sos_ptr<Record_type>           _key_type;           // wenn _key_descr_ptr->field_count() > 0
    Text_format                    _format;

    Bool                           _auto_commit;
    Bool                           _commit_at_end;
    int                            _store;                 // store() aktiv
    Bool                           _store_by_delete;
    Bool                           _empty_is_null;
    Bool                           _write_empty_as_null;
    Sos_string                     _identifier_quote_begin;
    Sos_string                     _identifier_quote_end;
    Dynamic_area                   _hilfspuffer;
    Sos_string                     _extra; // konstante Extrafelder

    Bool                           _create; // CREATE TABLE absetzen
    Bool                           _drop;
    Sos_simple_array<Sos_string>   _index_list;

};

//---------------------------------------------------------------------------------Sql_file_type

struct Sql_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "record/sql"; }
    virtual const char*         alias_name              () const { return "sql"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Sql_file> f = SOS_NEW_PTR( Sql_file );
        return +f;
    }
};

const Sql_file_type  _sql_file_type;
const Abs_file_type&  sql_file_type = _sql_file_type;

//----------------------------------------------------------------------------------append_name
// s.a. apppend_quoted_name()

void Sql_file::append_name( Area* buffer, const char* name )
{
    *buffer += _identifier_quote_begin;
    buffer->allocate_min( buffer->length() + strlen( name ) );

    char*       p = buffer->char_ptr() + buffer->length();
    const char* n = name;

    while(1) {
        char c = sos_toupper( *n++ );           // toupper() ist ein Oracle-Special
        if( !c ) break;
        *p++ = c == '-'? '_' : c;
    }

    buffer->length( p - buffer->char_ptr() );
    *buffer += _identifier_quote_end;
}

//-----------------------------------------------------------------Sql_file::append_quoted_name
// s.a. apppend_name()

void Sql_file::append_table_name( Area* stmt, const char* name )
{
    int len = length( name );
    
    stmt->append( name, len );
    Area( stmt->char_ptr() + stmt->length() - len, len ).upper_case();    // Oracle-Special
}

//-----------------------------------------------------------------Sql_file::append_quoted_name
// s.a. apppend_name()

void Sql_file::append_quoted_name( Area* stmt, const char* name )
{
    int len = length( name );
    
    stmt->append( _identifier_quote_begin );

    stmt->append( name, len );
    Area( stmt->char_ptr() + stmt->length() - len, len ).upper_case();    // Oracle-Special
    
    stmt->append( _identifier_quote_end );

}
// ------------------------------------------------------------------- Sql_file::Sql_file

Sql_file::Sql_file()
:
    _zero_ ( this+1 )
{
    _store_by_delete        = true;
  //_null_value_optimizing  = true;
}

// ------------------------------------------------------------------- Sql_file::~Sql_file

Sql_file::~Sql_file()
{
}

//------------------------------------------------------------------Sql_file::set_key_descr_ptr

void Sql_file::set_key_descr_ptr( Field_descr* field_descr )
{
    _key_descr_ptr = field_descr;
    if ( _key_descr_ptr ) {
        _required_length   = _key_descr_ptr->offset() + _key_descr_ptr->type().field_size();
        _key_type = SOS_CAST( Record_type, _key_descr_ptr->type_ptr() );
    }
}

//-----------------------------------------------------------------------Sql_file::prepare_open

void Sql_file::prepare_open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
     LOGBLOCK( "Sql_file::prepare_open" );

     Sos_string database;
   //Sos_string dialect;
     Sos_string date_format;  //jz 7.1.98 = std_date_format_ingres; // "dd-mon-yyyy";

     Sos_string date_time_format; 

     Sos_string field_names;
     Sos_string sql_modified_fields;

     for ( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
     {
        if ( opt.with_value( "table" ) )                _tablename = opt.value();
        else
      //if ( opt.with_value( "dialect" ) )              dialect  = opt.value(); // dto.
      //else
        if ( opt.with_value( "date-format" ) )          date_format = opt.value();
        else
        if ( opt.with_value( "date-time-format" ) )     date_time_format = opt.value();
        else
        if ( opt.with_value( "identifier-quote" ) )     _identifier_quote_begin = opt.as_char();
        else
        if ( opt.flag(       "store-by-delete" ) )      _store_by_delete  = opt.set();
        else  // Alternativ
        if ( opt.flag(       "store-by-update" ) )      _store_by_delete  = !opt.set();
        else
        if ( opt.flag(       "auto-commit" )     )      _auto_commit  = opt.set();
        else  // Alternativ
        if ( opt.flag(       "commit-at-end" )      )   _commit_at_end = opt.set();
        else
        if ( opt.flag(       "empty-is-null" )      )   _empty_is_null  = opt.set();
        else
        if ( opt.flag(       "write-empty-as-null" ) )  _write_empty_as_null = opt.set();
        else
        if ( opt.with_value( "sql-fields" ) )           field_names = opt.value();
        else
        if ( opt.with_value( "sql-command" ) )          _sql_command = opt.value();
        else
        if ( opt.with_value( "extra" ) )                _extra = opt.value();
        else
        if ( opt.flag(       "create" ) )               _create = opt.set();
        else
        if ( opt.flag(       "drop" ) )                 _drop = opt.set();
        else
        if ( opt.with_value( "idx" ) )                  _index_list.add( opt.value() );
        else
      //if ( opt.with_value( "sql-modify-fields" ) )    sql_modified_fields = opt.value();
      //else
        if ( opt.pipe() )                               _db_filename = opt.rest();
        else throw_sos_option_error( opt );
     }


     if ( _sql_command == empty_string ) {
        _sql_command = _db_filename;
     }

     if ( date_format      == "odbc" ) date_format      = std_date_format_odbc;        // "yyyy-mm-dd";
     if ( date_time_format == "odbc" ) date_time_format = std_date_time_format_odbc;   // "yyyy-mm-dd HH:MM:SS";

     //_identifier_quote_end = _identifier_quote_begin == "["? "]" : c_str( _identifier_quote_begin );
     _identifier_quote_end = _identifier_quote_begin;

     set_key_descr_ptr( +file_spec.key_specs()[ 0 ]._field_descr_ptr );

     _key_pos = file_spec.key_position();
     _key_len = file_spec.key_length();

     if( length( field_names ) > 2
      && field_names[ 0 ] == '('
      && field_names[ (int)length( field_names ) - 1 ] == ')' )
     {
         field_names = as_string( c_str( field_names ) + 1,
                                  length( field_names ) - 2 );
     }

     if( !file_spec._field_type_ptr )   // Kein Select mit Ergebnismenge, also SQL-Anweisungen werden geschrieben
     {
        _session.open( _db_filename, Any_file::Open_mode( open_mode & Any_file::inout ) );

        if( length( _identifier_quote_begin ) == 0 ) {
            _identifier_quote_begin = _session.identifier_quote_begin();
            _identifier_quote_end   = _session.identifier_quote_end();
        }
        if( length( date_format ) == 0 )  date_format = _session.date_format();
        
        if( length( date_time_format ) == 0 )  date_time_format = _session.date_time_format();


        // Satzbeschreibung von der Datenbank holen:
        _seq_filename = _db_filename;

        _seq_filename += " SELECT ";

        if( empty( field_names ) )  _seq_filename += "*";
        else {
            Dynamic_area stmt(1024);
            Sos_string field_name;

            const char* p = c_str( field_names );
            while(1) {
                while( sos_isspace( *p ) )  p++;
                
                const char* q = p;
                while( *q  &&  !sos_isspace( *q )  &&  *q != '.'  &&  *q != ',' )  q++;
                field_name = as_string( p, q-p );
                int len = length(field_name);
                if ( len >= 2 ) {
                    if ( field_name[0] == '"' && field_name[len-1] == '"' ) field_name = as_string(c_str(field_name)+1,len-2);
                }
                append_quoted_name( &stmt, c_str(field_name) );
                p = q;

                while( sos_isspace( *p ) ) p++;
                if( !*p )  break;
                if( *p == '.' || *p == ',' ) { stmt.append( *p++ ); }
            }
            // Zuweisen ...
            append( &_seq_filename, stmt.char_ptr(), stmt.length() );

        }

        _seq_filename += " FROM ";
        {
            Dynamic_area t(50);
            Sos_string tablename;
            append_table_name( &t, c_str( _tablename ) );
            tablename = as_string(t);
            _seq_filename += tablename;
        }

        if( _open_mode & Any_file::in  &&  _open_mode & Any_file::seq ) 
        {
            _file.open( _seq_filename, Any_file::Open_mode( Any_file::in | Any_file::seq ) );
            _type = +_file.spec()._field_type_ptr;
        }
        else 
        {
            LOGI( "Satzbeschreibung der Tabelle " << _tablename << " wird ermittelt\n" );
            Any_file f;
            f.open( _seq_filename + " WHERE 1=0", Any_file::in );
            _type = +f.spec()._field_type_ptr;
            f.close();
        }

        if( !_type )  throw_xc( "SOS-1171", "sql" );

        if( empty( _type->_name ) ) {
            _type->_name = _tablename;
            if( !empty( field_names ) )  _type->_name += "/selected";
        }

        _any_file_ptr->_spec._field_type_ptr = +_type;
     }
     else
     if( empty( field_names ) ) {
         _type = +file_spec._field_type_ptr;
     }
     else
     {
         // Eigentlich sollten hier die Feldtypen von der Datenbank geholt werden ...
         // Wir nehmen die Typen vom Aufrufer. jz
         _type = renamed_fields( file_spec._field_type_ptr, field_names,
                                 file_spec._field_type_ptr->name() + "/sql" );
         if( _key_type ) {
             for( int i = 0; i < _key_descr_ptr->type().field_count(); i++ ) {
                 _key_type->field_descr_ptr( i )->name(
                     _type->field_descr_ptr(
                        file_spec._field_type_ptr->field_index(
                            _key_type->field_descr_ptr( i )->name()
                        )
                     )->name()
                 );
             }
         }

     }

     if( length( _identifier_quote_begin ) == 0 ) { // immer noch keine Quotes?
        _identifier_quote_begin = "\"";
        _identifier_quote_end   = "\"";
     }

     if( length( date_format      ) == 0 ) date_format = std_date_format_ingres;      // "dd-mon-yyyy";
     if( length( date_time_format ) == 0 ) date_time_format = std_date_time_format_ingres; // "dd-mon-yyyy HH:MM:SS";

     _format.date( c_str( date_format ) );
     _format.date_time( c_str( date_time_format ) );


     //_modify_fields( sql_modified_fields ); // Read-only auswerten

     _open_mode = open_mode;
    //_file_spec = file_spec;
}


//-------------------------------------------------------------------------------append_sql_type

static void append_sql_type( Dynamic_area* area_ptr, const Field_descr& descr )
{
    // Oracle-Style
    uint field_size = descr.type().field_size();
    if ( descr.type().obj_is_type( tc_String0_type ) ) field_size--; // OHNE Null-Byte

    switch( descr.type().info()->_std_type ) {
    case std_type_bool:         *area_ptr += "NUMBER(1)"; break;
    case std_type_char:         
        if ( field_size > 255 ) *area_ptr += "LONG VARCHAR"; 
        else { *area_ptr += "CHAR(";  area_ptr->append( as_string( field_size ) ); *area_ptr += ")"; } 
        break;
    case std_type_varchar:      
        if ( field_size > 2000 ) *area_ptr += "LONG VARCHAR"; // Oracle kann bei Varchar bis zu 2K
        else { *area_ptr += "VARCHAR2(";  area_ptr->append( as_string( field_size ) ); *area_ptr += ")"; } 
        break;
    case std_type_decimal:      *area_ptr += "NUMBER(8,2)";break;
    case std_type_integer:      *area_ptr += "NUMBER(10)"; break;
    case std_type_float:        *area_ptr += "NUMBER"; break;
    case std_type_date:         *area_ptr += "DATE"; break;
    case std_type_time:         *area_ptr += "DATE"; break;
    case std_type_date_time:    *area_ptr += "DATE"; break;
    default:                    *area_ptr += "CHAR(250)"; break;
    }

    if ( !descr.nullable() ) {
        *area_ptr += " NOT NULL";
    }

}

//-------------------------------------------------------------------------------Sql_file::create_table_stmt


void Sql_file::create_table_stmt()
{
    Dynamic_area drop_stmt;
    Sos_string  single_key_name;
    Bool        single_key = false;
    int         i;

    Dynamic_area create_stmt( max_stmt_length );
    Bool erstes_feld;
    
    if ( _tablename == "" ) throw_xc( "NoTable" );
    drop_stmt.assign( "DROP TABLE " );
    append_table_name( &drop_stmt, _tablename );
    drop_stmt.append( ";" );

    create_stmt.assign( "CREATE TABLE " );
    append_table_name( &create_stmt, _tablename );
    create_stmt += " ( ";

    if ( !(_open_mode & out) ) throw_xc( "D403" );

    if ( _key_type && _key_type->field_count() == 1 ) {
        single_key = true;
        single_key_name = _key_type->field_descr( 0 ).name();
    }

    erstes_feld = true;
    for( i = 0; i < _type->field_count(); i++ )
    {
        const Field_descr& f = _type->field_descr( i );
        if( !erstes_feld )  create_stmt += ", ";
        erstes_feld = false;
        append_name( &create_stmt, f.name() );
        create_stmt += " ";
        append_sql_type( &create_stmt, f );
        if ( single_key && f.name() == single_key_name ) {
            create_stmt += " PRIMARY KEY";    
        }                                                     
    }

    if ( _key_type && !single_key ) {
        // Oracle-Style
        create_stmt += ", PRIMARY KEY ( ";
        for( i = 0; i < _key_type->field_count(); i++ ) {
            if( i > 0 )  create_stmt += ",";
            const Field_descr& f = _key_type->field_descr( i );
            append_name( &create_stmt, f.name() );
        }
        create_stmt += " )";
    }

    for ( i=_index_list.first_index(); i <= _index_list.last_index(); i++ ) {
        create_stmt += ", UNIQUE (";
        create_stmt += _index_list[i];
        create_stmt += ")";
    }

    create_stmt += " );";


    if( _drop ) {
        try {
            LOG( "Sql_file: drop_stmt= " << drop_stmt << '\n' );
            execute_stmt( drop_stmt );
        }
        catch( const Xc& ) {}
        try {
            //drop_stmt.lower_case();
            //LOG( "Sql_file: drop_stmt= " << drop_stmt << '\n' );
            //execute_stmt( drop_stmt );
        }
        catch( const Xc& ) {}
    }

    LOG( "Sql_file: create_stmt= " << create_stmt << '\n' );
    execute_stmt( create_stmt );

}

//------------------------------------------------------------------------------veraltetes_open

void Sql_file::veraltetes_open()
{
    //LOG( "DATEITYP SQL WIRD BENUTZT, UM KOMMANDOS IN DIE DATENBANK ZU SCHREIBEN\n" );

    _file.open( _sql_command,
                Any_file::Open_mode( ( _open_mode & ~Any_file::in & ~Any_file::binary) | Any_file::trunc ) );
    if( _create ) { create_table_stmt(); }
}

//-------------------------------------------------------------------------------Sql_file::open

void Sql_file::open( const char*, Open_mode, const File_spec& file_spec )
{
    if( !_key_type )  set_key_descr_ptr( +file_spec.key_specs()[ 0 ]._field_descr_ptr );

    if( (_open_mode & in) && (_open_mode & seq) )
    {
        //jz 10.9.97 _file.open( _db_filename,
        //            Any_file::Open_mode( ( _open_mode & ~Any_file::in ) | Any_file::trunc ) );
    } 
    else 
    if( _open_mode & out ) 
    {
        if( _create  ||  _sql_command != _db_filename )  veraltetes_open();
    }
}

//----------------------------------------------------------------------------------Sql_file::modify_fields
/*
void Sql_file::_modify_fields( const Sos_string& sql_modified_fields )
{
    if ( sql_modified_fields == empty_string ) {
        _write_record_type_ptr = _type;
        return;
    }

    Sos_string read_only_fields;
    char* p_start = (char*) c_str( sql_modified_fields );
    char* p = strchr( p_start, ':' );
    if ( !p ) throw_syntax_error( "sql_modified_fields" );

    if ( strcmpi(p+1,"ro") == 0 || strcmpi(p+1,"read-only") == 0 ) {
        read_only_fields = as_string( p_start, p-p_start );
    }

    if ( read_only_fields == empty_string ) {
        _write_record_type_ptr = _type;
    } else {
        _write_record_type_ptr = record_type_without_fields( _type, read_only_fields );
    }
}
*/
// ----------------------------------------------------------------------------- Sql_file::close

void Sql_file::close( Close_mode close_mode )
{
    if( _file.opened() )
    {
        if( _open_mode & out ) {
            if ( close_mode == close_error )  rollback();
            else
            if ( _commit_at_end  )            commit();
        }

        _file.close( close_mode );
    }

    _file.destroy();
}

// -------------------------------------------------------------------------- Sql_file::get_record

void Sql_file::get_record( Area& area )
{
    if( !_file.opened()  &&  length( _seq_filename ) ) {
        _file.open( _seq_filename, Any_file::Open_mode( Any_file::in | Any_file::seq ) );
    }

    _file.get( area );              // nur für (in seq)
}

// --------------------------------------------------------------------- Sql_file::get_record_key

//#define SAN_DEBUG 1

inline Bool check_stmt( const Area& stmt ) {
#if defined SAN_DEBUG
    const char* p_end = stmt.byte_ptr() + length(stmt);
    LOG( "check_stmt=" << stmt << '\n' );
    for ( const char* p = stmt.char_ptr(); p < p_end; p++ ) {
        if ( !isprint((int)*p) ) {
            LOG( " => failed at " << (int)(p-stmt.byte_ptr()) << '\n' );
            return false;
        }
    }
    return true;
#else
    return true;
#endif
}

#define CHECK_STMT check_stmt( _stmt )

void Sql_file::get_record_key( Area& area, const Key& key )
{
    if ( !_key_descr_ptr ) throw_xc( "SOS-1214", "sql" );  // Key-Beschreibung muß vorhanden sein!

    Any_file        f;
    Dynamic_area    buffer;

    _stmt.allocate_min( max_stmt_length );


    _stmt.assign( "SELECT " );
    append_field_names( &_stmt, *_type );
    _stmt.append( " FROM " );

    append_table_name( &_stmt, _tablename );

    append_where_clause( &_stmt, key );

    LOG( "Sql_file::get_record_key: " << _stmt << '\n' );

    try {
        f.open( _db_filename + " " + as_string( _stmt ), // js: Temporärer String => Probleme?
                Any_file::Open_mode( Any_file::in | Any_file::seq ) );
    } catch ( Xc& x ) {
        x.insert( c_str(_tablename) );
        throw;
    }

    try {
        f.get( &buffer );
    }
    catch ( const Eof_error& ) { throw_not_found_error(); }

    area.allocate_min( _type->field_size() );
    LOG( "_type->field_count()=" << _type->field_count() <<
         ",f.spec().field_type_ptr()->field_count()=" << f.spec().field_type_ptr()->field_count() << "\n" );
    copy_record( _type, area.byte_ptr(),
                 f.spec().field_type_ptr(), buffer.byte_ptr(), _empty_is_null );
    area.length( _type->field_size() );
}

// ----------------------------------------------------------------Sql_file::append_field_names

void Sql_file::append_field_names( Area* buffer, const Record_type& record_type )
{
    Bool erstes_feld;

#if defined SAN_DEBUG
    erstes_feld = true;
    for( int l = 0; l < record_type.field_count(); l++ )
    {
        const Field_descr& field_descr = record_type.named_field_descr( l );
        if( !erstes_feld )  LOG( ',' );
        erstes_feld = false;
        LOG( field_descr.name() );
    }
    LOG( '\n' );
#endif

    erstes_feld = true;
    for( int i = 0; i < record_type.field_count(); i++ )
    {
        const Field_descr& field_descr = record_type.named_field_descr( i );
        //if( !field_descr.read_only() ) {
            if( !erstes_feld )  *buffer += ',';
            erstes_feld = false;
            append_name( buffer, field_descr.name() );
        //}
#if defined SAN_DEBUG
if ( !check_stmt( *buffer ) ) {
    LOG( "Feld=" << field_descr.name() << '\n' );
}
#endif
    }
}

// ----------------------------------------------------------------------- Sql_file::put_record

void Sql_file::put_record( const Const_area& area )
{
    insert( area );  // Satz sequentiell rausschreiben (Kopieren einer Tabelle ?)
}

// --------------------------------------------------------------------------- Sql_file::insert

void Sql_file::insert( const Const_area& record )
{
/* Verbesserung:
   VALUES-Liste und Namensliste parallel in zwei Strings aufbauen,
   dabei _write_empty_as_null berücksichtigen.
*/
    LOGBLOCK( "Sql_file::insert()" );
    int  i;
    Bool erstes_feld;

    if ( !(_open_mode & out) ) throw_xc( "D403" );


    _stmt.allocate_min( max_stmt_length );

    _stmt.assign( "INSERT INTO " );
    append_table_name( &_stmt, _tablename );
    _stmt.append( " (" );

    erstes_feld = true;
    for( i = 0; i < _type->field_count(); i++ )
    {
        const Field_descr& f = _type->field_descr( i );
        if( !f.read_only() && !f.null( record.byte_ptr() ) ) {
            if( !erstes_feld )  _stmt += ',';
            erstes_feld = false;
            append_name( &_stmt, f.name() );
        }
    }
    // Extra-Feld Behandlung
    if ( length(_extra) != 0 ) {
        const char* s = c_str(_extra);
        const char* p = strchr( s, '=' );
        if ( p ) {
            Sos_string feld = as_string( s, p-s );
            if( !erstes_feld )  _stmt += ',';
            _stmt.append( c_str(feld) );
        } else throw_syntax_error( "extra" );
    }

    _stmt.append( ") VALUES (" );

    erstes_feld = true;
    try {
        for( i = 0; i < _type->field_count(); i++ )
        {
            const Field_descr& f = _type->field_descr( i );
            if( !f.read_only() && !f.null( record.byte_ptr() ) ) {
                if( !erstes_feld )  _stmt += ',';
                erstes_feld = false;
                append_value( f, record.byte_ptr(), &_stmt, &_hilfspuffer );
            }
        }
    }
    catch ( Xc& x ) {
        x.insert( c_str(_type->field_descr( i ).name()) );
        throw;
    }

    // Extra-Feld Behandlung
    if ( length(_extra) != 0 ) {
        const char* s = c_str(_extra);
        const char* p = strchr( s, '=' );
        if ( p ) {
            Sos_string value = p+1;
             if( !erstes_feld )  _stmt += ',';
             _stmt.append( c_str(value) );
        } else throw_syntax_error( "extra" );
    }

    _stmt += ')';

    try {
        execute_stmt( _stmt );
    }
    catch( Xc& x )
    {
        if ( strcmpi( x.code(), "INGRES-125E" ) == 0 ) { x.insert( c_str(_tablename) ); throw; }
        throw;
    }
}

//------------------------------------------------------------------------Sql_file::print_value

void Sql_file::append_value( const Field_descr& f, const Byte* p, Area* buffer, Area* hilfspuffer )
{
    if( f.null( p ) ) 
    {
        buffer->append( "NULL" );
    } 
    else 
    {
        Area                rest ( buffer->char_ptr() + buffer->length(), buffer->size() - buffer->length() );
        const Type_info*    info = f.type().info();

        if( !info->_quote
         || _format.date()[ 0 ] == '{'    // ODBC-Datumsformat  {d'yyyy-mm-dd'}  {dt'..'}  {t'...'}  jz 7.1.98
         && (    info->_std_type == std_type_date 
              || info->_std_type == std_type_date_time 
              || info->_std_type == std_type_time      ) )

        {
            f.write_text( p, &rest, _format );
        }
        else 
        {
            f.write_text( p, hilfspuffer, _format );

            if( hilfspuffer->length() == 0 )
            {
                if( _write_empty_as_null  &&  ( !_key_type  ||  !_key_type->field_descr_by_name_or_0( f.name() ) ) )       // Schlüsselfeld nicht NULL speichern!
                {
                    buffer->append( "NULL" );
                    return;
                }

                // Geht nicht, String0 ist VARCAHR.  jz 25.12.01 if( f.type_ptr()->info()->_std_type == std_type_char )  hilfspuffer->assign( " " );      // Für Oracle: "" ist gleichbedeutend mit NULL, " " aber nicht.
            }

            write_string( *hilfspuffer, &rest, '\'', '\'' );
        } 
        buffer->length( buffer->length() + rest.length() );
    }
}

// ---------------------------------------------------------------Sql_file::append_where_clause

void Sql_file::append_where_clause( Area* buffer, const Const_area& key )
{
    //LOGBLOCK( "Sql_file::print_where_field_names()" );
    if ( !_key_descr_ptr ) throw_xc( "SOS-1214", "sql" );

    buffer->append( " WHERE " );

    for( int i = 0; i < _key_type->field_count(); i++ )
    {
        if( i > 0 )  buffer->append( " AND " );
        const Field_descr& f = _key_type->named_field_descr( i );
        append_name( buffer, f.name() );
        *buffer += '=';
        if ( f.null( key.byte_ptr() ) ) {
            Sos_string name = _key_descr_ptr->name();
            name += '.';
            name += f.name();
            throw_xc( "SOS-1220", c_str( name ) );
        }

        append_value( f, key.byte_ptr(), buffer, &_hilfspuffer );
    }
}

//----------------------------------------------------------------------Sql_file::update_direct

void Sql_file::update_direct( const Const_area& record )
{
    // Sql_file::store() ruft auch diese Funktion
    LOGBLOCK( "Sql_file::update_direct()" );

    if ( !(_open_mode & out) ) throw_xc( "D403" );
    if ( !_key_descr_ptr )     throw_xc( "SOS-1214", "sql" );

/*
    if ( !_update_record_type_ptr ) {
        _update_record_type_ptr = record_type_without_key_fields( _type, _key_descr_ptr );
    }
*/

    _stmt.allocate_min( max_stmt_length );

    _stmt.assign( "UPDATE " );

    append_table_name( &_stmt, _tablename );

    _stmt.append( " SET " );

    Bool erstes_feld = true;
    for( int i = 0; i < _type->field_count(); i++ ) {
        const Field_descr& f = _type->field_descr( i );
        if( !f.read_only() ) {
            if( !erstes_feld )  _stmt += ',';
            erstes_feld = false;

            append_name( &_stmt, f.name() );
            _stmt += '=';
            append_value( f, record.byte_ptr(), &_stmt, &_hilfspuffer );
        }
    }

    // Extra-Feld Behandlung
    if ( length(_extra) != 0 ) {
        if( !erstes_feld )  _stmt += ',';
        _stmt.append( c_str(_extra) );
    }

    if( (int)( record.length() - _key_descr_ptr->offset() ) < (int)_key_type->field_size() )  throw_xc( "SOS-1229" );

    Const_area key ( record.byte_ptr() + _key_descr_ptr->offset(), _key_type->field_size() );

    append_where_clause( &_stmt, key );

    try {
        execute_stmt( _stmt );

        if( _file.record_count() == 0 )  throw_not_found_error();   //jz 28.8.2001  Warum fehlte das?
    }
    catch( const Not_found_error& ) { throw; }
    catch( Xc& x )
    {
        if ( strcmpi( x.code(), "INGRES-125E" ) == 0 ) { x.insert( c_str(_tablename) ); throw; }
        if ( strcmpi( x.code(), "INGRES-0000" ) != 0 )  throw;

        LOG( "Sql_file: INGRES-0000 soll Not_found_error sein!\n" );
        throw_not_found_error( "INGRES-0000", this );
    }
}

// ---------------------------------------------------------------------------- Sql_file::store

void Sql_file::store( const Const_area& record )
{
    Increment_semaphore<int> _x_ ( &_store );   // Für -auto-commit

    if ( !_key_descr_ptr ) throw_xc( "SOS-1214", "sql" );
    if( record.length() < _required_length )   throw_xc( "SOS-1215", record.length(), _required_length );

    try {
        if ( _store_by_delete )
        {
            try {
                del( Const_area( record.byte_ptr() + _key_descr_ptr->offset(),
                                 _key_descr_ptr->type().field_size() ) );
            }
            catch ( const Not_found_error& )
            {
                // ignorieren
            }
            insert( record );
        } else
        {
            try { update_direct( record ); }
            catch( const Not_found_error& ) { insert( record ); }
        }

        if ( _auto_commit ) commit();
    }
    catch(...) {
        if( _auto_commit )  rollback();
        throw;
    }
}

// ------------------------------------------------------------------------------ Sql_file::del

void Sql_file::del( const Key& key )
{
    if ( !(_open_mode & out) ) throw_xc( "D403" );

    _stmt.allocate_min( max_stmt_length );

    _stmt.assign( "DELETE FROM " );

    append_table_name( &_stmt, _tablename );

    append_where_clause( &_stmt, key );

    execute_stmt( _stmt );
}

// --------------------------------------------------------------------------- Sql_file::commit

void Sql_file::commit()
{
    if( !_file.opened() )  veraltetes_open();
    _file.put( "COMMIT" );
}

// ------------------------------------------------------------------------- Sql_file::rollback

void Sql_file::rollback()
{
    if( !_file.opened() )  veraltetes_open();
    _file.put( "ROLLBACK" );
}

//-----------------------------------------------------------------------Sql_file::execute_stmt

void Sql_file::execute_stmt( const Const_area& stmt )
{
    if( !_file.opened() )  veraltetes_open();

    if( !_auto_commit  || _store )
    {
        _file.put( stmt );
    }
    else {
        try {
            _file.put( stmt ); // und ab damit
            commit();
        }
        catch( const Xc& ) {
            rollback();
            throw;
        }
        catch( const exception& ) {
            rollback();
            throw;
        }
    }
}

} //namespace sos
