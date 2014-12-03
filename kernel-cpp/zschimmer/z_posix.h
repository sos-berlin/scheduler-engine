// $Id: z_posix.h 11394 2005-04-03 08:30:29Z jz $

#ifndef ZSCHIMMER_Z_POSIX_H
#define ZSCHIMMER_Z_POSIX_H

#include "base.h"

#ifdef Z_UNIX


//-------------------------------------------------------------------------------------------------

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------------------

//struct Event;
//struct Thread;

//-------------------------------------------------------------------------------------------------

void                            timespec_add                ( timespec* ts, double seconds );
void                            timespec_add                ( timespec* ts, const timeval& tv );

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
#endif
