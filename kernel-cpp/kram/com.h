// $Id: com.h 11394 2005-04-03 08:30:29Z jz $

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
