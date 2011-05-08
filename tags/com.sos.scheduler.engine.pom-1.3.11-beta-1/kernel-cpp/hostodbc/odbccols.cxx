//#define MODULE_NAME "odbccols"
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


//---------------------------------------------------------------------------------Odbc_column

struct Odbc_column
{
                                // Ein Destruktor wird nicht aufgerufen!

    Sos_limited_text<128>      _table_qualifier;
    Bool                       _table_qualifier_null;
    Sos_limited_text<128>      _table_owner;
    Bool                       _table_owner_null;
    Sos_limited_text<128>      _table_name;
    Sos_limited_text<128>      _column_name;
    int2                       _data_type;
    Sos_limited_text<128>      _type_name;
    int4                       _precision;
    Bool                       _precision_null;
    int4                       _length;
    int2                       _scale;
    int2                       _radix;
    Bool                       _radix_null;
    int2                       _nullable;
    Sos_limited_text<254>      _remarks;

    //static Sos_ptr<Record_type> _obj_type;
};

//----------------------------------------------------------------------------Odbc_columns_file

struct Odbc_columns_file : Abs_file
{
                                Odbc_columns_file       ();
                               ~Odbc_columns_file       ();

    void                        init                    ();
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        get_record              ( Area& );

  private:
    Sos_ptr<Record_type>       _record_type;
    Sos_ptr<Record_type>       _odbc_columns_type;
    Sos_string                 _qualifier;
    Sos_string                 _owner;
    Sos_string                 _table;
    int                        _i;
};


//-----------------------------------------------------------------------Odbc_columns_file_type

struct Odbc_columns_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "odbc_columns"; }

    virtual Sos_ptr<Abs_file> create_base_file          () const
    {
        Sos_ptr<Odbc_columns_file> f = SOS_NEW( Odbc_columns_file() );
        return +f;
    }
};

const Odbc_columns_file_type  _odbc_columns_file_type;
const Abs_file_type&           odbc_columns_file_type = _odbc_columns_file_type;

//-----------------------------------------------------------------------------------SQLColumns

DEFINE_ODBC_CALL_8( Sos_odbc_stmt, SQLColumns, HSTMT,
                    UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD, UCHAR*, SWORD )

RETCODE Sos_odbc_stmt::SQLColumns(
    HSTMT     hstmt, 
	UCHAR FAR *szTableQualifier,
	SWORD	  cbTableQualifier,
	UCHAR FAR *szTableOwner,
	SWORD	  cbTableOwner,
	UCHAR FAR *szTableName,
	SWORD	  cbTableName,
	UCHAR FAR *szColumnName,
	SWORD	  cbColumnName)
{
/*
    // Wer schneidet ab Unterstrich den Tabellennamen ab?  MS-Access 2.0!
    if( log_ptr ) {
        *log_ptr << "TableName=";
        if( szTableName ) log_ptr->write( szTableName, cbTableName == SQL_NTS? strlen( szTableName ) : cbTableName );
        else *log_ptr << "NULL";
        *log_ptr << '\n';
    }
*/

    Sos_string qualifier  = odbc_as_string( szTableQualifier, cbTableQualifier );
    Sos_string owner      = odbc_as_string( szTableOwner, cbTableOwner );
    Sos_string table_name = odbc_as_string( szTableName, cbTableName );

    //extern const Abs_file_type& odbc_columns_file_type;  // odbccols.cxx
    //call_for_linker( &odbc_columns_file_type );

    Sos_string filename = "hostAPI -in odbc_columns";
    //append_option( &filename, " -db="   , _conn->_data_source_name );
    filename += _conn->_dbms_param;
    append_option( &filename, " -table=", table_name );
    if( !empty( qualifier) )  append_option( &filename, " -qualifier=", qualifier );
    if( !empty( owner    ) )  append_option( &filename, " -owner="    , owner );

    return SQLExecDirect( hstmt, (Byte*)c_str( filename ), length( filename ) );
}

// ----------------------------------------------------------Odbc_columns_file::Odbc_columns_file

Odbc_columns_file::Odbc_columns_file()
:
    _i(0)
{
}

//----------------------------------------------------------Odbc_columns_file::~Odbc_columns_file

Odbc_columns_file::~Odbc_columns_file()
{
}

//----------------------------------------------------------------Odbc_columns_file::init

void Odbc_columns_file::init()
{
    _odbc_columns_type = Record_type::create();
    Record_type* t = _odbc_columns_type;
    Odbc_column* o = 0;

    t->name( "Odbc_columns" );
    t->allocate_fields( 12 );                                                               

    RECORD_TYPE_ADD_LIMTEXT_NULL( table_qualifier, 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( table_owner    , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( table_name     , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( column_name    , 0 );
    RECORD_TYPE_ADD_FIELD       ( data_type      , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( type_name      , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( precision      , 0 );
    RECORD_TYPE_ADD_FIELD       ( length         , 0 );
    RECORD_TYPE_ADD_FIELD       ( scale          , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( radix          , 0 );
    RECORD_TYPE_ADD_FIELD       ( nullable       , 0 );
    RECORD_TYPE_ADD_LIMTEXT     ( remarks        , 0 );
}

//---------------------------------------------------------------Odbc_columns_file::open

void Odbc_columns_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Any_file    file;
    Sos_string  filename = "sossql";

    init();

    if( open_mode & out )  throw_xc( "D127" );

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if ( opt.with_value( "db" ) )               append_option( &filename, " -db=", opt.value() );
        else
        if ( opt.with_value( "catalog" ) )          append_option( &filename, " -catalog=", opt.value() );
        else
        if ( opt.with_value( "qualifier" ) )        _qualifier = opt.value();  // wird ignoriert
        else
        if ( opt.with_value( "owner" ) )            _owner = opt.value();      // wird ignoriert
        else
        if ( opt.with_value( "user" ) )             _owner = opt.value();      // wird ignoriert
        else
        if ( opt.with_value( "table" ) )            _table = opt.value();
        else throw_sos_option_error( opt );
    }

    //file.prepare_open( "alias " + _table, File_base::in );  // eigentlich interessiert nur die Satzbeschreibung
    append_option( &filename, " -qualifier=", _qualifier );
    append_option( &filename, " -user=", _owner );
    append_option( &filename, " select * from ", _table );

    file.prepare_open( filename, File_base::in );  // eigentlich interessiert nur die Satzbeschreibung
    _record_type = (Record_type*)file.spec().field_type_ptr();
    file.close();

    if( !_record_type )  throw_xc( "SOS-1193" );

    _any_file_ptr->_spec._field_type_ptr = +_odbc_columns_type;

  //LOG( "odbccols: typ=" << *_record_type << ", " << _record_type->field_count() << "\n" );
}

//-----------------------------------------------------------------Odbc_columns_file::get_record

void Odbc_columns_file::get_record( Area& buffer )
{
    if( !_record_type || _i >= _record_type->field_count() )  throw_eof_error();

    buffer.allocate_min( _odbc_columns_type->field_size() );  //sizeof (Odbc_column) );
    buffer.length      ( _odbc_columns_type->field_size() );  //sizeof (Odbc_column) );
    memset( buffer.ptr(), 0, _odbc_columns_type->field_size() );  //sizeof (Odbc_column) );
    Odbc_column* c = (Odbc_column*)buffer.ptr();
    new (c) Odbc_column;

    Field_descr* f = _record_type->field_descr_ptr( _i++ );
    Field_type*  t = f->type_ptr();
    Type_param   type_param;

    if( t->obj_is_type( tc_Record_type )        // Record_type?
     && ((Record_type*)t)->_group_type )        // Hat ein Gruppenfeld (geht über alle Felder)?
    {
        t = ((Record_type*)t)->_group_type;         // Das nehmen wir!
    }

    t->get_param( &type_param );

  //c->_table_qualifier.assign( c_str( _qualifier ) );
    c->_table_qualifier_null = true;
    c->_table_owner    .assign( c_str( _owner     ) );
  //c->_table_owner_null = true;
    c->_table_name     .assign( c_str( _table     ) );

    c->_column_name.assign( f->name() );
    c_str( c->_column_name );
    char* p = c->_column_name.char_ptr();
    while( *p )  { if( *p == '-' )  *p = '_';  p++; }   // Cobol

  //c->_data_type      = odbc_sql_type( *t->info() );
    c->_data_type      = odbc_sql_type( *t->info(), t->field_size() );   //jz 17.9.97
    if( !c->_data_type )  throw_xc( "SOSODBC-12", f );

    ostrstream s ( c->_type_name.char_ptr(), c->_type_name.size() );
    s << *t;
    c->_type_name.length( s.pcount() );

    c->_precision      = type_param._precision;
    c->_precision_null = type_param._precision == 0;
    c->_length         = odbc_c_default_type_length( type_param ); // Anzahl Bytes für SQL_C_DEFAULT
    c->_scale          = type_param._scale;
    c->_radix          = type_param._radix;
    c->_radix_null     = c->_radix == 0;
    c->_nullable       = f->nullable()? SQL_NULLABLE : SQL_NO_NULLS;
    c->_remarks        = "";

  //ODBC_LOG( "odbccols: " << *t << ", name="<< c->_column_name << ", type=" << type_param._std_type << "/" << c->_data_type << ", length=" << c->_length << ", prec=" << c->_precision << "\n" );
}


} //namespace sos
