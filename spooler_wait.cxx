// $Id: spooler_wait.cxx,v 1.53 2002/12/02 17:19:36 jz Exp $
/*
    Hier sind implementiert

    Directory_watcher
    Wait_handles
*/


#include "spooler.h"

#include <sys/types.h>
#include <sys/timeb.h>

#ifdef Z_WINDOWS
#   include <io.h>         // findfirst()
#endif

#include "../kram/sleep.h"
#include "../kram/log.h"

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------windows_message_step
#ifdef Z_WINDOWS

void windows_message_step()
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

#endif
//-----------------------------------------------------------------------------------wait_for_event
#ifdef Z_WINDOWS
/*
bool wait_for_event( HANDLE handle, double wait_time )
{
    while( wait_time > 0 )
    {
        const double max_t = (double)(INT_MAX-10) / 1000.0;
        int ms = min( max_t, wait_time ) * 1000.0;

        LOG( "MsgWaitForMultipleObjects()\n" );
        int ret = MsgWaitForMultipleObjects( 1, &handle, FALSE, ms, QS_ALLINPUT ); 

        if( ret == WAIT_OBJECT_0 )  return true;
        
        if( ret == WAIT_OBJECT_0 + 1 )  windows_message_step();
        else
        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForSingleObject" );

        wait_time -= max_t;
    }

    return false;
}
*/
#endif
//-------------------------------------------------------------------------------------Event::Event
/*
Event::Event( const string& name )  
: 
    _zero_(this+1),
    _name(name) 
{
#ifdef Z_WINDOWS
    _handle = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( !_handle )  throw_mswin_error( "CreateEvent", name.c_str() );
#endif
}
*/
//------------------------------------------------------------------------------------Event::~Event
/*
Event::~Event()
{
    close();
}
*/
//-------------------------------------------------------------------------------------Event::close

void Event::close()
{
    FOR_EACH( vector<Wait_handles*>, _wait_handles, it )  (*it)->remove( this );
    _wait_handles.clear();

    //Warum ruft das Event::close()?:  Base_class::close();
    close_handle();
}

//------------------------------------------------------------------------------Event::close_handle
/*
void Event::close_handle()
{
#ifdef Z_WINDOWS
    if( _handle )  CloseHandle( _handle ), _handle = NULL;
#endif
}
*/
//------------------------------------------------------------------------------------Event::add_to

void Event::add_to( Wait_handles* w )                 
{ 
    Z_LOCK( _mutex )
    {
        _wait_handles.push_back(w); 
        w->add( this );
    }
}

//-------------------------------------------------------------------------------Event::remove_from

void Event::remove_from( Wait_handles* w )
{
    Z_LOCK( _mutex )
    {
        FOR_EACH( vector<Wait_handles*>, _wait_handles, it ) 
        {
            if( *it == w )  _wait_handles.erase( it );
        }

        w->remove( this );
    }
}

//--------------------------------------------------------------------------------------Event::wait
#ifdef Z_WINDOWS
/*
bool Event::wait( double wait_time )
{
    bool result = wait_for_event( handle(), wait_time );

    return result;
}
*/
#endif
//------------------------------------------------------------------------------------Event::signal
/*
void Event::signal( const string& name )
{
    THREAD_LOCK( _lock )
    {
        _signaled = true;
        _signal_name = name;

#       ifdef Z_WINDOWS
            SetEvent( _handle );  
#       endif
    }
}
*/
//------------------------------------------------------------------------------------Event::signal
/*
void Event::async_signal( const string& name )
{
#   ifdef Z_WINDOWS

        signal( name );

#   else

        // pthread_mutex_lock:
        // The  mutex  functions  are  not  async-signal  safe.  What  this  means  is  that  they
        // should  not  be  called from  a signal handler. In particular, calling pthread_mutex_lock 
        // or pthread_mutex_unlock from a signal handler may deadlock the calling thread.
        _signaled = true;

#   endif
}
*/
//--------------------------------------------------------------------------------Event::set_signal
/*
void Event::set_signal()
{
    LOGI( "Event(" << _name << "," << _signal_name << ").set_signal()\n" );

    THREAD_LOCK( _lock )
    {
        _signaled = true;
    }
}
*/
//-------------------------------------------------------------------------------------Event::reset
/*
void Event::reset()
{
    THREAD_LOCK( _lock )
    {
        _signaled = false;
        _signal_name = "";

#       ifdef Z_WINDOWS
            ResetEvent( _handle );
#       endif
    }
}
*/
//------------------------------------------------------------------------Event::signaled_then_reset
/*
bool Event::signaled_then_reset()
{
    if( !_signaled )  return false;
    
    bool signaled = false;

    THREAD_LOCK( _lock )
    {
        signaled = _signaled;
        reset();
    }

    return signaled;
}
*/
//---------------------------------------------------------------------------------Event::as_string
/*
string Event::as_string() const
{ 
    string result = "Ereignis " + _name; 
    if( !_signal_name.empty() )  result += " \"" + _signal_name + "\"";
    return result;
}
*/
//-----------------------------------------------------------------------Wait_handles::Wait_handles

Wait_handles::Wait_handles( const Wait_handles& o )
: 
    _spooler ( o._spooler ),
    _log     ( o._log ),
#ifdef Z_WINDOWS
    _handles ( o._handles ),
#endif
    _events  ( o._events ) 
{
}

//----------------------------------------------------------------------Wait_handles::~Wait_handles

Wait_handles::~Wait_handles()
{
    close();
}

//------------------------------------------------------------------------------Wait_handles::close

void Wait_handles::close()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Event_vector, _events, it )  
        {
            if( *it )  _log->warn( "Wait_handles wird vor " + (*it)->as_text() + " geschlossen" );
        }
    }
}

//------------------------------------------------------------------------------Wait_handles::clear

void Wait_handles::clear()
{
    THREAD_LOCK( _lock )
    {
        _events.clear();

#       ifdef Z_WINDOWS
            _handles.clear();
#       endif
    }
}

//---------------------------------------------------------------------------Wait_handles::signaled

bool Wait_handles::signaled()
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( Event_vector, _events, it )  
        {
            if( *it  &&  (*it)->signaled() )  return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------Wait_handles::operator += 

Wait_handles& Wait_handles::operator += ( Wait_handles& o )
{
    THREAD_LOCK( _lock )
    THREAD_LOCK( o._lock )      // Vorsicht, Deadlock-Gefahr!
    {
        //_events.reserve( _events.size() + o._events.size() );
        
        Event_vector::iterator   e = o._events.begin();
        vector<HANDLE>::iterator h = o._handles.begin();

        while( e != o._events.end() )
        {
            Event_vector::iterator e2 = _events.begin();
            while( e2 != _events.end()  &&  *e2 != *e )  e2++;
            if( e2 == _events.end() )
            {
                _events.push_back( *e );                             // Nur verschiedene hinzufügen
                Z_WINDOWS_ONLY( _handles.push_back( *h ) );
            }

            e++;
            Z_WINDOWS_ONLY( h++ );
        }
    }

    return *this;
}

//--------------------------------------------------------------------------------Wait_handles::add

void Wait_handles::add( z::Event* event )
{
    THREAD_LOCK( _lock )
    {
#       ifdef Z_WINDOWS
            _handles.push_back( event->handle() );
#       endif

        _events.push_back( event );
    }
}

//-------------------------------------------------------------------------Wait_handles::add_handle
#ifdef Z_WINDOWS
/*
void Wait_handles::add_handle( HANDLE handle )
{
    THREAD_LOCK( _lock )
    {
        _handles.push_back( handle );
        _events.push_back( NULL );
    }
}
*/
#endif
//-----------------------------------------------------------------------Wait_handles::remove_handle
#ifdef Z_WINDOWS

void Wait_handles::remove_handle( HANDLE handle, z::Event* event )
{
    THREAD_LOCK( _lock )
    {
        vector<HANDLE>::iterator it = _handles.begin();

        while( it != _handles.end() )
        {
            if( *it == handle )  break;
            it++;
        }

        if( it == _handles.end() ) {
            if( event )  _log->error( "Wait_handles::remove(" + event->as_text() + ") fehlt" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
                   else  _log->error( "Wait_handles::remove() fehlt" );
            return;
        }

        _events.erase( _events.begin() + ( it - _handles.begin() ) );
        _handles.erase( it );
    }
}

#endif
//-----------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( z::Event* event )
{
    if( !event )  return;

#   ifdef Z_WINDOWS

        remove_handle( event->handle(), event );

#   else

        // Das ist fast der gleiche Code wie von remove_handle(). Kann man das zusammenfassen? 26.11.2002

        THREAD_LOCK( _lock )
        {
            Event_vector::iterator it = _events.begin();

            while( it != _events.end() )
            {
                if( *it == event )  break;
                it++;
            }

            if( it == _events.end() ) {
                _log->error( "Wait_handles::remove(" + event->as_text() + ") fehlt" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
                return;
            }

            _events.erase( it );
            //_handles.erase( _handles.begin() + ( it - _handles.begin() )  );
        }

#   endif
}

//-------------------------------------------------------------------------------Wait_handles::wait
#ifdef Z_WINDOWS

int Wait_handles::wait( double wait_time )
{
    return wait_until( Time::now() + wait_time );
}

//-------------------------------------------------------------------------Wait_handles::wait_until

int Wait_handles::wait_until( Time until )
{
    timeb tm1, tm2;
    ftime( &tm1 );

    while(1)
    {
        Time now       = Time::now();
        Time today2    = now.midnight() + 2*3600;           // Heute 2:00 Uhr (für Sommerzeitbeginn: Uhr springt von 2 Uhr auf 3 Uhr)
        Time tomorrow2 = now.midnight() + 2*3600 + 24*3600;
        Time today3    = now.midnight() + 3*3600;           // Heute 3:00 Uhr (für Winterzeitbeginn: Uhr springt von 3 Uhr auf 2 Uhr)
        int  ret       = -1;

        if( now < today2  &&  until >= today2 )     ret = wait_until_2( today2 + 0.01 );
        else
        if( now < today3  &&  until >= today3 )     ret = wait_until_2( today3 + 0.01 );
        else 
        if( until >= tomorrow2 )                    ret = wait_until_2( tomorrow2 + 0.01 );
        else
            break;

        if( ret != -1 )  return ret;

        ftime( &tm2 );
        if( tm1.dstflag != tm2.dstflag )  _log->info( tm2.dstflag? "Sommerzeit" : "Winterzeit" );
                                    else  Z_DEBUG_ONLY( _log->debug9( "Keine Sommerzeitumschaltung" ) );
    }

    return wait_until_2(  until );
}

//-----------------------------------------------------------------------Wait_handles::wait_until_2
// Liefert Nummer des Events (0..n-1) oder -1 bei Zeitablauf

int Wait_handles::wait_until_2( Time until )
{
    bool    again   = false;
    HANDLE* handles = NULL;

    while(1)
    {
        double wait_time = until - Time::now();
        if( wait_time <= 0 )  break;
        int    sleep_time_ms = INT_MAX;
        int    t = ceil( min( (double)sleep_time_ms, wait_time * 1000.0 ) );

        if( t <= 0 )  break;
        if( again ) {
            if( t > 1800 )  return -1;  // Um eine halbe Stunde verrechnet? Das muss an der Sommerzeitumstellung liegen
            _log->info( "Noch " + sos::as_string(wait_time) + "s warten ..." );
        }

        THREAD_LOCK( _lock )
        {
            if( _log->log_level() <= log_debug9 )
            {
                string msg = "MsgWaitForMultipleObjects " + sos::as_string(t/1000.0) + "s (bis " + until.as_string() + ")  ";
                for( int i = 0; i < _handles.size(); i++ )
                {
                    if( i > 0 )  msg += ", ";
                    if( _events[i] )  msg += _events[i]->as_text() + " (0x" + as_hex_string( (int)_handles[i] ) + ")";
                               else   msg += "NULL";
                }
                _log->debug9( msg );
            }

            handles = new HANDLE [ _handles.size()+1 ];
            for( int i = 0; i < _handles.size(); i++ )  handles[i] = _handles[i];
        }

        DWORD ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, t, QS_ALLINPUT ); 
        
        delete [] handles;  handles = NULL;

        if( ret == WAIT_FAILED )  throw_mswin_error( "MsgWaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )
        {
            THREAD_LOCK( _lock )
            {
                int    index = ret - WAIT_OBJECT_0;
                z::Event* event = _events[ index ];
            
                if( event )
                {
                    event->set_signal();
                    if( _spooler->_debug )  _log->debug( event->as_text() );
                }

                return index;
            }
        }
        else
        if( ret == WAIT_OBJECT_0 + _handles.size() )
        {
            windows_message_step();
        }
        else
        if( ret == WAIT_TIMEOUT )  
            again = true;
        else
            throw_mswin_error( "MsgWaitForMultipleObjects" );
    }

    return -1;
}

#endif
//--------------------------------------------------------------------------Wait_handles::as_string

string Wait_handles::as_string() 
{
    string result;

    THREAD_LOCK( _lock )
    {
        if( _events.empty() )
        {
            result = "nichts";
        }
        else
        {
            FOR_EACH_CONST( Event_vector, _events, it )  
            {
                if( !result.empty() )  result += ", ";
                result += (*it)->as_text();
            }

            result = "{" + result + "}";
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Directory_reader
#ifdef Z_WINDOWS

struct Directory_reader
{
    Directory_reader() : _handle(-1) {}
    
    ~Directory_reader()
    { 
        close(); 
    }
    
    void close() 
    { 
        if( _handle != -1 )  _findclose( _handle ),  _handle = -1; 
    }

    string first( const string& dirname ) 
    { 
        string pattern = dirname + "/*";
        _finddata_t data;
        _handle = _findfirst( pattern.c_str(), &data ); 
        if( _handle == -1 )  throw_errno( errno, "_findfirst", dirname.c_str() );  
        return data.name; 
    }

    string next() 
    { 
        _finddata_t data;
        int ret = _findnext( _handle, &data ); 
        if( ret == -1 )  
        {
            if( errno == ENOENT )  return "";
            throw_errno( errno, "_findnext" ); 
        }

        return data.name; 
    }

    int  _handle;
};

//------------------------------------------------------------------Directory_watcher::close_handle

void Directory_watcher::close_handle()
{
    if( _handle )
    {
        FindCloseChangeNotification( _handle );
        _handle = NULL;
    }
}

//---------------------------------------------------------------Directory_watcher::watch_directory

void Directory_watcher::watch_directory( const string& directory, const string& filename_pattern )
{
    close();

    _directory = directory;
    _filename_pattern = filename_pattern;

    if( !filename_pattern.empty() )  _filename_regex.compile( filename_pattern );

    _handle = FindFirstChangeNotification( directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
    if( !_handle  ||  _handle == INVALID_HANDLE_VALUE )  _handle = NULL, throw_mswin_error( "FindFirstChangeNotification" );
}


//-------------------------------------------------------------------------Directory_watcher::match

bool Directory_watcher::match()
{
    Directory_reader dir;

    string filename = dir.first( _directory );

    while( !filename.empty() )
    {
        if( filename != "."  &&  filename != ".." )
        {
            if( _filename_regex.match( filename ) )  return true;
        }

        filename = dir.next();
    }

    return false;
}

//--------------------------------------------------------------------Directory_watcher::set_signal

void Directory_watcher::set_signal()
{
    try
    {
        if( _filename_pattern.empty()  ||  match() )  Event::set_signal();

        BOOL ok = FindNextChangeNotification( _handle );
        if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
    }
    catch( const Xc& x ) 
    {
        _log->error( "Überwachung des Verzeichnisses " + _directory + " wird nach Fehler beendet: " + x.what() ); 
        _directory = "";   // Damit erneutes start_when_directory_changed() diese (tote) Überwachung nicht erkennt.
        Event::set_signal();
        close();
    }
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
