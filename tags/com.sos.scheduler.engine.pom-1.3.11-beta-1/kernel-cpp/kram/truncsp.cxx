#include "precomp.h"
//#define MODULE_NAME "truncsp"
// truncsp.cpp

#include <string.h>

#include "sos.h"
#include "area.h"

using namespace std;
namespace sos {

int truncate_spaces( char* string )
{
    char *p = string + strlen(string);
    while (p-- != string  &&  *p == ' ');
    *(p+1) = 0;
    return long2int (p - string);
}

void truncate_spaces( Area* area )
{
    const char* p0 = area->char_ptr();
    const char* p  = p0 + area->length();

    while( p > p0  && *(p-1) == ' ' )  p--;

    area->length( p - p0 );
}

} //namespace sos
