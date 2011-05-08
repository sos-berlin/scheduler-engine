// sosprog.h                                   ©1996 SOS GmbH Berlin
//                                             Joacim Zschimmer

#ifndef __SOSPROG_H
#define __SOSPROG_H

#if defined SYSTEM_MFC
#   include <afxwin.h>
#else
    typedef int BOOL;
#   ifndef FALSE
#       define FALSE ((BOOL)0)
#   endif
#   ifndef TRUE
#       define TRUE  ((BOOL)1)
#   endif
#endif

#ifndef __SOSAPPL_H
#   include "sosappl.h"
#endif


namespace sos {

struct Sos_program 
#                       if defined SYSTEM_MFC
                           : CWinApp
#                       endif

{
                                Sos_program             ()          : _sos_appl(false) {}

#   if defined SYSTEM_MFC
    BOOL                        InitInstance            ();         // Einstieg in soswnmai.cxx
#   else
  //BOOL                        InitInstance            ()  {}
  //BOOL                        ExitInstance            ()  {}
#   endif

  //virtual int                 main                    ( int, char** ) = 0;

  //virtual int                _main                    ( int argc, char** argv );

  private:
    Sos_appl                   _sos_appl;               // Startet und beendet Sos_static
};


extern Sos_program* app_ptr;

} //namespace sos

#endif


