// sosstrea.h                                                   (c) SOS GmbH Berlin
//                                                                  Joacim Zschimmer

#ifndef __SOSSTREA_H
#define __SOSSTREA_H

#ifndef __AREA_H
#   include "area.h"
#endif

namespace sos
{


struct Sos_binary_istream
{
                                Sos_binary_istream      ();
                                Sos_binary_istream      ( const void*, uint length );
                                Sos_binary_istream      ( const Const_area& );
                             //~Sos_binary_istream      ();

    inline void                        reset                   ( const void*, uint length );
    inline void                        reset                   ( const Const_area& );

  //inline void                        read                    ( char*, uint size, uint* length_ptr = 0 );
    inline void                        read_fixed              ( void*, uint size );
    inline const Byte*                 read_bytes              ( uint count );
  //inline void                        read_area               ( Area* );
    inline void                        read_fixed_area         ( Area* );
  //inline void                        read_string             ( string* );
    inline void                        read_byte               ( Byte* );
    inline Byte                        read_byte               ();
    inline void                        read_char               ( char* );
    inline char                        read_char               ();
    inline void                        read_int                ( int* );
    inline int                         read_int                ();
    inline void                        read_uint               ( uint* );
    inline uint                        read_uint               ();
    inline void                        read_int1               ( int1* );
    inline int1                        read_int1               ();
    inline void                        read_uint1              ( uint1* );
    inline uint1                       read_uint1              ();
    inline void                        read_int2               ( int2* );
    inline int2                        read_int2               ();
    inline void                        read_uint2              ( uint2* );
    inline uint2                       read_uint2              ();
    inline void                        read_int4               ( int4* );
    inline int4                        read_int4               ();
    inline void                        read_uint4              ( uint4* );
    inline uint4                       read_uint4              ();
    inline void                        read_bool               ( Bool* );
    inline Bool                        read_bool               ();

  //inline int                         get                     ();

    inline void                        read_end                ();  // end()!
    inline Bool                        end                     ();


    inline void                        skip_bytes              ( uint );
    inline const void*                 ptr                     () const;
  //inline uint                        length                  () const;
    inline uint                        rest_length             () const;

//private:
    const Byte*                _buffer_ptr;
    const Byte*                _ptr;
    const Byte*                _end_ptr;
};

struct Sos_binary_ostream
{
                                Sos_binary_ostream      ();
                                Sos_binary_ostream      ( void*, uint size );
                                Sos_binary_ostream      ( const Area& area );
                             //~Sos_binary_ostream      ();

    inline void                 reset                   ( void*, uint size );
    inline void                 reset                   ( const Area& area );
    inline void                 rest_size               ( uint );

    inline void                 write_fixed             ( const void*, unsigned int length );
    inline void                 write_fixed_area        ( const Const_area& );
    inline void                 write_byte              ( const Byte& );
    inline void                 write_char              ( const char& );
    inline void                 write_int               ( const int& );
    inline void                 write_uint              ( const uint& );
    inline void                 write_int1              ( const int1& );
    inline void                 write_uint1             ( const uint1& );
    inline void                 write_int2              ( const int2& );
    inline void                 write_uint2             ( const uint2& );
    inline void                 write_int4              ( const int4& );
    inline void                 write_uint4             ( const uint4& );
    inline void                 write_bool              ( const Bool& );
  //int4                        position                () const;

    inline void                 write_byte_repeated     ( Byte, uint count );

    inline Area                 area                    () const;
    inline uint                 length                  () const;

    inline Byte*                space                   ( uint byte_count );
    inline uint                 space_left              () const;

//private:
    Byte*                      _buffer_ptr;
    Byte*                      _ptr;
    Byte*                      _end_ptr;
};


void read_iso_string ( Sos_binary_istream*,       Const_area*, uint field_size );
void write_iso_string( Sos_binary_ostream*, const Const_area&, uint field_size );

void read_iso_string ( Sos_binary_istream*,       char*      , uint field_size );
void write_iso_string( Sos_binary_ostream*, const char*      , uint field_size );

void read_ebcdic_string ( Sos_binary_istream*         ,       char*, uint field_size );
void write_ebcdic_string( Sos_binary_ostream*         , const char*, uint field_size );

void read_ebcdic_string ( Sos_binary_istream*,             Area*, uint field_size );
void write_ebcdic_string( Sos_binary_ostream*, const Const_area&, uint field_size );

void read_ebcdic_char   ( Sos_binary_istream*, char* );
char read_ebcdic_char   ( Sos_binary_istream* );
void write_ebcdic_char  ( Sos_binary_ostream*, char );

Bool        read_ebcdic_jn     ( Sos_binary_istream* );
inline void read_ebcdic_jn     ( Sos_binary_istream*, Bool* );
void        write_ebcdic_jn    ( Sos_binary_ostream*, Bool );

// Zur Kompatibilität:

inline void read_string_iso ( Sos_binary_istream* s, char* string_buffer, uint field_length )
{
    read_iso_string( s, string_buffer, field_length );
}

inline void write_string_iso( Sos_binary_ostream* s, const char* string , uint field_size   )
{
    write_iso_string( s, string, field_size );
}

inline void read_string_ebcdic ( Sos_binary_istream* s, char* string_buffer, uint field_length )
{
    read_ebcdic_string( s, string_buffer, field_length );
}

inline void write_string_ebcdic( Sos_binary_ostream* s, const char* string , uint field_size   )
{
    write_ebcdic_string( s, string, field_size );
}




#define SOS_BINARY_STREAM( EXTERNAL_TYPE, VAR )                               \
    if( read_mode )  read##EXTERNAL_TYPE( s, &VAR );                          \
               else  write##EXTERNAL_TYPE( s, VAR );

#define SOS_BINARY_STREAM_FUNCT( EXTERNAL_TYPE, VAR )                         \
    if( read_mode )  var = s->read##EXTERNAL_TYPE();                          \
               else  write##EXTERNAL_TYPE( s, VAR );

#define DECLARE_READ_AND_WRITE_BINARY_STREAM( CTYPE, EXTERNAL_TYPE )          \
    void read##EXTERNAL_TYPE( Sos_binary_ostream* s, CTYPE* value_ptr );      \
    void write##EXTERNAL_TYPE( Sos_binary_ostream* s, const CTYPE& value );

#define DEFINE_READ_AND_WRITE_BINARY_STREAM( CTYPE, EXTERNAL_TYPE )           \
    void read##EXTERNAL_TYPE( Sos_binary_istream* s, CTYPE& value_ptr )       \
    {                                                                         \
        read_or_write_##EXTERNAL_TYPE( true, s, value_ptr );                  \
    }                                                                         \
                                                                              \
    void write##EXTERNAL_TYPE( Sos_binary_ostream* s, const CTYPE& value )    \
    {                                                                         \
        read_or_write_##EXTERNAL_TYPE( false, s, &(CTYPE*)value );            \
    }


} //namespace sos


#include "sosstrea.inl"

#if defined JZ_TEST && defined SYSTEM_INCLUDE_TEMPLATES
//  #include <sosstrea.tpl>
#endif

#endif
