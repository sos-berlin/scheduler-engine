// sosstrea.inl                                                 (c) SOS GmbH Berlin
//                                                                  Joacim Zschimmer

#if !defined assert
#   include <assert.h>
#endif

#if !defined __XCEPTION_H
#   include <xception.h>
#endif

namespace sos
{

#define IMPLEMENT_SHIFT_WRITE( type, Type )                                                     \
    inline Sos_binary_ostream& operator << ( Sos_binary_ostream& s, const Type& value )         \
    {                                                                                           \
        s.write_##type( value );                                                                \
        return s;                                                                               \
    }

#define IMPLEMENT_STANDARD_WRITE_AND_READ( type, Type )                             \
    inline void write_##type( Sos_binary_ostream* s, const Type& data )             \
    {                                                                               \
        s->write_##type( data );                                                    \
    }                                                                               \
                                                                                    \
    inline void read_##type( Sos_binary_istream* s, Type* ptr )                     \
    {                                                                               \
        s->read_##type( ptr );                                                      \
    }



#if !defined SYSTEM_NOTALIGNED  ||  defined SYSTEM_SOLARIS

#   define IMPLEMENT_WRITE_METHOD( type, Type )                                                \
        inline void Sos_binary_ostream::write_##type( const Type& value )                       \
        {                                                                                       \
            memcpy( _ptr, &value, sizeof value );                                               \
            _ptr += sizeof value;                                                               \
        }


#   define IMPLEMENT_READ_PTR( type, Type )                                                    \
        inline Type Sos_binary_istream::read_##type()                                           \
        {                                                                                       \
            Type x;                                                                             \
            memcpy( &x, _ptr, sizeof x );                                                       \
            _ptr += sizeof x;                                                                   \
            return x;                                                                           \
        }

#else

#   define IMPLEMENT_WRITE_METHOD( type, Type )                                                 \
        inline void Sos_binary_ostream::write_##type( const Type& value )                       \
        {                                                                                       \
            *((Type*&)_ptr)++ = value;                                                          \
        }

#   define IMPLEMENT_READ_PTR( type, Type )                                                     \
        inline Type Sos_binary_istream::read_##type()                                           \
        {                                                                                       \
            return *((Type*&)_ptr)++;                                                           \
        }

#endif


#define IMPLEMENT_WRITE( type, Type )                                                           \
    IMPLEMENT_WRITE_METHOD( type, Type )


#define IMPLEMENT_SHIFT_READ( type, Type )                                                      \
    inline Sos_binary_istream& operator >> ( Sos_binary_istream& s, Type* ptr )                 \
    {                                                                                           \
        s.read_##type( ptr );                                                                   \
        return s;                                                                               \
    }

#define IMPLEMENT_READ( type, Type )                                                \
    IMPLEMENT_READ_PTR( type, Type )                                                \
                                                                                    \
    inline void Sos_binary_istream::read_##type( Type* ptr )                        \
    {                                                                               \
        *ptr = read_##type();                                                       \
    }



#define IMPLEMENT_SHIFT_WRITE_AND_READ( type, Type )                                \
    IMPLEMENT_SHIFT_WRITE( type, Type )                                             \
    IMPLEMENT_SHIFT_READ( type, Type )


#define IMPLEMENT_WRITE_AND_READ( type, Type )                                      \
    IMPLEMENT_WRITE( type, Type )                                                   \
    IMPLEMENT_READ( type, Type )                                                    \
    IMPLEMENT_STANDARD_WRITE_AND_READ( type, Type )


//-------------------------------------------------Sos_binary_ostream::Sos_binary_ostream

inline Sos_binary_ostream::Sos_binary_ostream()
{
    reset( 0, 0 );
}

//-------------------------------------------------Sos_binary_ostream::Sos_binary_ostream

inline Sos_binary_ostream::Sos_binary_ostream( void* buffer, uint size )
{
    reset( buffer, size );
}

//-------------------------------------------------Sos_binary_ostream::Sos_binary_ostream

inline Sos_binary_ostream::Sos_binary_ostream( const Area& area )
{
    reset( area );
}

//----------------------------------------------------------Sos_binary_ostream::~Sos_binary_ostream
/*
inline Sos_binary_ostream::~Sos_binary_ostream()
{
    assert( _ptr <= _end_ptr );
}
*/
//-------------------------------------------------------------------Sos_binary_ostream::reset

inline void Sos_binary_ostream::reset( void* ptr, uint size )
{
    _buffer_ptr = (Byte*) ptr;
    _ptr        = (Byte*) ptr;
    _end_ptr    = _ptr + size;
}

//-------------------------------------------------------------------Sos_binary_ostream::reset

inline void Sos_binary_ostream::reset( const Area& area )
{
    reset( area.ptr(), area.size() );
}

//-------------------------------------------------------------------Sos_binary_ostream::reset

inline void Sos_binary_ostream::rest_size( uint s )
{
    _end_ptr = _ptr + s;
}

//-------------------------------------------------Sos_binary_istream::Sos_binary_istream

inline Sos_binary_istream::Sos_binary_istream()
{
    reset( 0, 0 );
}

//-------------------------------------------------Sos_binary_istream::Sos_binary_istream

inline Sos_binary_istream::Sos_binary_istream( const void* ptr, uint length )
{
    reset( ptr, length );
}

//-------------------------------------------------Sos_binary_istream::Sos_binary_istream

inline Sos_binary_istream::Sos_binary_istream( const Const_area& area )
{
    reset( area );
}

//----------------------------------------------------------Sos_binary_istream::~Sos_binary_istream
/*
inline Sos_binary_istream::~Sos_binary_istream()
{
    assert( _ptr <= _end_ptr );
}
*/
//-------------------------------------------------------------------Sos_binary_istream::reset

inline void Sos_binary_istream::reset( const void* ptr, uint length )
{
    _buffer_ptr = (const Byte*) ptr;
    _ptr        = (const Byte*) ptr;
    _end_ptr    = _ptr + length;
}

//-------------------------------------------------------------------Sos_binary_istream::reset

inline void Sos_binary_istream::reset( const Const_area& area )
{
    reset( area.ptr(), area.length() );
}

//-------------------------------------------------------------Sos_binary_ostream::write_fixed

inline void Sos_binary_ostream::write_fixed( const void* ptr, uint length )
{
    memcpy( _ptr, ptr, length );
    _ptr += length;
}

//--------------------------------------------------------------------------------------write_bytes

inline void write_bytes( Sos_binary_ostream* s, const void* ptr, uint length )
{
    s->write_fixed( ptr, length );
}

//--------------------------------------------------------------Sos_binary_istream::read_fixed

inline void Sos_binary_istream::read_fixed( void* ptr, uint size )
{
    memcpy( ptr, _ptr, size );
    _ptr += size;
}

//---------------------------------------------------------------------------------------read_bytes

inline void read_bytes( Sos_binary_istream* s, void* ptr, uint length )
{
    s->read_fixed( ptr, length );
}

//--------------------------------------------------------------Sos_binary_istream::read_bytes

inline const Byte* Sos_binary_istream::read_bytes( uint count )
{
    const Byte* ptr = _ptr;
    _ptr += count;
    return ptr;
}

//--------------------------------------------------------Sos_binary_ostream::write_fixed_area

inline void Sos_binary_ostream::write_fixed_area( const Const_area& area )
{
    write_fixed( area.ptr(), area.length() );
}

//----------------------------------------------------------------------------------read_fixed_area

inline void Sos_binary_istream::read_fixed_area( Area* area )
{
    read_fixed( area->ptr(), area->size() );
    area->length( area->size() );
}

IMPLEMENT_STANDARD_WRITE_AND_READ( fixed_area, Area )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( fixed_area, Area )

//--------------------------------------------------------------------------------{write|read}_byte

IMPLEMENT_WRITE_AND_READ( byte, Byte )

//--------------------------------------------------------------------------------{write|read}_char

IMPLEMENT_WRITE_AND_READ( char, char )

//---------------------------------------------------------------------------------{write|read}_int

inline void Sos_binary_ostream::write_int( const int& value )
{
    write_int4( (int4)value );
}

inline int Sos_binary_istream::read_int()
{
    return (int)read_int4();
}

inline void Sos_binary_istream::read_int( int* ptr )
{
    read_int4( (int4*)ptr );
}

IMPLEMENT_SHIFT_WRITE_AND_READ   ( int, int )
IMPLEMENT_STANDARD_WRITE_AND_READ( int, int )

//--------------------------------------------------------------------------------{write|read}_uint

inline void Sos_binary_ostream::write_uint( const uint& value )
{
    write_uint4( (uint4)value );
}

inline uint Sos_binary_istream::read_uint()
{
    return (uint)read_uint4();
}

inline void Sos_binary_istream::read_uint( uint* ptr )
{
    read_uint4( (uint4*)ptr );
}

IMPLEMENT_STANDARD_WRITE_AND_READ( uint, uint )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( uint, uint )

//--------------------------------------------------------------------------------{write|read}_int1

IMPLEMENT_WRITE_AND_READ( int1, int1 )

//-------------------------------------------------------------------------------{write|read}_uint1

IMPLEMENT_WRITE_AND_READ( uint1, uint1 )

//--------------------------------------------------------------------------------{write|read}_int2

inline void Sos_binary_ostream::write_int2( const int2& value )
{
    _ptr[ 0 ] = (Byte) ( value >> 8 );
    _ptr[ 1 ] = (Byte) value;
    _ptr += 2;
}

inline void Sos_binary_istream::read_int2( int2* ptr )
{
    *ptr = read_int2();
}

inline int2 Sos_binary_istream::read_int2()
{
    int2 i =   ( (int2)_ptr[ 0 ] << 8 )
             |         _ptr[ 1 ];
    _ptr += 2;
    return i;
}

IMPLEMENT_STANDARD_WRITE_AND_READ( int2, int2 )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( int2, int2 )

//-------------------------------------------------------------------------------{write|read}_uint2

inline void Sos_binary_ostream::write_uint2( const uint2& value )
{
    _ptr[ 0 ] = (Byte) ( value >> 8 );
    _ptr[ 1 ] = (Byte) value;
    _ptr += 2;
}

inline void Sos_binary_istream::read_uint2( uint2* ptr )
{
    *ptr = read_uint2();
}

inline uint2 Sos_binary_istream::read_uint2()
{
    uint2 i =   ( (uint2)_ptr[ 0 ] << 8 )
              |          _ptr[ 1 ];
    _ptr += 2;
    return i;
}

IMPLEMENT_STANDARD_WRITE_AND_READ( uint2, uint2 )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( uint2, uint2 )

//--------------------------------------------------------------------------------{write|read}_int4

inline void Sos_binary_ostream::write_int4( const int4& value )
{
    _ptr[ 0 ] = (Byte) ( value >> 24 );
    _ptr[ 1 ] = (Byte) ( (uint4)value >> 16 );
    _ptr[ 2 ] = (Byte) ( (uint4)value >> 8 );
    _ptr[ 3 ] = (Byte) value;
    _ptr += 4;
}

inline void Sos_binary_istream::read_int4( int4* ptr )
{
    *ptr = read_int4();
}

inline int4 Sos_binary_istream::read_int4()
{
    int4 i =   ( ( int4)_ptr[ 0 ] << 24 )
             | ( (uint4)_ptr[ 1 ] << 16 )
             | ( (uint4)_ptr[ 2 ] <<  8 )
             |   (uint1)_ptr[ 3 ];
    _ptr += 4;
    return i;
}

IMPLEMENT_STANDARD_WRITE_AND_READ( int4, int4 )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( int4, int4 )

//-------------------------------------------------------------------------------{write|read}_uint4

inline void Sos_binary_ostream::write_uint4( const uint4& value )
{
    _ptr[ 0 ] = (Byte) ( value >> 24 );
    _ptr[ 1 ] = (Byte) ( value >> 16 );
    _ptr[ 2 ] = (Byte) ( value >> 8 );
    _ptr[ 3 ] = (Byte) value;
    _ptr += 4;
}

inline void Sos_binary_istream::read_uint4( uint4* ptr )
{
    *ptr = read_uint4();
}

inline uint4 Sos_binary_istream::read_uint4()
{
    uint4 i =   ( (uint4)_ptr[ 0 ] << 24 )
              | ( (uint4)_ptr[ 1 ] << 16 )
              | ( (uint2)_ptr[ 2 ] <<  8 )
              |   (uint1)_ptr[ 3 ];
    _ptr += 4;
    return i;
}

IMPLEMENT_STANDARD_WRITE_AND_READ( uint4, uint4 )
IMPLEMENT_SHIFT_WRITE_AND_READ   ( uint4, uint4 )

//----------------------------------------------------Sos_binary_[io]stream::{write|read}_bool

IMPLEMENT_WRITE_AND_READ( bool, Bool )

//-----------------------------------------------------Sos_binary_ostream::write_byte_repeated

inline void Sos_binary_ostream::write_byte_repeated( Byte b, uint count )
{
    memset( _ptr, b , count );
    _ptr += count;
}

inline void write_byte_repeated( Sos_binary_ostream* s, Byte b, uint count )
{
    s->write_byte_repeated( b, count );
}

//--------------------------------------------------------------------Sos_binary_ostream::area

inline Area Sos_binary_ostream::area() const
{
    return Area( _buffer_ptr, length() );
}

//------------------------------------------------------------------Sos_binary_ostream::length


inline uint Sos_binary_ostream::length() const
{
    return (uint) ( _ptr - _buffer_ptr );
}

//-------------------------------------------------------------------Sos_binary_ostream::space

inline Byte* Sos_binary_ostream::space( uint byte_count )
{
    Byte* ptr = _ptr;
    _ptr += byte_count;
    return ptr;
}

//--------------------------------------------------------------Sos_binary_ostream::space_left

inline uint Sos_binary_ostream::space_left() const
{
    return (uint) ( _end_ptr - _ptr );
}

//----------------------------------------------------------------Sos_binary_istream::read_end

inline void Sos_binary_istream::read_end()
{
    if( _ptr != _end_ptr )  throw_xc( _ptr > _end_ptr? "SOS-1152" : "SOS-1153" );
}

//---------------------------------------------------------------------Sos_binary_istream::end

inline Bool Sos_binary_istream::end()
{
    return _ptr >= _end_ptr;
}

//--------------------------------------------------------------Sos_binary_istream::skip_bytes

inline void Sos_binary_istream::skip_bytes( uint i )
{
    _ptr += i;
}

//---------------------------------------------------------------------Sos_binary_istream::ptr

inline const void* Sos_binary_istream::ptr() const
{
    return _ptr;
}

//------------------------------------------------------------------Sos_binary_istream::length

/*
inline uint Sos_binary_istream::length() const
{
    return _end_ptr - _buffer_ptr;
}
*/

//-------------------------------------------------------------Sos_binary_istream::rest_length

inline uint Sos_binary_istream::rest_length() const
{
    return (uint) ( _end_ptr - _ptr );
}

//------------------------------------------------------------------------------write_iso_char

inline void write_iso_char( Sos_binary_ostream* s , char c )
{
    write_char( s, c );
}

//-------------------------------------------------------------------------------read_iso_char

inline char read_iso_char( Sos_binary_istream* s )
{
    return s->read_byte();
}

//-------------------------------------------------------------------------------read_iso_char

inline void read_iso_char( Sos_binary_istream* s, char* object_ptr )
{
    *object_ptr = read_iso_char( s );
}

//---------------------------------------------------------------------------write_ebcdic_char
/*
inline void write_ebcdic_char( Sos_binary_ostream* s , char c )
{
    write_iso_char( s, iso2ebc[ (Byte)c ] );
}

//----------------------------------------------------------------------------read_ebcdic_char

inline char read_ebcdic_char( Sos_binary_istream* s )
{
    return ebc2iso[ (Byte)read_iso_char( s ) ];
}

//----------------------------------------------------------------------------read_ebcdic_char

inline void read_ebcdic_char( Sos_binary_istream* s, char* object_ptr )
{
    *object_ptr = read_ebcdic_char( s );
}

//-------------------------------------------------------------------------------read_ebcdic_number

inline int4 read_ebcdic_number( const Byte* ptr, const Byte* end_ptr, int field_size )
{
    int4 number;

    read_ebcdic_number( ptr, end_ptr, &number, field_size );
    return number;
}

//-----------------------------------------------------------------------------------read_ebcdic_jn

inline void read_ebcdic_jn( const Byte* ptr, const Byte* end_ptr, Bool* object_ptr )
{
    *object_ptr = read_ebcdic_jn( ptr, end_ptr );
}




//----------------------------------------------------------------------------read_ebcdic_char

inline char read_ebcdic_char( const Byte* ptr, const Byte* )
{
    return ebc2iso[ *ptr ];
}

//----------------------------------------------------------------------------read_ebcdic_char

inline void read_ebcdic_char( const Byte* ptr, const Byte* end_ptr, char* object_ptr )
{
    assert( ptr < end_ptr );
    *object_ptr = read_ebcdic_char( ptr, end_ptr );
}

//---------------------------------------------------------------------------write_ebcdic_char

inline void write_ebcdic_char( Byte* ptr, Byte* end_ptr, char c )
{
    assert( ptr < end_ptr );
    *ptr =iso2ebc[ (Byte)c ] );
}
*/

} //namespace sos
