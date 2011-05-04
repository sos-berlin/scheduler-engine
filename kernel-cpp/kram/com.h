// $Id$

#ifdef _WIN32

#   define SYSTEM_HAS_COM
#   define SYSTEM_HAS_IDISPATCH

#   include <windows.h>
//#   include <ole2.h>
#   include <comdef.h>
#   include <unknwn.h>     // IUnknown

#else

#   include "../zschimmer/com.h"

#endif
