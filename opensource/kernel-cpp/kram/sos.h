// sos.h                                                (c) SOS GmbH Berlin

#ifndef __SOS_H
#define __SOS_H

#if !defined __SYSDEP_H
#    include "sysdep.h"
#endif

#if !defined _ASSERT_H  && !defined __ASSERT_H
#    include <assert.h>
#endif

#if defined __GNUC__ && !defined Z_SOLARIS
#   include <strstream>
#endif


#include "sos_simple.h"
#include "../zschimmer/base.h"


#include <new>
#include <string>               // Damit Standard Template Library (STL) eingezogen ist

#define SYSTEM_STD_CPP_LIBRARY  // Und damit u.a. operator new(void*) 

//---------------------------------------------------------------------------------operator new
#if !defined SYSTEM_STD_CPP_LIBRARY //|| defined SYSTEM_GNU

inline void* operator new( size_t, void* ptr )
{
    return ptr;
}

#ifdef SYSTEM_DELETE_WITH_PARAMS
    inline void operator delete( void* ptr, void* )		// Für VC++ 6: warning C4291
    {
    }
#endif

#endif

//--------------------------------------------------------------------------------namespace sos

namespace sos
{

//--------------------------------------------------------------------------------------#define

#if defined SOS_SMALL_CODE
#   define SOS_OPTIMIZE_SIZE
#endif
//#define SOS_OPTIMIZE_SPEED
//#define SOS_DEBUGGED         // Auf zusätzliche Prüfungen zur Laufzeit verzichten

//----------------------------------------------------------------------------------CONCATENATE

//#define STRING( str ) #str
#define CONCATENATE( token1, token2 ) token1##token2

//----------------------------------------------------------------------------------MODULE_NAME

#if !defined( MODULE_NAME )
//    static int define_MODULE_NAME_fehlt;
#   define MODULE_NAME "unknown"
#endif

//----------------------------------------------------------------Has_constructions_module_name
/*
struct Has_constructions_module_name
{
                                Has_constructions_module_name() : _constructions_module_name ( MODULE_NAME ) {}

    const char*                _constructions_module_name;
};
*/
//---------------------------------------------------------------------------------------SOURCE

#if !defined( SOURCE )
#   if defined( __cplusplus )
#       define SOURCE  MODULE_NAME ".cxx"
#    else
#       define SOURCE  MODULE_NAME ".c"
#   endif
#endif

//----------------------------------------------------------------------------------------NO_OF

// Anzahl der Elemente eines Array's:

#define NO_OF( array )  ( sizeof (array) / sizeof (array)[0] )

//-----------------------------------------------------------------------------------BASE_CLASS
#define BASE_CLASS( BASE )      private: typedef BASE Base_class; public:
//-------------------------------------------------------------------------------ASSERT_VIRTUAL
#define ASSERT_VIRTUAL( NAME )  (void)&Base_class::NAME;
//----------------------------------------------------------------------------------DECLARE_BIT

// #defines, die Methoden deklarieren und definieren:


// DECLARE_BIT deklariert und definiert Methoden für ein Bit in einem Byte

#define DECLARE_BIT(name,byte,mask) \
    Bool name() const    { return (byte & mask) != 0; } \
    void name( Bool b )  { if( b )  byte |= mask;  else  byte &= ~mask; }

//------------------------------------------------------------------------------DECLARE_BIT_NOT

// DECLARE_BIT_NOT deklariert und definiert Methoden für ein invertiertes Bit in einem Byte

#define DECLARE_BIT_NOT(name,byte,mask) \
    Bool name() const    { return (byte & mask) == 0; } \
    void name( Bool b )  { if( !b )  byte |= mask;  else  byte &= ~mask; }

//---------------------------------------------------------------------------------DECLARE_BITS

// DECLARE_BITS deklariert und definiert Methoden für Bits in einer Variable

#define DECLARE_BITS(Type,name,var,shift,length) \
    Type name() const    { return (Type) ( ((unsigned int)var >> shift) & ~(-1 << length) ); } \
    void name( Type v )  { var &= ~(~(-1 << length) << shift); var |= (int)v << shift; }

//-----------------------------------------------------------------------DECLARE_ACCESS_METHODS

// DECLARE_ACCESS_METHODS deklariert und definiert Methoden für den Zugriff auf eine Variable

#define DECLARE_ACCESS_METHODS( Type, name )                           \
    void            name( Type const& value )  { _##name = value; }    \
    Type const&     name() const               { return _##name;  }    \
  /*Type _##name;  wird außerhalb des Makros deklariert */

//-----------------------------------------------------------------------DECLARE_PRIVATE_MEMBER

// DECLARE_PRIVATE_MEMBER deklariert ein privates Element und die öffentlichen Methoden für den Zugriff

#define DECLARE_PRIVATE_MEMBER( Type, name )  \
  public:                                     \
    DECLARE_ACCESS_METHODS( Type, name )      \
  private:                                    \
    Type _##name;                             \
  public:                 // Endet in public!!

#define SOS_PRIV( TYPE, NAME )  DECLARE_PRIVATE_MEMBER( TYPE, NAME )

//------------------------------------------------------------------------DECLARE_PUBLIC_MEMBER

// DECLARE_PUBLIC_MEMBER deklariert ein öffentliches Element und die öffentlichen Methoden für den Zugriff

#define DECLARE_PUBLIC_MEMBER( Type, name )   \
  public:                                     \
    DECLARE_ACCESS_METHODS( Type, name )      \
    Type _##name;

#define SOS_PUB( TYPE, NAME )  DECLARE_PUBLIC_MEMBER( TYPE, NAME )

//---------------------------------------------------------------------------------BITWISE_ENUM

#if !defined SYSTEM_GNU /* defined __BORLANDC__ */
    // Borland 4.5 liefert int statt Enum
#   define BITWISE_ENUM( Enum )
# else
#   define BITWISE_ENUM( Enum )                                                                  \
        inline Enum  operator|  ( Enum  a, Enum b ) { return Enum( (int) a | (int) b ); }         \
        inline Enum& operator|= ( Enum& a, Enum b ) { return a = Enum( a | b ); }
#endif

//----------------------------------------------------------------------------------------RENEW

// Zerstören und wieder konstruieren:
#define RENEW( OBJECT, TYPE )           \
    do {                                \
        (OBJECT).~TYPE();               \
        new( (void*)&(OBJECT) ) TYPE;   \
    } while(0)

//-------------------------------------------------------------------------------------SOS_INIT

#define SOS_INIT( NAME )                                                                      \
    static struct Sos_init_##NAME                                                             \
    {                                                                                         \
        Sos_init_##NAME ();                                                                   \
    }                                                                                         \
    _sos_init_##NAME##_;                                                                      \
                                                                                              \
    Sos_init_##NAME::Sos_init_##NAME()

// Verwendung:
// SOS_INIT( beispiel )
// {
//     var = 4711;
// }

//-------------------------------------------------------------------------------------SHOW_MSG

#if !defined( SHOW_MSG_SIZE )
#   define SHOW_MSG_SIZE 500
#endif

#   define SHOW_MSG( text )                                                     \
    {                                                                           \
        Dynamic_area _buffer_ ( SHOW_MSG_SIZE );                                \
        ostrstream _s_ ( _buffer_.char_ptr(), _buffer_.size() );                \
        _s_ << text;                                                            \
        _buffer_.length( _s_.pcount() );                                        \
        _buffer_ += '\0';                                                       \
        ::sos::show_msg( _buffer_.char_ptr() );                                 \
    }

#if 0 // mit ostringstream:
#   define SHOW_MSG( text )                                                     \
    {                                                                           \
        ostringstream _s_;                                                      \
        _s_ << text;                                                            \
        ::sos::show_msg( _s_.str() );                                           \
    }
#endif

    extern void show_msg( const char* );         // definiert in sossv2.cxx
    inline void show_msg( const string& text )   { show_msg( text.c_str() ); }

#   define SHOW_QUERY( text )                                         \
    query_user( text )

    extern int query_user( const char* );         // definiert in Applikation

// #else
//    #define SHOW_MSG( text )  { cerr << text << endl; /*LOG( "SHOW_MSG(\"" << text << "\" )\n");*/}
//#endif

//-------------------------------------------------------------------------------------SHOW_ERR

#define SHOW_ERR( text )      SHOW_MSG( text )

//#define local static


//#define take(type) * (type far*) &
#define zuweit(array,ptr) (ptr  >  & array [0] + sizeof array)

//--------------------------------------------------------------------------------------abs ABS

#ifdef abs
#undef abs
#endif

#define ABS( a )     ( (a) < 0 ? -(a) : (a) )

//------------------------------------------------------------------------------------------abs

template<class T>
inline T abs( T t )  { return t < 0? -t : t; }

//------------------------------------------------------------------------------------------sgn

template< class T >
inline T sgn( T t )
{
    return  t <  0? -1
          : t == 0?  0
          :          1;
}

//------------------------------------------------------------------------------------------neg

template< class T >
inline void neg( T* number_ptr )
{
    *number_ptr = -*number_ptr;
}

//-------------------------------------------------------------------------------------negative

template< class NUMERIC_TYPE >
inline Bool negative( const NUMERIC_TYPE& number )
{
    return number < 0 ?  true  :  false;
}

inline Bool negative( uint1 )  { return false; }
inline Bool negative( uint2 )  { return false; }
inline Bool negative( uint4 )  { return false; }

//-------------------------------------------------------------------------------------round_up

inline int round_up( int a, int r )  { int d = a % r; return d? a + r - d : a; }

//------------------------------------------------------------------------------min max MIN MAX

#ifdef min
#  undef min
#endif

#ifdef max
#   undef max
#endif

#ifndef MIN
#   define MIN( a, b )  ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#   define MAX( a, b )  ( (a) > (b) ? (a) : (b) )
#endif

template<class T>
inline T min( T a, T b )  { return a < b ? a : b; }

#define DEF_MIN_MAX( T_erg, T_a, T_b )                                                        \
   inline T_erg min( T_a a, T_b b )  { return (T_erg)a < (T_erg)b ? a : b; }                  \
   inline T_erg max( T_a a, T_b b )  { return (T_erg)a > (T_erg)b ? a : b; }

  DEF_MIN_MAX( int1, int1, int1)
  DEF_MIN_MAX( int2, int1, int2)
  DEF_MIN_MAX( int4, int1, int4)
  DEF_MIN_MAX( int2, int1,uint1)
  DEF_MIN_MAX( int4, int1,uint2)
  DEF_MIN_MAX( int , int1, int )
  DEF_MIN_MAX( int4, int1,uint )
//DEF_MIN_MAX( int8, int1,uint4)    // Stimmt nicht

  DEF_MIN_MAX( int2, int2, int1)
  DEF_MIN_MAX( int2, int2, int2)
  DEF_MIN_MAX( int4, int2, int4)
  DEF_MIN_MAX( int2, int2,uint1)
  DEF_MIN_MAX( int4, int2,uint2)
//DEF_MIN_MAX( int8, int2,uint4)
  DEF_MIN_MAX( int , int2, int )
  DEF_MIN_MAX( int4, int2,uint )

  DEF_MIN_MAX( int4, int4, int1)
  DEF_MIN_MAX( int4, int4, int2)
  DEF_MIN_MAX( int4, int4, int4)
  DEF_MIN_MAX( int4, int4,uint1)
  DEF_MIN_MAX( int4, int4,uint2)
//DEF_MIN_MAX( int8, int4,uint4)
  DEF_MIN_MAX( int4, int4, int )
  DEF_MIN_MAX( int4, int4,uint )

  DEF_MIN_MAX( int2,uint1, int1)
  DEF_MIN_MAX( int2,uint1, int2)
//DEF_MIN_MAX( int8,uint1, int4)
  DEF_MIN_MAX(uint1,uint1,uint1)
  DEF_MIN_MAX(uint2,uint1,uint2)
  DEF_MIN_MAX(uint4,uint1,uint4)
  DEF_MIN_MAX( int ,uint1, int )
  DEF_MIN_MAX(uint ,uint1,uint )

  DEF_MIN_MAX( int4,uint2, int1)
  DEF_MIN_MAX( int4,uint2, int2)
  DEF_MIN_MAX( int4,uint2, int4)
  DEF_MIN_MAX(uint2,uint2,uint1)
  DEF_MIN_MAX(uint2,uint2,uint2)
  DEF_MIN_MAX(uint4,uint2,uint4)
  DEF_MIN_MAX( int4,uint2, int )
  DEF_MIN_MAX(uint ,uint2,uint )

//DEF_MIN_MAX( int8,uint4, int1)
//DEF_MIN_MAX( int8,uint4, int2)
//DEF_MIN_MAX( int8,uint4, int4)
  DEF_MIN_MAX(uint4,uint4,uint1)
  DEF_MIN_MAX(uint4,uint4,uint2)
  DEF_MIN_MAX(uint4,uint4,uint4)
//DEF_MIN_MAX( int8,uint4, int )
  DEF_MIN_MAX(uint4,uint4,uint )

  DEF_MIN_MAX( int , int , int1)
  DEF_MIN_MAX( int , int , int2)
  DEF_MIN_MAX( int4, int , int4)
//DEF_MIN_MAX( int8, int ,uint1)
//DEF_MIN_MAX( int8, int ,uint2)
//DEF_MIN_MAX( int8, int ,uint4)
  DEF_MIN_MAX( int , int , int )
//DEF_MIN_MAX(uint4, int ,uint )

//DEF_MIN_MAX( int8,uint , int1)
//DEF_MIN_MAX( int8,uint , int2)
//DEF_MIN_MAX( int8,uint , int4)
  DEF_MIN_MAX( int ,uint ,uint1)
  DEF_MIN_MAX(uint ,uint ,uint2)
  DEF_MIN_MAX(uint4,uint ,uint4)
//DEF_MIN_MAX( int8,uint , int )
  DEF_MIN_MAX(uint ,uint ,uint )


template<class T>
inline T max( T a, T b )  { return a > b ? a : b; }
//inline long max( long a, long b )  { return a > b ? a : b; }

#define __MINMAX_DEFINED            // Für Borland C++ 4.02  stdlib.h

//-------------------------------------------------------------------------------------long2int

inline int long2int (long l)  {
    assert (((l << ((sizeof (long) - sizeof (int)) * 8))
		>> ((sizeof (long) - sizeof (int)) * 8)) == l);
    return (int) l;
}

inline int l2i( long l)  { return long2int( l ); }

extern int truncate_spaces( char* string );

//--------------------------------------------------------------------------------EBCDIC, ASCII

#if defined ( SYSTEM_DOS ) && !defined ( SYSTEM_WIN )
    extern const char  _far tbebcasc [256];
    extern const Byte  _far tbascebc [256];
#   define ebc2iso tbebcasc
#   define iso2ebc tbascebc
# else
#   if defined SYSTEM_BORLAND  &&  defined __WIN32__    /* Wegen Fehler in Borland C++ 5.00 */
        extern char*  ebc2iso;// [256];
        extern Byte*  iso2ebc;// [256];
        extern char*  ebc2iso_german;// [256];
        extern Byte*  iso2ebc_german;// [256];
#    else
        extern char   ebc2iso [256];
        extern Byte   iso2ebc [256];
        extern char   ebc2iso_german [256];
        extern Byte   iso2ebc_german [256];
#   endif
    //const uchar [256] & tbebcasc = ebc2iso;
    //const uchar [256] & tbascebc = iso2ebc;
#   define tbebcasc ebc2iso
#   define tbascebc iso2ebc
#endif

//--------------------------------------------------------------------------------------sos_log

extern void sos_log( const char* );     // s.a. log.h

//-----------------------------------------------------------------------------------SOS_DELETE

#define SOS_DELETE( ptr )     sos_delete( ptr )  //( delete ptr, ptr = 0 )

#if !defined DELETE     /* winnt.h kennt auch DELETE */
//#define DELETE( ptr )         SOS_DELETE( ptr )  // Ist in winnt.h anders definiert
#endif

template< class T >
inline void sos_delete( T*& ptr )
{
    T* ptr2 = ptr;
    ptr = 0;
    delete ptr2;
}

//-------------------------------------------------------------------------------------SOS_FREE

#define SOS_FREE( PTR ) \
    do { \
        Byte* p = PTR;  PTR = 0; \
        sos_free( p ); \
    } while(0)


//-------------------------------------------------------------------------------------0-string

inline const char*      c_str       ( const char* string     )     { return string;           }

//--------------------------------------------------------------------------Increment_semaphore

template< class TYPE >
struct Increment_semaphore
{
            Increment_semaphore( TYPE* p ) : _ptr ( p ) { (*p)++; }
   virtual ~Increment_semaphore()                       { (*_ptr)--; }

  private:
    TYPE* _ptr;
};

// Beispiel: Increment_semaphore<int>( &sema );

//-------------------------------------------------------------------------------------exchange

template< class TYPE >
inline void exchange( TYPE* a, TYPE* b )
{
    TYPE x = *a;
    *a = *b;
    *b = x;
}

//-----------------------------------------------------------------------------------Close_mode

enum Close_mode
{
	close_normal,           // Objekt normal schließen
	close_error,            // Änderungen nach Möglichkeit nicht durchführen.
    close_cursor            // Nur für Any_file
};

//---------------------------------------------------------------------------------_argc, _argv

//#if !defined __DLL__
    extern int     _argc;
    extern char**  _argv;
//#endif

//---------------------------------------------------------------------------------------strlwr
//---------------------------------------------------------------------------------------strupr
//-----------------------------------------------------------------------------------------itoa
//--------------------------------------------------------------------------------------stricmp
//-------------------------------------------------------------------------------------strnicmp

#if defined SYSTEM_GNU || defined SYSTEM_SOLARIS
    void strlwr( char* );
    void strupr( char* );
    char* itoa( int value, char* str, int radix ); // wie in Borlandc 4.02
#   define stricmp( s1, s2 )        strcasecmp( s1, s2 )
#   define strnicmp( s1, s2, len )  strncasecmp( s1, s2, len )
#endif

} //namespace sos

#include "../kram/sosobjba.h"       /* Sos_object_base, Sos_pointer */
#include "../kram/xception.h"       /* Xc */
//#include <sosstat.h>        /* Sos_static */

//=======================================================================================xlat.h
//  #include <xlat.h>

#ifndef __XLAT_H
#define __XLAT_H

namespace sos
{


#if defined SYSTEM_I386  &&  ( defined SYSTEM_DOS  ||  defined SYSTEM_WIN16 )   /* Intel */

    extern "C" void _far* _cdecl _far xlat_asm (     // Wie memcpy, mit xlat
        void  _far*       destination,
        const void _far*  source,
        uint2             length,
        const Byte _far*  tabelle
    );

    inline void* xlat( void* d, const void* s, const uint2 l, const Byte* t )
    {
        return xlat_asm( d, s, l, t );
    }

#elif defined SYSTEM_WIN16

    // Borland will __asm nicht inline übersetzen

    void xlat( void* d, const void* s, unsigned int l, const Byte* t );

#else

    inline void xlat( void* d, const void* s, unsigned int l, const Byte* t )
    {
        for( unsigned int i = 0; i < l; i++ ) {
            ((Byte*)d)[i] = t[ ((const Byte*)s)[ i ]];
        }
    }

#endif


inline void xlat( void* dest, const void* source, unsigned int len, const char* table )
{
    xlat( dest, source, len, (const Byte*)table );
}

inline void xlat( void* data, unsigned int length, const Byte* table )
{
    xlat( data, data, length, table );
}


} //namespace sos

#endif

//=============================================================================================

#include "area.h"           // für SHOW_MSG
#include "log.h"

namespace sos
{
//----------------------------------------------------------------is_absolute_filename
// in stdfile.cxx

        Bool is_absolute_filename( const char* filename );
inline  Bool is_absolute_filename( const string& filename )  { return is_absolute_filename( filename.c_str() ); }

//-------------------------------------------------------------------------is_filename
// in stdfile.cxx

        Bool is_filename( const char* filename );
inline  Bool is_filename( const string& filename )  { return is_filename( filename.c_str() ); }

//---------------------------------------------------------------------------------remove_directory
// in file/dir.cxx

void remove_directory( const string& path, bool force = false );

//-------------------------------------------------------------------------------------------------

} //namespace sos


#endif
