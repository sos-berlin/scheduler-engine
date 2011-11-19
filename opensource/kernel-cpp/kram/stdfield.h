// stdfield.h                                            © 1995 SOS GmbH Berlin

#ifndef __STDFIELD_H
#define __STDFIELD_H

#if !defined __SOSFIELD_H
#   include "../kram/sosfield.h"
#endif

namespace sos
{

//------------------------------------------------------------------------------------Char_type

struct Char_type : Field_type
{
                                Char_type               ()                      : Field_type( &_type_info, sizeof (char) ) {}

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* p ) const { return *p == '\0'; }
    virtual void                set_null                ( Byte* p ) const       { *p = '\0'; }
    Bool                        empty                   ( const Byte* ) const;

    void                        write_text              ( const Byte* p, Area*, const Text_format& ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& ) const;
    void                        check_type              ( const char* ) {}

  //void                       _get_param               ( Type_param* ) const;

    static Listed_type_info    _type_info;
};

extern Char_type char_type;

DEFINE_ADD_FIELD( char, char_type );

//------------------------------------------------------------------------------------Bool_type

struct Bool_type : Field_type
{
                                Bool_type               ( int size = sizeof (Bool) )                                      : Field_type( &_type_info, size ) {}

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const Bool* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Bool_type bool_type;     // Bool
extern Bool_type bool_1_type;   // Byte

#if defined SYSTEM_BOOL
    DEFINE_ADD_FIELD( bool, bool_type );
#endif

//-------------------------------------------------------------------------------------Int_type

struct Int_type : Field_type
{
                                Int_type                ()                                      : Field_type( &_type_info, sizeof (int) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const  { return *(int*)a < *(int*)b? -1 : *(int*)a > *(int*)b? +1 : 0; }
  //virtual int                 op_add                  ( Byte* a, const Byte* b ) const        { *(int*)a += *(int*)b; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const int* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

typedef Int_type C_int_field;   // zur Kompatibilität, jz 10.3.96

extern Int_type int_type;

DEFINE_ADD_FIELD( int, int_type );

extern void write_text( int, Area* );
extern void read_text( int*, const char* );

//------------------------------------------------------------------------------------Uint_type

struct Uint_type : Field_type
{
                                Uint_type               ()                                      : Field_type( &_type_info, sizeof (uint) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const  { return *(uint*)a < *(uint*)b? -1 : *(uint*)a > *(uint*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const uint* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Uint_type uint_type;

DEFINE_ADD_FIELD( uint, uint_type );

//------------------------------------------------------------------------------------Int1_type

struct Int1_type : Field_type
{
                                Int1_type               ()                                      : Field_type( &_type_info, sizeof (int1) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(int1*)a < *(int1*)b? -1 : *(int1*)a > *(int1*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const int1* ) {}

    static Listed_type_info    _type_info;
};

extern Int1_type int1_type;

//-----------------------------------------------------------------------------------Uint1_type

struct Uint1_type : Field_type
{
                                Uint1_type              ()                                      : Field_type( &_type_info, sizeof (uint1) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(uint1*)a < *(uint1*)b? -1 : *(uint1*)a > *(uint1*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const uint1* ) {}

    static Listed_type_info    _type_info;
};

extern Uint1_type uint1_type;

//-----------------------------------------------------------------------------------Short_type

struct Short_type : Field_type
{
                                Short_type              ()                                      : Field_type( &_type_info, sizeof (short) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(short*)a < *(short*)b? -1 : *(short*)a > *(short*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const short* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Short_type short_type;

DEFINE_ADD_FIELD( short, short_type );

extern Short_type  int16_type;

//----------------------------------------------------------------------------------Ushort_type

struct Ushort_type : Field_type
{
                                Ushort_type             ()                                      : Field_type( &_type_info, sizeof (unsigned short) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(unsigned short*)a < *(unsigned short*)b? -1 : *(unsigned short*)a > *(unsigned short*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const unsigned short* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Ushort_type ushort_type;

DEFINE_ADD_FIELD( unsigned short, ushort_type );

extern Ushort_type   uint16_type;

//------------------------------------------------------------------------------------Long_type

struct Long_type : Field_type
{
                                Long_type               ()                                      : Field_type( &_type_info, sizeof (long) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(long*)a < *(long*)b? -1 : *(long*)a > *(long*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Long_type long_type;
DEFINE_ADD_FIELD( long, long_type );

typedef Long_type  Int32_type;
extern Int32_type  int32_type;

//-----------------------------------------------------------------------------------Ulong_type

typedef unsigned long ulong;

struct Ulong_type : Field_type
{
                                Ulong_type              ()                                      : Field_type( &_type_info, sizeof (ulong) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(ulong*)a < *(ulong*)b? -1 : *(ulong*)a > *(ulong*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const ulong* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

extern Ulong_type ulong_type;
DEFINE_ADD_FIELD( ulong, ulong_type );

extern Ulong_type  uint32_type;

//------------------------------------------------------------------------------------Int64_type
#ifdef SYSTEM_INT64

struct Int64_type : Field_type
{
                                Int64_type              ()                                      : Field_type( &_type_info, sizeof (int64) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(int64*)a < *(int64*)b? -1 : *(int64*)a > *(int64*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}

    static Listed_type_info    _type_info;
};

extern Int64_type int64_type;
DEFINE_ADD_FIELD( int64, int64_type );

#endif
//-----------------------------------------------------------------------------------Uint64_type
#ifdef SYSTEM_INT64

struct Uint64_type : Field_type
{
                                Uint64_type             ()                                      : Field_type( &_type_info, sizeof (uint64) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(uint64*)a < *(uint64*)b? -1 : *(uint64*)a > *(uint64*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}

    static Listed_type_info    _type_info;
};

extern Uint64_type uint64_type;
DEFINE_ADD_FIELD( uint64, uint64_type );

#endif
//-----------------------------------------------------------------------------Scaled_int64_type
#ifdef SYSTEM_INT64

struct Scaled_int64_type : Field_type
{
                                Scaled_int64_type       ( int scale )                           : Field_type( &_type_info, sizeof (int64) ), _scale(scale) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(int64*)a < *(int64*)b? -1 : *(int64*)a > *(int64*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}
    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;

    static Listed_type_info    _type_info;

    int                        _scale;                  // Position des Dezimalkommas >= 0, <= 19

  protected:
};

#endif
//----------------------------------------------------------------------------Scaled_uint64_type
#ifdef SYSTEM_INT64

struct Scaled_uint64_type : Field_type
{
                                Scaled_uint64_type      ( int scale )                           : Field_type( &_type_info, sizeof (uint64) ), _scale(scale) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(uint64*)a < *(uint64*)b? -1 : *(uint64*)a > *(uint64*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}
    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;

    static Listed_type_info    _type_info;

    int                        _scale;                  // Position des Dezimalkommas >= 0

  protected:
};

#endif
//---------------------------------------------------------------------------------Big_int_type
#if defined SYSTEM_INT64

    typedef Int64_type    Big_int_type;
    extern  Big_int_type& big_int_type;

/*
struct Big_int_type : Field_type
{
                                Big_int_type            ()                                      : Field_type( &_type_info, sizeof (Big_int) ) {}

    virtual int                 compare                 ( const Byte* a, const Byte* b ) const   { return *(Big_int*)a < *(Big_int*)b? -1 : *(Big_int*)a > *(Big_int*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const int* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

DEFINE_ADD_FIELD( Big_int, big_int_type );
*/

#else

    typedef Int32_type Big_int_type;
  //extern Big_int_type big_int_type;
    extern  Int32_type& big_int_type;

#endif

//-------------------------------------------------------------------------------Currency_type
#ifdef SYSTEM_INT64

struct Currency_type : Field_type
{
                                Currency_type           ()                                      : Field_type( &_type_info, sizeof (int64) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(int64*)a < *(int64*)b? -1 : *(int64*)a > *(int64*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const long* ) {}

    static Listed_type_info    _type_info;

  protected:
};

extern Currency_type currency_type;

#endif
//----------------------------------------------------------------------------------Double_type

struct Double_type : Field_type
{
                                Double_type             ()                                      : Field_type( &_type_info, sizeof (double) ), _scale(0), _scale_null(true) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return *(double*)a < *(double*)b? -1 : *(double*)a > *(double*)b? +1 : 0; }

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;
    void                        check_type              ( const double* ) {}
    double                      as_double               ( const Byte* p ) const                 { return *(double*)p; }

    static Listed_type_info    _type_info;

  protected:
    void                       _get_param               ( Type_param* ) const;

  public:
    int                        _scale;
    Bool                       _scale_null;
};

extern Double_type double_type;
DEFINE_ADD_FIELD( double, double_type );

//----------------------------------------------------------------------------------Text_type
// Feld mit Blanks aufgefüllt, beim Lesen werden die Blanks abgeschnitten;
// Nicht 0-terminiert.
// Mit 0x00 aufgefüllt: NULL

struct Text_type : Field_type
{
                                Text_type               ( int field_size )     : Field_type( &_type_info, field_size ) { _rtrim = true; }

#if defined SYSTEM_GNU
    void                        check_type              ( char** ) {}
#else
    void                        check_type              ( char(*)[] ) {}
#endif

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* p ) const { return *p == '\0'; }
    virtual void                set_null                ( Byte* p ) const;
    Bool                        empty                   ( const Byte* ) const;

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte*       p, const char*, const Text_format& f ) const;

    static Listed_type_info    _type_info;

  protected:
    uint                       _v_field_length          ( const Byte*, const Byte* ) const;
  //void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;
};

typedef Text_type Text_string;

//---------------------------------------------------------------------------------String0_type

struct String0_type : Field_type
{
    BASE_CLASS( Field_type )
                                String0_type            ( int field_size )     : Field_type( &_type_info, field_size + 1 ) {}

#if defined SYSTEM_GNU
    void                        check_type              ( char** ) {}
#else
    void                        check_type              ( char(*)[] ) {}
#endif

    Bool                        empty                   ( const Byte* p ) const             { return _v_field_length( p, p + field_size() ) == 0; }
    uint                        length                  ( const Byte* s ) const;
    void                        field_copy              ( Byte*, const Byte* ) const;
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;
    void                        read_other_field        ( Byte*, const Field_type*, const Byte*,
                                                          Area*, const Text_format& ) const;

    static Listed_type_info    _type_info;

//protected:
    void                       _get_param               ( Type_param* ) const;
    uint                       _v_field_length          ( const Byte*, const Byte* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_String0_type || Base_class::_obj_is_type( t ); }
};

typedef String0_type Text_string0;

extern String0_type const_string0_type;           // Nur zum Lesen

//-------------------------------------------------------------------------------------Iso_char

//typedef Text_char Iso_char;

//--------------------------------------------------------------------------------Iso_text_type

typedef Text_type Iso_text_type;
typedef Text_type Iso_string;

//-----------------------------------------------------------------------------Iso_string0_type

typedef String0_type Iso_string0_type;
typedef Text_string0 Iso_string0;

//-------------------------------------------------------------------------------Xlat_char_type

struct Xlat_char_type : Field_type
{
                                Xlat_char_type          ( const char* write_table, const Byte* read_table ) : Field_type( &_type_info, sizeof (char) ),_write_table(write_table),_read_table(read_table) {}

    DECLARE_PUBLIC_MEMBER( const char*, write_table  )
    DECLARE_PUBLIC_MEMBER( const Byte*, read_table   )

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* p ) const { return *p == 0x00; }
    virtual void                set_null                ( Byte* p ) const       { *p = 0x00; }
    Bool                        empty                   ( const Byte* ) const;

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
};

typedef Xlat_char_type Xlat_char;

//-------------------------------------------------------------------------------Xlat_text_type

struct Xlat_text_type : Field_type
{
                                Xlat_text_type          ( int field_size, const char* write_table, const Byte* read_table );

    uint                        field_size              () const  { return _field_size; }
    DECLARE_PUBLIC_MEMBER( const char*, write_table  )
    const Byte*                 read_table              () const                { return _read_table; }
    void                        read_table              ( const Byte* r_table ) { _read_table = r_table; _space = r_table[ (int) ' ' ]; }

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* p ) const       { memset( p, 0x00, _field_size ); }
    Bool                        empty                   ( const Byte* ) const;

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& ) const;

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
    uint                       _v_field_length          ( const Byte*, const Byte* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;

  private:
    const Byte*                _read_table;
    Byte                       _space;                  // == read_table()[ ' ' ]
};

typedef Xlat_text_type Xlat_string;

//------------------------------------------------------------------------------------Area_type

struct Area_type : Field_type
{
                                Area_type               ()                  : Field_type( &_type_info, sizeof (Area) ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return ::sos::compare( *(Area*)a,  *(Area*)b ); }

    virtual void                clear                   ( Byte* p ) const   { ((Area*)p)->length( 0 ); }
    virtual Bool                field_equal             ( const Byte*, const Byte* ) const;

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

    string                      as_string               ( const Byte* p ) const                 { return string( ((Area*)p)->char_ptr(), ((Area*)p)->length() ); }

    void                        check_type              ( const Area* ) {}

    static Listed_type_info    _type_info;

  protected:
  //void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;
};

extern Area_type area_type;

DEFINE_ADD_FIELD( Area, area_type );

//-------------------------------------------------------------------------------Null_area_type

struct Null_area_type : Area_type
{
    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* p ) const { return ((Area*)p)->length() == 0; }
    virtual void                set_null                ( Byte* p ) const       { ((Area*)p)->length( 0 ); }
    void                       _obj_print               ( ::std::ostream* s ) const;

  //static Listed_type_info    _type_info;

  //void                       _get_param               ( Type_param* ) const;
};

extern Null_area_type null_area_type;

//------------------------------------------------------------------------------Sos_string_type

#ifdef __SOSSTRNG_H

struct Sos_string_type : Field_type
{
                                Sos_string_type         ()          : Field_type( &_type_info, sizeof (Sos_string) ) {}

    static Listed_type_info    _type_info;

  protected:
    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const   { return strcmp( c_str( *(Sos_string*)a ), c_str( *(Sos_string*)b ) ); }

    void                        write_text              ( const Byte*, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& f ) const;

    void                        check_type              ( const Sos_string* ) {}

  //void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* s ) const;
};

extern Sos_string_type sos_string_type;

DEFINE_ADD_FIELD( Sos_string, sos_string_type );

#endif

//-----------------------------------------------------------------------Little_endian_int_type

struct Little_endian_int_type : Field_type
{
                                Little_endian_int_type  ( int bits )                            : Field_type( &_type_info, (bits+7) / 8 ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const;

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;

    void                       _obj_print               ( ::std::ostream* ) const;


    static Listed_type_info    _type_info;
};

extern Little_endian_int_type   little_endian_int_type;

//-----------------------------------------------------------------------Ulittle_endian_int_type

struct Ulittle_endian_int_type : Field_type
{
                                Ulittle_endian_int_type ( int bits )                            : Field_type( &_type_info, (bits+7) / 8 ) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const;

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;

    void                       _obj_print               ( ::std::ostream* ) const;

    static Listed_type_info    _type_info;
};

extern Ulittle_endian_int_type  ulittle_endian_int_type;

//----------------------------------------------------------------Scaled_ittle_endian_int_type

struct Scaled_little_endian_int_type : Field_type
{
                                Scaled_little_endian_int_type( int bits, int scale = 0 )        : Field_type( &_type_info, (bits+7) / 8 ), _scale(scale) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const;

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;

    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* ) const;

    int                        _scale;                  // 0 bis maximale Anzahl Dezimalziffern (5,10,19)

    static Listed_type_info    _type_info;
};

//-----------------------------------------------------------------------Scaled_ulittle_endian_int_type

struct Scaled_ulittle_endian_int_type : Field_type
{
                                Scaled_ulittle_endian_int_type ( int bits, int scale = 0 )      : Field_type( &_type_info, (bits+7) / 8 ), _scale(scale) {}

    virtual int                 op_compare              ( const Byte* a, const Byte* b ) const;

    void                        write_text              ( const Byte* p, Area* s, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char* s, const Text_format& f ) const;

    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( ::std::ostream* ) const;

    int                        _scale;

    static Listed_type_info    _type_info;
};

//----------------------------------------------------------------------------------Void_method

typedef void (Sos_object_base::*Void_method_ptr)();

//-----------------------------------------------------------------------------Void_method_type

struct Void_method_type : Method_type
{
    BASE_CLASS( Method_type )

    void                        call                    ( Void_method_ptr, Sos_object_base* ) const;

 //?void                        check_type              ( Void_method_ptr ) {}

  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Void_method_type || Base_class::_obj_is_type( t ); }
    void                       _obj_print               ( ::std::ostream* ) const;
};

extern Void_method_type void_method_type;

inline void add_method( Record_type* t, Void_method_ptr method_ptr, const char* name )
{
    t->add_method( &void_method_type, name, (Any_method_ptr)method_ptr );
}

} //namespace sos

#endif
