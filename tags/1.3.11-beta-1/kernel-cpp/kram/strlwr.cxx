#include "precomp.h"
//#define MODULE_NAME "strlwr"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sysdep.h"

#if !defined SYSTEM_BORLAND  &&  !defined SYSTEM_MICROSOFT

#include "sos.h"

using namespace std;
using namespace sos;

void strlwr( char* str )
{
    while( *str ) {
        if( *str >= 'A'  &&  *str <= 'Z' ) {
            *str += 'a' - 'A';
        }
        str++;
    }
}

#endif
