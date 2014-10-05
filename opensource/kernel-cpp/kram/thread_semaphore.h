// $Id: thread_semaphore.h 11394 2005-04-03 08:30:29Z jz $

// Frei von sos.h!

#include "../zschimmer/mutex.h"

using zschimmer::Mutex;
using zschimmer::Mutex_guard;

typedef Mutex                   Thread_semaphore;
