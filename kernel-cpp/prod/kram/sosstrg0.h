// sosstrg0.h

#ifndef __SOSSTRG0_H
#define __SOSSTRG0_H

#if !defined __SYSDEP_H
#   include "sysdep.h"
#endif

namespace sos
{

typedef unsigned int uint;

#if !defined __STRING_H  &&  !defined _STRING_H
#   include <string.h>
#endif

inline uint             length      ( const char* s )   { return strlen( s ); }
inline const char*      c_str       ( const char* );    // in sos.h implementiert
       Big_int          as_big_int  ( const char* );
       Ubig_int         as_ubig_int ( const char* );
       long             as_long     ( const char* );
       unsigned long    as_ulong    ( const char* );
       Bool             as_bool     ( const char* );
       Bool             as_bool     ( const char*, Bool deflt );
       int              as_int      ( const char* );
       int              as_int      ( const char*, int deflt );
       uint             as_uint     ( const char* );
       short            as_short    ( const char* );
       unsigned short   as_ushort   ( const char* );

#ifdef SYSTEM_INT64
       int64            as_int64    ( const char* );
       uint64           as_uint64   ( const char* );
#endif
       int32            as_int32    ( const char* );
       uint32           as_uint32   ( const char* );
       int16            as_int16    ( const char* );
       uint16           as_uint16   ( const char* );
       int              as_uintK    ( const char* );    // nK: *1024, nM: *1024*1024

inline int4             as_int4     ( const char* text )     { return as_int32( text ); }
inline uint4            as_uint4    ( const char* text )     { return as_uint32( text ); }
inline int2             as_int2     ( const char* text )     { return as_int16( text ); }

inline uint2            as_uint2    ( const char* text )     { return as_uint16( text ); }


       double     c_str_as_double   ( const char* );
inline double           as_double   ( const char* s )   { return c_str_as_double( s ); }
       Bool            _empty       ( const char* );
inline Bool             empty       ( const char* );
inline uint             position    ( const char*, char to_find, unsigned int pos = 0 );
       uint             position    ( const char*, const char* to_find, unsigned int pos = 0 );
       uint             length_without_trailing_char( const char* , uint len, char );
inline uint             length_without_trailing_spaces( const char* ptr, uint len )             { return length_without_trailing_char( ptr, len, ' ' ); }
inline uint             length_without_trailing_spaces( const char* str )                       { return length_without_trailing_spaces( str, strlen( str ) ); }

       unsigned int     position_in_string0( const char* str, char to_find, unsigned int pos );

void                   _ltrim       ( char* );
inline void             ltrim       ( char* str )                                               { if( str[ 0 ] <= ' ' )  _ltrim( str ); }
void                    rtrim       ( char* );
void                    trim        ( char* );

//=======================================================================================INLINE

//-------------------------------------------------------------------------------------position

inline unsigned int position( const char* str, char to_find, unsigned int pos )
{
    return position_in_string0( str, to_find, pos );
}

//----------------------------------------------------------------------------------------empty

inline Bool empty( const char* s )
{
#   if !defined SOS_OPTIMIZE_SIZE
        if( !s           )  return 1;
        if( s[0] == '\0' )  return 1;
#   endif
    return _empty( s );
}


} //namespace sos

#endif
