//#define MODULE_NAME "odbcstat"
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


namespace sos {

//-------------------------------------------------------------------------------Odbc_statistic

struct Odbc_statistic
{
                                // Ein Destruktor wird nicht aufgerufen!

    Sos_limited_text<128>       _table_qualifier;
    Bool                        _table_qualifier_null;
    Sos_limited_text<128>       _table_owner;
    Bool                        _table_owner_null;
    Sos_limited_text<128>       _table_name;
    int2                        _non_unique;
    Bool                        _non_unique_null;
    Sos_limited_text<128>       _index_qualifier;
    Bool                        _index_qualifier_null;
    Sos_limited_text<128>       _index_name;
    Bool                        _index_name_null;
    int2                        _type;
    int2                        _seq_in_index;
    Bool                        _seq_in_index_null;
    Sos_limited_text<128>       _column_name;
    Bool                        _column_name_null;
    char                        _collation;
    int4                        _cardinality;
    Bool                        _cardinality_null;
    int4                        _pages;
    Bool                        _pages_null;
    Sos_limited_text<128>       _filter_condition;
    Bool                        _filter_condition_null;
};

//-------------------------------------------------------------------------Odbc_statistics_file

struct Odbc_statistics_file : Abs_file
{
                                Odbc_statistics_file    ();
                               ~Odbc_statistics_file    ();

    void                        init                    ();
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        get_record              ( Area& );

  private:
    Fill_zero                  _zero_;
    Sos_ptr<Record_type>       _key_type;
    Sos_ptr<Record_type>       _odbc_statistics_type;
    Sos_string                 _qualifier;
    Sos_string                 _owner;
    Sos_string                 _table;
    int                        _rowid_length;
    int                        _i;
};


//--------------------------------------------------------------------Odbc_statistics_file_type

struct Odbc_statistics_file_type : Abs_file_type
{
    virtual const char* name      () const { return "odbc_statistics"; }

    virtual Sos_ptr<Abs_file> create_base_file   () const
    {
        Sos_ptr<Odbc_statistics_file> f = SOS_NEW( Odbc_statistics_file() );
        return +f;
    }
};

const Odbc_statistics_file_type  _odbc_statistics_file_type;
const Abs_file_type&              odbc_statistics_file_type = _odbc_statistics_file_type;

//--------------------------------------------------------------------------------SQLStatistics

DEFINE_ODBC_CALL_8( Sos_odbc_stmt, SQLStatistics, HSTMT,
                    UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD, UWORD, UWORD )

RETCODE Sos_odbc_stmt::SQLStatistics(
    HSTMT     hstmt, 
	UCHAR FAR *szTableQualifier,
	SWORD	  cbTableQualifier,
	UCHAR FAR *szTableOwner,
	SWORD	  cbTableOwner,
	UCHAR FAR *szTableName,
	SWORD	  cbTableName,
	UWORD	 ,//fUnique, nur SQL_INDEX_UNIQUE möglich
	UWORD	  /*fAccuracy*/)
{
    Sos_string table_name = odbc_as_string( szTableName, cbTableName );
    Sos_string owner      = odbc_as_string( szTableOwner, cbTableOwner );
    Sos_string qualifier  = odbc_as_string( szTableQualifier, cbTableQualifier );

    //extern const Abs_file_type& odbc_statistics_file_type;  // odbcstat.cxx
    //call_for_linker( &odbc_statistics_file_type );

    Sos_string filename = "hostAPI -in "
                        /*"sort -fields=(non_unique,type,index_qualifier,index_name,seq_in_index) | "*/
                          "odbc_statistics";
    //append_option( &filename, " -db=", _conn->_data_source_name );
    filename += _conn->_dbms_param;

    if( length( qualifier ) )  append_option( &filename, " -qualifier=", qualifier );
    if( length( owner     ) )  append_option( &filename, " -owner="    , owner     );;
    append_option( &filename, " -table=", table_name );

    return SQLExecDirect( hstmt, (Byte*)c_str( filename ), length( filename ) );
}

// --------------------------------------------------Odbc_statistics_file::Odbc_statistics_file

Odbc_statistics_file::Odbc_statistics_file()
:
    _zero_(this+1)
{
}

//--------------------------------------------------Odbc_statistics_file::~Odbc_statistics_file

Odbc_statistics_file::~Odbc_statistics_file()
{
}

//-------------------------------------------------------------------Odbc_statistics_file::init

void Odbc_statistics_file::init()
{
    _odbc_statistics_type = Record_type::create();
    Record_type*    t = _odbc_statistics_type;
    Odbc_statistic* o = 0;

    t->name( "Odbc_statistics" );
    t->allocate_fields( 13 );

    RECORD_TYPE_ADD_LIMTEXT_NULL( table_qualifier  , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( table_owner      , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( table_name       , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( non_unique       , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( index_qualifier  , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( index_name       , 0 );
    RECORD_TYPE_ADD_FIELD       ( type             , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( seq_in_index     , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( column_name      , 0 );
    RECORD_TYPE_ADD_FIELD       ( collation        , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( cardinality      , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( pages            , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( filter_condition , 0 );
}

//---------------------------------------------------------------Odbc_statistics_file::open

void Odbc_statistics_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Any_file             file;
    Sos_string           filename = "sossql ";

    init();

    if( open_mode & out )  throw_xc( "D127" );

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( "db" ) )               append_option( &filename, " -db=", opt.value() );
        else
        if ( opt.with_value( "catalog" ) )          append_option( &filename, " -catalog=", opt.value() );
        else
        if ( opt.with_value( "qualifier" ) )        _qualifier = opt.value();
        else
        if ( opt.with_value( "owner" ) )            _owner = opt.value();
        else
        if ( opt.with_value( "user" ) )             _owner = opt.value();
        else
        if ( opt.with_value( "table" ) )            _table = opt.value();
        else throw_sos_option_error( opt );
    }

    append_option( &filename, " -qualifier=", _qualifier );
    append_option( &filename, " -user=", _owner );
    append_option( &filename, " select * from ", _table );

    file.prepare_open( filename, File_base::in );  // eigentlich interessiert nur die Satzbeschreibung
    const Field_descr* f = file.spec()._key_specs._key_spec.field_descr_ptr();
    if( f )  _key_type = SOS_CAST( Record_type, f->type_ptr() );  // Schlüssel MUSS in einem Record_type eingewickelt sein.
    if( file.key_length()  &&  (int)file.key_position() < 0  )  _rowid_length = file.key_length();
    file.close();

    _i = -1;    // erster Satz liefert Information über die Tabelle

    _any_file_ptr->_spec._field_type_ptr = +_odbc_statistics_type;
}

//-------------------------------------------------------------Odbc_statistics_file::get_record

void Odbc_statistics_file::get_record( Area& buffer )
{
    buffer.allocate_min( _odbc_statistics_type->field_size() );   //sizeof (Odbc_statistic) );
    buffer.length      ( _odbc_statistics_type->field_size() );   //sizeof (Odbc_statistic) );
    memset( buffer.ptr(), 0, buffer.size() );
    Odbc_statistic* c = (Odbc_statistic*)buffer.ptr();
    new(c) Odbc_statistic;


  //c->_table_qualifier       .assign( c_str( _qualifier ) );     // null!
    c->_table_qualifier_null = true;
    c->_table_owner           .assign( c_str( _owner     ) );     // null!
  //c->_table_owner_null = true;
    c->_table_name            .assign( c_str( _table     ) );

    if( _i == -1 ) {
        c->_non_unique_null       = true;
        c->_index_qualifier_null  = true;
        c->_index_name_null       = true;
        c->_type                  = SQL_TABLE_STAT;
        c->_seq_in_index_null     = true;
        c->_column_name_null      = true;
        c->_collation             = '\0';   // NULL
      //c->_cardinality           = number of rows
        c->_cardinality_null      = true;
        c->_pages_null            = true;
        c->_filter_condition_null = true;
    } else {
        if( _key_type ) {
            if( _i >= _key_type->field_count() )  throw_eof_error();

            Field_descr* f = _key_type->field_descr_ptr( _i );
          //Field_type*  t = f->type_ptr();

            c->_non_unique            = false;
            c->_index_qualifier_null  = true;
            c->_index_name            = "primary_key";
            c->_type                  = SQL_INDEX_OTHER;
            c->_seq_in_index          = _i + 1;

            c->_column_name.assign( f->name() );
            c_str( c->_column_name );
            char* p = c->_column_name.char_ptr();
            while( *p )  { if( *p == '-' )  *p = '_';  p++; }   // Cobol '-' durch '_' ersetzen

            c->_collation             = '\0';   // NULL
          //c->_cardinality           = number of unique values in the index
            c->_cardinality_null      = true;
            c->_pages_null            = true;
            c->_filter_condition_null = true;
        }
/*                      Der Index ROWID muss in SQLColumns als Spalte geliefert werden. ROWID ist kein Index.
                        Wie geht ODBC mit der ROWID um?
        else
        if( _rowid_length ) {
            if( _i > 0 ) throw_eof_error();

            c->_non_unique            = false;
            c->_index_qualifier_null  = true;
            c->_index_name            = "ROWID";
            c->_type                  = SQL_INDEX_OTHER;
            c->_seq_in_index          = _i + 1;
            c->_column_name           = "ROWID";
            c->_collation             = '\0';   // NULL
          //c->_cardinality           = number of unique values in the index
            c->_cardinality_null      = true;
            c->_pages_null            = true;
            c->_filter_condition_null = true;
        }
*/
        else throw_eof_error();
    }
 
    _i++;
}


} //namespace sos
