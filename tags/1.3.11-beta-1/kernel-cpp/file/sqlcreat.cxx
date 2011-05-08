#define MODULE_NAME "sqlcreat"
#define COPYRIGHT   "©1997 SOS GmbH Berlin"
#define AUTHOR      "Jörg Schwiemann"

#include <sosstrng.h>
#include <sos.h>
#include <sosfield.h>
#include <sosopt.h>
#include <absfile.h>

/*
    Nur schreiben: macht aus Typ <name,sqltype> ein SQL CREATE Statement
*/

struct Sql_create_formatter {
                               Sql_create_formatter();
    void                       open( const Sos_string& table, Bool crlf = true );

    void                       put( const Sos_string& name, const Sos_string& sql_type, const Sos_string& comment );

    Sos_string                 get_result();
    static Sos_string          sql_type( const Field_descr& descr );
 private:
    void                       write_header( const Sos_string& table );
    void                       write_footer();

    Bool                       _second;
    char                       _zeilentrenner;
    Sos_string                 _create_statement;
};

Sql_create_formatter::Sql_create_formatter() :
    _zeilentrenner('\n'),
    _second(false)
{
}

void Sql_create_formatter::open( const Sos_string& table, Bool crlf )
{
    if ( !crlf ) _zeilentrenner = ' ';
    write_header( table );
}

void Sql_create_formatter::write_header( const Sos_string& table )
{
    _create_statement = "create table " + table + " (\n";
}


void Sql_create_formatter::write_footer()
{
    _create_statement += "\n)";
}

Sos_string Sql_create_formatter::get_result()
{
    write_footer();
    return _create_statement;
}

void Sql_create_formatter::put( const Sos_string& name, const Sos_string& sql_type, const Sos_string& comment )
{
    if ( sql_type == empty_string ) {
#if 0
        // nur Kommentar
        _create_statement += "// \t";
        _create_statement += name;
        if ( comment != empty_string ) {
            _create_statement += "\t\t\t\t\t";
            _create_statement += comment;
        }
        _create_statement += _zeilentrenner;
#endif
    } else {
        // Ein Tabellenfeld
        if ( _second ) {
            _create_statement += ",\n"; // Feldtrenner
        } else _second = true;
        _create_statement += name;
        _create_statement += "\t\t\t ";
        _create_statement += sql_type;
#if 0
        if ( comment != empty_string && _zeilentrenner == '\n' ) {
            _create_statement += "\t\t// ";
            _create_statement += comment;
        }
#endif
    }
}


Sos_string Sql_create_formatter::sql_type( const Field_descr& descr )
{
    Sos_string sql_type;
    const Type_info* info = descr.type().info();

    // Auswertung: Precision, Skalierung, etc.
    switch ( info->_std_type ) {
        case std_type_none:
                                    throw_xc( "sql_type unknown", "decimal" );
                                    break;
        case std_type_char:         // Feste Länge, Blanks am Ende zählen nicht
                                    sql_type = "CHAR("; sql_type += as_string(descr.type().field_size()); sql_type += ")";
                                    break;
        case std_type_varchar:      // variable Länge, Blanks am Ende zählen
                                    sql_type = "VARCHAR("; sql_type += as_string(descr.type().field_size()); sql_type += ")";
                                    break;
        case std_type_decimal:      // Festkomma, mit Präzision und Skalierung
                                    throw_xc( "sql_type unknown", "decimal" );
                                    break;
        case std_type_integer:      // Big_int (weitester Typ), ohne Skalierung (?)
                                    sql_type = "INTEGER";
                                    break;
        case std_type_float:        // Big_float (weitester Typ)
                                    throw_xc( "sql_type unknown", "float" );
                                    break;
        case std_type_date:         // Datum
                                    sql_type = "DATE";
                                    break;
        case std_type_time:         // Zeit
                                    sql_type = "TIME";
                                    break;
        case std_type_date_time:    // Datum und Zeit
                                    sql_type = "TIMESTAMP";
                                    break;
        case std_type_bool:
        default: break;
    }

    if ( !descr.nullable() ) sql_type += " NOT NULL";
    return sql_type;
}


// File
struct Sql_create_file : Abs_file
{
                                Sql_create_file             ();
                               ~Sql_create_file             ();

    virtual void                close                  ( Close_mode );
    void                        open                   ( const char*, Open_mode, const File_spec& );
    virtual void                insert                 ( const Const_area& o );

    virtual File_info           info                   ()                                       { return _file.info(); }

  //static void                 erase                  ( const char* filename );

  protected:
    virtual void                put_record             ( const Const_area& o );
    void                        any_file_ptr           ( Any_file* p ) { _any_file_ptr = p; }

  private:

    Sql_create_formatter       _formatter;
    Any_file*                  _any_file_ptr;
    Any_file                   _file;
    Sos_ptr<Record_type>       _record_type;
};

//----------------------------------------------------------------------statics

struct Sql_create_file_type : Abs_file_type
{
    virtual const char* name() const { return "sqlcreate"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Sql_create_file> f = SOS_NEW_PTR( Sql_create_file );
        return +f;
    }
};

const Sql_create_file_type  _sql_create_file_type;
const Abs_file_type&    sql_create_file_type = _sql_create_file_type;

//---------------------------------------------------------------Sql_create_file::Sql_create_file

Sql_create_file::Sql_create_file()
{
}

//--------------------------------------------------------------Sql_create_file::~Sql_create_file

Sql_create_file::~Sql_create_file()
{
}

//-------------------------------------------------------------------------Sql_create_file::open

void Sql_create_file::open( const char* param, Open_mode mode, const File_spec& spec )
{
    if ( mode & in && mode & out ) throw_xc( "D049" );
    //if( name[ 0 ] == '|' )  name++;     // pipe

    Sos_string filename;
    Sos_string table;
    Bool       lf = true;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( 't', "table"   ))   table = opt.value();
        else
        if( opt.flag(      'n', "nocrlf"   ))   lf = !opt.set();
        else
        if ( opt.pipe() )                       filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _record_type = +spec._field_type_ptr;
         if( !_record_type  )  throw_xc( "SOS-1171", "sqlcreat" );

    _file.open( filename, mode, spec );
    _formatter.open( table, lf );
}


//-------------------------------------------------------------------------Sql_create_file::close

void Sql_create_file::close( Close_mode mode )
{
    if ( mode == close_normal ) {
        Sos_string result = _formatter.get_result();
        _file.put( c_str(result) );
    }
    _file.close( mode );
}

void Sql_create_file::put_record( const Const_area& o )
{
    Sos_string name     = as_string( _record_type->field_descr_ptr(0), o.byte_ptr() );
    Sos_string sql_type = as_string( _record_type->field_descr_ptr(1), o.byte_ptr() );
    Sos_string comment  = as_string( _record_type->field_descr_ptr(3), o.byte_ptr() );

    _formatter.put( name, sql_type, comment );
}

void Sql_create_file::insert( const Const_area& o )
{
    put_record( o );
}


// SQL CREATE STATEMENT aus einem Record_type

Sos_string get_sql_create_statement( const Sos_ptr<Record_type> t, const Sos_string& table )
{
    Sql_create_formatter formatter;
    formatter.open( table );

    for ( int i=0; i < t->field_count(); i++ ) {
        Field_descr& descr = *t->field_descr_ptr(i);
        Sos_string sql_type = Sql_create_formatter::sql_type( descr );
        formatter.put( descr.name(), sql_type, empty_string );
    }
    return formatter.get_result();
}

