// $Id: z_posix_signal.h 11394 2005-04-03 08:30:29Z jz $

#ifndef ZSCHIMMER_Z_POSIX_MUTEX_H
#define ZSCHIMMER_Z_POSIX_MUTEX_H

#ifdef Z_UNIX

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------------------

string                          signal_description          ( int signal );

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
#endif
