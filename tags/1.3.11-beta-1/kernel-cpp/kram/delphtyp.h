// delphtyp.h                                        (c) SOS GmbH Berlin
//                                                      Joacim Zschimmer

#ifndef __DELPHTYP_H
#define __DELPHTYP_H


//---------------------------------------------------------------------------Delphi_string_type
//#if defined __SOSFIELD_H

struct Delphi_string_type : Field_type
{
                                Delphi_string_type      ( int size = 255 ) : _size ( size ) {}

    void                        print                   ( const Byte* p, ostream* s, const Text_format& f ) const;
    void                        input                   (       Byte* p, istream* s, const Text_format& f ) const;

  protected:
    uint                       _v_field_size            () const    { return 1 + _size; }

    int                        _size;
};

extern Delphi_string_type delphi_string_type;


#endif

