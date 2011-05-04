// colsfile.cxx                                ©1997 SOS GmbH Berlin
//                                             Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_ODBC         // ist abhängig von ODBC, müsste aber nicht sein (wegen odbctype.h>

#if defined SYSTEM_WIN
#    ifndef STRICT
#       define STRICT
#    endif
#    include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"
#include "../file/odbctype.h"

//#include <fldfile.h>

// s.a. odbccols.cxx Odbc_column
// Funktion an ODBC SQLColumns angelehnt, daher auch die Feldnamen
   

namespace sos {
using namespace std;

//-----------------------------------------------------------------------------------Columns_file

struct Columns_file : Abs_file
{
    struct Column
    {
                                    // Ein Destruktor wird nicht aufgerufen!
    
        Sos_limited_text<128>      _column_name;
        int2                       _data_type;              // SQL_xxx
        Sos_limited_text<128>      _type_name;
        int4                       _precision;
        Bool                       _precision_null;
        int4                       _display_size;
        int4                       _length;
        int2                       _scale;
        Bool                       _scale_null;
        int2                       _radix;
        Bool                       _radix_null;
        int2                       _nullable;
        Bool                       _unsigned;
        int4                       _offset;
        int4                       _size;
        Sos_limited_text<254>      _remarks;
        Sos_limited_text<100>      _odbc_type_name;

        int2                       _nesting;                // 1: Oberste Stufe, > 1: verschachtelt

    };

    struct Stack_entry
    {
                                    Stack_entry         ( Record_type* t = NULL ) : _record_type( t ), _index(0) {}

        Record_type*               _record_type;
        int                        _index;
    };

                                Columns_file            ();
                               ~Columns_file            ();

  //void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
  //void                        close                   ( Close_mode );

 protected:
    void                        get_record              ( Area& area );

    Fill_zero                  _zero_;
    Sos_simple_array<Stack_entry> _stack;
  //Stack_entry                _stack[ 50 ];
    Sos_ptr<Record_type>       _column_type;
    Sos_ptr<Record_type>       _record_type;
  //Sos_ptr<Field_descr>       _key_descr;
  //Bool                       _with_record;
    Bool                       _name_list_only;
    string                     _create_table;
};


//-----------------------------------------------------------------------------Columns_file_type

struct Columns_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "columns"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Columns_file> f = SOS_NEW( Columns_file );
        return +f;
    }
};

const Columns_file_type        _columns_file_type;
const Abs_file_type&            columns_file_type = _columns_file_type;

//---------------------------------------------------------------------------------------remark

static void remark( Record_type* t, const char* text ) 
{
    t->field_descr_ptr( t->field_count() - 1 )->_remark = text;
}

//-------------------------------------------------------------------Columns_file::Columns_file

Columns_file::Columns_file()
:
    _zero_ ( this+1 )
{
    _stack.obj_const_name( "Columns_file::_stack" ); 
    _column_type = Record_type::create();
    Record_type* t = _column_type;
    Column* o = 0;

    t->name( "Column" );
    t->allocate_fields( 15 );

    RECORD_TYPE_ADD_FIELD       ( nesting        , 0 );   remark( t, "Grad Verschachtelung. 1 ist die oberste Stufe" );
    RECORD_TYPE_ADD_LIMTEXT     ( column_name    , 0 );   remark( t, "Name des Feldes" );
    RECORD_TYPE_ADD_FIELD       ( data_type      , 0 );   remark( t, "SQL-Datentyp wie in ODBC" );
    RECORD_TYPE_ADD_LIMTEXT     ( type_name      , 0 );   remark( t, "Name des Datentyps" );
    RECORD_TYPE_ADD_FIELD_NULL  ( precision      , 0 );   remark( t, "Anzahl der Stellen zur Basis radix (s.u.)" );
    RECORD_TYPE_ADD_FIELD       ( display_size   , 0 );   remark( t, "Anzahl der Bytes für den ODBC-Default-Typ" );
    RECORD_TYPE_ADD_FIELD       ( length         , 0 );   remark( t, "Anzahl der für die interne Darstellung benötigten Bytes" );
    RECORD_TYPE_ADD_FIELD_NULL  ( scale          , 0 );   remark( t, "Anzahl der Nachkommastellen zur Basis radix" );
    RECORD_TYPE_ADD_FIELD_NULL  ( radix          , 0 );   remark( t, "Null, 2 oder 10: Basis für scale und precision" );
    RECORD_TYPE_ADD_FIELD       ( unsigned       , 0 );   remark( t, "ohne Vorzeichen" );
    RECORD_TYPE_ADD_FIELD       ( nullable       , 0 );   remark( t, "true, wenn Feld nichtige Werte enthält" );
    RECORD_TYPE_ADD_FIELD       ( offset         , 0 );   remark( t, "Position im Datensatz (beginnend bei 0)" );
    RECORD_TYPE_ADD_FIELD       ( size           , 0 );   remark( t, "Anzahl der zur Darstellung benötigten Zeichen" );
    RECORD_TYPE_ADD_LIMTEXT     ( remarks        , 0 );   remark( t, "Kommentar" );
    RECORD_TYPE_ADD_LIMTEXT     ( odbc_type_name , 0 );   remark( t, "ODBC-Typname" );
}

//------------------------------------------------------------------Columns_file::~Columns_file

Columns_file::~Columns_file()
{
}

//---------------------------------------------------------------------------Columns_file::open

void Columns_file::open( const char* parameter, Open_mode open_mode, const File_spec& spec )
{
    Any_file   file;
    Sos_string filename;
    Bool       key      = false;

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
      //if( opt.flag( "record" ) )  _with_record = opt.set();
      //else
        if( opt.flag( "key" ) )             key = true;                // Schlüssel statt Datensatz liefern
        else
        if( opt.flag( 'n', "name-list" ) )  _name_list_only = true;
        else
        if( opt.flag( "create-table" ) )    _create_table = "ORACLE";
        else
        if( opt.param() || opt.pipe() )  filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    if( open_mode & out )  throw_xc( "D127" );

    file.prepare( filename, open_mode, spec );

    if( key ) {
        Field_descr* f = file.spec()._key_specs._key_spec._field_descr_ptr;
        if( !f )  throw_xc( "SOS-1214" );
        _record_type = (Record_type*)f->type_ptr();
        if( !_record_type )  throw_xc( "SOS-1214" );
    } else {
        _record_type =  (Record_type*)file.spec().field_type_ptr();
    }
    //_key_descr_type = file.spec()._key_specs._key_spec._field_descr_ptr;

    file.close();

    if( !_record_type )  throw_xc( "SOS-1193" );

    if( !_name_list_only  &&  _create_table.empty() ) {
        _any_file_ptr->_spec._field_type_ptr = +_column_type;
    }

    _stack.add( +_record_type );
}

//---------------------------------------------------------------------Columns_file::get_record

void Columns_file::get_record( Area& buffer )
{
    Stack_entry* se = NULL;

    se = &_stack[ _stack.last_index() ];

    while( se->_index >= se->_record_type->field_count() )
    {
        if( _stack.last_index() == 0 )  throw_eof_error();
        _stack.last_index( _stack.last_index() - 1 );
        se = &_stack[ _stack.last_index() ];
        se->_index++;
    }

    Field_descr* f = se->_record_type->field_descr_ptr( se->_index );
    Field_type*  t = f->type_ptr();
    Type_param   type_param;
    Bool         is_record = false;

    int nesting = _stack.last_index() + 1;

    // Nächstes Feld:
    if( t && t->obj_is_type( tc_Record_type ) )  _stack.add( (Record_type*)t );      // verschachtelt
                                           else  se->_index++;

    if( _name_list_only ) {
        buffer.assign( f->name() );
        char* p = buffer.char_ptr();
        while( *p )  { if( *p == '-' )  *p = '_';  p++; }   // Cobol
        buffer += ',';
    }
    else
    {
        if( t->obj_is_type( tc_Record_type )        // Record_type?
         && ((Record_type*)t)->_group_type )        // Hat ein Gruppenfeld (geht über alle Felder)?
        {
            t = ((Record_type*)t)->_group_type;         // Das nehmen wir!
        }

        t->get_param( &type_param );


        if( !_create_table.empty() )
        {
            string line;

            line = f->name();
            line += " ";
            while( line.length() < 40 )  line += " ";

            // ...

            line += ",";
        }
        else
        {
            buffer.allocate_length( _column_type->field_size() );   //sizeof (Column) );
            memset( buffer.ptr(), 0, _column_type->field_size() );  //sizeof (Column) );
            Column* c = (Column*)buffer.ptr();
            new (c) Column;

            //jz 6.12.97 c->_nesting = _stack.last_index() + 1;
            c->_nesting = nesting;

            if( t ) 
            {
                c->_data_type      = odbc_sql_type( *t->info(), t->field_size() );
                //if( !c->_data_type )  throw_xc( "SOSODBC-12", f );   // 0: unbekannt (z.B. ein Record)

                ostrstream s ( c->_type_name.char_ptr(), c->_type_name.size() );
                s << *t;
                c->_type_name.length( s.pcount() );

                is_record = t->obj_is_type( tc_Record_type );
            }

            c->_column_name.assign( f->name() );

            c->_precision      = type_param._precision;
            c->_precision_null = type_param._precision == 0;
            c->_display_size   = type_param._display_size;
            c->_length         = odbc_c_default_type_length( type_param ); // Anzahl Bytes für SQL_C_DEFAULT
            c->_scale          = type_param._scale;
            c->_scale_null     = type_param._scale == 0; //type_param._scale_null;
            c->_radix          = type_param._radix;
            c->_radix_null     = c->_radix == 0;
            c->_unsigned       = type_param._unsigned;
            c->_nullable       = f->nullable()? SQL_NULLABLE : SQL_NO_NULLS;
            c->_offset         = f->offset();
            c->_size           = f->type_ptr()->field_size();
            c->_remarks        = f->_remark;


            string name = name_of_sqltype( c->_data_type );
    
            switch( c->_data_type )
            {
                case SQL_BIT          : break;
                case SQL_TINYINT      : break;
                case SQL_SMALLINT     : break;
                case SQL_INTEGER      : break;
                case SQL_BIGINT       : break;

                case SQL_CHAR         : 
                case SQL_VARCHAR      : 
                case SQL_LONGVARCHAR  : 
                case SQL_NUMERIC      :
                case SQL_DECIMAL      : 
                case SQL_BINARY       : 
                case SQL_VARBINARY    : 
                case SQL_LONGVARBINARY: name += "(" + as_string( c->_precision );
                                        if( c->_scale )  name += "," + as_string( c->_scale );
                                        name += ")";
                                        break;

                case SQL_REAL         : break;
                case SQL_FLOAT        : break;
                case SQL_DOUBLE       : break;
                case SQL_DATE         : break;
                case SQL_TIME         : break;
                case SQL_TIMESTAMP    : break;
            }

            if( !c->_nullable )  name += " NOT NULL";

            c->_odbc_type_name = name;
        }
    }
}


} //namespace sos

#endif

