// $Id: threads.h 12593 2007-01-30 01:00:28Z jz $

#ifndef ZSCHIMMER_THREADS_H
#define ZSCHIMMER_THREADS_H

#include "mutex.h"

//-------------------------------------------------------------------------------------------------

#ifdef Z_WINDOWS

#   include "z_windows.h"
#   include "z_windows_thread.h"

    namespace zschimmer 
    {
        typedef windows::Event                  Event;
        typedef windows::Thread                 Thread;
        typedef windows::Thread_local_storage   Thread_local_storage;

        using windows::current_thread_id;
    }

#else

#   include "z_posix.h"
#   include "z_posix_thread.h"
#   include "z_posix_event.h"

    namespace zschimmer 
    {
        typedef posix::Mutex                Mutex;
        typedef posix::Event                Event;
        typedef posix::Thread               Thread;
        typedef posix::Thread_specific_data Thread_local_storage;

        using posix::current_thread_id;
    }

#endif

//-------------------------------------------------------------------------------------------------

//#include "log.h"

namespace zschimmer {

//--------------------------------------------------------------------------------------Thread_data

template< class THREAD_DATA >
struct Thread_data
{
                                Thread_data             ()                                          : _mutex( "Thread_data" ) {}
                               ~Thread_data             ();

    void                        thread_detach           ();
    THREAD_DATA*                get                     ();

    THREAD_DATA                 operator *              ()                                          { return *get(); }
    THREAD_DATA*                operator ->             ()                                          { return get(); }
    THREAD_DATA*                operator +              ()                                          { return get(); }

                                operator bool           ()                                          { return is_allocated(); }
    bool                        operator !              ()                                          { return !is_allocated(); }
    bool                        is_allocated            ()                                          { return _tls.is_allocated()  &&  _tls.get() != NULL; }

  private:
    Mutex                      _mutex;
    Thread_local_storage       _tls;
    THREAD_DATA*               _data;
};

//-----------------------------------------------------------Thread_data<THREAD_DATA>::~Thread_data

template< class THREAD_DATA >
Thread_data<THREAD_DATA>::~Thread_data()
{
    //Z_LOGI( "~Thread_data\n" );

    if( _tls.is_allocated() )  thread_detach();
}

//----------------------------------------------------------Thread_data<THREAD_DATA>::thread_detach

template< class THREAD_DATA >
void Thread_data<THREAD_DATA>::thread_detach()
{
    //Z_LOGI( "Thread_data::thread_detach\n" );
    
    if( _tls.is_allocated() )
    {
        THREAD_DATA* thread_data = (THREAD_DATA*)_tls.get();;
        if( thread_data )
        {
            //Z_LOGI( "Thread_data::thread_detach delete " << (void*)thread_data << '\n' );
            delete thread_data;
            _tls.set( NULL );
        }
    }
}

//--------------------------------------------------------------------Thread_data<THREAD_DATA>::get

template< class THREAD_DATA >
THREAD_DATA* Thread_data<THREAD_DATA>::get()
{ 
    if( !_tls.is_allocated() )
    {
        Z_MUTEX( _mutex )  if( !_tls.is_allocated() )  _tls.allocate();
    }

    THREAD_DATA* thread_data = (THREAD_DATA*)_tls.get();
    
    if( !thread_data )
    {
        //Z_LOG( "Thread_data::get new THREAD_DATA\n" );
        thread_data = new THREAD_DATA;
        _tls.set( thread_data );
    }

    return thread_data;
}

//-------------------------------------------------------------------------------------Simple_event

struct Simple_event : Event_base
{
                                Simple_event                ( const string& name = "" )              : Event_base(name), _zero_(this+1), _mutex(name) {}
                                Simple_event                ( Dont_create, const string& name = "" ) : Event_base(name), _zero_(this+1), _mutex(name) {}
                               ~Simple_event                ()                                      {}

    virtual void                close                       ()                                      {}
    void                        create                      ()                                      {}

    void                        signal                      ( const string& name );
    void                        async_signal                ( const char* name );
    bool                        signaled_then_reset         ();
    void                        reset                       ();
    void                        wait                        ();
    bool                        wait                        ( double seconds );
    bool                        valid                       () const                                { return true; }


    Fill_zero                  _zero_;
    Mutex                      _mutex;
};

//---------------------------------------------------------------------------------------Signalable

struct Signalable {
    virtual void on_event_signaled() = 0;
};

//-----------------------------------------------------------------------------------Callback_event

struct Callback_event : Simple_event {
    private: Signalable* const _signalable;

    public: Callback_event(Signalable* s) : _signalable(s) {}

    public: Callback_event(Signalable* s, const string& name) : Simple_event(name), _signalable(s) {}

    public: void signal(const string& name);
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
