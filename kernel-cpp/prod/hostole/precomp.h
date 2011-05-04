// precompiled headers

#include "../zschimmer/com.h"

#if defined _MSC_VER  /* Führt hin und wieder zum Compilerabsturz */
//#   pragma option /Yc /Yu           // create and use precompiled headers
#   if defined SYSTEM_STD_CPP_LIBRARY           
//#       include <cctype>                        // isdigit() wird von den neuen Include-Dateien verlangt
#   endif
#   include <stdio.h>               // Wird von afx.h eingezogen. Besser hier damit sysdep.h stdin etc. neutralisieren kann
#   include <stdexcpt.h>            // xmsg bzw. exception
#   define SYSTEM_PRECOMPILED_HEADERS
#endif

#if defined __BORLANDC__
//#   define SYSTEM_PRECOMPILED_HEADERS       // "cannot, initialized data in header", bei vorbesetzen Parametern
#endif


#if defined SYSTEM_PRECOMPILED_HEADERS

#   include <stdlib.h>
#   include <limits.h>
//#   include <ctype.h>
#   include <stdio.h>

#   include "../kram/sysdep.h"
#   include "../kram/sosstrng.h"
#   include "../kram/sos.h"

#   include "../kram/log.h"
#   include "../kram/stdfield.h"
#   include "../kram/soslimtx.h"
#   include "../file/absfile.h"
#   include "../file/anyfile.h"
#   include "../kram/sosopt.h"
#   include "../kram/sosarray.h"

#endif
