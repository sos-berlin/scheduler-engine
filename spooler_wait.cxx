// $Id: spooler_wait.cxx,v 1.6 2001/01/22 13:42:08 jz Exp $
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


//---------------------------------------------------------------Directory_watcher::watch_directory

void Directory_watcher::watch_directory( const string& directory )
{
    if( _handle )  CloseHandle( _handle );

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

//--------------------------------------------------------------------------Directory_watcher::close

void Directory_watcher::close()
{ 
    if( _handle ) 
    {
        CloseHandle( _handle );  
        _handle = NULL; 
    }
}

//--------------------------------------------------------------------------------Wait_handles::add

void Wait_handles::add( HANDLE handle, const string& name, Task* task )
{
    _handles.push_back( handle );
    _entries.push_back( Entry( name, task ) );
}

//------------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( HANDLE handle )
{
    if( !handle )  return;

    vector<HANDLE>::iterator it = _handles.begin();

    while( it != _handles.end() )
    {
        if( *it == handle )  break;
        it++;
    }

    if( it == _handles.end() ) {
        LOG( "*** Wait_handles::remove " << handle << " fehlt\n" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
        return;
    }

    _handles.erase( it );
    _entries.erase( _entries.begin() + ( it - _handles.begin() ) );
}

//--------------------------------------------------------------------------------Wait_handles::wait
#ifdef SYSTEM_WIN

void Wait_handles::wait( double wait_time )
{
    while( wait_time > 0 )
    {
        int sleep_time_ms = INT_MAX;
        
        int t = min( (double)sleep_time_ms, wait_time * 1000.0 );

        DWORD ret = WaitForMultipleObjects( _handles.size(), &_handles[0], FALSE, t ); 

        if( ret == WAIT_FAILED )  throw_mswin_error( "WaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )
        {
            int    index = ret - WAIT_OBJECT_0;
            Entry* entry = &_entries[ index ];
            Task*  task = entry->_task;
            string msg = "Ereignis " + as_string(index) + " - " + entry->_event_name;
            
            if( task ) 
            {
                task->_log.msg( msg );
                if( _handles[ index ] == task->_directory_watcher._handle )  task->_directory_watcher._signaled = true;
                                                                       else  throw_xc( "Wait_handles::wait", entry->_event_name );
            }
            else
                _spooler->_log.msg( msg );

            return;
        }

        if( ret != WAIT_TIMEOUT )  throw_mswin_error( "WaitForMultipleObjects" );

        wait_time -= sleep_time_ms / 1000;
    }
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos




