#include "precomp.h"
//#define MODULE_NAME "assert"
// assert.cpp
//                                                      (c) Joacim Zschimmer

#include <stdlib.h>

#include "../kram/sos.h"
#include "../kram/log.h"

using namespace std;
namespace sos {


struct Assert_error {};

void sos_assertfail (
    const char* condition,
    const char* file_name,
    const char* source_file_name,
    int         lineno
) {
    SHOW_ERR( "\n\nAssertion " << condition << " failed in "
              << file_name << '(' << dec << lineno << ") in source "
              << source_file_name );
              
#   if defined SYSTEM_EXCEPTIONS
        throw Assert_error();
#    else
        int abort_wird_gerufen;
        abort();
#   endif        
}

} //namespace sos
