#include "precomp.h"
//#define MODULE_NAME "strupr"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "sysdep.h"
#if !defined SYSTEM_BORLAND  &&  !defined SYSTEM_MICROSOFT

#include "sos.h"

using namespace std;

namespace sos 
{

void strupr( char* str )
{
    while( *str ) {
        if( *str >= 'a'  &&  *str <= 'z' ) {
            *str += 'A' - 'a';
        }
        str++;
    }
}

}
#endif
