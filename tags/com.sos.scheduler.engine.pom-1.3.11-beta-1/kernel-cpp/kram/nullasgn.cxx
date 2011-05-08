#include "precomp.h"
//#define MODULE_NAME "nullasgn"
// 1.9.92                                                       Joacim Zschimmer

#include "sysdep.h"

#if defined SYSTEM_DOS

#include <stdlib.h>
#include <string.h>

#include "jzincl.h"
#include "nullasgn.h"

using namespace std;
namespace sos {

//-----------------------------------------------------------------------------

Null_pointer_assignment null_pointer_assignment;

//-----------------------------------------------------------------------------

Null_pointer_assignment::Null_pointer_assignment()
{
    initialized = false;
}

//-----------------------------------------------------------------------------

void Null_pointer_assignment::init()
{
    initialized = false;
}

//-----------------------------------------------------------------------------

int Null_pointer_assignment::operator() ()
{
#   if defined SYSTEM_DOS  &&  !defined SYSTEM_WIN
        if (!initialized) {
            memcpy( null_data, (void*)0, sizeof null_data );
            initialized = true;
        }
        return memcmp( null_data, (void*)0, sizeof null_data );
#    else
        return 0;
#   endif
}

} //namespace sos

#endif
