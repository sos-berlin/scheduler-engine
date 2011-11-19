// borstrng.h                               © 1995 SOS GmbH Berlin
// Borland-Strings; Vor sos.h einziehen

#ifndef __BORSTRNG_H
#define __BORSTRNG_H

#if defined __BORLANDC__

/*
    // cstring.h zieht windows.h ein, also in windows.h alles abschalten:

    #define STRICT            // strikte Parametertypen

  //#define NOKERNEL          // KERNEL APIs and definitions
    #define NOGDI             // GDI APIs and definitions
    #define NOUSER            // USER APIs and definitions
    #define NOSOUND           // Sound APIs and definitions
    #define NOCOMM            // Comm driver APIs and definitions
    #define NODRIVERS         // Installable driver APIs and definitions

    #define OEMRESOURCE       // OEM Resource values
    #define NONLS             // All NLS defines and routines
    #define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
    #define NOKANJI           // Kanji support stuff.
    #define NOMINMAX          // min() and max() macros
    #define NOLOGERROR        // LogError() and related definitions
    #define NOPROFILER        // Profiler APIs
    #define NOMEMMGR          // Local and global memory management
    #define NOLFILEIO         // _l* file I/O routines
    #define NOOPENFILE        // OpenFile and related definitions
    #define NORESOURCE        // Resource management
    #define NOATOM            // Atom management
    #define NOLANGUAGE        // Character test routines
    #define NOLSTRING         // lstr* string management routines
    #define NODBCS            // Double-byte character set routines
    #define NOKEYBOARDINFO    // Keyboard driver routines
    #define NOGDICAPMASKS     // GDI device capability constants
    #define NOCOLOR           // COLOR_* color values
    #define NOGDIOBJ          // GDI pens, brushes, fonts
    #define NODRAWTEXT        // DrawText() and related definitions
    #define NOTEXTMETRIC      // TEXTMETRIC and related APIs
    #define NOSCALABLEFONT    // Truetype scalable font support
    #define NOBITMAP          // Bitmap support
    #define NORASTEROPS       // GDI Raster operation definitions
    #define NOMETAFILE        // Metafile support
    #define NOSYSMETRICS      // GetSystemMetrics() and related SM_* definitions
    #define NOSYSTEMPARAMSINFO // SystemParametersInfo() and SPI_* definitions
    #define NOMSG             // APIs and definitions that use MSG structure
    #define NOWINSTYLES       // Window style definitions
    #define NOWINOFFSETS      // Get/SetWindowWord/Long offset definitions
    #define NOSHOWWINDOW      // ShowWindow and related definitions
    #define NODEFERWINDOWPOS  // DeferWindowPos and related definitions
    #define NOVIRTUALKEYCODES // VK_* virtual key codes
    #define NOKEYSTATES       // MK_* message key state flags
    #define NOWH              // SetWindowsHook and related WH_* definitions
    #define NOMENUS           // Menu APIs
    #define NOSCROLL          // Scrolling APIs and scroll bar control
    #define NOCLIPBOARD       // Clipboard APIs and definitions
    #define NOICONS           // IDI_* icon IDs
    #define NOMB              // MessageBox and related definitions
    #define NOSYSCOMMANDS     // WM_SYSCOMMAND SC_* definitions
    #define NOMDI             // MDI support
    #define NOCTLMGR          // Control management and controls
    #define NOWINMESSAGES     // WM_* window messages
    #define NOHELP            // Help support
*/
#if defined _Windows  &&  !defined __WINDOWS_H
    // Damit windows.h nicht eingezogen werden muß, wird hier das Nötige deklariert:

#   if !defined __SYSDEP_H
#       include <sysdep.h>
#   endif

    SOS_DECLARE_MSWIN_HANDLE( HINSTANCE )
    typedef unsigned int UINT;

#   if defined __WIN32__
#       define AnsiToOem        CharToOemA
#       define OemToAnsi        OemToCharA
#       define AnsiToOemBuff    CharToOemBuffA
#       define OemToAnsiBuff    OemToCharBuffA
#       define WINUSERAPI       __declspec(dllimport)
#       define WINAPI           __stdcall
        typedef int BOOL;

        extern "C" WINUSERAPI BOOL WINAPI CharToOemA( const char* lpszSrc, char* lpszDst);
        extern "C" WINUSERAPI BOOL WINAPI OemToCharA( const char* lpszSrc, char* lpszDst);
        extern "C" WINUSERAPI BOOL WINAPI CharToOemBuffA( const char* lpszSrc, char* lpszDst, unsigned long cchDstLength);
        extern "C" WINUSERAPI BOOL WINAPI OemToCharBuffA( const char* lpszSrc, char* lpszDst, unsigned long cchDstLength);

#       define __WINDOWS_H
#           include <cstring.h>
#       undef __WINDOWS_H
#    else
        extern "C" void _far _pascal AnsiToOem(const char _huge*, char _huge*);
        extern "C" void _far _pascal OemToAnsi(const char _huge*, char _huge*);
        extern "C" void _far _pascal AnsiToOemBuff(const char _far*, char _far*, unsigned int );
        extern "C" void _far _pascal OemToAnsiBuff(const char _far*, char _far*, unsigned int);

#       define __WINDOWS_H
#           include <cstring.h>
#       undef __WINDOWS_H
#   endif

# else
#   include <cstring.h>
#endif

//#include <cstring.h>

inline const char*   c_str    ( const string& str )                         { return str.c_str(); }
inline unsigned int  length   ( const string& str )                         { return str.length(); }

inline string& operator += ( string& str, char c )
{
    str.append( &c, 0, 1 );
    return str;
}

inline unsigned int position( const string& str, const string& to_find, unsigned int pos = 0 )
{
    unsigned int p = str.find( to_find, pos );
    return p == NPOS? length( str ) : p;
}

void assign( string*, const char* );
//inline void assign( Sos_string* str, const char* o, int len )  { *str = as_string( o, len ); }

#if SYSTEM_BORLAND == 0x500  &&  defined SYSTEM_WIN16DLL      // Den hat Borland vergessen:
    inline string::string( char c )
    throw( xalloc, string::lengtherror )
    {
        p = new TStringRef( c, 1 );
    }
#endif

#endif

#endif
