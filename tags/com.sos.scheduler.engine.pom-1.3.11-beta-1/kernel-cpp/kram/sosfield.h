//sosfield.h                                            © 1995 SOS GmbH Berlin

#ifndef __SOSFIELD_H
#define __SOSFIELD_H

#if !defined __SOSSTRNG_H
#   include "../kram/sosstrng.h"        // Für Dyn_field_descr und Method_descr
#endif

#if !defined __XCEPTION_H
#    include "../kram/xception.h"
#endif

#if !defined __AREA_H
#    include "../kram/area.h"
#endif

#if !defined __SOSARRAY_H
#    include "../kram/sosarray.h"       // Copy_field u.a.
#endif

namespace sos
{

#define NEW_SOSFIELD_TYPES 1

const int max_field_name_length  =  100;
const int max_display_field_size = 2000;
const int default_field_size     = 1024;

//----------------------------------------------------------------------------Field_descr_flags

typedef uint Field_descr_flags;

                                 // 0x00FF     // zur freien Verwendung
const Field_descr_flags view_key  = 0x0100;    // Schlüssel
const Field_descr_flags view_db   = 0x0200;    // Feld für Datenbank
const Field_descr_flags view_sv   = 0x0400;    // Feld für Dynamic_dialog (Starview)

//---------------------------------------------------------------------------Method_descr_flags

typedef uint Method_descr_flags;

//-------------------------------------------------------------------------Field_copy_direction

enum Field_copy_direction { field_read, field_write };  // für copy_field()

//-------------------------------------------------------------------------------------forwards

//bc5 typedef Sos_string;
struct  SOS_CLASS Dyn_obj;
struct  String0_area;
struct  SOS_CLASS Method_type;
struct  Record_type;
struct  Field_descr;
//struct  Xlat_string;        // Solaris 4.0.1
template<class T> struct Sos_array;

//------------------------------------------------------------------------field_names_are_equal

Bool field_names_are_equal( const char*, const char* );

//----------------------------------------------------------------------------------Text_format

// gehört in ein allgemeines header file:
/*  Textformate:
    Felder durch TAB getrennt, Strings ohne Anführungszeichen, TAB mit Backslash
    Felder durch TAB getrennt, Strings in Anführungszeichen, C-Notation mit Backslash
    MS-Windows-reginale Notation: Felder durch Listentrenner getrennt, Strings ohne Anführungszeichen
    Feldnamen einfügen mit Gleichheitszeichen: name=value
    Typnamen einfügen mit Klammern: Typ(value)
*/

struct Text_format
{
                                Text_format             () : /*_char_quote ( '\'' ), _string_quote ( '"' ), _escape_quote( true ), _with_name ( false ),  */ _raw(false), _scale(0), _text(false), _german(false), _separator ( '\t' ),_decimal_symbol( '.' ), _date_code(0),_date_time_code(0) {}


  //Text_format&                char_quote              ( char c )   { _char_quote = c; return *this; }
  //char                        char_quote              () const     { return _char_quote; }
  //Text_format&                string_quote            ( char c )   { _string_quote = c; return *this; }
  //char                        string_quote            () const     { return _string_quote; }
    void                        has_separator           ( Bool )     { _separator = 'a'; }
    Bool                        has_separator           () const     { return _separator != 'a'; }
    Text_format&                separator               ( char c )   { _separator = c; return *this; }
    char                        separator               () const     { return _separator; }
    char                        decimal_symbol          () const     { return _decimal_symbol; }
    void                        decimal_symbol          ( char c )   { _decimal_symbol = c; }
  //Text_format&                with_name               ( Bool b )   { _with_name = b; return *this; }
  //Bool                        with_name               () const     { return _with_name; }
  //Text_format&                with_nesting            ( Bool b )   { _with_nesting = b; return *this; }
  //Bool                        with_nesting            () const     { return _with_nesting; }

    // folgende Date-Methoden sind in sosdate.cxx implementiert:
    const char*                 date                    () const;    // Datumsformat (s. sosdate.h)
    void                        date                    ( const char* ); // Nur standardisierte Formate möglich (s. sosdate.h)
    void                        date_code               ( int i )      { _date_code = i; }  // siehe sosfield.cxx
    int                         date_code               () const        { return _date_code; }

    const char*                 date_time               () const;    // Datumzeitformat (s. sosdate.h)
    void                        date_time               ( const char* ); // Nur standardisierte Formate möglich (s. sosdate.h)
    void                        date_time_code          ( int i )      { _date_time_code = i; }  // siehe sosfield.cxx
    int                         date_time_code          () const        { return _date_time_code; }

    DECLARE_PUBLIC_MEMBER( Bool, raw )                  // Möglichst original
    DECLARE_PUBLIC_MEMBER( int , scale )                // Anzahl der Nachkommastellen bei Ganzzahl
    DECLARE_PUBLIC_MEMBER( Bool, text )                 // Text ist als Text, nicht numerisch zu interpertieren.
                                                        // Ebcdic_numeric_type(6).read_text(p,"12") --> F1F200000000
                                                        // Für sossql4.cxx like-Zugriffsoptimierung
  //DECLARE_PUBLIC_MEMBER( Bool, escape_quote )         // true: '\' voranstellen, false: Quote verdoppeln
    DECLARE_PUBLIC_MEMBER( Bool, german)                // Ebcdic_text soll nach deutscher Code-Variane umsetzen (äöü statt {|})

  private:
  //char                       _char_quote;
  //char                       _string_quote;
    char                       _separator;
    char                       _decimal_symbol;
  //Bool                       _with_name;
  //Bool                       _with_nesting;
    Byte                       _date_code;
    Byte                       _date_time_code;
};

const extern Text_format std_text_format;
      extern Text_format raw_text_format;

//-------------------------------------------------------------------------------------Std_type
#ifndef _hostware_type_library_H_   // Std_type ist identisch in hostole.odl definiert 

enum Std_type
{
    std_type_none = 0,
    std_type_char = 1,              // Feste Länge, Blanks am Ende zählen nicht
    std_type_varchar = 2,           // variable Länge, Blanks am Ende zählen
    std_type_decimal = 3,           // Festkomma, mit Präzision und Skalierung
    std_type_integer = 4,           // Big_int (weitester Typ), ohne Skalierung (?)
    std_type_float = 5,             // Big_float (weitester Typ)
    std_type_date = 6,              // Datum
    std_type_time = 7,              // Zeit
    std_type_date_time = 8,         // Datum und Zeit,
    std_type_bool = 9
}; 

#endif

//-----------------------------------------------------------------------------------is_numeric

inline Bool is_numeric( Std_type t ) { return t == std_type_decimal
                                           || t == std_type_integer
                                           || t == std_type_float; }

//------------------------------------------------------------------------------------Type_info

struct Type_info
{
                                Type_info               ();
                               ~Type_info               ();

    void                        normalize               ();     // Nur für statische Variablen aufrufen!
    int                         max_precision_10        () const;

    Fill_zero                  _zero_;
    Std_type                   _std_type;
    const char*                _name;
    Bool                       _unsigned;
    Bool                       _nullable;
    int                        _max_size;
    int                        _max_precision;
    int                        _radix;                      // 0, 2 oder 10: Basis für _max_precision
    int                        _min_scale;
    int                        _max_scale;
    Bool                       _quote;
    Bool                       _exact_char_repr;
    Bool                       _field_copy_possible;
    int                        _alignment;                  //? Array_type
    int                        _display_size;
  //Byte                       _reserve [ 64 ];
};

//const extern Type_info default_type_info;

//-----------------------------------------------------------------------------Listed_type_info

struct Listed_type_info : Type_info                             // Nur für statische Variablen!
{
                                Listed_type_info        ();
                               ~Listed_type_info        ();

    void                        normalize               ();     

    Listed_type_info*          _tail;
    static Listed_type_info*   _head;

  private:
                                Listed_type_info        ( const Listed_type_info& );  // Nicht implementiert
    Listed_type_info&           operator =              ( const Listed_type_info& );  // Nicht implementiert
};

//-----------------------------------------------------------------------------------Type_param
// Wird von Type_info::get_param() gefüllt:

struct Type_param
{
    int                         precision_10        () const;           // Präzision bei radix=10 oder radix=0

    Std_type                   _std_type;
    int                        _size;               // in Bytes
    int                        _display_size;       // in Bytes oder 0
    int                        _precision;          // Anzahl relevanter Stellen, wenn numerisch
    int                        _radix;              // 0, 2 oder 10: Basis für _precision
    int                        _scale;              // Anzahl Nachkommastellen
    Bool                       _unsigned;
    const Type_info*           _info_ptr;
    Bool                       _scale_null;         // true: _scale ist ungültig
};

//-----------------------------------------------------------------------------------Field_type

struct Field_type : Sos_self_deleting                   // Beschreibt einen Feldtyp
{
    BASE_CLASS( Sos_self_deleting )

                                Field_type              ( const Type_info* info, uint size ) : _zero_(this+1),_info(info),_field_size( size )     { obj_const_name( info->_name ); }
                              //Field_type              ( const Listed_type_info* info, uint size ) : _info(info),_field_size( size )     { obj_const_name( info->_name ); }
#   if defined SYSTEM_SOLARIS
                               ~Field_type              ();     // (Solaris 4.0.1)
#   endif

    // Nur übergangsweise:
    int                         field_count             () const;
    const Field_descr&          field_descr             ( int i ) const;
    Field_descr*                field_descr_ptr         ( int i );
    Field_descr*                field_descr_ptr         ( const char* name );
    Field_descr*                field_descr_ptr         ( const string& name )                                                      { return field_descr_ptr( name.c_str() ); }
    Field_descr*                field_descr_by_name_or_0( const string& name )                                                      { return field_descr_by_name_or_0( name.c_str() ); }
    Field_descr*                field_descr_by_name_or_0( const char* name );
    const Field_descr&          named_field_descr       ( int i ) const;

    uint                        field_length            ( const Const_area& area ) const                                            { return field_length( area.byte_ptr(), area.byte_ptr() + area.length() ); }
    uint                        field_length            ( const Byte* ptr, const Byte* end_ptr = 0 ) const /* Anzahl der Bytes */   { return _v_field_length( ptr, end_ptr? end_ptr : ptr + field_size() ); }
    uint                        field_size              () const /* Anzahl der Bytes */                                             { return _field_size; }

    virtual Field_type*         simple_type             ()                                                                          { return this; }
    virtual void                field_copy              ( Byte* p, const Byte* s ) const; // Kopierkonstruktor

    virtual Bool                nullable                () const; // Feldwert kann nichtig sein?
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* ) const;
    virtual Bool                empty                   ( const Byte* ) const;  // Liefert Leer-String? Für Field_subtype
    Bool                       _empty                   ( const Byte* ) const;  // true, wenn write_text() Leerstring liefert
    virtual Bool                empty_is_null           () const;
    virtual void                write_null_as_empty     ( Bool )                                                                    {}

    virtual void                construct               ( Byte* ) const;
    virtual void                destruct                ( Byte* ) const;
    virtual void                clear                   ( Byte* ) const;
    virtual Bool                field_equal             ( const Byte*, const Byte* ) const;
    virtual int                 alignment               () const;
    Byte*                       align                   ( Byte* p       ) const { return       (Byte*)round_up( (int)p, alignment() ); }
    const Byte*                 align                   ( const Byte* p ) const { return (const Byte*)round_up( (int)p, alignment() ); }

  //void                        print_selected          ( const Byte* p, ostream* s, const Text_format& f,
  //                                                      const Sos_array<int>& field_numbers ) const;
  //void                        input_selected          ( Byte* p, istream* s, const Text_format& f,
  //                                                      const Sos_array<int>& field_numbers ) const;
  //virtual void                write_text_selected     ( const Byte* p, Area*, const Text_format&,
  //                                                      const Sos_array<int>& field_numbers ) const;
  //virtual void                read_text_selected      ( Byte*, const Const_area&, const Text_format&,
  //                                                      const Sos_array<int>& field_numbers ) const;
    virtual void                write_text              ( const Byte*, Area*, const Text_format& = raw_text_format ) const = 0;
    virtual void                read_text               ( Byte*, const char*, const Text_format& = raw_text_format  ) const = 0;
            void                print                   ( const Byte*, ::std::ostream*, const Text_format&,
                                                          char quote = '\0', char quote_quote = '\\' ) const;
            void                input                   ( Byte*, ::std::istream*, const Text_format& ) const;
    void                        assign                  ( Byte*, const Dyn_obj&, Area* hilfspuffer = 0 ) const;

    virtual void                read_other_field        ( Byte*, const Field_type*, const Byte*,
                                                          Area* /*hilfspuffer*/, const Text_format& = raw_text_format ) const;

    const Type_info*            info                    () const                                { return _info; }
    void                        get_param               ( Type_param* ) const;
    virtual bool                is_numeric              () const                                { return sos::is_numeric( _info->_std_type ); }     // jz 27.5.01 für Fehrmann, Dia-Nielsen, s. Record_type


    virtual string              as_string               ( const Byte* ) const;
    virtual int                 as_int                  ( const Byte* ) const;
    virtual int64               as_int64                ( const Byte* ) const;
    virtual Big_int             as_big_int              ( const Byte* ) const;
    virtual double              as_double               ( const Byte* ) const;
  //virtual Sos_optional_date_time as_date_time         ( const Byte* ) const;
    virtual int                 op_compare              ( const Byte*, const Byte* ) const;
    virtual void                op_add                  ( Byte*, const Byte* ) const;


    Fill_zero                  _zero_;
    uint                       _field_size;
    Bool                       _rtrim;                  // Rechte Blanks abschneiden, nur bei manchen Typen

  protected:
    virtual void               _get_param               ( Type_param* ) const;
    virtual uint               _v_field_length          ( const Byte*, const Byte* ) const { return field_size(); }     // Anzahl der Bytes

    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Field_type || Base_class::_obj_is_type( t ); }
    void                       _obj_print               ( ::std::ostream* ) const;

    const Type_info*           _info;
};

//--------------------------------------------------------------------------------Field_subtype

struct Field_subtype : Field_type   // Für einen Typ, der auf einen anderen Typ aufbaut, also Text_date_type, Numeric_type etc.
{
    BASE_CLASS( Field_type )

                                Field_subtype           ( const Sos_ptr<Field_type>& base_type );
                                Field_subtype           ( const Type_info*, const Sos_ptr<Field_type>& base_type );

    Field_type*                 base_type               () const                                { return _base_type; }
    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_Field_subtype || Base_class::_obj_is_type( t ); }

    virtual Bool                nullable                () const;
    virtual Bool                null                    ( const Byte* p ) const;
    virtual void                set_null                ( Byte* p ) const;
    virtual Bool                empty_is_null           () const;
    virtual void                construct               ( Byte* ) const;
    virtual void                destruct                ( Byte* ) const;
    virtual void                clear                   ( Byte* ) const;
    virtual Bool                field_equal             ( const Byte*, const Byte* ) const;
    virtual int                 alignment               () const;

  protected:
    void                       _obj_print               ( ::std::ostream* s ) const;

    Type_info                  _type_info;
    Sos_ptr<Field_type>        _base_type;
    Bool                       _empty_is_null;
};

//----------------------------------------------------------------------------------Field_descr

struct Field_descr : Sos_self_deleting                  // Beschreibt ein Feld in einem Satz (Name, Offset, Typ, Nullflagoffset)
{
    BASE_CLASS( Sos_self_deleting )
    DEFINE_OBJ_COPY( Field_descr )                      // Virtueller Copy-Konstruktor

                                Field_descr             ();
                                Field_descr             ( const Sos_ptr<Field_type>& type,
                                                          const char*                name_ptr,
                                                          long                       offset           = -1,
                                                          long                       null_flag_offset = -1,
                                                          Field_descr_flags          flags            = 0 );
                                Field_descr             ( const Field_descr& );  
                               ~Field_descr             ();

    static Sos_ptr<Field_descr> create                  ();
    static Sos_ptr<Field_descr> create                  ( const Field_descr& );

    Field_descr&                operator =              ( const Field_descr& );

    const Field_type&           type                    () const                        { return *_type_ptr;   }
    Field_type*                 type_ptr                () const                        { return _type_ptr;   }

    template< class T >
    void                        set_type                ( const T& t )                  { _type_ptr = +t; }

    Field_type*                 simple_type             () const                        { return _type_ptr? _type_ptr->simple_type() : NULL; }
    long                        offset                  () const                        { return _offset; }
    void                        offset                  ( long o )                      { _offset = o; }
    Byte*                       ptr                     ( Byte* p ) const               { return (Byte*)( (long)p + _offset ); }
    const Byte*                 const_ptr               ( const Byte* p ) const         { return (const Byte*)( (long)p + _offset ); }
    void                        name                    ( const Sos_string& name )      { _name = name; }
    void                        name                    ( const char* name )            { ::sos::assign( &_name, name ); }
    void                        name_ptr                ( const char* name       )      { ::sos::assign( &_name, name ); }
    const char*                 name                    () const                        { return c_str( _name );  }
    void                        write_text              ( const Byte*      , Area*, const Text_format& = raw_text_format ) const;
    void                        write_text              ( const Const_area&, Area*, const Text_format& = raw_text_format ) const;

    void                        read_text               ( Byte*, const char*, const Text_format& = raw_text_format  ) const;
    void                        read_text               ( Area*, const char*, const Text_format& = raw_text_format  ) const;

    void                        set_string              ( Byte* p, const string& str, const Text_format& f = raw_text_format  ) const { read_text( p, str.c_str(), f ); } 
    void                        set_string              ( Area* r, const string& str, const Text_format& f = raw_text_format  ) const { read_text( r, str.c_str(), f ); } 

    void                        print                   ( const Byte*      , ::std::ostream*, const Text_format&,
                                                          char quote = '\0', char quote_quote = '\\' ) const;
    void                        print                   ( const Const_area&, ::std::ostream*, const Text_format&,
                                                          char quote = '\0', char quote_quote = '\\' ) const;
    void                        input                   ( Byte*, ::std::istream*, const Text_format& ) const;
    void                        input                   ( Area*, ::std::istream*, const Text_format& ) const;
    void                        assign                  ( Byte*, const Dyn_obj&, Area* hilfspuffer = 0 ) const;
    void                        assign                  ( Byte*, const Field_type*, const Byte*, Area* hilfspuffer ) const;
    void                        assign                  ( Byte*, const Field_descr*, const Byte*, Area* hilfspuffer ) const;

    string                      as_string               ( const Byte* p ) const         { return type_ptr()->as_string( const_ptr( p ) ); }
    int                         as_int                  ( const Byte* p ) const         { return type_ptr()->as_int( const_ptr( p ) ); }
    int64                       as_int64                ( const Byte* p ) const         { return type_ptr()->as_int64( const_ptr( p ) ); }
    Big_int                     as_big_int              ( const Byte* p ) const         { return type_ptr()->as_big_int( const_ptr( p ) ); }
    double                      as_double               ( const Byte* p ) const         { return type_ptr()->as_double( const_ptr( p ) ); }
    Bool                        has_null_flag           () const                        { return _null_flag_offset != -1; }
    Bool                        null_flag               ( const Byte* p ) const         { return *(Bool*)( (long)p + _null_flag_offset ); }
    void                        set_null_flag           ( const Byte* p, Bool value = true ) const { *(Bool*)( (long)p + _null_flag_offset) = value; }

    Bool                        nullable                () const                        { return has_null_flag() || type().nullable(); }
    Bool                        null                    ( const Byte* p ) const;
    void                        set_null                ( Byte* ) const;

    void                        add_to                  ( Record_type* );        // setzt ggfs. _offset
    void                        add_null_flag_to        ( Record_type* );        // setzt ggfs. _null_flag_offset

    void                        construct               ( Byte* record_ptr ) const;
    void                        clear                   ( Byte* record_ptr ) const;

    void                        write_null_as_empty     ( Bool );  // set_null() -> "", wenn std_type == std_type_char
    Bool                        write_null_as_empty     () const      { return _write_null_as_empty; }


    long                       _offset;                 // kann auch absolute Adresse sein
    long                       _null_flag_offset;       // kann auch absolute Adresse sein
    Field_descr_flags          _flags;
    Sos_ptr<Field_type>        _type_ptr;
    int                        _precision;

    DECLARE_PRIVATE_MEMBER( Bool, read_only )           // wird von sosfield nicht geprüft!
    Sos_string                 _remark;

  protected:
    void                        write_text_shorted      ( const Const_area&, Area*, const Text_format& = raw_text_format ) const;
    void                        print_shorted           ( const Const_area& a, ::std::ostream* s, const Text_format& f,
                                                          char quote, char quote_quote ) const;
    void                       _xc_insert_field_name    ( Xc* ) const;
  //virtual const Field_type&   type_virtual            () const = 0;
    void                       _obj_print               ( ::std::ostream* ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Field_descr || Base_class::_obj_is_type( t ); }

    Sos_string                 _name;
    Bool                       _write_null_as_empty;    // set_null() --> read_text( "" )
};

typedef Field_descr     Dyn_field_descr;
typedef Dyn_field_descr Dyn_named_field_descr;

//----------------------------------------------------------------------------Array_field_descr

struct Array_field_descr : Field_descr
{
    BASE_CLASS( Field_descr )
    DEFINE_OBJ_COPY( Array_field_descr )                // Virtueller Copy-Konstruktor

                                Array_field_descr       ();
                               ~Array_field_descr       ();

    long                        elem_offset             ( int index, int dim = 0 ) const;
    void                        elem_print              ( int, const Byte*, ::std::ostream*, const Text_format&,
                                                          char quote = '\0', char quote_quote = '\\' ) const;
    void                        elem_write_text         ( int, const Byte*, Area*, const Text_format& ) const;
    void                        set_array_distances     ( Array_field_descr* outer = 0 );

    void                       _obj_print               ( ::std::ostream* ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Array_field_descr || Base_class::_obj_is_type( t ); }

    struct Dim {
        int                    _first_index;
        int                    _elem_count;
        int                    _distance;               // Felddistanzen (wegen Cobol)
    };

    Fill_zero                  _zero_;
    Dim                        _dim [ 3 ];              // max. dreidimensional
    int                        _elem_count [ 3 ];
    int                        _distance [ 3 ];         // Felddistanzen (wegen Cobol)
    int                        _level;                  // Nummer des ersten hier neu beschriebenen Dimension
    int                        _dim_count;              // Anzahl der Dimensionen einschließlich diese Felds

  private:
    void                       _xc_insert_field_name    ( Xc*, int index ) const;
    void                       _throw_index_error       ( int index ) const;
};

//-----------------------------------------------------------------------------------Any_method

typedef void (__cdecl Sos_object_base::*Any_method_ptr)(...);

//----------------------------------------------------------------------------------Method_type

struct Method_type : Field_type
{
    BASE_CLASS( Field_type )

                                Method_type             ();
                               ~Method_type             ();

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;

  //void                        call                    ( Any_method_ptr, Sos_object_base*, void* result, void* params );
  //void                        call                    ( Void_method_ptr m, Sos_object_base* o ) { o->*m(); }

    Sos_ptr<Field_type>        _result_type;            // 0: void f(...)
    Sos_ptr<Field_type>        _object_type;
    Sos_static_ptr<Record_type> _param_type;             // 0: f()
  //Sos_ptr<Record_type>       _param_type;             // 0: void f();  1. Parameter: Ergebnis, 2.: this, ab 3.: Argumente

  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Method_type || Base_class::_obj_is_type( t ); }
    void                       _obj_print               ( ::std::ostream* s ) const;
};

//---------------------------------------------------------------------------------Method_descr

struct Method_descr : Sos_self_deleting                  // Beschreibt eine Methode in einem Objekt
{
                                Method_descr            ( const Sos_ptr<Method_type>&, const char* name, Any_method_ptr, Method_descr_flags = 0 );

    const Method_type&          type                    () const                        { return *_type;   }
    Method_type*                type_ptr                () const                        { return _type;   }
    void                        name                    ( const Sos_string& name )      { _name = name; }
    void                        name_ptr                ( const char* name       )      { ::sos::assign( &_name, name ); }
    const char*                 name                    () const                        { return c_str( _name );  }

    void                        call                    ( Byte* record_ptr ) const      { (((Sos_object_base*)record_ptr)->*_method)(); }

    Any_method_ptr             _method;
    Method_descr_flags         _flags;

  protected:
    void                       _obj_print               ( ::std::ostream* ) const;

    Sos_string                 _name;
    Sos_ptr<Method_type>       _type;
};

//----------------------------------------------------------------------------------Record_type

struct Record_type : Field_type      // Aus Feldern zusammengesetztes Feld (Satz, entspricht struct in C)
{
    BASE_CLASS( Field_type )

                                Record_type             ( int default_field_count = 0 );
                               ~Record_type             ();

    static Sos_ptr<Record_type> create                  ();
    Fill_zero                  _zero_;

    virtual Field_type*         simple_type             ();
    void                        write_null_as_empty     ( Bool );                               // set_null() -> "", wenn std_type == std_type_char

    void                        name                    ( const Sos_string& n )                 { _name = n; }
    const Sos_string&           name                    () const                                { return _name; }
    void                        name                    ( int i, const Sos_string& name )       { _field_descr_array[ i ]->name( name ); }

    void                        add_field               ( const Sos_ptr<Field_descr>& f )       { add_field( +f ); }
    void                        add_field               ( Field_descr* );
    void                        add_field               ( const Sos_ptr<Field_type>&, const char* name, long offset = -1, long null_offset = -1, Field_descr_flags = 0 );
    void                        add_field               ( Field_type*, const char* name, long offset, long null_offset = -1, Field_descr_flags = 0 ); // Platzoptimierung
    Field_descr*                add_field               ( const char* type_definition, const char* field_name, bool nullable = false );
    Field_descr*                add_field               ( const string& type_definition, const string& field_name, bool nullable = false )  { return add_field( c_str(type_definition), c_str(field_name), nullable ); }
    void                        append_fields           ( const Sos_ptr<Record_type>& );

    Field_descr*                last_field_descr        ()                                      { return field_descr_ptr( _field_count - 1 ); }

    int                         field_count             () const                                { return _field_count; }
    void                        field_count             ( int c )                               { _field_count = c; }
    int                         field_index             ( const char* name ) const;
  //const Field_descr&          field_descr             ( const char* name ) const              { return _field_descr( field_index( name ) ); }
    const Field_descr&          field_descr             ( int i ) const                         { return *_field_descr( i ); }
    Field_descr*                field_descr_ptr         ( int i ) const                         { return _field_descr( i ); }
    Field_descr*                field_descr_ptr         ( const char* name ) const              { return _field_descr_by_name( name ); }
    Field_descr*                field_descr_by_name_or_0( const char* name ) const;
    Field_descr*                field_descr_ptr         ( const string&  name ) const           { return _field_descr_by_name( c_str( name ) ); }
    Field_descr*                field_descr_by_offset   ( long offset ) const;
    void                        set_array_distances     ( Array_field_descr* outer = 0 );

    void                        add_method              ( Method_type*, const char* name, const Any_method_ptr& );   // Platzoptimierung
    void                        add_method              ( const Sos_ptr<Method_type>&, const char* name, const Any_method_ptr& );
    void                        add_method              ( const Sos_ptr<Method_descr>& m )      { _method_descr_array.add( m ); }
    int                         method_count            () const                                { return _method_descr_array.count(); }
    Method_descr*               method_descr            ( int i ) const;
    const Sos_ptr<Method_descr>& last_method_descr      ()                                      { return _method_descr_array[ _method_descr_array.last_index() ]; }

    virtual void                construct               ( Byte* ) const;
    virtual void                destruct                ( Byte* ) const;
    virtual void                clear                   ( Byte* ) const;
    virtual void                field_copy              ( Byte*, const Byte* ) const; 
    virtual Bool                field_equal             ( const Byte*, const Byte* ) const;
    virtual int                 alignment               () const;

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

    string                      as_string               ( int field_no, const Byte* p ) const           { return field_descr_ptr( field_no )->as_string( p ); }
    string                      as_string               ( const char* field_name, const Byte* p ) const { return as_string( field_index( field_name ), p ); }
    string                      as_string               ( const string& field_name, const Byte* p ) const { return as_string( field_name.c_str()       , p ); }

    void                        print_field_names       ( ::std::ostream*, const Text_format& ) const;
    void                        append_field_names      ( Area*, const Text_format& ) const;

    SOS_PRIV( Bool,             flat_scope )            // Verschachtelungen erscheinen flach, alle offsets zum selben Record-Beginn
    void                        allocate_methods        ( int n )                          { _method_descr_array.size( max( n, _method_descr_array.size() ) ); }
    void                        allocate_fields         ( int n )                          { _field_descr_array.size( max( n, _field_descr_array.size() ) ); }

    Field_descr*               _field_descr             ( int i ) const                       { return _field_descr_array[ i ]; }
    Field_descr*               _field_descr_by_name     ( const char* name ) const;//         { return _field_descr( field_index( name ) ); }
    Method_descr*              _method_descr            ( int i ) const;

    void                        remove_fields           ()                                  { _field_descr_array.size( 0 ); }
    virtual bool                is_numeric              () const                            { return _group_type? _group_type->is_numeric() : Field_type::is_numeric(); }   // jz 27.5.01 für Fehrmann, Dia-Nielsen

    void                       _obj_print               ( ::std::ostream* s ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Record_type || Base_class::_obj_is_type( t ); }

    friend struct               Field_descr;


  //protected:
    int                        _field_count;
    Sos_string                 _name;
    Sos_simple_array< Sos_ptr<Method_descr> > _method_descr_array;
    Sos_simple_array< Sos_ptr<Field_descr> >  _field_descr_array;

  public:
    Sos_ptr<Field_type>        _group_type;             // Für Cobol: format.raw() ==> diesen Typ für die Gruppe benutzen
    long                       _offset_base;            // Für Cobol, sonst 0   //jz 16.5.97  
    Bool                       _byte_type;              // clear: memset(0); field_copy: memcpy auf den ganzen Record (z.B. bei Cobol-Type, nicht bei sos_string_type)
};

typedef Record_type Dyn_record_type;

Sos_ptr<Record_type>            make_record_type        ( const char* record_type_definition );
inline Sos_ptr<Record_type>     make_record_type        ( const string& record_type_definition )    { return make_record_type( c_str(record_type_definition) ); }

Sos_ptr<Field_type>             make_type               ( const char* type_definition );
inline Sos_ptr<Field_type>      make_type               ( const string& type_definition )             { return make_type( c_str(type_definition) ); }

//-------------------------------------------------------------------------------Record_type_as

template< int FIELD_COUNT >
struct Record_type_as : Dyn_record_type
{
                                Record_type_as          () : Dyn_record_type( FIELD_COUNT ) {}
};

//-----------------------------------------------------------------------------------Array_type
/* 15.9.96
struct Array_type: Record_type
{
    BASE_CLASS( Record_type )//??

                                Array_type              ( const Sos_ptr<Field_type>&, int count );
                               ~Array_type              ();

    void                        name_format             ( const char* n )   { _name_format = n; }
    void                        first_index             ( int i )           { _first_index = i; }

    Field_type*                 elem_type               () const            { return _elem_type; }
    long                        elem_offset             ( int i ) const;
  //virtual void                print                   ( const Byte*, ostream*, const Text_format& ) const;
  //virtual void                input                   (       Byte*, istream*, const Text_format& ) const;
    virtual int                 alignment               () const;

  protected:
    Field_descr*               _field_descr             ( int ) const;
  //Sos_ptr<Field_descr>       _field_descr_by_name     ( const char* name ) const;
    void                       _obj_print               ( ostream* s ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const       { return t == tc_Array_type || Base_class::_obj_is_type( t ); }

  private:
    Sos_ptr<Field_type>        _elem_type;
    Sos_string                 _name_format;
    int                        _first_index;
  //int                        _field_count;
    Sos_ptr<Field_descr>       _current_field_descr;
};
*/
//-----------------------------------------------------------------------------DEFINE_ADD_FIELD
// Definiert add_field für die verschienden C-Typen (auch eigene)

#define DEFINE_ADD_FIELD( TYPE, FIELD_TYPE )                                                \
                                                                                            \
    inline void add_field( Record_type* t, const TYPE* offset, const char* name,            \
                           const Bool* null_offset = (Bool*)-1, uint flags = 0 )            \
    {                                                                                       \
        t->add_field( &FIELD_TYPE, name, (long)offset, (long)null_offset, flags );                                     \
    }


//------------------------------------------------------------------------RECORD_TYPE_ADD_FIELD

#define RECORD_TYPE_ADD_FIELD( NAME, FLAGS )   add_field( t, &o->_##NAME, #NAME, (Bool*)-1, FLAGS )

//-------------------------------------------------------------------RECORD_TYPE_ADD_FIELD_NULL

#define RECORD_TYPE_ADD_FIELD_NULL( NAME, FLAGS )   \
    add_field( t, &o->_##NAME, #NAME, &o->_##NAME##_null, FLAGS )

//---------------------------------------------------------------------RECORD_TYPE_ADD_FIELD_AS

#define RECORD_TYPE_ADD_FIELD_AS( NAME, FLAGS, FIELD_TYPE_PTR )                               \
    do {                                                                                      \
        (FIELD_TYPE_PTR)->check_type( &o->_##NAME );                                          \
        t->add_field( FIELD_TYPE_PTR, #NAME, (long)&o->_##NAME, -1, FLAGS );                   \
    } while(0)

//----------------------------------------------------------------RECORD_TYPE_ADD_FIELD_NULL_AS

#define RECORD_TYPE_ADD_FIELD_NULL_AS( NAME, FLAGS, FIELD_TYPE_PTR )                          \
    do {                                                                                      \
        (FIELD_TYPE_PTR)->check_type( &o->_##NAME );                                          \
        t->add_field( FIELD_TYPE_PTR, #NAME, (long)&o->_##NAME, (long)&o->_##NAME##_null, FLAGS )\
    } while(0)


#define RECORD_TYPE_ADD_CHAR( NAME, FLAGS )                                                         \
{                                                                                                   \
    Sos_ptr<String0_type> st = SOS_NEW( String0_type( sizeof o->_##NAME - 1 ) );                    \
    t->add_field( +st, #NAME, (long)&o->_##NAME, -1, 0 );                                           \
}

#define RECORD_TYPE_ADD_CHAR_NULL( NAME, FLAGS )                                                    \
{                                                                                                   \
    Sos_ptr<String0_type> st = SOS_NEW( String0_type( sizeof o->_##NAME - 1 ) );                    \
    t->add_field( +st, #NAME, (long)&o->_##NAME, (long)&o->_##NAME##_null, 0 );                     \
}

//---------------------------------------------------------------------DEFINE_STD_FIELD_TYPE_AS

#define DEFINE_STD_FIELD_TYPE_AS( TYPE, FIELD_TYPE, AS_TYPE )                                 \
                                                                                              \
struct FIELD_TYPE : Field_type                                                                \
{                                                                                             \
                                FIELD_TYPE              ()     : Field_type( &default_type_info, sizeof (TYPE) ) {}  \
                                                                                              \
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const; \
    void                        read_text               ( Byte*, const char*, const Text_format& ) const; \
                                                                                              \
    void                        check_type              ( const TYPE* ) {}                    \
                                                                                              \
  protected:                                                                                  \
    void                       _obj_print               ( ostream* s ) const            { *s << #TYPE; } \
};                                                                                            \
                                                                                              \
inline void FIELD_TYPE::write_text( const Byte* p, Area* buffer, const Text_format& ) const   \
{                                                                                             \
    ::write_text( (AS_TYPE)*(TYPE*)p, buffer );                                               \
}                                                                                             \
                                                                                              \
inline void FIELD_TYPE::read_text( Byte* p, const char* text, const Text_format& ) const      \
{                                                                                             \
    AS_TYPE x;                                                                                \
    ::read_text( &x, text );                                                                  \
    *p = (TYPE)x;                                                                             \
}                                                                                             \
                                                                                              \
extern FIELD_TYPE _##FIELD_TYPE;                                                              \
                                                                                              \
DEFINE_ADD_FIELD( TYPE, _##FIELD_TYPE );

//------------------------------------------------------------------------DEFINE_STD_FIELD_TYPE

#define DEFINE_STD_FIELD_TYPE( TYPE, FIELD_TYPE )                                 \
        DEFINE_STD_FIELD_TYPE_AS( TYPE, FIELD_TYPE, TYPE )

//------------------------------------------------------------------------------Field_converter

struct Field_converter : Sos_self_deleting
{
    enum Direction { dir_record_to_object, dir_object_to_record };

                                Field_converter         ();
                                Field_converter         ( const Sos_ptr<Record_type>& object,
                                                          const Sos_ptr<Record_type>& record );
                               ~Field_converter         ();

    void                        init                    ( const Sos_ptr<Record_type>& object,
                                                          const Sos_ptr<Record_type>& record )  { prepare_for_equal_names( object, record ); }
    void                        prepare_for_equal_names ( const Sos_ptr<Record_type>& object,
                                                          const Sos_ptr<Record_type>& record );
    void                        prepare_for_tuples      ( const Sos_ptr<Record_type>& object,
                                                          const Sos_ptr<Record_type>& record );

    void                        read                    ( void* object, const Const_area& record );
    void                        write                   ( const void* object, Area* record );

    void                        copy                    ( void* dest, const void* source, Direction );
  //void                        copy                    ( Area* dest, const Const_area& source, Direction ) const;

  //void                        copy                    ( Area* dest_record, const void* source_object ) const;
  //void                        copy                    ( void* dest_object, const Const_area& source_record ) const;

     DECLARE_PRIVATE_MEMBER( Bool, empty_is_null )

  private:
    struct Field_pair {
                                Field_pair              ()                             : _o(0),_r(0) {}
                                Field_pair              ( const Field_descr* object_field,
                                                          const Field_descr* record_field ) : _o(object_field),_r(record_field) {}
        const Field_descr* _o;
        const Field_descr* _r;
    };

    Sos_ptr<Record_type>         _object_type;
    Sos_ptr<Record_type>         _record_type;
    int                          _record_size;
    Sos_simple_array<Field_pair> _table;
    Dynamic_area                 _hilfspuffer;
    Bool                         _set_null_first;       // Wenn nicht alle Felder berücksichtigt sind
};

//-------------------------------------------------------------------------Field_converter_as<>

template< class TYPE >
struct Field_converter_as : Field_converter
{
                                Field_converter_as      () {}
                                Field_converter_as      ( const Sos_ptr<Record_type>& object,
                                                          const Sos_ptr<Record_type>& record )    : Field_converter( object, record ) {}

    void                        read                    ( TYPE* object, const Const_area& record )  { Field_converter::read( object, record ); }
    void                        write                   ( const TYPE& object, Area* record )       { Field_converter::write( &object, record ); }

    void                        copy                    ( TYPE* dest, const Const_area& source )  { Field_converter::read( dest, source ); }
    void                        copy                    ( Area* dest, const TYPE& source )        { Field_converter::write( &source, dest ); }
};


//------------------------------------------------------------------------------------copy_xxxx
// Quellwert darf nicht nichtig sein!

//void copy_value( const Field_type* dest, Byte*, const Field_type* src, const Byte* );
void copy_value( const Field_type* dest, Byte*, const Field_type* src, const Byte*, Area* hilfspuffer );
void copy_value( const Field_type* dest, Byte*, const Field_type* src, const Byte* );
void copy_value( const Field_descr* dst, Byte*, const Field_descr* src, const Byte*, Area* hilfspuffer );
void copy_value( const Field_descr* dst, Byte*, const Field_descr* src, const Byte* );

//-----------------------------------------------------------------------------------copy_field

void copy_field( const Field_descr*, Byte* dest, const Field_descr*, const Byte* src, Bool empty_is_null  );
void copy_field( const Field_descr*, Byte* dest, const Field_descr*, const Byte* src, Area* hilfspuffer, Bool empty_is_null  );

inline void copy_field( const Field_descr* g, Byte* dest, const Field_descr* f, const Byte* src )
{   // BC 5.00 versteht den Default-Parameter nicht
    copy_field( g, dest, f, src, false );
}

inline void copy_field( const Field_descr* g, Byte* dest, const Field_descr* f, const Byte* src,
                        Area* hilfspuffer )
{   // BC 5.00 versteht den Default-Parameter nicht
    copy_field( g, dest, f, src, hilfspuffer, false );
}

//----------------------------------------------------------------------------------copy_record
// Kopiert jedes Feld

void copy_record( const Record_type* dest_type, void* dest_ptr,
                  const Record_type* source_type, const void* src_ptr,
                  Bool empty_is_null = false );

//---------------------------------------------------------------record_type_of_selected_fields

Sos_ptr<Record_type> record_type_of_selected_fields( const Sos_ptr<Record_type>&,
                                                     const Sos_string& fieldnames,
                                                     const Sos_string& new_type_name = empty_string );

//------------------------------------------------------------------record_type_without_fields

Sos_ptr<Record_type> record_type_without_fields( const Sos_ptr<Record_type>&,
                                                 const Sos_string& fieldnames,
                                                 const Sos_string& new_type_name = empty_string );

//-----------------------------------------------------------------record_type_where_flags_true

Sos_ptr<Record_type> record_type_where_flags_true( const Sos_ptr<Record_type>&, Field_descr_flags,
                                                   const Sos_string& new_type_name = empty_string );

//-----------------------------------------------------------------fields_where_flags_true

Sos_string fields_where_flags_true( const Sos_ptr<Record_type>&, Field_descr_flags );

//--------------------------------------------------------------------------------moved_offsets

Sos_ptr<Record_type> moved_offsets( const Sos_ptr<Record_type>&, int diff,
                                    const Sos_string& new_type_name = empty_string );

//---------------------------------------------------------------------------------move_offsets

void move_offsets( Record_type*, int diff );

//--------------------------------------------------------------------------------renamed_fields

Sos_ptr<Record_type> renamed_fields( const Sos_ptr<Record_type>&, const Sos_string& names,
                                     const Sos_string& new_type_name = empty_string );

//--------------------------------------------------------------------------------rename_fields

Sos_ptr<Record_type> rename_fields( Sos_ptr<Record_type> to_rename, const Sos_string& names,
                                    const Sos_string& new_type_name = empty_string );

//-------------------------------------------------------------------------------modified_field

Sos_ptr<Field_descr> modified_field( Field_descr*, const Sos_string& descr );

//------------------------------------------------------------------------------modified_fields

Sos_ptr<Record_type> modified_fields( Sos_ptr<Record_type>, const Sos_string& names,
                                      const Sos_string& new_type_name = empty_string );


//-----------------------------------------------------------read_field_name_and_interpretation

void read_field_name_and_interpretation( const char** pp,
                                         Sos_string* name, Sos_string* interpretation,
                                         char separator );

//--------------------------------------------------------------------------------modify_fields

void modify_fields( Sos_ptr<Record_type>, const Sos_string& names_and_types );

//---------------------------------------------------------------------------------modify_field

void modify_field( Field_descr* field_descr, const Sos_string& name_and_type );

//------------------------------------------------------------------------------key_field_descr

Sos_ptr<Field_descr> key_field_descr( const Sos_ptr<Record_type>& selected_key_type,
                                      const Sos_string& name );

//----------------------------------------------------------------------------copy_field_descrs
// Kopiert (tief) alle Feldbeschreibungen des Record_types und berechnet neue Offsets
// mit Field_descr::add_to(Record_type*).
// Verschachtelte Record_types werden nicht kopiert, deren Referenzen bleiben erhalten.

Sos_ptr<Record_type> copy_field_descrs( const Sos_ptr<Record_type>& source_type,
                                        const Sos_string& new_type_name = empty_string );

//-----------------------------------------------------------------------------copy_record_type
// Kopiert (flach) alle Feldbeschreibungen des Record_types orginalgetreu.
// Verschachtelte Record_types werden nicht kopiert, deren Referenzen bleiben erhalten.

Sos_ptr<Record_type> copy_record_type( const Sos_ptr<Record_type>& source_type,
                                       const Sos_string& new_type_name = empty_string );

// Sollte Record_type::obj_copy() sein
//----------------------------------------------------------------------key_field_descr_by_name

Sos_ptr<Field_descr> key_field_descr_by_name( const Sos_ptr<Record_type>&, const Sos_string& field_names,
                                              const Sos_string& name = "key" );



//-------------------------------------------------------------------------flattend_record_type

Sos_ptr<Record_type> flattend_record_type( const Sos_ptr<Record_type>& source_type, 
                                           Bool with_groups = false,
                                           const Sos_string& new_type_name = empty_string );

//void add_flattend_record_type( Record_type* type, Record_type* source_type );

//------------------------------------------------------------------------------Konvertierungen

Bool                    as_bool             ( const Field_type*   , const Byte*   );
inline Bool             as_bool             ( const Field_descr* f, const Byte* p )  { return as_bool     ( f->type_ptr(), f->const_ptr( p ) ); }
Bool                    as_bool             ( const Field_type*   , const Byte*  , Bool deflt);
Bool                    as_bool             ( const Field_descr* f, const Byte* p, Bool deflt );
int                     as_int              ( const Field_type*   , const Byte*   );
inline int              as_int              ( const Field_descr* f, const Byte* p )  { return as_int      ( f->type_ptr(), f->const_ptr( p ) ); }
long                    as_long             ( const Field_type*   , const Byte*   );
inline long             as_long             ( const Field_descr* f, const Byte* p )  { return as_long     ( f->type_ptr(), f->const_ptr( p ) ); }
Big_int                 as_big_int          ( const Field_type*   , const Byte*   );
inline Big_int          as_big_int          ( const Field_descr* f, const Byte* p )  { return as_big_int  ( f->type_ptr(), f->const_ptr( p ) ); }
inline double           as_double           ( const Field_type*  t, const Byte* p )  { return t->as_double(p); }
inline double           as_double           ( const Field_descr* f, const Byte* p )  { return f->type_ptr()->as_double( f->const_ptr( p ) ); }
Const_area_handle       as_text_area        ( const Field_type*   , const Byte*   );
inline Const_area_handle as_text_area       ( const Field_descr* f, const Byte* p )  { return as_text_area( f->type_ptr(), f->const_ptr( p ) ); }
char                    as_char             ( const Field_type*   , const Byte*   );
inline char             as_char             ( const Field_descr* f, const Byte* p )  { return as_char( f->type_ptr(), f->const_ptr( p ) ); }
inline Sos_string       as_string           ( const Field_type*  t, const Byte* p )  { return t->as_string( p ); }
inline Sos_string       as_string           ( const Field_descr* f, const Byte* p )  { return f->as_string( p ); }

//----------------------------------------------------------------------------cobol_record_type

Sos_ptr<Record_type> cobol_record_type( const Sos_string& filename );

//----------------------------------------------------------------------------frame_record_type

Sos_ptr<Record_type> frame_record_type( ::std::istream* s, const Source_pos&, Bool check_eof = false );
Sos_ptr<Record_type> frame_record_type( const char* filename );

inline Sos_ptr<Record_type> frame_record_type( const Sos_string& filename )
{
    return frame_record_type( c_str( filename ) );
}
//------------------------------------------------------------------------------sql_record_type

Sos_ptr<Record_type> sql_record_type( ::std::istream*, Bool check_eof = false );

inline Sos_ptr<Record_type> sql_record_type( ::std::istream& s )
{
    return sql_record_type( &s );
}

Sos_ptr<Record_type> sql_record_type( const char* filename );

inline Sos_ptr<Record_type> sql_record_type( const Sos_string& filename )
{
    return sql_record_type( c_str( filename ) );
}


//======================================================================================inlines

//---------------------------------------------------------------Field_access_as<OBJECT>::value
/*
template< class OBJECT >
inline OBJECT Field_access_as<OBJECT>::value( const Byte* ptr ) const
{
    OBJECT object;
    read( ptr, &object );
    return object;
}
*/
//--------------------------------------------------------------------------template copy_field

template< class FIELD_TYPE, class OBJECT >                         // aus sosfield.h
/*inline*/ void copy_field( const FIELD_TYPE& field_type, Byte* ptr,
                            OBJECT* object_ptr, Field_copy_direction copy_direction )
{
    if( copy_direction == field_read ) {
        read_field( field_type, (const Byte*)ptr, object_ptr );
    } else {
        write_field( field_type, ptr, *object_ptr );
    }
}

//===============================================================================templates

#if defined SYSTEM_INCLUDE_TEMPLATES
//#    include <sosfield.tpl>
#endif


} //namespace sos
#endif
