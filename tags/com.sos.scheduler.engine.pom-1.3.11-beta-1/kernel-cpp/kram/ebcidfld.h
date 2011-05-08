// ebcdifld.h                                              © 1995 SOS GmbH Berlin

#ifndef __EBCDIFLD_H
#define __EBCDIFLD_H

//--------------------------------------------------------------------------------------Ebcdic_char

struct Ebcdic_char {  Ebcdic_char() {/*gcc 2.5.8*/} int x;  };

inline void read_field ( const Ebcdic_char&, const Byte*, char* );
inline char read_field ( const Ebcdic_char&, const Byte* );
inline void write_field( const Ebcdic_char&,       Byte*, char  );

//------------------------------------------------------------------------------------Ebcdic_string

struct Ebcdic_string
{
                                Ebcdic_string           ( int byte_count )     : _byte_count ( byte_count )  {}

    DECLARE_PUBLIC_MEMBER( int, byte_count )
};

void        read_field ( const Ebcdic_string&, const Byte*, char*, uint* length_ptr = 0 );          // Blanks abgeschitten
inline void read_field ( const Ebcdic_string&, const Byte*, Area* string0 );  // Blanks abgeschitten
void        write_field( const Ebcdic_string&,       Byte*, const char*, int length );

//---------------------------------------------------------------------Ebcdic_string_as<BYTE_COUNT>

template< int BYTE_COUNT >
struct Ebcdic_string_as : Ebcdic_string
{
                                Ebcdic_string_as        ()          : Ebcdic_string( BYTE_COUNT )  {}

#   if defined SYSTEM_GNU
        friend void             read_field( const Ebcdic_string_as<BYTE_COUNT>&, const Byte*, const Byte*, Area* );
        friend void             write_field( const Ebcdic_string_as<BYTE_COUNT>&, Byte*, Byte*, const Const_area& );
#   endif
};

//----------------------------------------------------------------------------------------Ebcdic_jn

struct Ebcdic_jn {  Ebcdic_jn() {/*gcc 2.5.8*/}  };

void read_field ( const Ebcdic_jn&, const Byte*, Bool* );
void write_field( const Ebcdic_jn&,       Byte*, Bool  );

//-----------------------------------------------------------------------------------------Ebcdic_x

struct Ebcdic_x /*: Field_type*/ {  Ebcdic_x() {/*gcc 2.5.8*/}  };

inline void read_field ( const Ebcdic_x&, const Byte*, Bool* );
inline void write_field( const Ebcdic_x&,       Byte*, Bool  );

//------------------------------------------------------------------------------------Ebcdic_number

struct Ebcdic_number
{
                                Ebcdic_number           ( int byte_count )     : _byte_count ( byte_count )  {}

    DECLARE_PUBLIC_MEMBER( int, byte_count )
};

inline int4 read_field ( const Ebcdic_number&, const Byte*, const Byte* end );

// Pauschal für alle Typen:

#if defined SYSTEM_GNU  // gcc 2.5.8: untenstehende templates greifen nicht
inline void read_field ( const Ebcdic_number&, const Byte*, int4* );
inline void write_field( const Ebcdic_number&,       Byte*, int4 );
#endif

template< class NUMERIC >
inline void read_field ( const Ebcdic_number&, const Byte*, NUMERIC* );

template< class NUMERIC >
inline void write_field( const Ebcdic_number&,       Byte*, const NUMERIC& );

//---------------------------------------------------------------------Ebcdic_number_as<BYTE_COUNT>

template< int BYTE_COUNT >
struct Ebcdic_number_as : Ebcdic_number
{
                                Ebcdic_number_as   ()                     : Ebcdic_number( BYTE_COUNT )  {}

#   if defined SYSTEM_GNU      /* gcc 2.5.8 */
        friend void             read_field( const Ebcdic_number_as<BYTE_COUNT>&, const Byte*, int4* );
        friend void             write_field( const Ebcdic_number_as<BYTE_COUNT>&, Byte*, Byte*, int4 );
#   endif
};


//==========================================================================================inlines

#if defined SYSTEM_INCLUDE_TEMPLATES
#   include "ebcdifld.tpl"
#endif

//----------------------------------------------------------------------read_field(Ebcdic_char,...)

inline char read_field( const Ebcdic_char&, const Byte* ptr )
{
    return ebc2iso[ (int)*ptr ];
}

//----------------------------------------------------------------------read_field(Ebcdic_char,...)

inline void read_field( const Ebcdic_char& type, const Byte* ptr, char* object_ptr )
{
    *object_ptr = read_field( type, ptr );
}

//--------------------------------------------------------------------write_field(Ebcdic_char,...)

inline void write_field( const Ebcdic_char&, Byte* ptr, char object )
{
    *ptr = iso2ebc[ (int)object ];
}

//--------------------------------------------------------------------read_field(Ebcdic_string,...)

inline void read_field( const Ebcdic_string& type, const Byte* ptr, Area* string0_ptr )
{
    uint len;
    string0_ptr->allocate_min( type.byte_count() + 1 );
    read_field( type, ptr, string0_ptr->char_ptr(), &len );
    string0_ptr->length( len );
}

//-------------------------------------------------------------------write_field(Ebcdic_string,...)

template< class STRING >
inline void write_field( const Ebcdic_string& type, Byte* ptr, const STRING& string )
{
    write_field( type, ptr, c_str( string ), length( string ));
}

//-------------------------------------------------------------------------read_field(Ebcdic_number,...)
#if !defined SYSTEM_GNU   // bad argument 1 for function `void read_field(const struct Ebcdic_char &, const unsigned char *, const unsigned char *, char *)' (type was struct Ebcdic_number)

inline int4 read_field( const Ebcdic_number& type, const Byte* ptr )
{
    int4  number;
    read_field( type, ptr, &number );
    return number;
}

#endif
//----------------------------------------------------------------------------read_field(Ebcdic_x,)

inline void read_field( const Ebcdic_x&, const Byte* ptr, Bool* object_ptr )
{
    *object_ptr = *ptr != 0x40/*EBCDIC-space*/;
}

//---------------------------------------------------------------------------write_field(Ebcdic_x,)

inline void write_field( const Ebcdic_x&, Byte* ptr, Bool object )
{
    *ptr = object? 0xE7/*EBCDIC 'X'*/ : 0x40/*EBCDIC ' '*/;
}

