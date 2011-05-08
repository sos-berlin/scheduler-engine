/*  ctype.h

    Defines the locale aware ctype macros.

*/

/*
 *      C/C++ Run Time Library - Version 6.5
 *
 *      Copyright (c) 1987, 1994 by Borland International
 *      All Rights Reserved.
 *
 *      Verändert, jz 8.10.95
 */

#ifndef __SOSCTYPE_H
#define __SOSCTYPE_H

namespace sos
{

extern unsigned char sosctype_tab[ 257 ];

/* character classes */

#define SOS_IS_SP     1           /* space */
#define SOS_IS_DIG    2           /* digit */
#define SOS_IS_UPP    4           /* upper case */
#define SOS_IS_LOW    8           /* lower case */
#define SOS_IS_HEX   16           /* [0..9] or [A-F] or [a-f] */
#define SOS_IS_CTL   32           /* control */
#define SOS_IS_GRAPH 64           /* darstellbares Zeichen */
#define SOS_IS_BLK  128           /* blank */

#define SOS_IS_ALPHA    (SOS_IS_UPP | SOS_IS_LOW)
#define SOS_IS_ALNUM    (SOS_IS_DIG | SOS_IS_ALPHA)
//#define SOS_IS_GRAPH    (SOS_IS_ALNUM | SOS_IS_HEX | SOS_IS_PUN)

inline int sos_isalnum( int c )   { return sosctype_tab[ c + 1 ] & (SOS_IS_ALNUM); }
inline int sos_isalnum( char c )  { return sos_isalnum( (int)(unsigned char)c ); }

inline int sos_isalpha( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_ALPHA); }
inline int sos_isalpha( char c )  { return sos_isalpha( (int)(unsigned char)c ); }

inline int sos_iscntrl( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_CTL); }
inline int sos_iscntrl( char c )  { return sos_iscntrl( (int)(unsigned char)c ); }

inline int sos_isdigit( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_DIG); }
inline int sos_isdigit( char c )  { return sos_isdigit( (int)(unsigned char)c ); }

inline int sos_isgraph( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_GRAPH); }
inline int sos_isgraph( char c )  { return sos_isgraph( (int)(unsigned char)c ); }

inline int sos_islower( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_LOW); }
inline int sos_islower( char c )  { return sos_islower( (int)(unsigned char)c ); }

inline int sos_isprint( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_GRAPH | SOS_IS_BLK); }
inline int sos_isprint( char c )  { return sos_isprint( (int)(unsigned char)c ); }

//inline int sos_ispunct( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_PUN); }
inline int sos_isspace( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_SP); }
inline int sos_isspace( char c )  { return sos_isspace( (int)(unsigned char)c ); }

inline int sos_isupper( int c )   { return sosctype_tab[ 1 + c ] & (SOS_IS_UPP); }
inline int sos_isupper( char c )  { return sos_isupper( (int)(unsigned char)c ); }

inline int sos_isxdigit( int c )  { return sosctype_tab[ 1 + c ] & (SOS_IS_HEX); }
inline int sos_isxdigit( char c ) { return sos_isxdigit( (int)(unsigned char)c ); }

inline int sos_isctype( int c, int type )  { return sosctype_tab[ 1 + c ] & type; }
inline int sos_isctype( char c, int type ) { return sos_isctype( (int)(unsigned char)c, type ); }


} //namespace sos

#endif /* __CTYPE_H */
