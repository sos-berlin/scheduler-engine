// precompiled headers

#if defined _MSC_VER  /* Führt hin und wieder zum Compilerabsturz */
#   if defined SYSTEM_STD_CPP_LIBRARY           
#   endif
#   include <stdio.h>               // Wird von afx.h eingezogen. Besser hier damit sysdep.h stdin etc. neutralisieren kann
#   include <stdexcpt.h>            // xmsg bzw. exception
#   define SYSTEM_PRECOMPILED_HEADERS
#endif

#include <string.h>

#if defined SYSTEM_PRECOMPILED_HEADERS

#   include <stdlib.h>
#   include <limits.h>
#   include <ctype.h>
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
