// $Id$

#include "zschimmer.h"

#ifdef Z_WINDOWS

#include "z_windows_event.h"
#include "threads.h"
#include "log.h"

using namespace std;

namespace zschimmer {
namespace windows {


//-----------------------------------------------------------------------------windows_message_step

static void windows_message_step()
{
    MSG msg;

    while(1)
    {
        msg.message = WM_NULL;

        int ret = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );
        if( !ret )  break;
        if( msg.message == WM_NULL )  break;

        //TranslateMessage( &msg ); 
        DispatchMessage( &msg ); 
    }
}

//-----------------------------------------------------------------------------------wait_for_event

static bool wait_for_event( HANDLE handle, double wait_time )
{
    assert( handle );

    do
    {
        const double max_t = (double)(INT_MAX-10) / 1000.0;
        int ms = int( min( max_t, wait_time ) * 1000.0 );
        if( ms < 0 )  ms = 0;

        if( ms > 0 )  Z_LOG( "MsgWaitForMultipleObjects(" << (void*)handle << "," << ( ms * 1000.0 ) << "s)\n" );
        int ret = MsgWaitForMultipleObjects( 1, &handle, FALSE, ms, QS_ALLINPUT ); 

        if( ret == WAIT_OBJECT_0 )  return true;
        
        if( ret == WAIT_OBJECT_0 + 1 )  windows_message_step();
        else
        if( ret != WAIT_TIMEOUT )  throw_mswin( "MsgWaitForMultipleObjects" );

        wait_time -= max_t;
    }
    while( wait_time > 0 );

    return false;
}

//-------------------------------------------------------------------------------------Event::Event

Event::Event( const string& name )
: 
    _zero_(this+1),
    _mutex( "Event " + name)
{
    set_name( name );
    create();
}

//----------------------------------------------------------------------------------- Event::~Event

Event::~Event()
{
}

//------------------------------------------------------------------------------------Event::create

void Event::create()
{
    //Z_LOG( "CreateEvent " << _name << "\n" );
    _handle = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( !_handle )  throw_mswin( "CreateEvent", _name.c_str() );
}

//--------------------------------------------------------------------------------Event::set_signal
/*
void Event::set_signal()
{
    Z_MUTEX( _mutex )
    {
        Z_LOG( "Event(" << _name << "," << _signal_name << ").set_signal()\n" );

        _signaled = true;
    }
}
*/
//------------------------------------------------------------------------------------Event::signal

void Event::signal( const string& name )
{
    Z_MUTEX( _mutex )
    {
        _signaled = true;
        _signal_name = name;

        if( current_thread_id() != _waiting_thread_id )
        {
            //Z_LOG2( "event", "Event(" << _name << "," << _signal_name << ").signal()  SetEvent()\n" );

            BOOL ok = SetEvent( _handle );  
            if( !ok )  throw_mswin( "SetEvent", _name.c_str() );
        }
    }
}

//------------------------------------------------------------------------------Event::async_signal
// Keinen System-Aufruf machen! (Außer SetEvent)

void Event::async_signal( const char* )
{
    _signaled = true;
    SetEvent( _handle );  
}

//------------------------------------------------------------------------Event::signaled_then_reset

bool Event::signaled_then_reset()
{
    if( !_signaled )  return false;
    
    bool signaled = false;

    Z_MUTEX( _mutex )
    {
        signaled = _signaled;
        reset();
    }

    return signaled;
}

//-------------------------------------------------------------------------------------Event::reset

void Event::reset()
{
    Z_MUTEX( _mutex )
    {
        //Z_DEBUG_ONLY( Z_LOG( "Event(" << _name << "," << _signal_name << ").reset()  ResetEvent()\n" ); )

        _signaled = false;
        _signal_name = "";

        //Es ist ein Autoreset-Event. Das ist besser, denn dann muss nicht immer wieder ResetEvent() gerufen werden.
        //BOOL ok = ResetEvent( _handle );
        //if( !ok )  throw_mswin( "ResetEvent", _name.c_str() );
    }
}

//--------------------------------------------------------------------------------------Event::wait

bool Event::wait( double wait_time )
{
    if( _signaled ) 
    {
        reset();
        return true;
    }

    if( wait_for_event( _handle, wait_time ) )
    {
        _signaled = true;
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif

