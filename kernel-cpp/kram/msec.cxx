#include "precomp.h"
//#define MODULE_NAME "msec"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sos.h"
#include "../kram/msec.h"

#if defined SYSTEM_WIN

#   include <windows.h>

    using namespace std;
    namespace sos {

    int64 elapsed_msec()
    {
        return GetTickCount();
    }

    } //namespace sos

#elif defined SYSTEM_SOLARIS                    // gettimeofday() scheint's nicht zu geben

#   include <sys/time.h>

    using namespace std;
    namespace sos {

    int64 elapsed_msec()
    {
        hrtime_t hrtime = gethrtime();          // high resolution time (ns)

        return (int64)( hrtime / 1000000 );
    }

    } //namespace sos

#else

    //#include <sys/types.h>
    //#include <unistd.h>
#   include <sys/time.h>

    using namespace std;
    namespace sos {

    int64 elapsed_msec()
    {
        struct timeval   tv;

        gettimeofday( &tv, 0 );

        return (int64)tv.tv_sec * 1000 + (int64)tv.tv_usec / 1000;
    }

    } //namespace sos
#endif
