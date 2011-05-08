#include "precomp.h"
//#define MODULE_NAME "itoa"
// itoa.cxx                                 (c) Jörg Schwiemann

#include "sysdep.h"

#if defined SYSTEM_GNU  || defined SYSTEM_SOLARIS || defined SYSTEM_HPUX

#include <string.h>
#include <strstream>

using namespace std;
namespace sos {


char* itoa( int value, char* str, int radix )
{
    if ( radix != 10 ) {} // ???
    char buf[17+1];
    memset( buf, 0, sizeof buf );
    ostrstream s( buf, sizeof buf );
    s << value;
    strcpy( str, buf );
    return str;
}

} //namespace sos

#endif

