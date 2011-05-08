//                                                  ©1996, SOS GmbH Berlin

#ifndef __ODBCTYPE_H
#define __ODBCTYPE_H

namespace sos
{

SWORD        odbc_sql_type               ( const Type_info&, int size );
inline SWORD odbc_sql_type               ( const Type_info& info )                    { return odbc_sql_type( info, info._max_size ); }
inline SWORD odbc_sql_type               ( const Type_param& param )                  { return odbc_sql_type( *param._info_ptr, param._size ); }
inline SWORD odbc_c_default_type         ( SWORD sql_type );
SWORD        odbc_c_default_type         ( const Type_info&, int size );
inline SWORD odbc_c_default_type         ( const Type_info& info )                    { return odbc_c_default_type( info, info._max_size ); }
inline SWORD odbc_c_default_type         ( const Type_param& param )                  { return odbc_c_default_type( *param._info_ptr, param._size ); }
SWORD        odbc_c_default_type_length  ( SWORD fCType );
SWORD        odbc_c_default_type_length  ( const Type_param& );
Sos_string   odbc_as_string              ( const void* ptr, SWORD len );
SWORD        sql_to_c_default            ( SWORD SqlType, int odbc_version, Bool usigned, SWORD scale, SQLUINTEGER precision );
string       name_of_sqltype             ( SWORD SqlType );
string       name_of_ctype               ( SWORD cType );


/*
extern const SWORD sql_to_c_default[ SQL_TYPE_MAX - SQL_TYPE_MIN + 1 ];

inline int odbc_c_default_type( SWORD sql_type )
{
    return sql_to_c_default[ sql_type - SQL_TYPE_MIN ];
}
*/
//-----------------------------------------------------------------------------Odbc_c_date_type

typedef tagDATE_STRUCT Odbc_c_date;

struct Odbc_c_date_type : Field_type
{
                                Odbc_c_date_type        () : Field_type( &_type_info, sizeof (Odbc_c_date) )  {}

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;

    void                        check_type              ( const Odbc_c_date* ) {}

    static Type_info           _type_info;

  protected:
    void                       _get_param               ( Type_param* ) const;
};

extern Odbc_c_date_type odbc_c_date_type;
DEFINE_ADD_FIELD( Odbc_c_date, odbc_c_date_type )

//------------------------------------------------------------------------Odbc_c_date_time_type

typedef tagTIMESTAMP_STRUCT Odbc_c_date_time;

struct Odbc_c_date_time_type : Field_type
{
                                Odbc_c_date_time_type   () : Field_type( &_type_info, sizeof (Odbc_c_date_time) )  {}

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;

    void                        check_type              ( const Odbc_c_date* ) {}

    static Type_info           _type_info;

  protected:
    void                       _get_param               ( Type_param* ) const;
};

extern Odbc_c_date_time_type odbc_c_date_time_type;
DEFINE_ADD_FIELD( Odbc_c_date_time, odbc_c_date_time_type )


//-----------------------------------------------------------------------------Sos_odbc_binding

struct Sos_odbc_binding         // Für SQLBindParameter(), SqlGetData() und SQLBindCol()
{
                                Sos_odbc_binding        () : _zero_(this+1),_null_flag(true), _default_length( SQL_NO_TOTAL ) {}
                                Sos_odbc_binding        ( const Sos_odbc_binding& );

    Sos_odbc_binding&           operator =              ( const Sos_odbc_binding& );

    void                        prepare                 ( Field_type* for_default = 0 );
    void                        prepare                 ( Field_descr* for_default );    // berücksichtigt _group_type
    void*                       rgbValue                () const            { return _field->ptr( 0 ); }

    Fill_zero                  _zero_;
  //Bool                       _valid;                  // Ist das ganze überhaupt gültig
    Sos_ptr<Field_descr>       _field;
    Bool                       _null_flag;              // _field._null_flag_offset == &_null_flag
    SWORD                      _fParamType;             // SQL_PARAM_INPUT, ...
    SWORD                      _fSqlType;               // SQL_TYPE_xxx
    SWORD                      _fCType;                 // SQL_C_TYPE_xxx
    SWORD                      _scale;
    SQLUINTEGER                _precision;              //        
    SDWORD                     _cbValueMax;             // Max. Größe
    SDWORD*                    _pcbValue;               // Länge, SQL_NTS, SQL_NULL_DATA
    SDWORD                     _default_length;
    SDWORD                     _length;                 // Für SQLBindParameter
    Dynamic_area               _buffer;                 // Für SQLPutData
    SDWORD                     _buffer_length;          // Für SQLPutData: _buffer_length = _buffer.length(); _pcbValue = &_buffer_length
    bool                       _nullable;               // Neu 25.10.2001 für Oracle und As_decimal, wird nur in odbc.cxx gesetzt
};

/*
    Sos_odbc_binding::_field auf Sos_ptr<> umstellen.
    In odbc.cxx kann der Record_type von der Anwendung noch verwendet werden, wenn
    die odbc-Datei schon längst geschlossen ist.
*/


} //namespace sos

#endif


