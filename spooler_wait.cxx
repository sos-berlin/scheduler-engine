// $Id: spooler_wait.cxx,v 1.14 2001/02/16 18:23:12 jz Exp $
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

//-----------------------------------------------------------------------------windows_message_step

void windows_message_step()
{
    MSG msg;
    if( GetMessage( &msg, NULL, 0, 0 ) != -1 )   TranslateMessage( &msg ), DispatchMessage( &msg ); 
}

//-----------------------------------------------------------------------------------wait_for_event

bool wait_for_event( HANDLE handle, double wait_time )
{
    while( wait_time > 0 )
    {
        const double max_t = (double)(INT_MAX-10) / 1000.0;
        int ms = min( max_t, wait_time ) * 1000.0;

        int ret = MsgWaitForMultipleObjects( 1, &handle, FALSE, ms, QS_ALLINPUT ); 

        if( ret == WAIT_OBJECT_0 )  return true;
        
        if( ret == WAIT_OBJECT_0 + 1 )  windows_message_step();
        else
        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForSingleObject" );

        wait_time -= max_t;
    }

    return false;
}

//-------------------------------------------------------------------------------------Event::Event

Event::Event( const string& name )  
: 
    _zero_(this+1),
    _name(name) 
{
    _handle = CreateEvent( NULL, FALSE, FALSE, NULL );
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
    _wait_handles.clear();
    
    if( _handle )  CloseHandle( _handle ), _handle = NULL;
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
/*
    THREAD_LOCK( _lock )
    {
        bool signaled = _signaled;
        _signaled = false;
        if( signaled )  return true;

        _waiting = true;
    }
*/
    bool result = wait_for_event( handle(), wait_time );

//  _waiting = false;

    return result;
}

//------------------------------------------------------------------------------------Event::signal

void Event::signal( const string& name )
{
    THREAD_LOCK( _lock )
    {
        _signal_name = name;
//      if( _waiting )  
            { SetEvent( _handle );  return; }
/*
        if( !_signaled )
        {
            FOR_EACH( vector<Wait_handles*>, _wait_handles, it )
            {
                Wait_handles* w = *it;
                
                THREAD_LOCK( w->_lock )
                {
                    _signaled = true;
                    if( w->_waiting )  { SetEvent( _handle );  return; }
                }
            }
        }
*/
    }
}

//--------------------------------------------------------------------------------Event::set_signal

void Event::set_signal()
{
    THREAD_LOCK( _lock )
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

    THREAD_LOCK( _lock )
    {
        signaled = _signaled;
        _signaled = false;
    }

    return signaled;
}

//---------------------------------------------------------------------------------Event::as_string

string Event::as_string() const
{ 
    string result = "Ereignis " + _name; 
    if( !_signal_name.empty() )  result += " (" + _signal_name + ")";
    return result;
}

//----------------------------------------------------------------------Wait_handles::~Wait_handles

Wait_handles::~Wait_handles()
{
    close();
}

//------------------------------------------------------------------------------Wait_handles::close

void Wait_handles::close()
{
    FOR_EACH( vector<Event*>, _events, it )  _log->warn( "Wait_handles wird vor " + (*it)->as_string() + " geschlossen" );
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

    vector<Event*>::iterator it = _events.begin();

    while( it != _events.end() )
    {
        if( *it == event )  break;
        it++;
    }

    if( it == _events.end() ) {
        _log->error( "Wait_handles::remove(" + event->as_string() + ") fehlt" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
        return;
    }

    _events.erase( it );
    _handles.erase( _handles.begin() + ( it - _events.begin() ) );
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
/*
    THREAD_LOCK( _lock )
    {
        FOR_EACH( vector<Event*>, _events, it)  if( (*it)->signaled() ) { _log->msg( "Ereignis " + (*it)->name() ); return; }
        _waiting = true;
    }
*/
    bool again = false;

    while(1)
    {
        double wait_time = until - Time::now();
        int    sleep_time_ms = INT_MAX;
        int    t = ceil( min( (double)sleep_time_ms, wait_time * 1000.0 ) );

        if( t <= 0 )  break;
        if( again )  _log->msg( "Noch " + sos::as_string(wait_time) + "s warten ..." );

        //_log->msg( "WaitForMultipleObjects " + as_string(t) + "ms" );
        DWORD ret = MsgWaitForMultipleObjects( _handles.size(), &_handles[0], FALSE, t, QS_ALLINPUT ); 

        if( ret == WAIT_FAILED )  throw_mswin_error( "WaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )
        {
            //_waiting = false;

            //while(1)
            {
                Event* event = _events[ ret - WAIT_OBJECT_0 ];
                event->set_signal();
                _log->msg( "Ereignis " + event->name() );
              //DWORD ret = WaitForMultipleObjects( _handles.size(), &_handles[0], FALSE, 0 ); 
              //if( ret == WAIT_TIMEOUT )  break;
              //if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )  continue;
              //throw_mswin_error( "WaitForMultipleObjects" );
            }
            break;
        }
        else
        if( ret == WAIT_OBJECT_0 + _handles.size() )
        {
            windows_message_step();
        }
        else
        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForMultipleObjects" );
        else
            again = true;
    }

}

#endif

//--------------------------------------------------------------------------Wait_handles::as_string

string Wait_handles::as_string() const
{
    string result;

    if( _events.empty() )
    {
        return "nichts";
    }
    else
    {
        FOR_EACH_CONST( vector<Event*>, _events, it )  
        {
            if( !result.empty() )  result += ", ";
            result += (*it)->as_string();
        }

        return "{" + result + "}";
    }
}

//---------------------------------------------------------------Directory_watcher::watch_directory

void Directory_watcher::watch_directory( const string& directory )
{
    close();

    _handle = FindFirstChangeNotification( directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME );
    if( _handle == INVALID_HANDLE_VALUE )  _handle = NULL, throw_mswin_error( "FindFirstChangeNotification" );
}

//-------------------------------------------------------------------Directory_watcher::watch_again

void Directory_watcher::watch_again()
{
    reset();

    BOOL ok = FindNextChangeNotification( _handle );
    if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos




