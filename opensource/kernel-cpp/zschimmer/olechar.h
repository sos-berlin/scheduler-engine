// $Id: olechar.h 11980 2006-03-20 14:49:29Z jz $

#ifndef __ZSCHIMMER_OLECHAR
#define __ZSCHIMMER_OLECHAR

#ifndef Z_WINDOWS

#include <string.h>

//------------------------------------------------------------------------------------------OLECHAR

// #if __STDC_ISO_10646__ >= yyyymm

#ifdef Z_OLECHAR_IS_WCHAR
  //typedef wchar_t             OLECHAR;
#else
    typedef unsigned short      OLECHAR;
#   define Z_OLECHAR_IS_UINT16
#endif

typedef OLECHAR*                LPOLESTR;
typedef const OLECHAR*          LPCOLESTR;

//------------------------------------------------------------------------------------------wmemchr

inline void                     wmemcpy                 ( OLECHAR* d, const OLECHAR* s, int l )      { memcpy( d, s, l * sizeof(OLECHAR) ); }
const OLECHAR*                  wmemchr                 ( const OLECHAR* s, OLECHAR c, int len );
const OLECHAR*                  wcschr                  ( const OLECHAR* s, OLECHAR c );
int                             wcslen                  ( const OLECHAR* s );
int                             wcscmp                  ( const OLECHAR* a, const OLECHAR* b );
int                             wcscasecmp              ( const OLECHAR* a, const OLECHAR* b );
int                             wcsncmp                 ( const OLECHAR* a, const OLECHAR* b, int length );
int                             wcsncmp                 ( const OLECHAR* a, const wchar_t* b, int length );


#ifndef Z_OLECHAR_IS_WCHAR//  sizeof(OLECHAR) != sizeof(wchar_t) ?
    int                         wcscmp                  ( const OLECHAR* a, const wchar_t* b );
    inline int                  wcscmp                  ( const wchar_t* a, const OLECHAR* b )      { return -wcscmp(b,a); }
#endif

#endif

#endif
