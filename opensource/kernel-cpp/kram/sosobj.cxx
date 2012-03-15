#include "precomp.h"
//#define MODULE_NAME "sosobj"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#define DONT_LOG

#include <limits.h>

#include "../kram/sysdep.h"
#include "../kram/sysxcept.h"
#include "../kram/sosstrng.h"       // <- storable.h <- soslimtx.h <- sosobjd.h
#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/log.h"
#include "../kram/msec.h"
#include "../kram/sosobj.h"

using namespace std;
namespace sos {

//----------------------------------------------------------------------------Sos_object::close

void Sos_object::close( Close_mode )
{
}

} //namespace sos
