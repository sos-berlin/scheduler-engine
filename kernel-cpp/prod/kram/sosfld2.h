// sosfld2.h                                    ©1996, SOS GmbH Berlin

#ifndef __SOSFLD2_H
#define __SOSFLD2_H

#ifndef __SOSFIELD_H
#   include "sosfield.h"
#endif

#ifndef __SOSLIMTX_H
#   include "soslimtx.h"
#endif

namespace sos
{

//---------------------------------------------------------------------------------As_bool_type

struct As_bool_type : Field_subtype
{
                                As_bool_type            ( const Sos_ptr<Field_type>& base_type, const Sos_string& falsch, const Sos_string& wahr );

    Bool                        null                    ( const Byte* ) const;
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

  //static Type_info           _type_info;

  protected:
    void                       _obj_print               ( ostream* s ) const;

    Sos_limited_text<30>       _falsch;
    Sos_limited_text<30>       _wahr;
};

//------------------------------------------------------------------------------As_numeric_type

struct As_numeric_type : Field_subtype
{
                                As_numeric_type         ( const Sos_ptr<Field_type>& base_type );

    bool                        null                    ( const Byte* ) const;
    void                        set_null                ( Byte* ) const;
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;
};

//------------------------------------------------------------------------------As_decimal_type

struct As_decimal_type : Field_subtype
{
                                As_decimal_type         ( const Sos_ptr<Field_type>& base_type, int precision = 0, int scale = 0, bool nullable = true );

    bool                        null                    ( const Byte* ) const;
    void                        set_null                ( Byte* ) const;
    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;
    void                       _get_param               ( Type_param* ) const;
    bool                        nullable                () const                                { return _nullable; }

    int                        _precision;
    int                        _scale;
    bool                       _nullable;
};

//---------------------------------------------------------------------------------As_text_type

struct As_text_type : Field_subtype
{
                                As_text_type            ( const Sos_ptr<Field_type>& base_type, int size = 0 );

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

  //static Type_info           _type_info;

  protected:
    int                        _size;
};

//-------------------------------------------------------------------------As_saacke_posnr_type

struct As_saacke_posnr_type : Field_subtype
{
                                As_saacke_posnr_type    ( const Sos_ptr<Field_type>& base_type );

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

  //static Type_info           _type_info;
};


} //namespace sos

#endif
