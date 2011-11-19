#include "precomp.h"
//#define MODULE_NAME "sossqlty"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sysdep.h"

#if defined SYSTEM_ODBC

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#include "sosstrng.h"

#include "sql.h"
#include "sqlext.h"

#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../kram/stdfield.h"
#include "../file/absfile.h"
#include "../kram/soslimtx.h"
#include "../file/odbctype.h"

using namespace std;
namespace sos {

//-------------------------------------------------------------------------------Odbc_data_type

struct Odbc_data_type
{
                                // Ein Destruktor wird nicht aufgerufen!

    Sos_limited_text<128>      _type_name;
    int2                       _data_type;
    int4                       _precision;
    Bool                       _precision_null;
    Sos_limited_text<128>      _literal_prefix;
    Bool                       _literal_prefix_null;
    Sos_limited_text<128>      _literal_suffix;
    Bool                       _literal_suffix_null;
    Sos_limited_text<128>      _create_params;
    Bool                       _create_params_null;
    int2                       _nullable;
    int2                       _case_sensitive;
    int2                       _searchable;
    Bool                       _searchable_null;
    int2                       _unsigned_attribute;
    Bool                       _unsigned_attribute_null;
    int2                       _money;
    int2                       _auto_increment;
    Bool                       _auto_increment_null;
    Sos_limited_text<128>      _local_type_name;
    Bool                       _local_type_name_null;
    int2                       _minimum_scale;
    Bool                       _minimum_scale_null;
    int2                       _maximum_scale;
    Bool                       _maximum_scale_null;
};

//-------------------------------------------------------------------------Odbc_data_types_file

struct Odbc_data_types_file : Abs_file
{
                                Odbc_data_types_file    ();
                               ~Odbc_data_types_file    ();

    void                        init                    ();
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        get_record              ( Area& );

  private:
    Sos_ptr<Record_type>       _odbc_data_type_type;
    Listed_type_info*          _ptr;
};


//--------------------------------------------------------------------Odbc_data_types_file_type

struct Odbc_data_types_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "odbc_data_types"; }
    virtual Sos_ptr<Abs_file> create_base_file          () const
    {
        Sos_ptr<Odbc_data_types_file> f = SOS_NEW( Odbc_data_types_file );
        return +f;
    }
};

const Odbc_data_types_file_type  _odbc_data_types_file_type;
const Abs_file_type&              odbc_data_types_file_type = _odbc_data_types_file_type;

// --------------------------------------------------Odbc_data_types_file::Odbc_data_types_file

Odbc_data_types_file::Odbc_data_types_file()
:
    _ptr ( Listed_type_info::_head )
{
}

//--------------------------------------------------Odbc_data_types_file::~Odbc_data_types_file

Odbc_data_types_file::~Odbc_data_types_file()
{
}

//-------------------------------------------------------------------Odbc_data_types_file::init

void Odbc_data_types_file::init()
{
    _odbc_data_type_type = Record_type::create();
    Record_type* t = _odbc_data_type_type;
    Odbc_data_type* o = 0;

    t->name( "Odbc_data_type" );
    t->allocate_fields( 15 );

    RECORD_TYPE_ADD_LIMTEXT     ( type_name         , 0 );
    RECORD_TYPE_ADD_FIELD       ( data_type         , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( precision         , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( literal_prefix    , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( literal_suffix    , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( create_params     , 0 );
    RECORD_TYPE_ADD_FIELD       ( nullable          , 0 );
    RECORD_TYPE_ADD_FIELD       ( case_sensitive    , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( searchable        , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( unsigned_attribute, 0 );
    RECORD_TYPE_ADD_FIELD       ( money             , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( auto_increment    , 0 );
    RECORD_TYPE_ADD_LIMTEXT_NULL( local_type_name   , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( minimum_scale     , 0 );
    RECORD_TYPE_ADD_FIELD_NULL  ( maximum_scale     , 0 );
}

//-------------------------------------------------------------------Odbc_data_types_file::open

void Odbc_data_types_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    init();

    if( open_mode & out )  throw_xc( "D127" );

    _any_file_ptr->_spec._field_type_ptr = +_odbc_data_type_type;
}

//-------------------------------------------------------------Odbc_data_types_file::get_record

void Odbc_data_types_file::get_record( Area& buffer )
{
TRYAGAIN:
    if( !_ptr )  throw_eof_error();

    buffer.allocate_min( _odbc_data_type_type->field_size() );  //sizeof (Odbc_data_type) );   sizeof(Odbc_data_type) ist auf 4Byte aufgerundet!
    buffer.length      ( _odbc_data_type_type->field_size() );  //sizeof (Odbc_data_type) );
    memset( buffer.ptr(), 0, sizeof (Odbc_data_type) );
    Odbc_data_type* c = (Odbc_data_type*)buffer.ptr();
    new (c) Odbc_data_type;

    Listed_type_info* t = _ptr;

    c->_type_name = t->_name;
    c->_data_type = odbc_sql_type( *t ); 
    if( c->_data_type == 0 )  { _ptr = _ptr->_tail; goto TRYAGAIN; }
    c->_precision = t->max_precision_10();
    c->_precision_null = t->_max_precision == 0;

    if( t->_quote ) {
        c->_literal_prefix = "\'";
        c->_literal_suffix = "\'";
    } else {
        c->_literal_prefix_null = true;
        c->_literal_suffix_null = true;
    }

    c->_create_params_null   = true;
    c->_nullable             = t->_nullable;
    c->_case_sensitive       = t->_std_type == std_type_char || t->_std_type == std_type_varchar;
    c->_searchable           = true;
    c->_unsigned_attribute   = t->_unsigned;
    c->_money                = false;
    c->_auto_increment       = false;
    c->_local_type_name_null = false;
    c->_minimum_scale        = t->_min_scale;
    c->_maximum_scale        = t->_max_scale;

    _ptr = t->_tail;
}

} //namespace sos

#endif
