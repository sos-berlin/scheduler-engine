// optimize.h                                           © 1995 SOS GmbH Berlin

//#if !defined __SYSDEP_H
//#    include <sysdep.h>
//#endif

#if !defined SOS_SMALL_CODE
#   define SOS_INLINE
#   define CHECK_STACK_OVERFLOW
#endif

#if !defined _DEBUG  &&  !defined DEBUG
#   if defined _MSC_VER
#       pragma auto_inline( on )    // Compiler löst selbst manche Funktionen inline auf
#       pragma optimize( "y", off ) // y: generate frame pointer
#       pragma optimize( "g", on )  // g: global
#       if !defined SOS_SMALL_CODE
#           pragma optimize( "s", off )  // s: size
#           pragma optimize( "t", on  )  // t: time
#       endif
#   endif

//#     if defined SYSTEM_SOLARIS
//          #pragma ... -O4
//#     endif

#   if defined __BORLANDC__
#       pragma option -Ot          // Zeit-Optimierung
#       pragma option -vi          // inlines auflösen
#       pragma option -xf          // Exception-Prolog inline (ergibt mehr Code)
#       if !defined CHECK_STACK_OVERFLOW
#           pragma option -N-          // Stack-Überlaufprüfung abschalten
#       endif
#   endif
#endif



