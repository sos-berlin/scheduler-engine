#ifndef __ZSCHIMMER_CALL_H
#define __ZSCHIMMER_CALL_H

#include "zschimmer.h"

namespace zschimmer {

struct Call : zschimmer::Object
{
    virtual ~Call();
    virtual void call() const = 0;
};

}


#endif
