#ifndef __SOSSTRNG_H
#define __SOSSTRNG_H

#if !defined __SYSDEP_H
#   include "sysdep.h"
#endif

#include "../kram/sosstrg0.h"       // position_in_string0

// Windows: Borland Stringklasse: string
// Solaris: Tools.h++ Klasse: RWCString
// Sonst selbst programmiert

/*
#if !defined SOS_STRING || !defined SOS_STRING_STARVIEW ||  !defined SOS_STRING_BORLAND || !defined SOS_STRING_SOS
#   if defined SYSTEM_SOLARIS
#       define SOS_STRING_RW            // Rogue Wave Tools.h++
#   elif defined SYSTEM_STARVIEW
#       define SOS_STRING_STARVIEW      
#   elif defined __BORLANDC__
#       define SOS_STRING_BORLAND
#   elif defined SYSTEM_GNU
#       define SOS_STRING_STL
#   else
#       define SOS_STRING_SOS
#   endif
#endif
*/

#define SOS_STRING_STL


#if defined SOS_STRING_STL
#   include <string>
#elif defined SOS_STRING_RW
#   include <rw/cstring.h>
#   include <rw/regexp.h>
#elif defined _MSC_VER
#   include <afx.h>        // MFC Microsoft Foundation Classes
#elif defined SOS_STRING_STARVIEW
#   include <svstring.h>
#elif defined SOS_STRING_BORLAND
#   include <borstrng.h>
#else
#   if !defined __STRING_H  &&  !defined _STRING_H
#       include <string.h>
#   endif
#   if !defined __ASSERT_H
#       include <assert.h>
#   endif
#endif


namespace sos
{


#if defined SOS_STRING_STL

    typedef std::string Sos_string;
    
  //inline std::string   as_string( const char* strng, unsigned int length  )   { return std::string( strng, length ); }
    inline std::string   as_string( const char* strng, unsigned int length  )   { return zschimmer::make_string( strng, length ); }   // Für gcc 3.1 (ab gcc 3.2.2 nicht mehr nötig)

    inline const char*   c_str    ( const std::string& strng )                  { return strng.c_str();   }
    inline unsigned int  length   ( const std::string& strng )                  { return strng.length(); }

    inline unsigned int position( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 )
    {
        size_t p = str.find( to_find, pos );
        return p < 0 || p > length(str)? length( str ) : p;
    }

    inline void assign( Sos_string* str, const char* o )           { *str = o; }


#elif defined SOS_STRING_RW

    typedef RWCString Sos_string;
  //typedef RWCString string; // ???

    inline RWCString     as_string( const char* string, unsigned int length )   { return RWCString( string, length ); }
    inline const char*   c_str    ( const RWCString& string )                   { return string.data();   }
    inline unsigned int  length   ( const RWCString& string )                   { return string.length(); }

    inline unsigned int position( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 )
    {
        int RWCSTRING_FIND_NUR_MIT_REGEX;
        RWCRegexp regex ( c_str( to_find ));
	    size_t erg = str.index( regex, NULL, pos );
        return erg == RW_NPOS ? length( str ) : erg;
    }

    // Gibt es eine direkte Routine?:
    //inline void assign( Sos_string* str, const char* o, int len )  { *str = as_string( o, len ); }
    inline void assign( Sos_string* str, const char* o )           { *str = o; }

#elif defined _MSC_VER

    //schon von afx.h erledigt:
//#   define WIN32_EXTRA_LEAN
//#   define WIN32_LEAN_AND_MEAN
//#   define NOSERVICE
//#   define NOMCX
//#   define NOIME
//#   define VC_EXTRALEAN

    inline CString as_string( const char* string, unsigned int length )  { return CString( string, length ); }

    inline const char*   c_str    ( const CString& string )                   { return string;   }
    inline unsigned int  length   ( const CString& string )                   { return string.GetLength(); }

    inline unsigned int position( const CString& str, const CString& to_find, unsigned int pos = 0 )
    {
        size_t erg = str.Find( c_str( to_find ) + pos );
        return erg == -1? length( str ) : erg;
    }

    // Gibt es eine direkte Routine?:
    //inline void assign( Sos_string* str, const char* o, int len )  { *str = as_string( o, len ); }
    inline void assign( CString* str, const char* o )           { *str = o; }

    ostream& operator << ( ostream& s, const CString& str );  // in mfcstrng.cxx

    typedef CString Sos_string;

//#elif defined __BORLANDC__
#elif defined SOS_STRING_STARVIEW

    String as_string( const char* string, unsigned int length );

    typedef String Sos_string;

//#elif defined __BORLANDC__
#elif defined SOS_STRING_BORLAND

    typedef string Sos_string;

    string as_string( const char* str, unsigned int length );
#else


#   define OWN_SOSSTRING


    struct Sos_string
    {
                            Sos_string              ();
        inline              Sos_string              ( const char* );
                            Sos_string              ( char );
        inline              Sos_string              ( const char*, int length );
                            Sos_string              ( const Sos_string& );
                           ~Sos_string              ();

        Sos_string&         operator =              ( const Sos_string& str )                     { assign( c_str( str ), length( str ) ); }
        Sos_string&         operator =              ( const char* str )                 		  { assign( str ); }

        friend inline unsigned int  length          ( const Sos_string& );
        friend inline const char*   c_str           ( const Sos_string& );
        inline char         operator [ ]            ( int i ) const;
        inline Sos_string   operator +              ( const Sos_string& ) const;
        inline Sos_string   operator +              ( const char* str   ) const                   { Sos_string a = *this; a += str; return a; }
        inline Sos_string&  operator +=             ( char );
        inline Sos_string&  operator +=             ( const char* str )                           { append( str ); }
        inline Sos_string&  operator +=             ( const Sos_string& str )                     { append( c_str( str ), length( str ) ); }
        inline void         copy                    ( char*, int offset, int length ) const;
        inline void         assign                  ( const char* );
               void         assign                  ( const char*, int length );
        inline void         append                  ( const char* str )                           { append( str, strlen( str ) ); }
        inline void         append                  ( const char*, int length );
        inline void         append                  ( char );

      private:
        void                size                    ( int size );

        int                _size;                   // Inklusive 0-Byte
        int                _length;
        char*              _ptr;

        static /*const*/ char  _null_string [ 1 ];
    };

/*
    unsigned   int position( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 );
    Sos_string sub_string( const Sos_string str, int pos, int length );
*/
    //istream& operator>> ( istream&, Sos_string& );
    inline ostream& operator<< ( ostream& s, const Sos_string& str ) { s.write( c_str( str ), length( str ) ); }

    inline void          assign   ( Sos_string* str, const char* o )           { *str = o; }
    inline Sos_string    as_string( const char* str, unsigned int length )     { return Sos_string( str, length ); }
#endif

//------------------------------------------------------------------------------as_string(char)

#if defined SYSTEM_STRING_BORLAND  &&  defined SYSTEM_WIN16DLL  /* Borland C++ 5.00 */  
    inline Sos_string as_string( char c )  { char str[2]; str[0]=c; str[1]='\0'; return Sos_string( str ); }
#elif defined SOS_STRING_STL
    inline std::string as_string( char c )  { return std::string( &c, 1 ); }
#else
    inline Sos_string as_string( char c )  { return Sos_string( c ); }
#endif

//---------------------------------------------------------------------------------empty_string

extern const Sos_string empty_string;

//--------------------------------------------------------------------------- String-Funktionen

struct Const_area;
struct Area;

inline Sos_string       as_string   ( const Const_area& area );    // definiert in area.inl
inline Sos_string       as_string   ( const char* string     )     { return Sos_string( string ); }

       Sos_string       as_string   ( Big_int );
       Sos_string       as_string   ( Ubig_int );
       Sos_string       as_hex_string( Big_int );
       Sos_string       as_hex_string( int );
       Sos_string       as_hex_string( char );
       uint32           hex_as_int32( const string& str );

inline Sos_string       as_string   ( int i )                      { return as_string( (Big_int)i ); }
inline Sos_string       as_string   ( uint i )                     { return as_string( (Ubig_int)i ); }
inline Sos_string       as_string   ( short i )                    { return as_string( (Big_int)i ); }
inline Sos_string       as_string   ( unsigned short i )           { return as_string( (Ubig_int)i ); }
       string           as_string   ( double );
#if defined SYSTEM_BIG_INT
inline Sos_string       as_string   ( long i )                     { return as_string( (Big_int)i ); }
inline Sos_string       as_string   ( unsigned long i )            { return as_string( (Ubig_int)i ); }
#endif

inline const char*      c_str       ( const Sos_string&      );
inline unsigned int     length      ( const Sos_string&      );
       Bool            _empty       ( const Sos_string&      );    // nichts als whitespaces
inline Bool             empty       ( const Sos_string&      );    // nichts als whitespaces
//inline void             empty       ( Sos_string* string_ptr )     { *string_ptr = ""; }

inline uint             length_without_trailing_spaces( const string& str )             { return length_without_trailing_spaces( str.c_str(), str.length() ); }
inline int              length_without_trailing_char  ( const string& str, char c )      { return length_without_trailing_char( str.c_str(), str.length(), c ); }
       void             ltrim                         ( string* );
inline string           ltrim                         ( const string& str )              { string result = str; ltrim(&result); return result; }
inline void             rtrim                         ( string* str )                    { str->erase( length_without_trailing_spaces( *str ) ); }
inline string           rtrim                         ( const string& str )              { return str.substr( 0, length_without_trailing_spaces( str ) ); }
       void             trim                          ( string* );
       string           trim                          ( const string& );


//--------------------------------------------------------------------------------------compare
/*
#if defined SOS_STRING_BORLANDxxxx  ||  defined __GNU__
// Damit (const char*) nicht zu Sos_string konvertiert werden muß

Bool operator <  ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) <  0; }
Bool operator <= ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) <= 0; }
Bool operator == ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) == 0; }
Bool operator != ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) != 0; }
Bool operator >= ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) >= 0; }
Bool operator >  ( const Sos_string& a, const char* b )  { return strcmp( c_str( a ), b ) >  0; }

Bool operator <  ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) <  0; }
Bool operator <= ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) <= 0; }
Bool operator == ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) == 0; }
Bool operator != ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) != 0; }
Bool operator >= ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) >= 0; }
Bool operator >  ( const char* a, const Sos_string& b )  { return strcmp( a, c_str( b ) ) >  0; }

#endif
*/
//---------------------------------------------------------------------------------------as_int

int                     as_int      ( const Sos_string& );
int                     as_int      ( const Sos_string&, int deflt );
uint                    as_uint     ( const Sos_string& );
Big_int                 as_big_int  ( const Sos_string& );
Ubig_int                as_ubig_int ( const Sos_string& );

inline double           as_double   ( const string& str )                       { return as_double( str.c_str() ); }

inline bool             as_bool     ( const string& str )                       { return as_bool( str.c_str() ); }
inline bool             as_bool     ( const string& str, bool deflt )           { return as_bool( str.c_str(), deflt ); }


void       append         ( Sos_string* string_ptr, const char* ptr, int length );
Sos_string sub_string     ( const Sos_string& str, int pos, int len = -1 );
Sos_string expanded_string( const Area& area, char c = ' ' );
Sos_string expanded_string( const Sos_string& str, int len, char c = ' ' );
Sos_string expanded_string( const Const_area& area, int len,  char c = ' ' );

       Sos_string quoted_string  ( const char*, char quote1, char quote2, int len );
inline Sos_string quoted_string  ( const char* str, char q1 = '"' , char q2 = '\\' )            { return quoted_string( str, q1, q2, length( str ) ); }
inline Sos_string quoted_string  ( const Sos_string& str, char q1 = '"' , char q2 = '\\' )      { return quoted_string( c_str( str ), q1, q2, length( str ) ); }

       int        should_be_quoted( const char* );

// Nur in quote1 setzen, wenn ein Zeichen nicht { sos_isalnum(), '_' }
       Sos_string cond_quoted_string  ( const char* str, char q1 = '"' , char q2 = '\\' );
inline Sos_string cond_quoted_string  ( const Sos_string& str, char q1 = '"' , char q2 = '\\' ) { return cond_quoted_string( c_str( str ), q1, q2 ); }

       void       append_option( Sos_string*, const char* opt, const char* value );
// append_option( &param, "-option=", value );  // setzt bei Bedarf Anführungszeichen
inline void       append_option( Sos_string* str , const char* opt, const Sos_string& value ) { append_option( str, opt, c_str( value ) ); }
       string     right_string ( const string&, int len );

const int NO_POS = -1;

inline unsigned int find( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 )
{
#   if !defined SYSTEM_SOLARIS   /* Borland kann sich nicht für das richtige position() entscheiden */
        uint p = position( c_str( str ), c_str( to_find ), pos );
#    else
        uint p = position( str, to_find, pos );
#   endif
    return p == length( str )? (unsigned int)NO_POS : p;  //jz 1.11.98 NO_POS == -1, aber uint???
}

//unsigned int position       ( const Sos_string& str, const Sos_string& to_find, uint pos = 0 );
//uint regex_position ( const Sos_string& str, const Sos_string& regex, uint pos = 0 );
inline int valid_position ( const Sos_string& str, unsigned int pos )
{
    return pos < length( str );
}
/*
operator +

ltrim   blanks am anfang abschneiden
rtrim
substring
substitute/gsub  später
upper
lower

zahlenkonvertierung  int4 uint4 Sos_number           as_int()      object_store

*/
//const int STRING_LENGTH = -1;
//Sos_string sub_string( const Sos_string str, unsigned int pos = 0, unsigned int length = STRING_LENGTH );

//----------------------------------------------------------------------------------------

inline unsigned int position( const Sos_string& str, char to_find, unsigned int pos = 0 )
{
    return position_in_string0( c_str( str ), to_find, pos );
}

//------------------------------------------------------------------------------------OWN_SOSSTRING
#if defined OWN_SOSSTRING

inline Sos_string::Sos_string()
:
    _size   ( 1 ),
    _length ( 0 ),
    _ptr    ( _null_string )
{
}


inline Sos_string::Sos_string( const char* str )
:
    _size   ( 1 ),
    _length ( 0 ),
    _ptr    ( _null_string )
{
    assign( str );
}


inline Sos_string::Sos_string( const char* str, int length )
:
    _size   ( 1 ),
    _length ( 0 ),
    _ptr    ( _null_string )
{
    assign( str, length );
}


inline unsigned int length( const Sos_string& string )
{
    return string._length;
}


inline const char* c_str( const Sos_string& string )
{
    return string._ptr;
}


inline char Sos_string::operator[] ( int i ) const
{
    assert( i < _size );
    return _ptr[ i ];
}


inline void Sos_string::copy( char* buffer_ptr, int offset, int len ) const
{
    memcpy( buffer_ptr, _ptr + offset, len );
}



inline void Sos_string::assign( const char* str )
{
    assign( str, strlen( str ) );
}

inline Sos_string& Sos_string::operator+= ( char c )
{
    append( c );
    return *this;
}

inline int  compare     ( const Sos_string& a, const Sos_string& b )  { return strcmp( c_str( a ), c_str( b ) ); }
inline int  operator == ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) == 0; }
inline int  operator != ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) != 0; }
inline int  operator <  ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) <  0; }
inline int  operator <= ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) <= 0; }
inline int  operator >  ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) >  0; }
inline int  operator >= ( const Sos_string& a, const Sos_string& b )  { return compare( a, b ) >= 0; }

/* wg. strlen nach sosstrng.cxx
inline Sos_string Sos_string::operator+ ( const char* string )
{
    Sos_string s = *this;
    s.append( string, strlen( string ) );
    return s;
}
*/

inline Sos_string operator+ ( const Sos_string& a, const Sos_string& b )
{
    Sos_string s = a;
    s.append( c_str( b ), ::length( b ) );
    return s;
}

#endif


inline Bool empty( const Sos_string& str )
{
#   if !defined SOS_OPTIMIZE_SIZE
        if( length( str ) == 0 )  return 1;
#   endif
    return _empty( str );
}


} //namespace sos

#endif
