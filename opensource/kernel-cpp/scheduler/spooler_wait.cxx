// $Id: spooler_wait.cxx 14153 2010-12-14 13:34:54Z ss $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
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
#endif

#include "../kram/sleep.h"
#include "../kram/log.h"
#include "Java_thread_unlocker.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

#ifdef Z_WINDOWS
    const DWORD SOS_WAIT_TIMEOUT = -2;                     // WAIT_TIMEOUT ist WAIT_OBJECT_0 + 256 und kann damit mit einem
                                                           // lfd. Prozess (No. 256) kollidieren.
#endif

//------------------------------------------------------------------------------------console_width
#ifdef Z_WINDOWS

static int console_width()
{

    CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info; 
    memset( &console_screen_buffer_info, 0, sizeof console_screen_buffer_info );

    BOOL ok = GetConsoleScreenBufferInfo( GetStdHandle(STD_ERROR_HANDLE), &console_screen_buffer_info );

    return ok? console_screen_buffer_info.dwSize.X 
             : 80;
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

//------------------------------------------------------------------------ait_handles::Wait_handles

Wait_handles::Wait_handles( Spooler* spooler )
: 
    _zero_(this+1),
    _spooler(spooler),
    _log( spooler->log() ),
    _lock("Wait_handles") 
{
}

//-----------------------------------------------------------------------Wait_handles::Wait_handles

Wait_handles::Wait_handles( const Wait_handles& o )
: 
    _zero_(this+1),
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
            if( *it )  
            {
                _log->warn( "Closing Wait_handles before " + (*it)->as_text() );
//                Z_WINDOWS_ONLY( Z_DEBUG_ONLY( assert( !"Closing Wait_handles before ..." ) ) );
            }
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
            if( *it  &&  (*it)->signaled() )  
            { 
                _catched_event = *it;
                //Z_LOG2( _spooler->_scheduler_wait_log_category, **it << " signaled!\n" );  
                return true; 
            }
        }
    }

    return false;
}

//------------------------------------------------------------------------Wait_handles::operator += 

Wait_handles& Wait_handles::operator += ( Wait_handles& o )
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

    return *this;
}

//--------------------------------------------------------------------------------Wait_handles::add

void Wait_handles::add( System_event* event )
{
#       ifdef Z_WINDOWS
            assert( event->handle() );
            _handles.push_back( event->handle() );
#       endif

        _events.push_back( event );
}

//-----------------------------------------------------------------------------Wait_handles::remove

void Wait_handles::remove( System_event* event )
{
    if( !event )  return;

        // Das ist fast der gleiche Code wie von remove_handle(). Kann man das zusammenfassen? 26.11.2002

            Event_vector::iterator it = _events.begin();

            while( it != _events.end() )
            {
                if( *it == event )  break;
                it++;
            }

            if( it == _events.end() ) {
                _log->error( "Wait_handles::remove(" + event->as_text() + "): Unknown event" );     // Keine Exception. Das wäre nicht gut in einem Destruktor
                return;
            }

            Z_WINDOWS_ONLY( _handles.erase( _handles.begin() + ( it - _events.begin() )  ) );
            _events.erase( it );
}

//-------------------------------------------------------------------------Wait_handles::wait_until
// Liefert Nummer des Events (0..n-1) oder -1 bei Zeitablauf

bool Wait_handles::wait_until( const Time& until, const Object* wait_for_object, const Time& resume_until, const Object* resume_object )
{
#   ifdef Z_DEBUG
        if( z::Log_ptr log = _spooler->_scheduler_wait_log_category )
        {
            *log << Z_FUNCTION << " ";
            *log << " until=" << until << ", ";
            if( wait_for_object )  *log << "  for " << wait_for_object->obj_name() << ", ";
            *log << *this << "\n";
        }
#   endif


    // until kann 0 sein
    _catched_event = NULL;

    //2006-06-18: Nicht gut bei "nichts_getan": Wir brauchen auch neue Ereignisse, v.a. TCP.    if( signaled() )  return true;

#ifdef Z_WINDOWS

    bool    result    = false;
    bool    again     = false;
    HANDLE* handles   = NULL;
    //bool    waitable_timer_set = false;
    BOOL    ok;
    Time    now       = Time::now();


    if( now < until  &&  _spooler->_waitable_timer )
    {
        if( !resume_until.is_never() )
        {
            LARGE_INTEGER gmtime;

            gmtime.QuadPart = -(int64)( ( resume_until - now ).as_double() * 10000000 );  // Negativer Wert bedeutet relative Angabe in 100ns.
            if( gmtime.QuadPart < 0 )
            {
                Z_LOG2( _spooler->_scheduler_wait_log_category, "SetWaitableTimer(" << ( gmtime.QuadPart / 10000000.0 ) << "s: " << resume_until.as_string() << ")"  
                        << ( resume_object? " for " + resume_object->obj_name() : "" ) << "\n" );

                ok = SetWaitableTimer( _spooler->_waitable_timer, &gmtime, 0, NULL, NULL, TRUE );   // Weckt den Rechner
                if( !ok )  z::throw_mswin( "SetWaitableTimer" );

                _spooler->_is_waitable_timer_set = true;
            }
        }
        else
        if( _spooler->_is_waitable_timer_set )
        {
            ok = CancelWaitableTimer( _spooler->_waitable_timer );
            if( !ok )  z::throw_mswin( "CancelWaitableTimer" );

            _spooler->_is_waitable_timer_set = false;
        }
    }


    while(1)
    {
        double wait_time         = max( 0.0, (until - now).as_double() );
        int    max_sleep_time_ms = INT_MAX-1;
        int    t                 = (int)ceil( min( (double)max_sleep_time_ms, wait_time * 1000.0 ) );
        DWORD  ret               = SOS_WAIT_TIMEOUT;


        if( t <= 0 )  if( again )  break;
                             else  t = 0;  //break;

        // 30min sind 1800s, aber t ist in Millisekunden
        if( again ) {
            if( t > 1800 )  { result = false; break; }  // Um mehr als eine halbe Stunde verrechnet? Das muss an der Sommerzeitumstellung liegen
        }

#       ifdef Z_DEBUG
            Z_LOG2( t > 0? "scheduler.wait" : "scheduler.loop", "MsgWaitForMultipleObjects " << sos::as_string(t/1000.0) << "s (" << wait_time << "s, bis " << until << ( wait_for_object? " auf " + wait_for_object->obj_name() : "" ) << ")  " << as_string() << "\n" );
#       endif

        handles = new HANDLE [ _handles.size()+1 ];
        for( int i = 0; i < _handles.size(); i++ )  handles[i] = _handles[i];

        // Regelmässige Ausgabe von Text auf der Konsole
        if( _spooler  &&  _spooler->_print_time_every_second )
        {
            int     console_line_length = 0;
            double  step                = 0.05;  // Der erste Schritt 1/20s, dann 1s
            
            while(1)
            {
                double remaining = (until - Time::now()).as_double();
                if( remaining < 0.7 )  break;

                /**
                 * \change  JS-471 - Aufruf über sosMsgWaitForMultipleObjects
                 * \b oldcode from 2010-04-01
                 * \code
                    ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, (int)( ceil( min( step, remaining ) * 1000 ) ), QS_ALLINPUT ); 
                   \endcode
                 */
                ret = sosMsgWaitForMultipleObjects( _handles.size(), handles, (int)( ceil( min( step, remaining ) * 1000 ) ) ); 
                if( ret != SOS_WAIT_TIMEOUT )  break;

                step = 1.0;

                Time now = Time::now();
                Duration rest = until - now;
                t = (int)ceil( min( (double)max_sleep_time_ms, rest.as_double() * 1000.0 ) );

                S console_line;
                console_line << Time::now().as_string( time::without_ms );
                
                if( !until.is_never() ||  wait_for_object )
                {
                    console_line << " (";
                    if (!until.is_never()) {
                        Time r = Time(rest.as_double());
                        int days = r.day_nr();
                        if( days > 0 )  console_line << days << "d+";
                        console_line << r.time_of_day().as_string( time::without_ms ) << "s";
                        if( days > 0 )  console_line << " until " << Time( until ).as_string();
                    }
                    if( wait_for_object )  console_line << " for " << wait_for_object->obj_name();
                    console_line << ")";
                }

                string l = console_line.to_string().substr( 0, console_width() - 1 );
                console_line_length = l.length();
                l += '\r';
                cerr << l << flush;
            }

            if( ret == SOS_WAIT_TIMEOUT )
            {
                if( t > 0  &&  console_line_length == 0 )  cerr << _spooler->_wait_counter << '\r', console_line_length = 20;//_spooler->_wait_rotating_bar();
                /**
                 * \change  JS-471 - Aufruf über sosMsgWaitForMultipleObjects
                 * \b oldcode from 2010-04-01
                 * \code
                    ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, max( 0, t ), QS_ALLINPUT ); 
                   \endcode
                 */
                ret = sosMsgWaitForMultipleObjects( _handles.size(), handles, max( 0, t ) ); 
            }

            if( console_line_length )  cerr << string( console_line_length, ' ' ) << '\r' << flush;  // Zeile löschen

        // normale Bearbeitung von scheduler-Prozessen
        } else {
        /**
         * \change  JS-471 - Aufruf über sosMsgWaitForMultipleObjects
         * \b oldcode from 2010-04-01
         * \code
           ret = MsgWaitForMultipleObjects( _handles.size(), handles, FALSE, t, QS_ALLINPUT ); 
           \endcode
         */
         ret = sosMsgWaitForMultipleObjects( _handles.size(), handles, t ); 
        }
        
        delete [] handles;  handles = NULL;

        if( ret == WAIT_FAILED )  throw_mswin_error( "MsgWaitForMultipleObjects" );        // z.B. > 63 handles

        if( ret >= WAIT_OBJECT_0  &&  ret < WAIT_OBJECT_0 + _handles.size() )           // "normales" handle
        {
                int            index = ret - WAIT_OBJECT_0;
                z::Event_base* event = _events[ index ];                                // scheduler-internes event-object
            
                if( event )
                {
                    if( t > 0 )  Z_LOG2( _spooler->_scheduler_wait_log_category, "... " << event->as_text() << "\n" );
                    if( event != &_spooler->_waitable_timer )  event->set_signaled( "MsgWaitForMultipleObjects" );            // signal für "event" gesetzt
                }
                else
                    if( t > 0 )  Z_LOG2( _spooler->_scheduler_wait_log_category, "... Event " << index << "\n" );

                _catched_event = event;

                if( event != &_spooler->_waitable_timer )  result = true;                // PC aufwecken
                break;
        }
        else
        if( ret == WAIT_OBJECT_0 + _handles.size() )
        {
            windows::windows_message_step();
        }
        else
        if( ret == SOS_WAIT_TIMEOUT )  
        {
            again = true;
        }
        else
            throw_mswin_error( "MsgWaitForMultipleObjects", z::as_string(ret) );


        now = Time::now();
    }

    return result;

#else  // UNIX-Variante ...

    {
        Time now = Time::now();

        //if( until > now )
            Z_LOG2( _spooler->_scheduler_wait_log_category, "wait_until " << until.as_string() << " (" << (double)( until - now ) << "s)" <<
                ( wait_for_object? " auf " + wait_for_object->obj_name() : "" ) << " " << as_string() << "\n" );

        ptr<Socket_wait> wait = _spooler->_connection_manager->create_wait();

        for( int i = _events.size() - 1; i >= 0; i-- )   if( _events[i] )  wait->add( _events[i] );

        wait->set_polling_interval( 0.1 );

        int ret;
        {
            Java_thread_unlocker unlocker ( _spooler );
            int ret = wait->wait( (double)( until - now ) );
        }
        return ret > 0;
    }

#endif
}

//-------------------------------------------------------Wait_handles::sosMsgWaitForMultipleObjects
#ifdef Z_WINDOWS
/**
* \brief Steuerung der weiteren Verarbeitung der Prozess-Handles
* \detail 
* Normalerweise werden die Prozesshandles unter Windows durch MsgWaitForMultipleObjects verarbeitet. Diese Funktion
* kann aber max. 63 Prozesshandles verwalten, deshalb werden bei einer größeren Anzahl von Prozessen diese Blockweise
* an MsgWaitForMultipleObjects übergeben.
*
* \version 1.3.8
* \since 2010-04-01 13:07 JS-471
* \author ss
*
* \param nCount - Anzahl der Handles
* \param pHandles - Array der Handles
* \param dTimeout - max. Wartezeit in Sekunden
*
* \return Nummer des Prozesses, für den ein event ausgelöst worden ist.
*/

DWORD Wait_handles::sosMsgWaitForMultipleObjects(unsigned int nCount, HANDLE *pHandles, DWORD dTimeout)
{
    DWORD  ret;

    if (nCount < MAXIMUM_WAIT_OBJECTS)
    {
        ret = myMsgWaitForMultipleObjects( nCount, pHandles, FALSE, dTimeout, QS_ALLINPUT );    // wie bisher
        if (ret == WAIT_TIMEOUT) ret = SOS_WAIT_TIMEOUT;
    } else {
        #ifdef Z_DEBUG
            Z_LOG2( "scheduler.wait", "MsgWaitForMultipleObjects: " << nCount << " processes waiting (wait time " << dTimeout << "s\n" );
        #endif
        ret = sosMsgWaitForMultipleObjects64( nCount, pHandles, dTimeout );   // polling ...
    }

    return ret;
}

#endif
//--------------------------------------------------------Wait_handles::myMsgWaitForMultipleObjects
#ifdef Z_WINDOWS

DWORD Wait_handles::myMsgWaitForMultipleObjects(unsigned int nCount, HANDLE *pHandles, BOOL waitAll, DWORD timeout, DWORD wakeMask) 
{
    Java_thread_unlocker unlocker ( _spooler );
    DWORD result = MsgWaitForMultipleObjects(nCount, pHandles, waitAll, timeout, wakeMask);
    return result;
}

#endif
//-----------------------------------------------------Wait_handles::sosMsgWaitForMultipleObjects64
#ifdef Z_WINDOWS
/**
* \brief Polling für alle Prozesse
* \detail 
* Laufen mehr als 63 Prozesse parallel werden alle Prozesse in Blöcken zu jeweils 63 regelmässig abgefragt.
*
* \version 1.3.8
* \since 2010-04-01 13:15 JS-471
* \author Stefan Schädlich
*
* \param nCount - Anzahl der Handles
* \param pHandles - Array der Handles
* \param dTimeout - max. Wartezeit in Sekunden
*
* \return Nummer des Prozesses, für den ein event ausgelöst worden ist.
*/

DWORD Wait_handles::sosMsgWaitForMultipleObjects64(unsigned int nCount, HANDLE *pHandles, DWORD dTimeout )
{
    int     result            = SOS_WAIT_TIMEOUT;
    int     max_handles       = MAXIMUM_WAIT_OBJECTS - 1;
    Time    now               = Time::now();

    int     max_sleep_time_ms = INT_MAX-1;    // max. Wartezeit in Millisekunden
    Time    until             = now + Duration(dTimeout/1000.0);
    bool    isWaiting         = true;
    int     timeoutCounter    = 0;

    while (isWaiting) {    
        timeoutCounter++;
        int stepTimeout = calculateStepTimeout(timeoutCounter);  // 1 Millisekunde

        isWaiting = false;
        int blockCount = ( nCount + max_handles - 1 ) / max_handles;
        int remainingCount = nCount;
#       ifdef Z_DEBUG
            Z_LOG2( "scheduler.wait", "performing MsgWaitForMultipleObjects for " << blockCount << " blocks until " << until << " is reached (steptimeout=" << stepTimeout << ").\n" );
#       endif

        for (int i=0; i < blockCount; i++ ) {  // MAXIMUM_WAIT_OBJECTS handles pro block
            int c = min( max_handles, remainingCount);  // c kann beim letzten Block unter MAXIMUM_WAIT_OBJECTS fallen
            assert(c > 0);
            remainingCount -= c;
            int remainingMillis = (int)((until - now).as_double() * 1000);
            int timeout = (i == 0) ? min(stepTimeout, remainingMillis ) : 0;  // Wartet 1s für den ersten Block, sonst nur Abfrage der Handles
            assert(timeout >= 0);   // Ist diese Bedingung NICHT erfüllt, bricht die Ausführung ab (nur im Windows debug-Modus)

#           ifdef Z_DEBUG
                Z_LOG2( "scheduler.wait", "block " << (i+1) << ": " << c << " items, " << timeout << "ms, remaining millis=" << remainingMillis << ".\n" );
#           endif

            int ret = myMsgWaitForMultipleObjects( c, pHandles + (i * max_handles), FALSE, timeout, QS_ALLINPUT );
            if( ret != WAIT_TIMEOUT ) {
                if( ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + c )
                    result = i * max_handles + ret;
                else
                    result = ret;
                break;
            }
        }

        if( result == SOS_WAIT_TIMEOUT ) {
            now = Time::now();
            if( now < until )
                isWaiting = true;
        }
    }

#   ifdef Z_DEBUG
        Z_LOG2( "scheduler.wait", "result=" << result << "\n" );
#   endif
    return result;
}

#endif
//--------------------------------------------------------------------------Wait_handles::as_string

int Wait_handles::calculateStepTimeout(int timeoutCounter)
{
    return max (min(timeoutCounter / 10, 1000), 1);
}

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

//--------------------------------------------------------------Directory_reader::~Directory_reader
    
Directory_watcher::Directory_reader::~Directory_reader()
{ 
    close(); 
}

//----------------------------------------------------------------------------Directory_reader::get

ptr<zschimmer::file::File_info> Directory_watcher::Directory_reader::get() 
{ 
    ptr<zschimmer::file::File_info> result;

    while(1)
    {
        result = read();
        _first_read = true;
        if( !result )  break;

        string name = result->path().name();
        if( name == "."  )  continue;
        if( name == ".." )  continue;
        if( !_regex || _regex->match( name ) )  break;

        Z_DEBUG_ONLY( Z_LOG2( "scheduler.directory", Z_FUNCTION << "    " << name << " nicht übernommen\n" ); ) 
    }

    //Z_DEBUG_ONLY( Z_LOG2( "zschimmer", Z_FUNCTION << " => " << result << "\n" ); ) 

    if( result )  result->path().set_directory( _directory_path );
    return result;
}

//---------------------------------------------------------------Directory_reader::Directory_reader
#ifdef Z_WINDOWS

Directory_watcher::Directory_reader::Directory_reader( Directory_watcher* w ) 
: 
    _zero_(this+1),
    _directory_path( w->_directory),
    _regex( w->_filename_pattern == ""? NULL : &w->_filename_regex ),
    _handle(-1) 
{
}

//---------------------------------------------------------------Directory_reader::Directory_reader
    
Directory_watcher::Directory_reader::Directory_reader( const File_path& directory, Regex* regex ) 
: 
    _zero_(this+1),
    _directory_path( directory.path() ),
    _regex( regex ),
    _handle(-1) 
{
}
    
//--------------------------------------------------------------------------Directory_reader::close

void Directory_watcher::Directory_reader::close() 
{ 
    if( _handle != -1 )  _findclose( _handle ),  _handle = -1; 
}

//--------------------------------------------------------Directory_watcher::Directory_reader::read

ptr<zschimmer::file::File_info> Directory_watcher::Directory_reader::read() 
{ 
    ptr<zschimmer::file::File_info>   result = Z_NEW( zschimmer::file::File_info );
    _finddata_t                 data;

    if( !_first_read )
    {
        string pattern = _directory_path + "/*";

        memset( &data, 0, sizeof data );

        Z_LOG2( "scheduler.wait", "_findfirst(" << quoted_string(pattern) << ") ...\n" );

        _handle = _findfirst( pattern.c_str(), &data ); 
        if( _handle == -1 )  throw_errno( errno, "_findfirst", _directory_path.c_str() );  

        Z_LOG2( "scheduler.wait", "_findfirst(" << quoted_string(pattern) << ") OK\n" );
    }
    else
    {
        int ret = _findnext( _handle, &data ); 
        if( ret == -1 )  
        {
            if( errno == ENOENT )  return NULL;
            throw_errno( errno, "_findnext" ); 
        }
    }

    result->path().set_name( data.name );
    result->set_create_time     ( data.time_create == -1? 0 : data.time_create );
    result->set_last_access_time( data.time_access == -1? 0 : data.time_access );
    result->set_last_write_time ( data.time_write  == -1? 0 : data.time_write  );

    return result;
}


#else

//--------------------------------------------Directory_watcher::Directory_reader::Directory_reader

Directory_watcher::Directory_reader::Directory_reader( Directory_watcher* w ) 
: 
    _zero_(this+1),
    _directory_path( w->_directory ),
    _regex( w->_filename_pattern == ""? NULL : &w->_filename_regex ),
    _handle(NULL)
{
}
    
//--------------------------------------------Directory_watcher::Directory_reader::Directory_reader

Directory_watcher::Directory_reader::Directory_reader( const File_path& path, Regex* regex ) 
: 
    _zero_(this+1),
    _directory_path( path ),
    _regex( regex ),
    _handle(NULL)
{
}
    
//-------------------------------------------------------Directory_watcher::Directory_reader::close
    
void Directory_watcher::Directory_reader::close() 
{ 
    if( _handle )  closedir( _handle ), _handle = NULL;
}

//--------------------------------------------------Directory_watcher::Directory_reader::read_first
    
ptr<zschimmer::file::File_info> Directory_watcher::Directory_reader::read() 
{ 
    if( !_first_read )
    {
        Z_LOG2( "scheduler.directory", "opendir(" << quoted_string(_directory_path) << ") ...\n" );

        _handle = opendir( _directory_path.c_str() );
        if( !_handle )  throw_errno( errno, "opendir", _directory_path.c_str() );

        Z_LOG2( "scheduler.directory", "opendir(" << quoted_string(_directory_path) << ") OK\n" );
    }

    struct dirent* entry = readdir( _handle );
    if( !entry )  return NULL;

    ptr<zschimmer::file::File_info> result = Z_NEW( zschimmer::file::File_info );
    result->path().set_name( entry->d_name );
    return result;
}

#endif
//------------------------------------------------------------------Directory_watcher::close_handle

void Directory_watcher::close_handle()
{
#   ifdef Z_WINDOWS
        if( _handle )
        {
            Z_LOG2( "scheduler", "FindCloseChangeNotification()\n" );
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

        Z_LOG2( "scheduler", "FindFirstChangeNotification( \"" << directory << "\", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
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
//----------------------------------------------------------------------Directory_watcher::signaled

bool Directory_watcher::signaled()
{ 
#   ifdef Z_WINDOWS
        return has_changed(); 
#    else
        return Event::signaled();
#   endif
}

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

        Z_LOGI2( "scheduler.directory", Z_FUNCTION << "  WaitForSingleObject()\n" );
        int ret = WaitForSingleObject( _handle, 0 );
        if( ret == WAIT_FAILED )  throw_mswin( "WaitForSingleObject" );
        return ret == WAIT_OBJECT_0;

#   else

        Z_LOGI2( "scheduler.directory", Z_FUNCTION << "\n" );

        bool changed = false;

        try
        {
            Filenames*          new_f   = &_filenames[ 1 - _filenames_idx ];
            Filenames*          old_f   = &_filenames[ _filenames_idx ];
            Filenames::iterator o       = old_f->begin();

            new_f->clear();

            Directory_reader dir ( this );
        
            while(1)
            {
                ptr<zschimmer::file::File_info> f = dir.get();
                if( !f )  break;
                string filename = f->path().name(); 

                new_f->push_back( filename ); 

                if( o == old_f->end()  ||  filename < *o )  // Datei hinzugefügt?
                {
                    changed = true; 
                    Z_LOG2( "scheduler.directory", Z_FUNCTION << " new file     " << filename << "\n" ); 
                }
                else
                for(; o != old_f->end()  &&  *o < filename; o++ )  
                { 
                    if( _filename_pattern == "" )  changed = true; 
                    Z_LOG2( "scheduler.directory", Z_FUNCTION << " deleted file " << *o << "\n" ); 
                }

                if( o != old_f->end()  &&  *o == filename )  o++;
            }

            for(; o != old_f->end(); o++ )  
            { 
                if( _filename_pattern == "" )  changed = true; 
                Z_LOG2( "scheduler.directory", Z_FUNCTION << " deleted_file " << *o << "\n" ); 
            }

            dir.close();

            _filenames_idx = 1 - _filenames_idx;
        }
        catch( const exception& x ) 
        {
            // S.a. set_signaled() für Windows

            if( throw_error )  throw;

            _log->error( message_string( "SCHEDULER-300", _directory, x ) );   // "Überwachung des Verzeichnisses " + _directory + " wird nach Fehler beendet: " + x.what() ); 
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
    return Directory_reader( this ).get() != NULL;
}

//------------------------------------------------------------------Directory_watcher::set_signaled

void Directory_watcher::set_signaled()
{
#   ifdef Z_WINDOWS

        try
        {
            if( _filename_pattern.empty()  ||  match() )  Event::set_signaled( "Directory_watcher::set_signaled" );
                                                    else  Event::set_signaled( false );    // Signal von _event zurücknehmen

            Z_LOG2( "scheduler", "FindNextChangeNotification(\"" << _directory << "\")\n" );
            BOOL ok = FindNextChangeNotification( _handle );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
        }
        catch( const exception& x ) 
        {
            // S.a. has_changed() für Unix

            _log->error( message_string( "SCHEDULER-300", _directory, x ) );   // "Überwachung des Verzeichnisses " + _directory + " wird nach Fehler beendet: " + x.what() ); 
            _directory = "";   // Damit erneutes start_when_directory_changed() diese (tote) Überwachung nicht erkennt.
            Event::set_signaled( "Directory_watcher::set_signaled" );
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

} //namespace scheduler
} //namespace sos
