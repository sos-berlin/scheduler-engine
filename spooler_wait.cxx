// $Id: spooler_wait.cxx,v 1.10 2001/02/06 09:22:26 jz Exp $
/*
    Hier sind implementiert

    Directory_watcher
    Wait_handles
*/


#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/sleep.h"
#include "../kram/log.h"


namespace sos {
namespace spooler {


//-----------------------------------------------------------------------------------wait_for_event

bool wait_for_event( HANDLE handle, double wait_time )
{
    while( wait_time > 0 )
    {
        const double max_t = (double)(INT_MAX-10) / 1000.0;
        int ms = min( max_t, wait_time ) * 1000.0;

        int ret = WaitForSingleObject( handle, ms ); 

        if( ret == WAIT_OBJECT_0 )  return true;
        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForSingleObject" );

        wait_time -= max_t;
    }

    return false;
}

//-------------------------------------------------------------------------------------Event::Event

Event::Event( const string& name )  
: 
    Handle(NULL), 
    _zero_(this+1),
    _name(name) 
{
    _handle = CreateEvent( NULL, FALSE, FALSE, name.c_str() );
    if( !_handle )  throw_mswin_error( "CreateEvent", name.c_str() );
}

//------------------------------------------------------------------------------------Event::~Event

Event::~Event()
{
    close();
}

//-------------------------------------------------------------------------------------Event::close

void Event::close()
{
    FOR_EACH( vector<Wait_handles*>, _wait_handles, it )  (*it)->remove( this );
}

//-------------------------------------------------------------------------------------Event::add_to

void Event::add_to( Wait_handles* w )                 
{ 
    _wait_handles.push_back(w); 
    w->add( this );
}

//-------------------------------------------------------------------------------Event::remove_from

void Event::remove_from( Wait_handles* w )
{
    FOR_EACH( vector<Wait_handles*>, _wait_handles, it ) 
    {
        if( *it == w )  _wait_handles.erase( it );
    }

    w->remove( this );
}

//--------------------------------------------------------------------------------------Event::wait

bool Event::wait( double wait_time )
{
    THREAD_SEMA( _lock )
    {
        bool signaled = _signaled;
        _signaled = false;
        if( signaled )  return true;

        _waiting = true;
    }

    bool result = wait_for_event( handle(), wait_time );

    _waiting = false;

    return result;
}

//------------------------------------------------------------------------------------Event::signal

void Event::signal()
{
    THREAD_SEMA( _lock )
    {
        if( _waiting )  { SetEvent( _handle );  return; }

        if( !_signaled )
        {
            FOR_EACH( vector<Wait_handles*>, _wait_handles, it )
            {
                Wait_handles* w = *it;
                
                THREAD_SEMA( w->_lock )
                {
                    _signaled = true;
                    if( w->_waiting )  { SetEvent( _handle );  return; }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------Event::set_signal

void Event::set_signal()
{
    THREAD_SEMA( _lock )
    {
        _signaled = true;
    }

    //if( signaled )  signal_event();
}

//------------------------------------------------------------------------Event::signaled_then_reset

bool Event::signaled_then_reset()
{
    if( !_signaled )  return false;
    
    bool signaled = false;

    THREAD_SEMA( _lock )
    {
        signaled = _signaled;
        _signaled = false;
    }

    return signaled;
}

//--------------------------------------------------------------------------------Wait_handles::add

void Wait_handles::add( Event* event )
{
    _handles.push_back( event->handle() );
    _events.push_back( event );
}

//-----------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( Event* event )
{
    if( !event )  return;

    vector<HANDLE>::iterator it = _handles.begin();

    while( it != _handles.end() )
    {
        if( *it == event )  break;
        it++;
    }

    if( it == _handles.end() ) {
        LOG( "*** Wait_handles::remove " << event << " fehlt\n" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
        return;
    }

    _handles.erase( it );
    _events.erase( _events.begin() + ( it - _handles.begin() ) );
}

//--------------------------------------------------------------------------Wait_handles::wait_until

void Wait_handles::wait( double wait_time )
{
    wait_until( Time::now() + wait_time );
}

//--------------------------------------------------------------------------------Wait_handles::wait
#ifdef SYSTEM_WIN

void Wait_handles::wait_until( Time until )
{
    THREAD_SEMA( _lock )
    {
        FOR_EACH( vector<Event*>, _events, it)  if( (*it)->signaled() ) return;
        _waiting = true;
    }

    bool again = false;

    while(1)
    {
        double wait_time = until - Time::now();
        int    sleep_time_ms = INT_MAX;
        int    t = ceil( min( (double)sleep_time_ms, wait_time * 1000.0 ) );

        if( t <= 0 )  break;
        if( again )  _log->msg( "Noch " + as_string(wait_time) + "s warten ..." );
        again = true;

        //_log->msg( "WaitForMultipleObjects " + as_string(t) + "ms" );
        DWORD ret = WaitForMultipleObjects( _handles.size(), &_handles[0], FALSE, t ); 

        if( ret == WAIT_FAILED )  throw_mswin_error( "WaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )
        {
            Event* event = _events[ ret - WAIT_OBJECT_0 ];
            event->set_signal();
            _log->msg( "Ereignis " + event->name() );
            return;
        }

        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForMultipleObjects" );
    }

    _waiting = false;
}

#endif
//---------------------------------------------------------------Directory_watcher::watch_directory

void Directory_watcher::watch_directory( const string& directory )
{
    close();

    set_name( "start_when_directory_changed(\"" + directory + "\")" );

    _handle = FindFirstChangeNotification( directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME );
    if( _handle == INVALID_HANDLE_VALUE )  _handle = NULL, throw_mswin_error( "FindFirstChangeNotification" );
}

//-------------------------------------------------------------------Directory_watcher::watch_again

void Directory_watcher::watch_again()
{
    _signaled = false;

    BOOL ok = FindNextChangeNotification( _handle );
    if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos




