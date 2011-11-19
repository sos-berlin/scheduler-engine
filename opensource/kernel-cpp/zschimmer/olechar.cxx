// $Id: olechar.cxx 11940 2006-03-05 19:13:06Z jz $

#include "system.h"
#include "olechar.h"
#include <stdio.h>   //fprintf
#include <assert.h>


#ifndef Z_HPUX
#   include <wctype.h>
#endif

//-----------------------------------------------------------------------------------------towlower
#ifdef Z_HPUX

int towlower( int c )
{
    return (uint)c < 256? tolower( c ) : c;
}

#endif
//------------------------------------------------------------------------------------------wmemchr

const OLECHAR* wmemchr( const OLECHAR* s, OLECHAR c, int len )
{
    if( s == NULL )
    {
        if( len <= 0 )  return NULL;                    // wmemchr(NULL,c,-1) tolerieren wir.
        fprintf( stderr, "wmemchr(NULL,%d)!\n", len );  // Führt zum Absturz
    }

    const OLECHAR* p = s;
    while( (unsigned int)len > 0  &&  *p != c )  p++, len--;        // (uint), damit len=-1 als len=MAX_UINT gedeutet wird.
    return p; 
}

//-------------------------------------------------------------------------------------------wcschr

const OLECHAR* wcschr( const OLECHAR* s, OLECHAR c )
{
    if( !s )  return NULL;

    const OLECHAR* p = s;

    while( *p )
    {
        if( *p == c )  return p;
        p++;
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------wcslen

int wcslen( const OLECHAR* s )
{
    const OLECHAR* p = s;
    if( p )  while( *p )  p++;
    return p - s;
}

//-------------------------------------------------------------------------------------------wcscmp

int wcscmp( const OLECHAR* a, const OLECHAR* b )
{
    if( a == NULL )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL )  return +1;

    while( *a   &&  *a == *b )  a++, b++;

    return *a < *b? -1 :
           *a > *b? +1 
                  : 0;

/*
    while( *a | *b )
    {
        if( *a < *b )  return -1;
        if( *a > *b )  return +1;
        a++, b++;
    }

    return 0;
*/
}

//----------------------------------------------------------------------------------------wcscasecmp

int wcscasecmp( const OLECHAR* a, const OLECHAR* b )
{ 
    if( a == NULL )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL )  return +1;

    while( *a   &&  towlower(*a) == towlower(*b) )  a++, b++;

    OLECHAR aa = towlower(*a);
    OLECHAR bb = towlower(*b);

    return aa < bb? -1 :
           aa > bb? +1 
                  : 0;
}

//-------------------------------------------------------------------------------------------wcscmp
#ifdef __GNUC__   //  sizeof(OLECHAR) != sizeof(wchar_t)

int wcscmp( const OLECHAR* a, const wchar_t* b )
{
    if( a == NULL )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL )  return +1;

    while( *a   &&  *a == *b )  a++, b++;

    return *a < *b? -1 :
           *a > *b? +1 
                  : 0;
}

#endif
//------------------------------------------------------------------------------------------wcsncmp

int wcsncmp( const OLECHAR* a, const OLECHAR* b, int length )
{
    assert( length >= 0 );

    if( length == 0 )  return 0;
    if( a == NULL   )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL   )  return +1;

    while( length--  &&  *a   &&  *a == *b )  a++, b++;

    return length == 0? 0  :
           *a < *b    ? -1 :
           *a > *b    ? +1 
                      : 0;
}

//------------------------------------------------------------------------------------------wcsncmp
#ifdef __GNUC__   //  sizeof(OLECHAR) != sizeof(wchar_t)

int wcsncmp( const OLECHAR* a, const wchar_t* b, int length )
{
    assert( length >= 0 );

    if( length == 0 )  return 0;
    if( a == NULL   )  return b == NULL  ||  b[0] == '\0'? 0 : -1;
    if( b == NULL   )  return +1;

    while( length--  &&  *a   &&  *a == *b )  a++, b++;

    return length == 0? 0  :
           *a < *b    ? -1 :
           *a > *b    ? +1 
                      : 0;
}

#endif
//-------------------------------------------------------------------------------------------------

