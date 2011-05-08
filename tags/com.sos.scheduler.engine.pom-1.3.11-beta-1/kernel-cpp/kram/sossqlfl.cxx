#include "precomp.h"
//#define MODULE_NAME "sossqlfl"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../file/sosdb.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../kram/sossql2.h"

using namespace std;
namespace sos {

//--------------------------------------------------------------------------------Sossql_static

struct Sossql_static : Sos_database_static
{
                                Sossql_static           () : _log_eval( false ) {}

    Bool                       _log_eval;
};

//----------------------------------------------------------------------------------Sossql_file

struct Sossql_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "sossql"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Sossql_file> f = SOS_NEW( Sossql_file );
        return +f;
    }
};

const Sossql_file_type  _sossql_file_type;
const Abs_file_type&     sossql_file_type = _sossql_file_type;

//------------------------------------------------------------------------Sossql_session::_open

void Sossql_session::_open( Sos_database_file* database_file )
{
    _dbms = dbms_sossql;
    _dbms_name = "SOSSQL";

    Sossql_file* file = (Sossql_file*)database_file;

    if( !empty( file->_catalog_name ) ) 
    {
        _catalog_name = file->_catalog_name;
    }
    else
    {
        if( empty( _db_name ) ) {
            _catalog_name = "com | sossql_profile_catalog alias"; // oder 'sossql tables' ?
            // Nicht öffnen! sossql_profile_catalog nutzt sossql-select, das gibt eine Schleife
        } else {
            _catalog_name = "com | " + read_existing_profile_string( "", "sossql catalogs", c_str( _db_name ) );
            // Katalog offenhalten:
            _catalog.open( _catalog_name, Any_file::in );
        }
    }

    {
        Sos_string fname = as_string( c_str( _catalog_name ) + 6, length( _catalog_name ) - 6 );  // "com | "
        Sos_option_iterator opt ( fname );
        while( !opt.end() ) {
            if( opt.param() )  break;      // Bis Dateityp kommt
            else
            if( opt.with_value( opt.option() ) )  opt.value();
            opt.next();
        }
        fname = opt.rest();
        const char* p = c_str( fname ) + length( fname );
        while( p > c_str( fname )  &&  p[-1] != '|' )  p--;
        _filename_prefix = as_string( c_str( fname ), p - c_str( fname ) );
    }

    //const char* p = strstr( c_str( _catalog_name ), "-+- " );

    LOG( "Sossql_session::_open: _filename_prefix=" << _filename_prefix << "\n" );
}

//-----------------------------------------------------------------------Sossql_session::_close

void Sossql_session::_close( Close_mode )
{
}

//---------------------------------------------------------------Sossql_session::translate_limit

string Sossql_session::translate_limit( const string& stmt, int limit )
{
    _limit = limit;
    return stmt;
}

//---------------------------------------------------------------Sossql_session::_equal_session

Bool Sossql_session::_equal_session( Sos_database_file* database_file ) 
{
    // _db_name == file->_catalog_name !

    Sossql_file* file = (Sossql_file*)database_file;

    return !empty( _db_name )  ||  _catalog_name == file->_catalog_name;
}

// -------------------------------------------------------------Sossql_session::_execute_direct

void Sossql_session::_execute_direct( const Const_area& statement )
{
    //istrstream input_stream ( (char*)statement.char_ptr(), length( statement ) );

    //Sos_ptr<Sql_stmt> stmt = Sql_stmt::create( &input_stream, empty_string );
    Sos_ptr<Sql_stmt> stmt = Sql_stmt::create( as_string(statement), empty_string );
    stmt->_session = this;
    stmt->prepare();
    stmt->execute();
    if( _current_file )  _current_file->_row_count = stmt->_row_count;
    stmt->close();
}

//--------------------------------------------------------------------Sossql_session::_rollback

void Sossql_session::_rollback()
{
//rollback() wird schon bei close() gerufen ....    throw_xc( "SOSSQL-ROLLBACK?" );
}

//---------------------------------------------------------------------Sossql_file::Sossql_file

Sossql_file::Sossql_file()
:
    _zero_ ( this+1 )
{
}

//--------------------------------------------------------------------Sossql_file::~Sossql_file

Sossql_file::~Sossql_file()
{
    session_disconnect();
}

//------------------------------------------------------------------Sossql_file::_create_static

void Sossql_file::_create_static()
{
    Sos_ptr<Sossql_static> p = SOS_NEW( Sossql_static );
    //p->_log_eval = read_profile_bool( "", "sossql", "log-eval", false );
    _static = p;
}

//-----------------------------------------------------------------Sossql_file::_create_session

Sos_ptr<Sos_database_session> Sossql_file::_create_session()
{
    Sos_ptr<Sossql_session> p = SOS_NEW( Sossql_session );
    return +p;
}

//--------------------------------------------------------------------Sossql_file::prepare_open

void Sossql_file::prepare_open( const char* param, File_base::Open_mode open_mode, const File_spec& )
{
    Sos_string statement;
    Bool       catalog                  = false;
    Bool       types                    = false;
//    int        fSqlType = 0;
    Bool       log_eval                 = false;
    Sos_string empty_join_log;
    Bool       need_result_key          = false;
    Bool       order_tables             = true;
    uint       max_result_field_length  = max_sossql_field_size;
    uint       max_field_length         = max_sossql_field_size;
    int        max_reads                = INT_MAX;

    _max_records = INT_MAX;
    _limit = INT_MAX;


    _open_mode = open_mode;

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( database_option( opt ) )           {}
        else
        if( opt.flag      ( "catalog"                 ) )  catalog = opt.set();         // Liefert den Katalog der Tabellen
        else
        if( opt.with_value( "catalog"                 ) )  _catalog_name = opt.value();  // Stellt den Katalog direkt ein, statt -db=
        else
        if( opt.flag      ( "types"                   ) )  types   = true;
        else
        if( opt.with_value( "empty-join-log"          )
         || opt.with_value( "error-log"               ) )  empty_join_log = opt.value();
        else
        if( opt.flag      ( "log-eval"                ) )  log_eval = opt.set();
        else
        if( opt.flag      ( "key"                     ) )  need_result_key = opt.set();
        else
        if( opt.flag      ( "order-tables"            ) )  order_tables = opt.set();
        else
        if( opt.with_value( "max-field-length"        ) )  max_field_length = opt.as_uintK();
        else
        if( opt.with_value( "max-result-field-length" ) )  max_result_field_length = opt.as_uintK();
        else
        if( opt.with_value( "max-records"             ) )  _max_records = opt.as_uintK();
        else
        if( opt.with_value( "max-reads"               ) )  max_reads = opt.as_uintK();
        else
        if( opt.param() )                                  { statement = opt.rest(); break; }
        else 
            throw_sos_option_error( opt );
    }

    if( !empty( _db_name )  &&  !empty( _catalog_name ) ) {
        // jz 3.9.01: throw_xc wieder eingebaut. js 14.7.98: 
        throw_xc( "SOS-1385", c_str( _db_name ), c_str( _catalog_name ) );
        // _catalog_name hat Vorrang. Geht überhaupt beides? js
        //jz LOG( "Sossql_file::prepare_open: Fehler? catalog='" << c_str( _catalog_name ) << "' vs. db=" << c_str( _db_name ) << '\n' );
        //jz _db_name = "";
    }

    session_connect( "sossql" );

    if( catalog ) {
        if( !empty( statement ) )  throw_xc( "SOSSQL-SYNTAX" );

        _file.prepare( session()->_catalog_name, open_mode );
        _any_file_ptr->new_file( &_file ); // Sossql_file durch _file ersetzen
        return;
    }
    else
    if( types ) {
        if( !empty( statement ) )  throw_xc( "SOSSQL-SYNTAX" );

        _file.prepare( "odbc_data_types:", open_mode );
        _any_file_ptr->new_file( &_file ); // Sossql_file durch _file ersetzen
    }
    else
    if( !empty( statement ) )
    {
        //istrstream input_stream ( (char*)c_str( statement ), length( statement ) );
        //istream*   input = &input_stream;

        {
            Dynamic_area stmt;
            ((Sossql_session*)+_session)->_limit = _limit;  // Kann von convert_stmt() über translate_limit() ersetzt werden. Für %limit().
            _session->convert_stmt( Const_area( c_str( statement ), length( statement ) ), &stmt );
            statement = string( stmt.char_ptr(), length( stmt ) );
            _limit = ((Sossql_session*)+_session)->_limit;
        }

        //_stmt = Sql_stmt::create( input, empty_join_log );
        _stmt = Sql_stmt::create( statement, empty_join_log );

        _stmt->_session             = (Sossql_session*) +_session;
        _stmt->_log_eval            = log_eval || ((Sossql_static*)+_static)->_log_eval;
        _stmt->_max_field_length    = max_field_length;
        _stmt->_order_tables        = order_tables;
        _stmt->_max_reads           = max_reads;

        if( _stmt->obj_is_type( tc_Sql_select )  )
        {
            _select_stmt = (Sql_select*)+_stmt;

            _select_stmt->_sossql_file              = this;
            _select_stmt->_need_result_key          = need_result_key;
            _select_stmt->_max_result_field_length  = max_result_field_length;

            _stmt->prepare();

            if( _select_stmt->_resolve_star )      // select ...,tabelle.*,... ?
            {     
                // tabelle.* in die einzelnen Felder auflösen: Dazu bauen wir eine neue Satzbeschreibung:
                Sos_ptr<Record_type> type = Record_type::create();
                type->name( _select_stmt->_result_record_type->name() );  // Namen übernehmen

                for( int i = 0; i < _select_stmt->_result_record_type->field_count(); i++ ) 
                {
                    Sql_expr* expr = _select_stmt->_result_expr_array[ i ];
                    if( expr->_operator == op_field  &&  ((Sql_field_expr*)expr)->_resolve_star )   // tabelle.* ?
                    {
                        Record_type* t = SOS_CAST( Record_type, ((Sql_field_expr*)expr)->_field_descr->type_ptr() );
                        for( int j = 0; j < t->field_count(); j++ ) {
                            //1.11.98 (egcs) Sos_ptr<Field_descr> field = obj_copy( *t->field_descr_ptr( j ) );
                            Sos_ptr<Field_descr> field = OBJ_COPY( Field_descr, *t->field_descr_ptr( j ) );
                            field->_offset += _select_stmt->_result_record_type->field_descr_ptr( i )->_offset;
                            type->add_field( field );  
                        }
                    } 
                    else 
                    {
                        type->add_field( _select_stmt->_result_record_type->field_descr_ptr( i ) );
                    }
                }

                _any_file_ptr->_spec._field_type_ptr = +type;
            } 
            else 
            {
                _any_file_ptr->_spec._field_type_ptr = +_select_stmt->_result_record_type;
            }

            //if( _select_stmt->_result_key_descr ) {
            //    // Nur, wenn die Schlüsselfelder aller Tabellen selektiert sind!
            //    _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = +_select_stmt->_result_key_descr;
            //    _key_len = _select_stmt->_result_key_descr->type().field_size();
            //    _key_pos = _select_stmt->_result_key_descr->offset();
            if( _select_stmt->_key_len ) {
                _key_len = _select_stmt->_key_len;
                _key_pos = _select_stmt->_key_pos;
            } else {
                if( _select_stmt->_rowid_len ) {
                    _key_len = _select_stmt->_rowid_len;
                    _key_pos = -1;
                    current_key_ptr( &_select_stmt->_rowid );
                }
            }
        }
        else
        {
            _stmt->prepare();
        }
    }
}

//-----------------------------------------------------------------Sossql_file::bind_parameters

void Sossql_file::bind_parameters( const Record_type* param_record_type,
                                   const Byte* param_base )
{
    _stmt->bind_parameters( param_record_type, param_base );
}

//----------------------------------------------------------------------------Sossql_file::open

void Sossql_file::open( const char*, File_base::Open_mode, const File_spec& )
{
    if( _stmt ) {
        if( _select_stmt ) {
            ////_select_stmt->_callers_key_descr = spec._key_specs._key_spec._field_descr_ptr;
            //_any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = +_select_stmt->_result_key_descr;
        }
        _stmt->execute();
        _any_file_ptr->_record_count = _stmt->_row_count;
    } else {
        // Anweisungen (insert, delete etc.) werden mit put_record übergeben
    }
}

//----------------------------------------------------------------------------Sossql_file::open

void Sossql_file::execute()
{
    _stmt->execute();
    _any_file_ptr->_record_count = _stmt->_row_count;
    _stmt->close();
}

//--------------------------------------------------------------------Sossql_file::prepare_open

void Sossql_file::close( Close_mode )
{
    if( _stmt )  _stmt->close();
    session_disconnect();
}

//----------------------------------------------------------------------Sossql_file::get_record

void Sossql_file::get_record( Area& buffer )
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );
    
    if( _record_count >= _limit )  throw_eof_error();
    if( _record_count >= _max_records )  throw_xc( "SOS-SQL-92", _max_records );
    _record_count++;

    _select_stmt->fetch( &buffer );
}

//--------------------------------------------------------------------Sossql_file::get_position

void Sossql_file::get_position( Area* buffer )
{
    if( _record_count >= _limit )  throw_eof_error();
    if( _record_count > _max_records )  throw_xc( "SOS-SQL-92", _max_records );
    _record_count++;

    Abs_file::get_position( buffer );
    //if( !_select_stmt )  throw_xc( "SOS-1262" );
    //_select_stmt->get_position( &buffer );
}

//--------------------------------------------------------------------------Sossql_file::rewind

void Sossql_file::rewind( Key::Number )
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );
    _select_stmt->rewind();
}

//------------------------------------------------------------------Sossql_file::get_record_key

void Sossql_file::get_record_key( Area& buffer, const Key& key )
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );

    if( _record_count >= _limit )  throw_eof_error();
    if( _record_count > _max_records )  throw_eof_error( "SOS-SQL-92", _max_records );
    _record_count++;

    _select_stmt->get_key( &buffer, key );
}

//-----------------------------------------------------------------------------Sossql_file::set

void Sossql_file::set( const Key& key )
{
    if( !_select_stmt )  { Abs_file::set( key ); return; }
    _select_stmt->set_key( key );
}

//--------------------------------------------------------------------------Sossql_file::update

void Sossql_file::update( const Const_area& record )
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );

    _select_stmt->update( record );
}

//--------------------------------------------------------------------------Sossql_file::insert

void Sossql_file::insert( const Const_area& record )
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );
    _select_stmt->insert( record );
}

//-----------------------------------------------------------------------------Sossql_file::del

void Sossql_file::del()
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );
    _select_stmt->del();
}

//---------------------------------------------------------------------Sossql_file::current_key

const Const_area& Sossql_file::current_key()
{
    if( !_select_stmt )  throw_xc( "SOS-1262" );

    return _select_stmt->current_key();
}



} //namespace sos
