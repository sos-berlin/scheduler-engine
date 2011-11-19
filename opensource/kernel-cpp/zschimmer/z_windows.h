// $Id: z_windows.h 13786 2009-04-28 08:03:45Z jz $

#ifndef __ZSCHIMMER_WINDOWS_H
#define __ZSCHIMMER_WINDOWS_H

#include "base.h"

#ifdef Z_WINDOWS

#include <windows.h>
#include "mutex.h"


namespace zschimmer {
namespace windows {

struct Mutex;
struct Event;
struct Thread;

//-------------------------------------------------------------------------------------------------

void                            windows_message_step        ();
int                             show_window_value           ( const string& value_name );           // Liefert Wert für ShowWindows()
FILETIME                        filetime_from_systemtime    ( const SYSTEMTIME& );
time_t                          time_t_from_filetime        ( const FILETIME& );
double                   double_time_t_from_filetime        ( const FILETIME& );
FILETIME                        filetime_from_time_t        ( double );
FILETIME                        filetime_from_time_t        ( time_t );
HANDLE                          convert_to_noninheritable_handle( HANDLE );

//----------------------------------------------------------------------------is_windows_xp_or_more

bool is_windows_version_or_more( int expected_major, int expected_minor );

//---------------------------------------------------------------------------is_windows2000_or_more

inline bool is_windows2000_or_more()
{
    // Ab 5.0
    return ( GetVersion() & 0xFF ) >= 5;
}

//----------------------------------------------------------------------------is_windows_xp_or_more

inline bool                     is_windows_xp_or_more       ()                                      { return is_windows_version_or_more( 5, 1 ); }
int                             compare_systemtime          ( const SYSTEMTIME&, const SYSTEMTIME& );

//-----------------------------------------------------------------------------is_platform_win32_nt
/*
inline bool is_windows_nt_or_more()
{
    return GetVersion() & 0x80000000 != 0;
    //OSVERSIONINFO v;
    //v.dwOSVersionInfoSize = sizeof v;

    //return GetVersionEx( &v )  &&  v.dwPlatformId == VER_PLATFORM_WIN32_NT; )  // Windows NT oder Windows 2000?
}
*/
//------------------------------------------------------------------------------int64_from_filetime

inline int64 int64_from_filetime( FILETIME t )
{
    return ( (int64)t.dwHighDateTime << 32 ) + t.dwLowDateTime;
}

//--------------------------------------------------------------------------filename_from_hinstance

inline string filename_from_hinstance( HINSTANCE hinstance )
{
    char path[ _MAX_PATH+1 ];

    int len = GetModuleFileName( hinstance, path, sizeof path );
    if( !len )  throw_mswin( "GetModuleFileName" );

    return string( path, len );
}

//------------------------------------------------------------------------------Thread_safe_counter
/*
struct Thread_safe_counter
{
                                Thread_safe_counter         ( long32 i )                            : _counter(i) {}

                                operator long32             ()                                      { return _counter; }
    Thread_safe_counter&        operator =                  ( long32 i )                            { _counter = i;  return _counter; }

  post oder pre?
  Mit Unix zusammenfassen, nur InterlockedIncrement() und Datentyp unterscheiden sich
    Thread_safe_counter&        operator ++                 ()                                      { return InterlockedIncrement( &_counter ); }
    Thread_safe_counter&        operator --                 ()                                      { return InterlockedDecrement( &_counter ); }


    long32                     _counter;
};
*/           
//-------------------------------------------------------------------------------------------Handle

struct Handle 
{
                                Handle                      ( HANDLE h = NULL )                     : _handle(h) {}
                               ~Handle                      ()                                      { close_handle(); }
                       
    void                        operator =                  ( HANDLE h )                            { set_handle( h ); }
    void                        operator =                  ( size_t h )                            { set_handle( (HANDLE)h ); }
                                operator HANDLE             () const                                { return handle(); }
    bool                        operator !                  () const                                { return !valid(); }
  //HANDLE*                     operator &                  ()                                      { close_handle(); return &_handle; }
    HANDLE*                     addr_of                     ()                                      { close_handle(); return &_handle; }

    bool                        valid                       () const                                { return _handle != 0  &&  (size_t)_handle != -1; }
    void                        set_handle                  ( HANDLE h )                            { close(); _handle = h; }
    void                        set_handle_noninheritable   ( HANDLE );
    void                        take_handle                 ( Handle& h )                           { set_handle(h);  h._handle = NULL; }
    HANDLE                      handle                      () const                                { return (size_t)_handle == -1? 0 : _handle; }
 //?uint                        as_uint                     () const                                { return (uint)_handle; }
    virtual void                close                       ()                                      { close_handle(); }
    virtual void                close_handle                ();
    void                        convert_to_noninheritable    ();

    HANDLE                     _handle;

  private:
                                Handle                      ( const Handle& );                      // Nicht implementiert
    void                        operator =                  ( const Handle& );                      // Nicht implementiert
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
#endif
