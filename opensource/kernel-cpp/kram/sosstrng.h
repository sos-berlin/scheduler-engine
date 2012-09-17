#ifndef __SOSSTRNG_H
#define __SOSSTRNG_H

#if !defined __SYSDEP_H
#   include "sysdep.h"
#endif

#include "../kram/sosstrg0.h"       // position_in_string0


#   include <string>


namespace sos
{
    typedef std::string Sos_string;
    
    inline std::string   as_string( const char* strng, size_t length  )   { return zschimmer::make_string( strng, length ); }   // F�r gcc 3.1 (ab gcc 3.2.2 nicht mehr n�tig)

    inline const char*   c_str    ( const std::string& strng )                  { return strng.c_str();   }
    inline size_t        length   ( const std::string& strng )                  { return strng.length(); }

    unsigned int position( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 );

    inline void assign( Sos_string* str, const char* o )           { *str = o; }


//------------------------------------------------------------------------------as_string(char)

    inline std::string as_string( char c )  { return std::string( &c, 1 ); }

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

       Sos_string quoted_string  ( const char*, char quote1, char quote2, size_t len );
inline Sos_string quoted_string  ( const char* str, char q1 = '"' , char q2 = '\\' )            { return quoted_string( str, q1, q2, length( str ) ); }
inline Sos_string quoted_string  ( const Sos_string& str, char q1 = '"' , char q2 = '\\' )      { return quoted_string( c_str( str ), q1, q2, length( str ) ); }

       int        should_be_quoted( const char* );

// Nur in quote1 setzen, wenn ein Zeichen nicht { sos_isalnum(), '_' }
       Sos_string cond_quoted_string  ( const char* str, char q1 = '"' , char q2 = '\\' );
inline Sos_string cond_quoted_string  ( const Sos_string& str, char q1 = '"' , char q2 = '\\' ) { return cond_quoted_string( c_str( str ), q1, q2 ); }

       void       append_option( Sos_string*, const char* opt, const char* value );
// append_option( &param, "-option=", value );  // setzt bei Bedarf Anf�hrungszeichen
inline void       append_option( Sos_string* str , const char* opt, const Sos_string& value ) { append_option( str, opt, c_str( value ) ); }
       string     right_string ( const string&, int len );

const int NO_POS = -1;

inline unsigned int find( const Sos_string& str, const Sos_string& to_find, unsigned int pos = 0 )
{
#   if !defined SYSTEM_SOLARIS   /* Borland kann sich nicht f�r das richtige position() entscheiden */
        uint p = position( c_str( str ), c_str( to_find ), pos );
#    else
        uint p = position( str, to_find, pos );
#   endif
    return p == length( str )? (unsigned int)NO_POS : p;  //jz 1.11.98 NO_POS == -1, aber uint???
}

inline int valid_position ( const Sos_string& str, unsigned int pos )
{
    return pos < length( str );
}

//----------------------------------------------------------------------------------------

inline unsigned int position( const Sos_string& str, char to_find, unsigned int pos = 0 )
{
    return position_in_string0( c_str( str ), to_find, pos );
}


inline Bool empty( const Sos_string& str )
{
#   if !defined SOS_OPTIMIZE_SIZE
        if( length( str ) == 0 )  return 1;
#   endif
    return _empty( str );
}


} //namespace sos

#endif
