// $Id: spooler_wait.cxx,v 1.34 2002/06/18 07:35:46 jz Exp $
/*
    Hier sind implementiert

    Directory_watcher
    Wait_handles
*/


#include "../kram/sos.h"
#include "spooler.h"

#include <io.h>         // findfirst()
#include <sys/types.h>
#include <sys/timeb.h>
#include "../kram/sleep.h"
#include "../kram/log.h"


namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------windows_message_step

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

//-----------------------------------------------------------------------------------wait_for_event

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

    close_handle();
}

//------------------------------------------------------------------------------Event::close_handle

void Event::close_handle()
{
    if( _handle )  CloseHandle( _handle ), _handle = NULL;
}

//------------------------------------------------------------------------------------Event::add_to

void Event::add_to( Wait_handles* w )                 
{ 
    THREAD_LOCK( _lock )
    {
        _wait_handles.push_back(w); 
        w->add( this );
    }
}

//-------------------------------------------------------------------------------Event::remove_from

void Event::remove_from( Wait_handles* w )
{
    THREAD_LOCK( _lock )
    {
        FOR_EACH( vector<Wait_handles*>, _wait_handles, it ) 
        {
            if( *it == w )  _wait_handles.erase( it );
        }

        w->remove( this );
    }
}

//--------------------------------------------------------------------------------------Event::wait

bool Event::wait( double wait_time )
{
    bool result = wait_for_event( handle(), wait_time );

    return result;
}

//------------------------------------------------------------------------------------Event::signal

void Event::signal( const string& name )
{
    THREAD_LOCK( _lock )
    {
        _signaled = true;
        _signal_name = name;
        SetEvent( _handle );  
    }
}

//--------------------------------------------------------------------------------Event::set_signal

void Event::set_signal()
{
    LOGI( "Event(" << _name << "," << _signal_name << ").set_signal()\n" );

    THREAD_LOCK( _lock )
    {
        _signaled = true;
    }
}

//-------------------------------------------------------------------------------------Event::reset

void Event::reset()
{
    THREAD_LOCK( _lock )
    {
        _signaled = false;
        _signal_name = "";
        ResetEvent( _handle );
    }
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
        if( signaled )  ResetEvent( _handle );
    }

    return signaled;
}

//---------------------------------------------------------------------------------Event::as_string

string Event::as_string() const
{ 
    string result = "Ereignis " + _name; 
    if( !_signal_name.empty() )  result += " \"" + _signal_name + "\"";
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
    THREAD_LOCK( _lock )
    {
        FOR_EACH( vector<Event*>, _events, it )  
        {
            if( *it )  _log->warn( "Wait_handles wird vor " + (*it)->as_string() + " geschlossen" );
        }
    }
}

//--------------------------------------------------------------------------------Wait_handles::add

void Wait_handles::add( Event* event )
{
    THREAD_LOCK( _lock )
    {
        _handles.push_back( event->handle() );
        _events.push_back( event );
    }
}

//-------------------------------------------------------------------------Wait_handles::add_handle

void Wait_handles::add_handle( HANDLE handle )
{
    THREAD_LOCK( _lock )
    {
        _handles.push_back( handle );
        _events.push_back( NULL );
    }
}

//-----------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( Event* event )
{
    if( !event )  return;

    remove_handle( event->handle(), event );
}

//-----------------------------------------------------------------------Wait_handles::remove_handle

void Wait_handles::remove_handle( HANDLE handle, Event* event )
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
            if( event )  _log->error( "Wait_handles::remove(" + event->as_string() + ") fehlt" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
                   else  _log->error( "Wait_handles::remove() fehlt" );
            return;
        }

        _events.erase( _events.begin() + ( it - _handles.begin() ) );
        _handles.erase( it );
    }
}

//-------------------------------------------------------------------------------Wait_handles::wait

int Wait_handles::wait( double wait_time )
{
    return wait_until( Time::now() + wait_time );
}

//-------------------------------------------------------------------------Wait_handles::wait_until

int Wait_handles::wait_until( Time until )
{
    bool    again   = false;
    HANDLE* handles = NULL;

    while(1)
    {
        double wait_time = until - Time::now();
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
                    if( _events[i] )  msg += _events[i]->as_string() + " (0x" + as_hex_string( (int)_handles[i] ) + ")";
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
                Event* event = _events[ index ];
            
                if( event )
                {
                    event->set_signal();
                    if( _spooler->_debug )  _log->debug( event->as_string() );
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
            FOR_EACH_CONST( vector<Event*>, _events, it )  
            {
                if( !result.empty() )  result += ", ";
                result += (*it)->as_string();
            }

            result = "{" + result + "}";
        }
    }

    return result;
}

//---------------------------------------------------------------------------------Directory_reader

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
    if( _handle == INVALID_HANDLE_VALUE )  _handle = NULL, throw_mswin_error( "FindFirstChangeNotification" );
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

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos




