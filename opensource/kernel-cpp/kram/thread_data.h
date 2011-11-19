// $Id: thread_data.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __SOS_THREAD_DATA_H
#define __SOS_THREAD_DATA_H

#include "thread_semaphore.h"
#include "log.h"

namespace sos {

//--------------------------------------------------------------------------------------Thread_data

template< class THREAD_DATA >
struct Thread_data
{
                                Thread_data             ();
                               ~Thread_data             ();

    void                        thread_detach           ();
    THREAD_DATA*                get                     ();

    THREAD_DATA                 operator *              ()                                      { return *get(); }
    THREAD_DATA*                operator ->             ()                                      { return get(); }
    THREAD_DATA*                operator +              ()                                      { return get(); }
  //                            operator THREAD_DATA*   ()                                      { return get(); }

                                operator bool           ()                                      { return allocated(); }
    bool                        operator !              ()                                      { return !allocated(); }
    bool                        allocated               ();

  private:

#   ifdef SYSTEM_WIN
        Thread_semaphore       _lock;
        int                    _thread_index;
#    else
        THREAD_DATA*           _data;
#   endif

};

//-------------------------------------------------------------------------------------------------

#ifdef SYSTEM_WIN

    template< class THREAD_DATA >
    Thread_data<THREAD_DATA>::Thread_data()
    : 
        _lock("Thread_data"),
        _thread_index(TLS_OUT_OF_INDEXES)
    {
        //_tls_index = TlsAlloc();
    }


    template< class THREAD_DATA >
    Thread_data<THREAD_DATA>::~Thread_data()
    {
        LOGI( "~Thread_data\n" );

        if( _thread_index != TLS_OUT_OF_INDEXES )
        {
            thread_detach();

            LOG( "TlsFree()\n" );
            TlsFree( _thread_index );
            _thread_index = TLS_OUT_OF_INDEXES;
        }
    }

    
    template< class THREAD_DATA >
    void Thread_data<THREAD_DATA>::thread_detach()
    {
        LOGI( "Thread_data::thread_detach\n" );
        
        if( _thread_index != TLS_OUT_OF_INDEXES )
        {
            THREAD_DATA* thread_data = (THREAD_DATA*)TlsGetValue( _thread_index );
            if( thread_data )
            {
                LOGI( "Thread_data::thread_detach delete " << (void*)thread_data << '\n' );
                delete thread_data;
                TlsSetValue( _thread_index, NULL );
            }
        }
    }

    
    template< class THREAD_DATA >
    THREAD_DATA* Thread_data<THREAD_DATA>::get() 
    { 
        if( _thread_index == TLS_OUT_OF_INDEXES )
        {
            THREAD_LOCK( _lock )
            if( _thread_index == TLS_OUT_OF_INDEXES )
            {
                LOG( "Thread_data::get TlsAlloc()\n" );
                _thread_index = TlsAlloc();  
                if( _thread_index == TLS_OUT_OF_INDEXES )  throw_mswin_error( "TlsAlloc" );
            }
        }

        THREAD_DATA* thread_data = (THREAD_DATA*)TlsGetValue( _thread_index );
        
        if( !thread_data )
        {
            LOG( "Thread_data::get new Thread_data\n" );
            thread_data = new THREAD_DATA;
            TlsSetValue( _thread_index, thread_data );
        }

        return thread_data;
    }


    template< class THREAD_DATA >
    bool Thread_data<THREAD_DATA>::allocated() 
    { 
        return _thread_index != TLS_OUT_OF_INDEXES  &&  TlsGetValue( _thread_index ) != NULL;
    }

#else

    template< class THREAD_DATA >
    Thread_data<THREAD_DATA>::Thread_data() 
    {
        _data = new THREAD_DATA;
    }


    template< class THREAD_DATA >
    Thread_data<THREAD_DATA>::~Thread_data()
    {
        delete _data;
        _data = NULL;
    }

    
    template< class THREAD_DATA >
    void Thread_data<THREAD_DATA>::thread_detach()
    {
    }

    
    template< class THREAD_DATA >
    THREAD_DATA* Thread_data<THREAD_DATA>::get() 
    { 
        return _data;
    }

    template< class THREAD_DATA >
    bool Thread_data<THREAD_DATA>::allocated() 
    { 
        return true;
    }

#endif

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
