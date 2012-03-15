// sostimer.cxx

#include "precomp.h"

#include <limits.h>

#include "../kram/sos.h"
#include "../kram/sosstat.h"
#include "../kram/sostimer.h"

#include <time.h>

using namespace std;
namespace sos {

//------------------------------------------------------------------------------clock_as_double
// clock_as_double() gives the current time, in seconds, elapsed since 00:00:00 GMT, January 1, 1970.

double clock_as_double()
{
    time_t t;

    return (double)time( &t );
}

} //namespace sos

