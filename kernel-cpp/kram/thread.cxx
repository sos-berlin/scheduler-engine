// $Id$

#include "precomp.h"
#include "sos.h"
#include "thread_semaphore.h"
#include "thread.h"

#include <list>

namespace sos {

static int  tls_index;
static bool tls_index_set = false;

static std::list< Sos_ptr<Thread_container> >    container_list;
static Thread_semaphore                          list_lock ( "Thread_container" );


static struct Thread_static
{
    ~Thread_static() 
    {
        THREAD_LOCK( list_lock )  
        {
            if( tls_index_set )
            {
                container_list.clear();
         
                TlsFree( tls_index );
                tls_index_set = false;
            }
        }
    }
} z;

//---------------------------------------------------------------------------------thread_container

Thread_container* thread_container()
{
#   ifdef _WIN32

        if( !tls_index_set )
        {
            tls_index = TlsAlloc();
            if( tls_index == TLS_OUT_OF_INDEXES )  throw_mswin_error( "TlsAlloc", "thread_container" );
            tls_index_set = true;
        }

        Sos_ptr<Thread_container> tc = (Thread_container*)TlsGetValue( tls_index_set );
        if( !tc ) 
        {
            tc = SOS_NEW( Thread_container );
            TlsSetValue( tls_index, tc );
            THREAD_LOCK( list_lock )  container_list.push_back( tc );
        }

        return tc;

#    else

        static Thread_container tc;
        return &tc;

#   endif
}


//-------------------------------------------------------------------------------------------------

} //namespace sos