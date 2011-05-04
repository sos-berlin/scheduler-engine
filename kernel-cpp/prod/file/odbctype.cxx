//#define MODULE_NAME "odbctype"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#include "../kram/optimize.h"
#include "precomp.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_ODBC

#if defined SYSTEM_WIN
#   ifndef STRICT
#       define STRICT
#   endif
#   include <windows.h>

#endif


#include "sql.h"
#include "sqlext.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosfld2.h"
#include "../kram/stdfield.h"
#include "../kram/sosdate.h"
#include "../kram/log.h"
#include "odbctype.h"

//#define SOS_ORACLE_BOOL 1000

namespace sos {

//--------------------------------------------------------------------------------------------

Odbc_c_date_type        odbc_c_date_type;
Odbc_c_date_time_type   odbc_c_date_time_type;

//-----------------------------------------------------------------Odbc_c_date_type::_type_info

Type_info Odbc_c_date_type::_type_info;

SOS_INIT( odbc_c_date )
{
    Odbc_c_date_type::_type_info._std_type      = std_type_date;
    Odbc_c_date_type::_type_info._name          = "ODBC C Date";
    Odbc_c_date_type::_type_info._max_precision = 10;                 // "yyyy-mm-dd"
    Odbc_c_date_type::_type_info._max_precision = 10;                 // "yyyy-mm-dd"
    Odbc_c_date_type::_type_info.normalize();
}

//------------------------------------------------------------------------------name_of_sqltype

string name_of_sqltype( SWORD SqlType )
{
    switch( SqlType )
    {
        case SQL_CHAR         : return "SQL_CHAR";
        case SQL_VARCHAR      : return "SQL_VARCHAR";
        case SQL_LONGVARCHAR  : return "SQL_LONGVARCHAR";
        case SQL_BIT          : return "SQL_BIT";
        case SQL_TINYINT      : return "SQL_TINYINT";
        case SQL_SMALLINT     : return "SQL_SMALLINT";
        case SQL_INTEGER      : return "SQL_INTEGER";
        case SQL_BIGINT       : return "SQL_BIGINT";
        case SQL_NUMERIC      : return "SQL_NUMERIC";
        case SQL_DECIMAL      : return "SQL_DECIMAL";
        case SQL_REAL         : return "SQL_REAL";
        case SQL_FLOAT        : return "SQL_FLOAT";
        case SQL_DOUBLE       : return "SQL_DOUBLE";
        case SQL_DATE         : return "SQL_DATE";
        case SQL_TIME         : return "SQL_TIME";
        case SQL_TIMESTAMP    : return "SQL_TIMESTAMP";
        case SQL_BINARY       : return "SQL_BINARY";
        case SQL_VARBINARY    : return "SQL_VARBINARY";
        case SQL_LONGVARBINARY: return "SQL_LONGVARBINARY";
        default               : return "SQL_" + as_string( (int)SqlType );
    }
}
         
//---------------------------------------------------------------------------sqltype_is_numeric

bool sqltype_is_numeric( SWORD SqlType )
{
    switch( SqlType )
    {
      //case SQL_CHAR         : return false;
      //case SQL_VARCHAR      : return "SQL_VARCHAR";
      //case SQL_LONGVARCHAR  : return "SQL_LONGVARCHAR";
        case SQL_BIT          : 
        case SQL_TINYINT      : 
        case SQL_SMALLINT     : 
        case SQL_INTEGER      : 
        case SQL_BIGINT       : 
        case SQL_NUMERIC      : 
        case SQL_DECIMAL      : 
        case SQL_REAL         : 
        case SQL_FLOAT        : 
        case SQL_DOUBLE       : return true;
      //case SQL_DATE         : return "SQL_DATE";
      //case SQL_TIME         : return "SQL_TIME";
      //case SQL_TIMESTAMP    : return "SQL_TIMESTAMP";
      //case SQL_BINARY       : return "SQL_BINARY";
      //case SQL_VARBINARY    : return "SQL_VARBINARY";
      //case SQL_LONGVARBINARY: return "SQL_LONGVARBINARY";
        default               : return false;
    }
}
         
//--------------------------------------------------------------------------------name_of_ctype

string name_of_ctype( SWORD cType )
{
    switch( cType )
    {
        case SQL_C_BINARY:   return "SQL_C_BINARY";
        case SQL_C_BIT:      return "SQL_C_BIT";
        case SQL_C_CHAR:     return "SQL_C_CHAR";
        case SQL_C_DATE:     return "SQL_C_DATE";
        case SQL_C_DEFAULT:  return "SQL_C_DEFAULT";
        case SQL_C_DOUBLE:   return "SQL_C_DOUBLE";
        case SQL_C_FLOAT:    return "SQL_C_FLOAT";
        case SQL_C_LONG:     return "SQL_C_LONG";
        case SQL_C_SHORT:    return "SQL_C_SHORT";
        case SQL_C_SSHORT:   return "SQL_C_SSHORT";
        case SQL_C_SBIGINT:  return "SQL_C_SBIGINT";
        case SQL_C_UBIGINT:  return "SQL_C_UBIGINT";
        case SQL_C_TINYINT:  return "SQL_C_TINYINT";
        case SQL_C_STINYINT: return "SQL_C_STINYINT";
        case SQL_C_TIME:     return "SQL_C_TIME";
        case SQL_C_TIMESTAMP:return "SQL_C_TIMESTAMP";
        case SQL_C_SLONG:    return "SQL_C_SLONG";
        case SQL_C_ULONG:    return "SQL_C_ULONG";
        case SQL_C_USHORT:   return "SQL_C_USHORT";
        case SQL_C_UTINYINT: return "SQL_C_UTINYINT";
        default:             return "SQL_C_" + as_string(cType);
    }
}
         
//-----------------------------------------------------------------Odbc_c_date_type::write_text

void Odbc_c_date_type::write_text( const Byte* p, Area* buffer, const Text_format& f ) const
{
    //if( !field_size() )  return;   // null (?)

    Sos_optional_date date;

    date.assign_date( ((Odbc_c_date*)p)->year, ((Odbc_c_date*)p)->month, ((Odbc_c_date*)p)->day );
    sos_optional_date_type.write_text( (const Byte*)&date, buffer, f );
}

//------------------------------------------------------------------Odbc_c_date_type::read_text

void Odbc_c_date_type::read_text( Byte* p, const char* t, const Text_format& f ) const
{
    Sos_optional_date date;

    sos_optional_date_type.read_text( (Byte*)&date, t, f );

    ((Odbc_c_date*)p)->year  = date.year();
    ((Odbc_c_date*)p)->month = date.month();
    ((Odbc_c_date*)p)->day   = date.day();
}

//-----------------------------------------------------------------Odbc_c_date_type::_get_param

void Odbc_c_date_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}

//------------------------------------------------------------Odbc_c_date_time_type::_type_info

Type_info Odbc_c_date_time_type::_type_info;

SOS_INIT( odbc_c_date_time )
{
    Odbc_c_date_time_type::_type_info._std_type      = std_type_date_time;
    Odbc_c_date_time_type::_type_info._name          = "Odbc_c_timestamp";
    Odbc_c_date_time_type::_type_info._max_precision = 19;                 // "yyyy-mm-dd HH:MM:SS"
    Odbc_c_date_time_type::_type_info.normalize();
};

//------------------------------------------------------------Odbc_c_date_time_type::write_text

void Odbc_c_date_time_type::write_text( const Byte* p, Area* buffer, const Text_format& f ) const
{
    if( ((Odbc_c_date_time*)p)->fraction )  throw_xc( "SOS-1387" );

    Sos_optional_date_time date;

    date.assign_date( ((Odbc_c_date_time*)p)->year, ((Odbc_c_date_time*)p)->month, ((Odbc_c_date_time*)p)->day );
    date.set_time( ((Odbc_c_date_time*)p)->hour, ((Odbc_c_date_time*)p)->minute, ((Odbc_c_date_time*)p)->second );
    sos_optional_date_time_type.write_text( (const Byte*)&date, buffer, f );
}

//-------------------------------------------------------------Odbc_c_date_time_type::read_text

void Odbc_c_date_time_type::read_text( Byte* p, const char* t, const Text_format& f ) const
{
    Sos_optional_date_time date;

    sos_optional_date_time_type.read_text( (Byte*)&date, t, f );

    //? if( date.null() ) ... einfügen? jz 4.5.98

    ((Odbc_c_date_time*)p)->year     = date.year();
    ((Odbc_c_date_time*)p)->month    = date.month();
    ((Odbc_c_date_time*)p)->day      = date.day();

    ((Odbc_c_date_time*)p)->hour     = date.hour();
    ((Odbc_c_date_time*)p)->minute   = date.minute();
    ((Odbc_c_date_time*)p)->second   = date.second();
    ((Odbc_c_date_time*)p)->fraction = 0;
}

//------------------------------------------------------------Odbc_c_date_time_type::_get_param

void Odbc_c_date_time_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
}

//--------------------------------------------------------------------------odbc_c_default_type
/*
SWORD odbc_c_default_type( SWORD sql_type )
{
    for( int i = 0; i < NO_OF( sql_to_c_default ); i++ ) {
        if( sql_type == sql_to_c_default[ i ]->_sql )  return sql_to_c_default[ i ]._c_type;
    }
    return 0;
}
*/
//--------------------------------------------------------------------------------odbc_sql_type

SWORD odbc_sql_type( const Type_info& info, int size )  // s. ODBC Programmers Ref, Appendix D
{
    switch( info._std_type ) {
        case std_type_char     : return SQL_CHAR;
      //case std_type_varchar  : return info._max_size <= 254? SQL_VARCHAR : SQL_LONGVARCHAR;
        case std_type_varchar  : return size <= 254? SQL_VARCHAR : SQL_LONGVARCHAR;
        case std_type_decimal  : return SQL_DECIMAL;
        case std_type_date     : return SQL_DATE;
        case std_type_date_time: return SQL_TIMESTAMP;
        case std_type_time     : return SQL_TIME;
        case std_type_bool     : return SQL_BIT;
        case std_type_integer  :
            switch( size ) {
                case 1: return SQL_TINYINT;
                case 2: return SQL_SMALLINT;
                case 4: return SQL_INTEGER;
                default: return SQL_DECIMAL;    // ODBC kennt nicht int64. jz 14.2.00
            }
            break;
        case std_type_float    :
            switch( size ) {
                case 4: return SQL_REAL;
                case 8: return SQL_DOUBLE;
                default: break;
            }
            break;
        default: break;
    }

    LOG( "odbc_sql_type() Typ " << info._name << " kein SQL-Typ\n");
    return 0;
    //jz 17.4.00 Besser so?  return SQL_CHAR;
}

//--------------------------------------------------------------------------odbc_c_default_type

SWORD odbc_c_default_type( const Type_info& info, int size )  // s. ODBC Programmers Ref, Appendix D
{
    switch( info._std_type ) 
    {
        case std_type_char     : return SQL_C_CHAR;
        case std_type_varchar  : return SQL_C_CHAR;
        
        case std_type_decimal  : 
            if( info._max_precision >= 0 )
            {
                if( info._max_precision < 10  &&  info._max_scale == 0 )  return SQL_C_SLONG;
                if( info._max_precision <= 14                          )  return SQL_C_DOUBLE;
            }
            return SQL_C_CHAR;
        
        case std_type_date     : return SQL_C_DATE;
        case std_type_date_time: return SQL_C_TIMESTAMP;
        case std_type_time     : return SQL_C_TIME;

        case std_type_integer  :
            switch( size ) {
                case 1: return info._unsigned? SQL_C_UTINYINT : SQL_C_STINYINT;
                case 2: return info._unsigned? SQL_C_USHORT   : SQL_C_SSHORT;
                case 4: return info._unsigned? SQL_C_ULONG    : SQL_C_SLONG;
                case 8: return SQL_C_CHAR;
                default: break;
            }
            break;

        case std_type_float    :
            switch( size ) {
                case 4: return SQL_C_FLOAT;
                case 8: return SQL_C_DOUBLE;
                default: break;
            }
            break;

        default: break;
    }

    LOG( "odbc_c_default_type() Typ " << info._name << " kein C-Typ\n");
    
    return 0;
    //jz 17.4.00  Ist das besser: return SQL_C_CHAR;  // jz 17.4.00 Das geht immer
}

//-------------------------------------------------------------------odbc_c_default_type_length

SWORD odbc_c_default_type_length( SWORD fCType )
{
    switch( fCType )
    {
        case SQL_C_BINARY:   return SQL_NO_TOTAL;               // ???
        case SQL_C_BIT:      return 1;
      //case SQL_C_BOOKMARK:
        case SQL_C_CHAR:     return SQL_NO_TOTAL;               // ???
        case SQL_C_DATE:     return sizeof (Odbc_c_date);
        case SQL_C_DOUBLE:   return sizeof (double);
        case SQL_C_LONG:     return sizeof (long);
        case SQL_C_SLONG:    return sizeof (long);
        case SQL_C_SHORT:    return sizeof (short);
        case SQL_C_SSHORT:   return sizeof (short);
        case SQL_C_TINYINT:  return 1;
        case SQL_C_STINYINT: return 1;
        case SQL_C_TIME:     return sizeof (tagTIMESTAMP_STRUCT);
        case SQL_C_TIMESTAMP:return sizeof (Odbc_c_date_time);
        case SQL_C_ULONG:    return sizeof (unsigned long);
        case SQL_C_USHORT:   return sizeof (unsigned short);
        case SQL_C_UTINYINT: return 1;
        default: ;
    }
    return 0;
}

//-------------------------------------------------------------------odbc_c_default_type_length

SWORD odbc_c_default_type_length( const Type_param& param )
{
    SWORD fCType = odbc_c_default_type( *param._info_ptr, param._size );

    switch( fCType )
    {
        case SQL_C_BINARY:   return param._size;    //?
        case SQL_C_CHAR:     return param._display_size;
        default: ;
    }

    return odbc_c_default_type_length( fCType );
}

//-----------------------------------------------------------------------------sql_to_c_default

SWORD sql_to_c_default( SWORD SqlType, int v, Bool usigned, SWORD scale, SQLUINTEGER precision )
{
    // v==0x0200: ODBC 2.0

    switch( SqlType ) 
    {
        case SQL_CHAR         : return SQL_C_CHAR;
        case SQL_VARCHAR      : return SQL_C_CHAR;
        case SQL_LONGVARCHAR  : return SQL_C_CHAR;
        case SQL_BIT          : return SQL_C_BIT;
        case SQL_TINYINT      : return v < 0x0200? SQL_C_TINYINT : usigned? SQL_C_UTINYINT : SQL_C_STINYINT;
        case SQL_SMALLINT     : return v < 0x0200? SQL_C_SHORT   : usigned? SQL_C_USHORT   : SQL_C_SSHORT;
        case SQL_INTEGER      : return v < 0x0200? SQL_C_LONG    : usigned? SQL_C_ULONG    : SQL_C_SLONG;
        case SQL_BIGINT       : return SQL_C_CHAR;

        case SQL_NUMERIC      : //return SQL_C_CHAR;
        case SQL_DECIMAL      : 
        {
            if( scale == 0 ) 
            {
                //if ( precision == 1 )                  return SQL_C_BIT; //SOS_ORACLE_BOOL; // Konvention: Bool ist Scale 1!
                if( precision < 10 ) return v < 0x0200? SQL_C_LONG : usigned? SQL_C_ULONG : SQL_C_SLONG;
            } 
            return SQL_C_CHAR;
        }

        case SQL_REAL         : return SQL_C_FLOAT;
        case SQL_FLOAT        : return SQL_C_DOUBLE;
        case SQL_DOUBLE       : return SQL_C_DOUBLE;
        case SQL_DATE         : return SQL_C_DATE;
        case SQL_TIME         : return SQL_C_TIME;
        case SQL_TIMESTAMP    : return SQL_C_TIMESTAMP;
        case SQL_BINARY       : return SQL_C_BINARY;
        case SQL_VARBINARY    : return SQL_C_BINARY;
        case SQL_LONGVARBINARY: return SQL_C_BINARY;

        case -401             : return SQL_C_CHAR;          // Oracle 8: CLOB

                       default: break;
    }

    //jz 25.12.2001 return 0;
    return SQL_C_CHAR;          // Das geht meistens
}

//-----------------------------------------------------------Sos_odbc_binding::Sos_odbc_binding

Sos_odbc_binding::Sos_odbc_binding( const Sos_odbc_binding& o )
:
    _zero_ ( this+1 )
{
    *this = o;
}

//-----------------------------------------------------------Sos_odbc_binding::Sos_odbc_binding

Sos_odbc_binding& Sos_odbc_binding::operator= ( const Sos_odbc_binding& o )
{
    if( o._field ) {
        if( o.rgbValue() == o._buffer.byte_ptr() )  _field->offset( (long)_buffer.ptr() );
                                              else  _field = o._field;
    }

    _null_flag      = o._null_flag;
    _fParamType     = o._fParamType;
    _fCType         = o._fCType;
    _scale          = o._scale;
    _cbValueMax     = o._cbValueMax;

    if( o._pcbValue == &o._buffer_length )  _pcbValue = &_buffer_length;
                                      else  _pcbValue = o._pcbValue;

    _default_length = o._default_length;
    _buffer         = o._buffer;
    _buffer_length  = o._buffer_length;

    return *this;
}

//--------------------------------------------------------------------Sos_odbc_binding::prepare

void Sos_odbc_binding::prepare( Field_type* for_default )
{
    // _fCType, _cbValueMax müssen gesetzt sein.
    // _field._type_ptr und _default_length werden hier versorgt.

    SWORD                fCType = _fCType;

    if( fCType == SQL_C_DEFAULT  &&  for_default )
    {
        //Type_param param;
        //for_default->get_param( &param );
        fCType = odbc_c_default_type( *for_default->info(), for_default->field_size() );
    }

    //jz 21.5.97
    if( fCType == SQL_C_CHAR
     && _field 
     && _field->_type_ptr
     && _field->_type_ptr->obj_is_type( tc_String0_type )
     && ((String0_type*)+_field->_type_ptr)->field_size() == _cbValueMax )
    {
        // Der Typ war schon richtig
        return;
    }


    _field = Field_descr::create();
    _field->_precision = _precision;

    switch( fCType )
    {
      //case SQL_C_BINARY:

        case SQL_C_BIT:       _field->_type_ptr = &bool_1_type;       break;
      //case SQL_C_BOOKMARK:

        case SQL_C_CHAR: 
        {
            if( _cbValueMax <= 0 )  throw_xc( "Sos_odbc_binding::prepare", "_cbValueMax<=0?", _cbValueMax );
            
            Sos_ptr<String0_type> t = SOS_NEW( String0_type( _cbValueMax - 1 ) );

            if( _fSqlType == SQL_CHAR )  t->_rtrim = true;  // Hängende Blanks abschneiden (Oracle liefert sie)
            
            _field->_type_ptr = +t;

            if( _fSqlType == SQL_DECIMAL || _fSqlType == SQL_NUMERIC )
            {
                Sos_ptr<As_decimal_type> tt = SOS_NEW( As_decimal_type( +_field->_type_ptr, _precision, _scale, _nullable ) );
                _field->_type_ptr = +tt;
            }
            else
            if( sqltype_is_numeric( _fSqlType ) )
            {
                Sos_ptr<As_numeric_type> tt = SOS_NEW( As_numeric_type( +_field->_type_ptr ) );
                _field->_type_ptr = +tt;
            }

            break;
        }

        case SQL_C_DATE:      _field->_type_ptr = &odbc_c_date_type;   break;

        case SQL_C_DOUBLE: 
        {
            Sos_ptr<Double_type> t = &double_type;
            _field->_type_ptr = +t;
            break;
        }

      //case SQL_C_FLOAT:
        case SQL_C_LONG:
        case SQL_C_SLONG:     _field->_type_ptr = &long_type;          break;
        case SQL_C_SHORT:
        case SQL_C_SSHORT:    _field->_type_ptr = &short_type;         break;
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:  _field->_type_ptr = &int1_type;          break;
      //case SQL_C_TIME:
        case SQL_C_TIMESTAMP: _field->_type_ptr = &odbc_c_date_time_type;   break;
        case SQL_C_ULONG:     _field->_type_ptr = &ulong_type;         break;
        case SQL_C_USHORT:    _field->_type_ptr = &ushort_type;        break;
        case SQL_C_UTINYINT:  _field->_type_ptr = &uint1_type;         break;
      //case SQL_C_DEFAULT:
      //case SOS_ORACLE_BOOL: _field->_type_ptr = &bool_type;         break;

        default: {
            Sos_ptr<Field_descr> field = _field;
            _field = 0;
            throw_xc( "SOS-1353", field, fCType );  // S1C00?
        }
    }

    _default_length = odbc_c_default_type_length( fCType );
}

//--------------------------------------------------------------------Sos_odbc_binding::prepare

void Sos_odbc_binding::prepare( Field_descr* for_default )
{
    Field_type* type = for_default->type_ptr();

    prepare( type && type->obj_is_type( tc_Record_type ) && ((Record_type*)type)->_group_type? +((Record_type*)type)->_group_type : +type );
}

//---------------------------------------------------------------------------------------------

} //namespace sos

#endif
