// $Id: z_windows.cxx 13786 2009-04-28 08:03:45Z jz $

#include "zschimmer.h"

#ifdef Z_WINDOWS

#include "log.h"
#include "z_windows.h"

namespace zschimmer {
namespace windows {


//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-SHOWWINDOW-01", "Ungültiger Wert für ShowWindow(): " },
    {}
};

//--------------------------------------------------------------------------------------------const

const uint64 base_filetime = 116444736000000000LL;

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_windows )
{
    add_message_code_texts( error_codes ); 
}

//-----------------------------------------------------------------------is_windows_version_or_more

bool is_windows_version_or_more( int expected_major, int expected_minor )
{
    DWORD v = GetVersion();
    int major = v & 0xff;
    int minor = ( v >> 8 ) & 0xff;

    return major == expected_major  &&  minor >= expected_minor
        || major > expected_major;
}

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

        Z_LOG2( "windows.PeekMessage", "message=" << hex_from_int( msg.message ) <<
                                      " wParam=" << hex_from_int16( msg.wParam )  <<
                                      " lParam=" << hex_from_int( msg.lParam ) << "\n" );

        //TranslateMessage( &msg ); 
        DispatchMessage( &msg ); 
    }
}

//--------------------------------------------------------------------------------show_window_value

int show_window_value( const string& show_window_value_name )
{
    string s = ucase( show_window_value_name );
    if( string_begins_with( s, "SW_" ) )  s.erase( 0, 3 );

    if( s == "FORCEMINIMIZE"    )  return SW_FORCEMINIMIZE;     // Windows 2000/XP: Minimizes a window, even if the thread that owns the window is hung. This flag should only be used when minimizing windows from a different thread.
    if( s == "HIDE"             )  return SW_HIDE;              // Hides the window and activates another window.
    if( s == "MAXIMIZE"         )  return SW_MAXIMIZE;          // Maximizes the specified window.
    if( s == "MINIMIZE"         )  return SW_MINIMIZE;          // Minimizes the specified window and activates the next top-level window in the Z order.
    if( s == "RESTORE"          )  return SW_RESTORE;           // Activates and displays the window. If the window is minimized or maximized, the system restores it to its original size and position. An application should specify this flag when restoring a minimized window.
    if( s == "SHOW"             )  return SW_SHOW;              // Activates the window and displays it in its current size and position. 
    if( s == "SHOWDEFAULT"      )  return SW_SHOWDEFAULT;       // Sets the show state based on the SW_ value specified in the STARTUPINFO structure passed to the CreateProcess function by the program that started the application. 
    if( s == "SHOWMAXIMIZED"    )  return SW_SHOWMAXIMIZED;     // Activates the window and displays it as a maximized window.
    if( s == "SHOWMINIMIZED"    )  return SW_SHOWMINIMIZED;     // Activates the window and displays it as a minimized window.
    if( s == "SHOWMINNOACTIVE"  )  return SW_SHOWMINNOACTIVE;   // Displays the window as a minimized window. This value is similar to SW_SHOWMINIMIZED, except the window is not activated.
    if( s == "SHOWNA"           )  return SW_SHOWNA;            // Displays the window in its current size and position. This value is similar to SW_SHOW, except the window is not activated.
    if( s == "SHOWNOACTIVATE"   )  return SW_SHOWNOACTIVATE;    // Displays a window in its most recent size and position. This value is similar to SW_SHOWNORMAL, except the window is not actived.
    if( s == "SHOWNORMAL"       )  return SW_SHOWNORMAL;        // Activates and displays a window. If the window is minimized or maximized, the system restores it to its original size and position. An application should specify this flag when displaying the window for the first time.

    throw_xc( "Z-SHOWWINDOW-1", show_window_value_name );
}

//-------------------------------------------------------------------------------compare_systemtime

int compare_systemtime( const SYSTEMTIME& a, const SYSTEMTIME& b )
{
    return a.wYear < b.wYear? -1 :
           a.wYear > b.wYear? +1 :
           a.wMonth < b.wMonth? -1 :
           a.wMonth > b.wMonth? +1 :
           a.wDay < b.wDay? -1 :
           a.wDay > b.wDay? +1 :
           a.wHour < b.wHour? -1 :
           a.wHour > b.wHour? +1 :
           a.wMinute < b.wMinute? -1 :
           a.wMinute > b.wMinute? +1 :
           a.wSecond < b.wSecond? -1 :
           a.wSecond > b.wSecond? +1 :
           a.wMilliseconds < b.wMilliseconds? -1 :
           a.wMilliseconds > b.wMilliseconds? +1 : 0;
}

//-------------------------------------------------------------------------filetime_from_systemtime

FILETIME filetime_from_systemtime( const SYSTEMTIME& systemtime )
{
    BOOL     ok;
    FILETIME filetime;

    ok = SystemTimeToFileTime( &systemtime, &filetime );
    if( !ok )  z::throw_mswin( "SystemTimeToFileTime" );

    return filetime;
}

//----------------------------------------------------------------------double_time_t_from_filetime

double double_time_t_from_filetime( const FILETIME& filetime )
{
    return (double)( *(int64*)&filetime - base_filetime ) / 10000000.0;
}

//-----------------------------------------------------------------------------time_t_from_filetime

time_t time_t_from_filetime( const FILETIME& filetime )
{
    return (time_t)double_time_t_from_filetime( filetime );
}

//-----------------------------------------------------------------------------filetime_from_time_t

FILETIME filetime_from_time_t( double t )
{
    ULARGE_INTEGER result;
    result.QuadPart = (uint64)( t * 10000000.0 + 0.5 ) + base_filetime;
    return *(FILETIME*)&result;
}

//-----------------------------------------------------------------------------filetime_from_time_t

FILETIME filetime_from_time_t( time_t t )
{
    ULARGE_INTEGER result;
    result.QuadPart = (uint64)t * 10000000 + base_filetime;
    return *(FILETIME*)&result;
}

//-----------------------------------------------------------------convert_to_noninheritable_handle

HANDLE convert_to_noninheritable_handle( HANDLE handle )
{
    HANDLE result = 0;

    if( handle )
    {
        BOOL ok;
        ok = DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &result, 0, FALSE, DUPLICATE_SAME_ACCESS );
        if( !ok )  throw_mswin( "DuplicateHandle", Z_FUNCTION );

        ok = CloseHandle( handle );
        if( !ok )  throw_mswin( "CloseHandle", Z_FUNCTION );
    }

    return result;
}

//-----------------------------------------------------------------------------Handle::close_handle

void Handle::close_handle()
{ 
    if( valid() )
    { 
        Z_LOG2( "zschimmer", "CloseHandle(" << _handle << ")\n" );
        CloseHandle( _handle ); 
        _handle=0; 
    } 
}

//----------------------------------------------------------------Handle::convert_to_noninheritable

void Handle::convert_to_noninheritable()
{
    _handle = convert_to_noninheritable_handle( _handle );
}

//----------------------------------------------------------------Handle::set_handle_noninheritable

void Handle::set_handle_noninheritable( HANDLE h )
{ 
    close(); 
    _handle = h; 
    convert_to_noninheritable(); 
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif

