// $Id$    Joacim Zschimmer

#ifndef __Z_PERL_COM_H
#define __Z_PERL_COM_H

#include <list>
#include "com.h"


extern "C"
{
#   include "perl/CORE/EXTERN.h"
#   include "perl/CORE/perl.h" 
#   include "perl/CORE/XSUB.h"
}

#ifdef list
#   undef list  // Perl
#endif

#ifdef Z_SOS
#   define Z_PERL_IDISPATCH_PACKAGE_NAME "SOS::COM::IDispatch"
#else
#   define Z_PERL_IDISPATCH_PACKAGE_NAME "Zschimmer::COM::IDispatch"
#endif


namespace zschimmer {

extern const char               perl_com_pm             [];             // package zschimmer
extern "C" XS(                  perl_com_boot           );              // newXS
extern "C" XS(                  XS_Zschimmer_call       );              // sub zschimmer::call()

} //namespace zschimmer




#endif
