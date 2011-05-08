// delphi.h                                        (c) SOS GmbH Berlin
//                                                      Joacim Zschimmer

#ifndef __DELPHI_H
#define __DELPHI_H

namespace sos
{

//---------------------------------------------------------------------------Delphi_string_type
//#if defined __SOSFIELD_H

// String hat im ersten Byte die Anzahl der folgenden Zeichen.

struct Delphi_string_type : Field_type
{
                                Delphi_string_type      ( int size = 255 ) : Field_type( &_type_info, 1 + size ) {}

    void                        clear                   ( Byte* p ) const  { *p = 0; }                             
    void                        field_copy              ( Byte* p, const Byte* q ) const  { memcpy( p, q, min( (uint)*q + 1, _field_size ) ); }
    void                        write_text              ( const Byte*, Area*, const Text_format&  ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

    static Type_info           _type_info;
};

extern Delphi_string_type delphi_string_type;

//------------------------------------------------------------------------Delphi_tdatetime_type

// TDateTime ist ein double (8 Byte Fließkomma) mit der Anzahl Tagen seit dem
// 1. Januar 1 (ja, das Jahr eins).
// Die Tageszeit wird als Bruchteil eines Tages interpretiert, sechs Uhr morgens = 0.25.
// Die Tageszeit wird noch nicht unterstützt.
/*
struct Delphi_tdatetime_type : Field_type
{
                                Delphi_tdatetime_type   ()          : Field_type( sizeof (double) ) {}

    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*&, const Text_format& f ) const;
};

extern Delphi_tdatetime_type delphi_tdatetime_type;
*/

} //namespace sos

#endif

