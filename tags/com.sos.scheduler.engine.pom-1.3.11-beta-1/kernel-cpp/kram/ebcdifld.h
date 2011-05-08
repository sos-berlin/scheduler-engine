// ebcdifld.h                                              © 1995 SOS GmbH Berlin
// $Id$

/*
    7.11.99: [SIGN IS] LEADING|TRAILING [SEPARATE CHARACTER] eingebaut.  J. Zschimmer
*/

#ifndef __EBCDIFLD_H
#define __EBCDIFLD_H

#if !defined __STDFIELD_H
#   include "stdfield.h"
#endif

namespace sos
{

const int max_ebcdic_number_size = 31;
const int max_ebcdic_packed_size = 16;  // Bytes

//-------------------------------------------------------------------------------Ebcdic_type_flags

enum Ebcdic_type_flags
{
    ebc_none            = 0,
    ebc_bs2000          = 0x01,    
    ebc_mvs             = 0x02,     // MVS (sonst BS2000)
    ebc_german          = 0x04,     // Deutsche Variante
    ebc_international   = 0x08,     // Internationale Variante (Standard)
    ebc_ascii           = 0x10      // Zeichen sind in ASCII codiert (also nicht in EBCDIC), nur für Ebcdic_number_type, für cobfield.cxx, Eichenauer EBO, jz 4.1.00
};

//--------------------------------------------------------------------------------Ebcdic_text_type

struct Ebcdic_text_type : Xlat_string
{
                                Ebcdic_text_type        ( int field_size, Ebcdic_type_flags );      

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* ) const;
  //void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( std::ostream* s ) const;       // { *s << "Ebcdic_text(" << field_size() << ')'; }

    static Listed_type_info    _type_info;
};

//--------------------------------------------------------------------------------Ebcdic_number_type

struct Ebcdic_number_type : Field_type
{
                                Ebcdic_number_type      ( int field_size, Ebcdic_type_flags = ebc_none );

    Byte                        positive_sign           () const                { return _positive_sign_mask & 0xF0; }
    void                        positive_sign           ( Byte s )              { _positive_sign_mask = ( s < 0x10? s << 4 : s ) | 0x0F; }
    DECLARE_PUBLIC_MEMBER( int , scale         )  // Besser: protected

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* ) const;

    int                         op_compare              ( const Byte*, const Byte* ) const;

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

    virtual Bool                negative                ( const Byte* ) const;
    Bool                        ascii                   () const                { return _zone == 0x30; }       // Eichenauer EBO

    Byte                       _positive_sign_mask;

    static Listed_type_info    _type_info;

    Bool                       _separate_sign;          // SEPARATE CHARACTER   '+' oder '-'
    Bool                       _leading_sign;           // SIGN IS LEADING      Vorzeichen im ersten Byte, sonst im letzten Byte
    char                       _zone;                   // 0xF0 (EBCDIC) oder 0x30 (ASCII)
    char                       _plus;                   // '+' (EBCDIC oder ASCII)
    char                       _minus;                  // '+' (EBCDIC oder ASCII)
    char                       _blank;                  // ' ' (EBCDIC oder ASCII)

  protected:
                                Ebcdic_number_type      ( Type_info* info , int field_size, Ebcdic_type_flags flags )    : Field_type( info, field_size )  { set_flags( flags ); }

    void                        set_flags               ( Ebcdic_type_flags );
    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( std::ostream* ) const;

    Bool                       _unsigned;

};

//------------------------------------------------------------------Ebcdic_unsigned_number_type

struct Ebcdic_unsigned_number_type : Ebcdic_number_type
{
                                Ebcdic_unsigned_number_type( int field_size, Ebcdic_type_flags = ebc_none );

    Bool                        negative                ( const Byte* ) const;

  protected:
  //?jz 2.9.01 int                         op_compare              ( const Byte* a, const Byte* b ) const  { return Field_type::op_compare( a, b ); }

    void                       _get_param               ( Type_param* ) const;
  //void                       _obj_print               ( ostream* ) const;
};

//---------------------------------------------------------------------------Ebcdic_packed_type

struct Ebcdic_packed_type : Field_type
{
                                Ebcdic_packed_type      ( int field_size );

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* ) const;
    virtual int                 op_compare              ( const Byte*, const Byte* ) const;

    DECLARE_PRIVATE_MEMBER( Byte, positive_sign )
    DECLARE_PRIVATE_MEMBER( int , scale         )

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;

    Bool                        negative                ( const Byte* p ) const     { Byte s = p[ _field_size - 1 ] & 0x0F; return s == 0x0D || s == 0x0B; }

    static Listed_type_info    _type_info;

  protected:
    void                       _get_param               ( Type_param* ) const;
    void                       _obj_print               ( std::ostream* s ) const;
};


} //namespace sos

#endif
