// $Id: sosalloc.cxx 13964 2010-08-18 11:12:52Z jz $

#include "precomp.h"
#include "../kram/sysdep.h"

#include <stdlib.h>
#include <limits.h>             // UINT_MAX
#include <malloc.h>

#include "../zschimmer/zschimmer.h"
#include "../kram/sos.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../kram/sosalloc.h"

using namespace std;
namespace sos {

//------------------------------------------------------------------------------------sos_alloc

void* sos_alloc( int4 size, const char* info )
{
    void* p = zschimmer::z_malloc(size, info, 0);
    if( !p )  throw_no_memory_error( size );

    memset( p, 0, size );
    return p;
}

//-------------------------------------------------------------------------------------sos_free

void sos_free( void* ptr )
{
    return zschimmer::z_free( ptr );
}


} //namespace sos
