//#define MODULE_NAME "odbcspec"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif


#include <sql.h>
#include <sqlext.h>
#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../kram/stdfield.h"
#include "../file/absfile.h"
#include "sosodbc.h"

using namespace std;
namespace sos {

//--------------------------------------------------------------------------Odbc_special_column

struct Odbc_special_column
{
                                // Ein Destruktor wird nicht aufgerufen!

    int2                       _scope;
    Bool                       _scope_null;
    Sos_limited_text<128>      _column_name;
    int2                       _data_type;
    Sos_limited_text<128>      _type_name;
    int4                       _precision;
    Bool                       _precision_null;
    int4                       _length;
    int2                       _scale;
    int2                       _pseudo_column;
    Bool                       _pseudo_column_null;
};

//--------------------------------------------------------------------Odbc_special_columns_file

struct Odbc_special_columns_file : Abs_file
{
                                Odbc_special_columns_file();
                               ~Odbc_special_columns_file();

    void                        init                    ();
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        get_record              ( Area& );

  private:
    UWORD                      _fScope;
    Bool                       _nullable;
    Any_file                   _file;
    Sos_ptr<Record_type>       _key_type;
    Sos_ptr<Record_type>       _Odbc_special_columns_type;
    int                        _i;
    Bool                       _eof;
    int                        _key_pos;
    int                        _key_len;
};


//---------------------------------------------------------------Odbc_special_columns_file_type

struct Odbc_special_columns_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "Odbc_special_columns"; }

    virtual Sos_ptr<Abs_file> create_base_file          () const
    {
        Sos_ptr<Odbc_special_columns_file> f = SOS_NEW( Odbc_special_columns_file() );
        return +f;
    }
};

const Odbc_special_columns_file_type  _odbc_special_columns_file_type;
const Abs_file_type&                   odbc_special_columns_file_type = _odbc_special_columns_file_type;

//----------------------------------------------------------------------------SQLSpecialColumns

DEFINE_ODBC_CALL_9( Sos_odbc_stmt, SQLSpecialColumns, HSTMT,
                    UWORD, UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD, UWORD, UWORD )

RETCODE Sos_odbc_stmt::SQLSpecialColumns(
    HSTMT     hstmt, 
	UWORD	  fColType,
	UCHAR FAR *szTableQualifier,
	SWORD	  cbTableQualifier,
	UCHAR FAR *szTableOwner,
	SWORD	  cbTableOwner,
	UCHAR FAR *szTableName,
	SWORD	  cbTableName,
	UWORD	  fScope,
	UWORD	  fNullable)
{
    if( fColType == SQL_ROWVER ) {
        _eof = true;                // Keine Versionsfelder, also leere Menge liefern
        _field_count = 8;
        // SQLBindCol() muss möglich sein:
        //if( _col_bindings.last_index() < _field_count )  _col_bindings.last_index( _field_count );
        return SQL_SUCCESS;
    }
    else
    if( fColType == SQL_BEST_ROWID ) {
        _eof = true;                // Keine Versionsfelder, also leere Menge liefern
        _field_count = 8;
        return SQL_SUCCESS;
/*jz 22.4.97
        Sos_string qualifier  = odbc_as_string( szTableQualifier, cbTableQualifier );
        Sos_string owner      = odbc_as_string( szTableOwner, cbTableOwner );
        Sos_string table_name = odbc_as_string( szTableName, cbTableName );

        //extern const Abs_file_type& odbc_special_columns_file_type;  // odbcspec.cxx
        //call_for_linker( &odbc_special_columns_file_type );

        Sos_string filename = "hostAPI -in odbc_special_columns";
        append_option( &filename, " -db="   , _conn->_data_source_name );
        append_option( &filename, " -table=", table_name );
        if( !empty( qualifier) )  append_option( &filename, " -qualifier=", qualifier );
        if( !empty( owner    ) )  append_option( &filename, " -owner="    , owner );

        filename += " -scope= "; filename += as_string( fScope );

        if( fNullable == SQL_NO_NULLS       ) ;
        else
        if( fNullable == SQL_NULLABLE       )  filename += " -nullable";
        else return SQL_ERROR;

        return SQLExecDirect( hstmt, (Byte*)c_str( filename ), length( filename ) );
*/
    }

    return SQL_ERROR;
}

// ----------------------------------------Odbc_special_columns_file::Odbc_special_columns_file

Odbc_special_columns_file::Odbc_special_columns_file()
:
    _i(0)
{
}

//----------------------------------------Odbc_special_columns_file::~Odbc_special_columns_file

Odbc_special_columns_file::~Odbc_special_columns_file()
{
}

//--------------------------------------------------------------Odbc_special_columns_file::init

void Odbc_special_columns_file::init()
{
    _Odbc_special_columns_type = Record_type::create();
    Record_type* t = _Odbc_special_columns_type;
    Odbc_special_column* o = 0;

    t->name( "Odbc_special_columns" );
    t->allocate_fields( 8 );

    RECORD_TYPE_ADD_FIELD_NULL  ( scope          , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( column_name    , 0 );
    RECORD_TYPE_ADD_FIELD       ( data_type      , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( type_name      , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( precision      , 0 );
    RECORD_TYPE_ADD_FIELD       ( length         , 0 );
    RECORD_TYPE_ADD_FIELD       ( scale          , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( pseudo_column  , 0 );
}

//--------------------------------------------------------------Odbc_special_columns_file::open

void Odbc_special_columns_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Sos_string  filename = "sossql";
    Sos_string  table;

    init();

    if( open_mode & out )  throw_xc( "D127" );

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( "db" ) )               append_option( &filename, " -db=", opt.value() );
        else
        if ( opt.with_value( "catalog" ) )          append_option( &filename, " -catalog=", opt.value() );
        else
        if ( opt.with_value( "qualifier" ) )        append_option( &filename, " -qualifier=", opt.value() );
        else
        if ( opt.with_value( "owner" ) )            append_option( &filename, " -user=", opt.value() );
        else
        if ( opt.with_value( "table" ) )            table = opt.value();
        else
        if ( opt.with_value( "scope" ) )            _fScope = as_int( opt.value() );
        else
        if ( opt.flag( "scope" ) )                  _nullable = opt.set();
        else throw_sos_option_error( opt );
    }

    append_option( &filename, " select * from ", table );

    _file.prepare_open( filename, File_base::in );  // eigentlich interessiert nur die Satzbeschreibung

    const Field_descr* f = _file.spec()._key_specs._key_spec.field_descr_ptr();
    if( f )  _key_type = SOS_CAST( Record_type, f->type_ptr() );  // Schlüssel MUSS in einem Record_type eingewickelt sein.
    else {
        _key_pos = _file.key_position();
        _key_len = _file.key_length();
    }

    if( !_key_type && !_key_len )  throw_xc( "SOS-1214" );

    _any_file_ptr->_spec._field_type_ptr = +_Odbc_special_columns_type;
}

//--------------------------------------------------------Odbc_special_columns_file::get_record

void Odbc_special_columns_file::get_record( Area& buffer )
{
    Field_type* t           = NULL;
    Type_param  type_param;

    if( _eof )  throw_eof_error();
    if( !_key_len )  throw_eof_error();

    buffer.allocate_min( _Odbc_special_columns_type->field_size() );  //sizeof (Odbc_special_column) );
    buffer.length      ( _Odbc_special_columns_type->field_size() );  //sizeof (Odbc_special_column) );
    memset( buffer.ptr(), 0, _Odbc_special_columns_type->field_size() );  //sizeof (Odbc_special_column) );
    Odbc_special_column* c = (Odbc_special_column*)buffer.ptr();
    new (c) Odbc_special_column;

    if( _key_pos == -1 )   // ROWID
    {        
        throw_eof_error();
        //t = _file.rowid_type(); //->get_param( &type_param );

        if( _fScope > SQL_SCOPE_CURROW )  throw_eof_error();

        c->_scope          = SQL_SCOPE_CURROW;
        c->_column_name    = "ROWID";
        c->_pseudo_column  = SQL_PC_PSEUDO;

        _eof = true;
    }
    else 
    {
        if( !_key_type  ||  _i >= _key_type->field_count() )  throw_eof_error();

        Field_descr* f = _key_type->field_descr_ptr( _i++ );

        t = f->type_ptr();
        t->get_param( &type_param );

        c->_scope = SQL_SCOPE_SESSION; //SQL_SCOPE_CURROW;

        c->_column_name.assign( f->name() );
        c_str( c->_column_name );
        char* p = c->_column_name.char_ptr();
        while( *p )  { if( *p == '-' )  *p = '_';  p++; }   // Cobol

        c->_pseudo_column  = SQL_PC_NOT_PSEUDO;
    }

  //c->_data_type      = odbc_sql_type( *t->info() );
    c->_data_type      = odbc_sql_type( *t->info(), t->field_size() );  //jz 17.9.97
    if( !c->_data_type )  throw_xc( "odbcspec.unknown-type", t );

    ostrstream s ( c->_type_name.char_ptr(), c->_type_name.size() );
    s << t;
    c->_type_name.length( s.pcount() );

    c->_precision      = type_param.precision_10();
    c->_precision_null = type_param._precision == 0;
    c->_length         = odbc_c_default_type_length( type_param ); // Anzahl Bytes für SQL_C_DEFAULT
    c->_scale          = type_param._scale;
}


} //namespace sos
