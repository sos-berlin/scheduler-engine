// $Id$

#include "precomp.h"
#include "../kram/sysdep.h"

#ifdef SYSTEM_ODBC

#if defined _WIN32
#   include <windows.h>
#endif

#include "sql.h"
#include "sqlext.h"
#include "math.h"

#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "absfile.h"


namespace sos {


//--------------------------------------------------------------------------------Create_table_file

struct Create_table_file : Abs_file
{
    enum Dbms
    {
        dbms_unknown,
        dbms_oracle,
        dbms_jet,
        dbms_mysql
      //dbms_informix
    };
                                    Create_table_file   ();
                                 //~Create_table_file   ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );


  protected:
    void                            put_record          ( const Const_area& );
    void                            append_to_line      ( int position, const string& value );

  private:
    Fill_zero                      _zero_;
    Dbms                           _dbms;
    Sos_ptr<Record_type>           _columns_type;
    Any_file                       _file;
    string                         _line;
    int                            _decr1;
    bool                           _with_comment;
    bool                           _nl;
};

//---------------------------------------------------------------------------Create_table_file_type

struct Create_table_file_type : Abs_file_type
{
    virtual const char*         name                    () const        { return "create_table"; }
  //virtual const char*         alias_name              () const        { return ""; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Create_table_file> f = SOS_NEW( Create_table_file );
        return +f;
    }
};

const Create_table_file_type   _create_table_file_type;
const Abs_file_type&            create_table_file_type = _create_table_file_type;

//---------------------------------------------------------------------- Create_table_file::Create_table_file

Create_table_file::Create_table_file()
:
    _zero_(this+1)
  //_current_key( _current_key_buffer, sizeof _current_key_buffer )
{
}

//----------------------------------------------------------------------Create_table_file::~Create_table_file
/*
Create_table_file::~Create_table_file()
{
}
*/
//-----------------------------------------------------------------------------Create_table_file::open

void Create_table_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    string  table;
    string  filename;

    _decr1 = true;
    _nl = false;

    if( open_mode & in )  throw_xc( "D126" );      // Nur Schreiben möglich

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( opt.flag( "jet"    ) )  _dbms = dbms_jet;
        else
        if( opt.flag( "access" ) )  _dbms = dbms_jet;
        else
        if( opt.flag( "oracle" ) )  _dbms = dbms_oracle;
        else
        if( opt.flag( "mysql"  ) )  _dbms = dbms_mysql;
        else
/*
        if( opt.flag( "informix" ) )  _dbms = dbms_informix;
        else
        if( opt.flag( "sql" ) )  _dbms = dbms_sql;
        else
*/
        if( opt.with_value( "dbms") )
        {
            if( opt.value() == "jet" )        _dbms = dbms_jet;
            else
            if( opt.value() == "access" )        _dbms = dbms_jet;
            else
            if( opt.value() == "oracle" )        _dbms = dbms_oracle;
            else
            if( opt.value() == "mysql"  )        _dbms = dbms_mysql;
            else
/*
            if( opt.value() == "informix" )        _dbms = dbms_informix;
            else
            if( opt.value() == "sql" )        _dbms = dbms_sql;
            else
*/
                throw_xc( "SOS-1439", opt.value() );
        }
        else
        if( opt.with_value( "table"       ) )  table = opt.value();
        else
        if( opt.flag      ( "1"            ) )  _decr1 = opt.set();
        else
        if( opt.flag      ( "comment"      ) )  _with_comment = opt.set();
        else
        if( opt.flag      ( "nl"           ) )  _nl = opt.set();
        else
        if( opt.pipe() )                                { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    _file.open( filename, Abs_file::Open_mode( open_mode | seq & ~binary ), file_spec );

    _columns_type = +file_spec._field_type_ptr;
    if( !_columns_type )  throw_xc( "SOS-1440" );

    _line = "create table " + table + " (";
}

//----------------------------------------------------------------Create_table_file::append_to_line

void Create_table_file::append_to_line( int position, const string& value )
{
    if( _nl )
    {
        int rest = position - _line.length();
        while( rest > 0 )  _line.append( " " ),  rest--;
    }

    _line += value;
}

//-------------------------------------------------------------------------Create_table_file::close

void Create_table_file::close( Close_mode )
{
    if( _nl  &&  !_line.empty() )  _file.put( _line ),  _line = "";
    _line += ")";

    if( _dbms == dbms_jet )  
    {
        _line += ";";
        if( _nl )  _file.put( _line ),  _line = "";
    
        _line += "COMMIT";
    }
    
    _file.put( _line );
}

//--------------------------------------------------------------------Create_table_file::put_record

void Create_table_file::put_record( const Const_area& record )
{
    const Byte* p = record.byte_ptr();

    if( !_line.empty() )  
    {
        if(_line[ _line.length()-1] != '(' )  _line += ',';
        if( _nl )  _file.put( _line ),  _line = "";
    }
    
    if( record.length() < _columns_type->field_size() )  throw_xc( "SOS-1114", (int)record.length() );

    append_to_line( 4, "\"" + _columns_type->field_descr_ptr( "column_name" )->as_string( p ) + "\"" );
    _line += " ";

    int    data_type      = _columns_type->field_descr_ptr( "data_type" )->as_int( p );
    string odbc_type_name = _columns_type->field_descr_ptr( "odbc_type_name" )->as_string( p );
    int    radix          = _columns_type->field_descr_ptr( "radix"     )->null( p )? 0 : _columns_type->field_descr_ptr( "radix"     )->as_int(p);
    int    precision      = _columns_type->field_descr_ptr( "precision" )->null( p )? 0 : _columns_type->field_descr_ptr( "precision" )->as_int(p);
    int    scale          = _columns_type->field_descr_ptr( "scale"     )->null( p )? 0 : _columns_type->field_descr_ptr( "scale"     )->as_int(p);
    string remarks        = _columns_type->field_descr_ptr( "remarks" )->as_string( p );
    string type_name;

    switch( _dbms )
    {
        case dbms_jet:
        {
            switch( data_type )
            {
                case SQL_CHAR         : type_name = "char";   break;
                                        break;
                
                case SQL_VARCHAR      : type_name = "varchar";   break;
                                        break;
                
                case SQL_LONGVARCHAR  : if( precision > 0  &&  precision <= 255 )  type_name = "longchar";
                                                                             else  type_name = "varchar",  precision = 0;
                                        break;
                
                case SQL_BIT          : type_name = "boolean",  precision = 0;
                                        break;

                case SQL_TINYINT      : type_name = "byte",  precision = 0;
                                        break;

                case SQL_SMALLINT     : type_name = "short",  precision = 0;
                                        break;

                case SQL_INTEGER      : type_name = "integer",  precision = 0;
                                        break;

                case SQL_BIGINT       : 
                case SQL_NUMERIC      : 
                case SQL_DECIMAL      : if( scale <= 4  &&  precision <= ( radix == 2? 64 : 19 ) )  type_name = "currency",  precision = 0;
                                                                                              else  type_name = "number";
                                        break;

                case SQL_REAL         : 
                case SQL_FLOAT        : type_name = "single", precision = 0;
                                        break;

                case SQL_DOUBLE       : type_name = "double",  precision = 0;
                                        break;

                case SQL_DATE         : 
                case SQL_TIME         : 
                case SQL_TIMESTAMP    : type_name = "date",  precision = 0;
                                        break;

                case SQL_BINARY       : 
                case SQL_VARBINARY    : 
                case SQL_LONGVARBINARY: if( precision > 0  &&  precision < 255 )  type_name = "binary";
                                                                            else  type_name = "longbinary";
                                        break;

                default               : throw_xc( "SOS-1441", odbc_type_name );
            }

            break;
        }

        case dbms_oracle:
        {
            switch( data_type )
            {
                case SQL_CHAR         : type_name = "char";   break;
                                        break;
                
                case SQL_VARCHAR      : type_name = "varchar2";   break;
                                        break;
                
                case SQL_LONGVARCHAR  : if( precision > 0  &&  precision <= 4000 )  type_name = "varchar2";
                                                                              else  type_name = "clob",  precision = 0;
                                        break;
                
                case SQL_BIT          : type_name = "number(1)",  precision = 0;
                                        break;

                case SQL_TINYINT      : 
                case SQL_SMALLINT     : 
                case SQL_INTEGER      : 
                case SQL_BIGINT       : 
                case SQL_NUMERIC      : 
                case SQL_DECIMAL      : type_name = "number";
                                        break;

                case SQL_REAL         : 
                case SQL_FLOAT        : 
                case SQL_DOUBLE       : type_name = "float",  precision = 0;
                                        break;

                case SQL_DATE         : 
                case SQL_TIME         : 
                case SQL_TIMESTAMP    : type_name = "date",  precision = 0;
                                        break;

                case SQL_BINARY       : 
                case SQL_VARBINARY    : type_name = "raw";
                                        break;

                case SQL_LONGVARBINARY: if( precision > 0  &&  precision < 4000 )  type_name = "raw";
                                                                             else  type_name = "blob";
                                        break;

                default               : throw_xc( "SOS-1441", odbc_type_name );
            }

            break;
        }

        
        case dbms_mysql:
        {
            switch( data_type )
            {
                case SQL_CHAR         : type_name = "char";   break;
                                        break;
                
                case SQL_VARCHAR      : type_name = "varchar";   break;
                                        break;
                
                case SQL_LONGVARCHAR  : if( precision > 0  &&  precision <= 65535 )  type_name = "text",  precision = 0;
                                                                               else  type_name = "longtext",  precision = 0;
                                        break;
                
                case SQL_BIT          : type_name = "bool",  precision = 0;
                                        break;

                case SQL_TINYINT      : type_name = "tinyint",  precision = 0;  
                                        break;

                case SQL_SMALLINT     : type_name = "smallint",  precision = 0;
                                        break;

                case SQL_INTEGER      : type_name = "int",  precision = 0;
                                        break;

                case SQL_BIGINT       : type_name = "bigint",  precision = 0;
                                        break;

                case SQL_NUMERIC      : 
                case SQL_DECIMAL      : type_name = "decimal";
                                        break;

                case SQL_REAL         : 
                case SQL_FLOAT        : type_name = "float",  precision = 0;
                                        break;

                case SQL_DOUBLE       : type_name = "double",  precision = 0;
                                        break;

                case SQL_DATE         : type_name = "date",  precision = 0;
                                        break;

                case SQL_TIME         : type_name = "time",  precision = 0;
                                        break;

                case SQL_TIMESTAMP    : type_name = "timestamp",  precision = 0;
                                        break;

                case SQL_BINARY       : 
                case SQL_VARBINARY    : type_name = "tinyblob";
                                        break;

                case SQL_LONGVARBINARY: if( precision > 0  &&  precision <= 65535 )  type_name = "blob",  precision = 0;
                                                                               else  type_name = "longblob",  precision = 0;
                                        break;

                default               : throw_xc( "SOS-1441", odbc_type_name );
            }

        default:
            break;
        }
    }


    append_to_line( 50, type_name );

    if( precision )  
    {
        if( _decr1 && strnicmp( _columns_type->field_descr_ptr( "type_name" )->as_string(p).c_str(), "String0", 7 ) == 0 )  precision--;
        
        int prec = precision;

        if( radix == 2 )  prec = (int)ceil( precision * ::log(2.0) / ::log(10.0) );

        _line += "(";
        _line += as_string( prec );
        if( !_columns_type->field_descr_ptr( "scale" )->null( p ) )
        {
            _line += "," + _columns_type->field_descr_ptr( "scale" )->as_string( p );
        }
        _line += ")";
    }

    if( !_columns_type->field_descr_ptr( "nullable" )->as_int( p ) )  append_to_line( 70, " NOT NULL" );

    if( _with_comment )
    {
        append_to_line( 85, " /*" + odbc_type_name );
        append_to_line( 120, " " + remarks );
        _line += " */";
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
