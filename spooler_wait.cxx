// $Id$
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
#else
#   include <sys/time.h>
#   include <dirent.h>
#endif

#include "../kram/sleep.h"
#include "../kram/log.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

#ifndef Z_WINDOWS
    const double directory_watcher_interval = 0.5;      // Wartezeit in Sekunden zwischen zwei Verzeichnisüberprüfungen
#endif

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
//-------------------------------------------------------------------------------------Event::close

void Event::close()
{
    FOR_EACH( vector<Wait_handles*>, _wait_handles, it )  (*it)->remove( this );
    _wait_handles.clear();

#   ifdef Z_WINDOWS
        close_handle();     //Warum ruft das Event::close()?:  Base_class::close();
#   else
        Base_class::close();
#   endif
}

//------------------------------------------------------------------------------------Event::add_to

void Event::add_to( Wait_handles* w )                 
{ 
    Z_MUTEX( _mutex )
    {
        _wait_handles.push_back(w); 
        w->add( this );
    }
}

//-------------------------------------------------------------------------------Event::remove_from

void Event::remove_from( Wait_handles* w )
{
    Z_MUTEX( _mutex )
    {
        FOR_EACH( vector<Wait_handles*>, _wait_handles, it ) 
        {
            if( *it == w )  _wait_handles.erase( it );
        }

        w->remove( this );
    }
}

//-----------------------------------------------------------------------Wait_handles::Wait_handles

Wait_handles::Wait_handles( const Wait_handles& o )
: 
    _spooler ( o._spooler ),
    _log     ( o._log ),
    _lock    ( "Wait_handles" ),
    _events  ( o._events )
#ifdef Z_WINDOWS
   ,_handles ( o._handles )
#endif
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
    //THREAD_LOCK( _lock )
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
    //THREAD_LOCK( _lock )
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
    //THREAD_LOCK( _lock )
    {
        FOR_EACH( Event_vector, _events, it )  
        {
            if( *it  &&  (*it)->signaled() )  { Z_LOG2( "scheduler.wait", **it << " signaled!\n" );  return true; }
        }
    }

    return false;
}

//------------------------------------------------------------------------Wait_handles::operator += 

Wait_handles& Wait_handles::operator += ( Wait_handles& o )
{
    //THREAD_LOCK( _lock )
    //THREAD_LOCK( o._lock )      // Vorsicht, Deadlock-Gefahr!
    {
        Event_vector::iterator   e = o._events.begin();
        Z_WINDOWS_ONLY( vector<HANDLE>::iterator h = o._handles.begin(); )

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

void Wait_handles::add( System_event* event )
{
    //THREAD_LOCK( _lock )
    {
        Z_LOG2( "joacim", __FUNCTION__ << *event << "\n" );
#       ifdef Z_WINDOWS
            _handles.push_back( event->handle() );
#       endif

        _events.push_back( event );
    }
}

//--------------------------------------------------------------------------Wait_handles::add_handle
#ifdef Z_WINDOWS

void Wait_handles::add_handle( HANDLE handle ) //, System_event* event )
{
    //THREAD_LOCK( _lock )
    {
        // Nützt nicht viel, denn spooler.exe fügt mehrere Wait_handles zusammen (mit operator +=).
        if( _handles.size() >= MAXIMUM_WAIT_OBJECTS-1 )  throw_xc( "SCHEDULER-209", MAXIMUM_WAIT_OBJECTS-1 );   // Grenze für MsgWaitForMultipleObjects() ist 64

        _handles.push_back( handle );
        _events.push_back( NULL );  //event );
    }
}

#endif
//-----------------------------------------------------------------------Wait_handles::remove_handle
#ifdef Z_WINDOWS
/*
void Wait_handles::remove_handle( HANDLE handle, z::Event_base* event )
{
    //THREAD_LOCK( _lock )
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
*/
#endif
//-----------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( System_event* event )
{
    if( !event )  return;

//#   ifdef Z_WINDOWS

        //remove_handle( event->handle(), event );

//#   else

        // Das ist fast der gleiche Code wie von remove_handle(). Kann man das zusammenfassen? 26.11.2002

        //THREAD_LOCK( _lock )
        {
            Event_vector::iterator it = _events.begin();

            while( it != _events.end() )
            {
                if( *it == event )  break;
                it++;
            }

            if( it == _events.end() ) {
                _log->error( "Wait_handles::remove(" + event->as_text() + "): Ereignis ist nicht eingetragen" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
                return;
            }

            _events.erase( it );
            Z_WINDOWS_ONLY( _handles.erase( _handles.begin() + ( it - _events.begin() )  ) );
        }

//#   endif
}

//-------------------------------------------------------------------------------Wait_handles::wait

bool Wait_handles::wait( double wait_time )
{
    return wait_until( Time::now() + wait_time );
}

//-------------------------------------------------------------------------Wait_handles::wait_until

bool Wait_handles::wait_until( Time until )
{
    time_t t;
    tm     tm1, tm2;

    t = ::time(NULL);
    localtime_r( &t, &tm1 );
        

    while(1)
    {
        Time now       = Time::now();
        bool signaled  = false;

        if( tm1.tm_isdst )  // Wir haben Sommerzeit?
        {
            Time today3    = now.midnight() + 3*3600;            // Heute 3:00 Uhr (für Winterzeitbeginn: Uhr springt von 3 Uhr auf 2 Uhr)
            Time tomorrow3 = now.midnight() + 3*3600 + 24*3600;  // Morgen 3:00

            if( now < today3  &&  until >= today3 )    signaled = wait_until_2( today3 + 0.01 );
            else 
            if( until >= tomorrow3 )                   signaled = wait_until_2( tomorrow3 + 0.01 );
            else
                break;
        }
        else                // Wir haben Winterzeit?
        {
            Time today2    = now.midnight() + 2*3600;            // Heute 2:00 Uhr (für Sommerzeitbeginn: Uhr springt von 2 Uhr auf 3 Uhr)
            Time tomorrow2 = now.midnight() + 2*3600 + 24*3600;  // Morgen 3:00

            if( now < today2  &&  until >= today2 )    signaled = wait_until_2( today2 + 0.01 );
            else 
            if( until >= tomorrow2 )                   signaled = wait_until_2( tomorrow2 + 0.01 );
            else
                break;
        }

        if( signaled )  return signaled;

        //ftime( &tm2 );
        t = ::time(NULL);
        localtime_r( &t, &tm2 );
        if( tm1.tm_isdst != tm2.tm_isdst )  _log->info( tm2.tm_isdst? "Sommerzeit" : "Winterzeit" );
                                      else  Z_DEBUG_ONLY( _log->debug9( "Keine Sommerzeitumschaltung" ) );
    }

    return wait_until_2(  until );
}

//-----------------------------------------------------------------------Wait_handles::wait_until_2
// Liefert Nummer des Events (0..n-1) oder -1 bei Zeitablauf

bool Wait_handles::wait_until_2( Time until )
{
    if( signaled() )  return true;

#ifdef Z_WINDOWS

    bool    again   = false;
    HANDLE* handles = NULL;

    while(1)
    {
        double wait_time = until - Time::now();
        int    sleep_time_ms = INT_MAX;
        int    t = (int)ceil( min( (double)sleep_time_ms, wait_time * 1000.0 ) );

        if( t <= 0 )  if( again )  break;
                             else  t = 0;  //break;
        
        if( again ) {
            if( t > 1800 )  return false;  // Um mehr als eine halbe Stunde verrechnet? Das muss an der Sommerzeitumstellung liegen
            if( wait_time >= 1.0 )  _log->debug9( "Noch " + sos::as_string(wait_time) + "s warten ..." );
        }

        //THREAD_LOCK( _lock )
        {
          //if( _log->log_level() <= log_debug9 )
            if( t > 0 )  Z_LOG2( "scheduler.wait", "MsgWaitForMultipleObjects " << sos::as_string(t/1000.0) << "s (bis " << until << ")  " << as_string() << "\n" );

            handles = new HANDLE [ _handles.size()+1 ];
            for( int i = 0; i < _handles.size(); i++ )  handles[i] = _handles[i];
        }


#       ifdef Z_DEBUG
            bool print_time_every_second = _spooler->log_directory() == "*stderr";
#       else
            bool print_time_every_second = false;
#       endif

        DWORD ret;

        if( print_time_every_second )
        {
            double step = 1.0;
            while( Time::now() < until - step )
            {
                ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, (int)( ceil( step * 1000 ) ), QS_ALLINPUT ); 
                if( ret != WAIT_TIMEOUT )  goto WAIT_OK;
                Time now = Time::now();
                t = (int)ceil( ( until - now ) * 1000 );
                cerr << Time::now().as_string( Time::without_ms ) << " (" << ( -t / 1000 ) << "s)   \r";
            }

            ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, max( 0, t ), QS_ALLINPUT ); 
        }
        else
        {
            ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, t, QS_ALLINPUT ); 
        }
        
WAIT_OK:
        delete [] handles;  handles = NULL;

        if( ret == WAIT_FAILED )  throw_mswin_error( "MsgWaitForMultipleObjects" );

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )
        {
            //THREAD_LOCK( _lock )
            {
                int            index = ret - WAIT_OBJECT_0;
                z::Event_base* event = _events[ index ];
            
                if( event )
                {
                    if( t > 0 )  Z_LOG2( "scheduler.wait", "... Event " << event->as_text() << "\n" );
                    event->set_signaled();
                }
                else
                    if( t > 0 )  Z_LOG2( "scheduler.wait", "... Event " << index << "\n" );

                return true; //index;
            }
        }
        else
        if( ret == WAIT_OBJECT_0 + _handles.size() )
        {
            windows_message_step();
        }
        else
        if( ret == WAIT_TIMEOUT )  
        {
            again = true;
        }
        else
            throw_mswin_error( "MsgWaitForMultipleObjects" );
    }

    return false;

#else

    {
        Time now = Time::now();

      //if( _log->log_level() <= log_debug9  &&  until > now )   LOG( "wait_until " << until.as_string() << " " << as_string() << "\n" );
        if( until > now )   Z_LOG2( "scheduler.wait", "wait_until " << until.as_string() << " " << as_string() << "\n" );

        ptr<Socket_wait> wait = _spooler->_connection_manager->create_wait();

        for( int i = _events.size() - 1; i >= 0; i-- )   if( _events[i] )  wait->add( _events[i] );

        wait->set_polling_interval( now.as_time_t() < _spooler->_last_time_enter_pressed + 10.0? 0.1 
                                                                                               : 1.0 );

        int ret = wait->wait( (double)( until - now ) );
        return ret > 0;
      //return wait->wait( min( directory_watcher_interval, (double)( until - Time::now() ) ) );


/*

        // Weniger gut. Wir warten auf mehrere Ereignisse und müssen diese ständig reihum abfragen.
        //Rotating_bar rotating_bar = _log->log_level() <= log_debug9;

        while(1)
        {
            for( int i = _events.size() - 1; i >= 0; i-- )
            {
                Event* e = dynamic_cast<Event*>( _events[i] );

                if( typeid( *e ) == typeid (Directory_watcher) )
                {
                    if( ((Directory_watcher*)e)->has_changed() )  return i;
                }
                else
                {
                    bool signaled = e->wait( min( directory_watcher_interval, (double)( until - Time::now() ) ) );
                    if( signaled )  return i;  // Leider auch bei EINTR (EINTR soll ja unterbrechen, aber return i stimmt dann nicht)
                }
            }

            //rotating_bar();

            if( Time::now() >= until )  return -1;
        }
*/
    }

#endif
}

//--------------------------------------------------------------------------Wait_handles::as_string

string Wait_handles::as_string() 
{
    string result;

    //THREAD_LOCK( _lock )
    {
        if( _events.empty() )
        {
            result = "nichts";
        }
        else
        {
            result = "{";

            for( int i = 0; i < _events.size(); i++ )
            {
                if( i > 0 )  result += ", ";

#               ifdef Z_WINDOWS
                    result += as_hex_string( (int)_handles[i] );
                    result += ' ';
#               endif

                if( _events[i] )  result += _events[i]->as_text(); 
            }

            result += "}";
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
      //if( data.attrib &  ...dir... )  return data.name + Z_DIR_SEPARATOR;
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

      //if( data.attrib &  ...dir... )  return data.name + Z_DIR_SEPARATOR;
        return data.name; 
    }

    int  _handle;
};

#else

struct Directory_reader
{
    Directory_reader() : _handle(NULL) {}
    
    ~Directory_reader()
    { 
        close(); 
    }
    
    void close() 
    { 
        if( _handle )  closedir( _handle ), _handle = NULL;
    }

    string first( const string& dirname ) 
    { 
        _handle = opendir( dirname.c_str() );
        if( !_handle )  throw_errno( errno, "opendir", dirname.c_str() );

        return next();
    }

    string next() 
    { 
        struct dirent* entry = readdir( _handle );
        if( !entry )  return "";

        return entry->d_name;
    }

    DIR* _handle;
};

#endif
//------------------------------------------------------------------Directory_watcher::close_handle

void Directory_watcher::close_handle()
{
#   ifdef Z_WINDOWS
        if( _handle )
        {
            FindCloseChangeNotification( _handle );
            _handle = NULL;
        }

#   endif

    _filename_regex.close();
}

//---------------------------------------------------------------Directory_watcher::watch_directory

void Directory_watcher::watch_directory( const string& directory, const string& filename_pattern )
{
    close();

    _directory = directory;
    _filename_pattern = filename_pattern;

    if( !filename_pattern.empty() )  _filename_regex.compile( filename_pattern );

#   ifdef Z_WINDOWS

        _handle = FindFirstChangeNotification( directory.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
        if( !_handle  ||  _handle == INVALID_HANDLE_VALUE )  _handle = NULL, throw_mswin_error( "FindFirstChangeNotification", directory.c_str() );

#    else

        has_changed_2( true );    // Verzeichnisinhalt merken

#   endif
}

//-------------------------------------------------------------------------Directory_watcher::renew
#ifndef Z_WINDOWS

void Directory_watcher::renew()
{
    has_changed_2( true );
}

#endif
//-------------------------------------------------------------------Directory_watcher::has_changed

bool Directory_watcher::has_changed()
{
    bool changed = has_changed_2();
    if( changed )  set_signaled();
    return changed;
}

//-----------------------------------------------------------------Directory_watcher::has_changed_2

bool Directory_watcher::has_changed_2( bool throw_error )
{
#   ifdef Z_WINDOWS

        // Nach wait() zu rufen, damit _signaled auch gesetzt ist!
        if( Event::signaled() )  return true;


        // Wenn der Job mehrere Verzeichnisse überwacht, wird nur die Änderung des ersten signalisiert.
        // WaitForMultipleObjects() nur ein signalisiertes Objekt liefert.
        // Für diesen Fall fragen wir das Handle, ob es signalisiert worden ist.
        // Das bremst leider den Scheduler ein wenig.

        int ret = WaitForSingleObject( _handle, 0 );
        if( ret == WAIT_FAILED )  throw_mswin( "WaitForSingleObject" );
        return ret == WAIT_OBJECT_0;

#   else

        bool changed = false;

        try
        {
            Filenames*          new_f   = &_filenames[ 1 - _filenames_idx ];
            Filenames*          old_f   = &_filenames[ _filenames_idx ];
            Filenames::iterator o       = old_f->begin();

            new_f->clear();

            Directory_reader dir;
        
            string filename = dir.first( _directory.c_str() );
            while( filename != "" )
            {
                if( filename != "."  &&  filename != ".." )
                {
                    if( _filename_pattern.empty()  ||  _filename_regex.match( filename ) )
                    {
                        new_f->push_back( filename ); 
                        if( !changed )  if( o == old_f->end()  ||  *o != filename )  { changed = true; LOG( "Directory_watcher::has_changed: " << filename << "\n" ); }
                        if( o != old_f->end() )  o++;
                    }
                }

                filename = dir.next();
            }

            if( !changed )  if( o != old_f->end() )  { changed = true; LOG( "Directory_watcher::has_changed: " << filename << "\n" ); }

            dir.close();

            _filenames_idx = 1 - _filenames_idx;
        }
        catch( const exception& x ) 
        {
            // S.a. set_signaled() für Windows

            if( throw_error )  throw;

            _log->error( "Überwachung des Verzeichnisses " + _directory + " wird nach Fehler beendet: " + x.what() ); 
            _directory = "";   // Damit erneutes start_when_directory_changed() diese (tote) Überwachung nicht erkennt.
            Event::set_signaled();
            close();
            changed = true;
        }

        return changed;

#   endif
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
            if( _filename_regex.match( filename ) )  { Z_LOG2( "joacim", "Directory_watcher.match()  " << filename << " matches\n" ); return true; }
        }

        filename = dir.next();
    }

    return false;
}

//--------------------------------------------------------------------Directory_watcher::set_signaled

void Directory_watcher::set_signaled()
{
#   ifdef Z_WINDOWS

        try
        {
            if( _filename_pattern.empty()  ||  match() )  Event::set_signaled();
                                                    else  Event::set_signaled( false );    // Signal von _event zurücknehmen

            BOOL ok = FindNextChangeNotification( _handle );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
        }
        catch( const exception& x ) 
        {
            // S.a. has_changed() für Unix

            _log->error( "Überwachung des Verzeichnisses " + _directory + " wird nach Fehler beendet: " + x.what() ); 
            _directory = "";   // Damit erneutes start_when_directory_changed() diese (tote) Überwachung nicht erkennt.
            Event::set_signaled();
            close();
        }

#   else

        Event::set_signaled();

#   endif
}

//-------------------------------------------------------------------------Directory_watcher::reset

void Directory_watcher::reset()
{
    Z_MUTEX( _mutex )
    {
        _signaled = false;
        _signal_name = "";
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
