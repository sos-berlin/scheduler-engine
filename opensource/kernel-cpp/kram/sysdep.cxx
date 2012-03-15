// $Id: sysdep.cxx 13964 2010-08-18 11:12:52Z jz $

#include "precomp.h"

#include "../kram/sosstrng.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN
#   include <direct.h>     // getcwd
#   include <windows.h>
#   include <tlhelp32.h>
#else
#   include <limits.h>
#   include <unistd.h>
#   include <sys/times.h>
#endif


#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosalloc.h"
#include "../kram/sosdate.h"
#include "../kram/sosprof.h"

#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#else
#   include <unistd.h>              // read(), write(), close()

#   include <dlfcn.h>               // dladdr()
#endif

#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "../zschimmer/zschimmer.h"
#include "../zschimmer/file.h"

using namespace std;
namespace sos {



#ifdef SYSTEM_WIN
    string filename_of_hinstance( HINSTANCE hinstance );
#endif


#if !defined SYSTEM_EXCEPTIONS

/*
void Exceptions_aborts::operator= ( const void* );
{
    SHOW_MSG( "Unbekannte Exception in << ". Abbruch." );
    exit(9999);
}
*/

void Exception_aborts::operator= ( const Xc* x )
{
    SHOW_MSG( "Abbruch wegen Exception " << *x
              << "\nin " << _filename << ", Zeile " << _lineno << "(C++-Compiler kennt keine Exceptions)"
            /*<< "\ntry-Block in << __try_source_filename << ", Zeile " << __try_lineno*/ );
    exit(9999);
}

#endif
//----------------------------------------------------------------------------zero_return_value
#if defined __BORLANDC__ && defined SYSTEM_WIN16
#pragma option -k

void zero_return_value( uint size )
{
    __asm  push es;
    __asm  push di;
    __asm  push ax;
    __asm  push bx;
    __asm  push cx;
    __asm  mov bx, ss:[bp];
    __asm  and bx, -2;
    __asm  les di, ss:[bx+6];
    __asm  mov al, 0;
    __asm  mov cx, size;
    __asm  rep stosb;
    __asm  pop cx;
    __asm  pop bx;
    __asm  pop ax;
    __asm  pop di;
    __asm  pop es;
}
#pragma option -k.
#endif

//--------------------------------------------------------------------delayed_loading_dll_notify
#if defined SYSTEM_MICROSOFT

#include <Delayimp.h>

ostream& operator << ( ostream& s, const DelayLoadInfo dli )
{
    s << dli.szDll;                                          // name of dll
    s << ", ";
    if( dli.dlp.fImportByName )  s << dli.dlp.szProcName;    // name of procedure
                           else  s << dli.dlp.dwOrdinal;     // ordinal of procedure
    s << ", ";
    if( dli.hmodCur )  s << (void*)dli.hmodCur << ' ' << dli.hmodCur << "  " << file_version_info( filename_of_hinstance( dli.hmodCur ) );  // the hInstance of the library we have loaded
                 else  s << "NULL";
    return s;
}


extern "C"
FARPROC WINAPI delayed_loading_dll_notify( uint dliNotify, DelayLoadInfo* pdli )
{
    const char* notify = "?";

    switch( dliNotify ) 
    {
        case dliStartProcessing:		// used to bypass or note helper only
          //notify = "Start Processing";
          //break;
            return NULL;

        case dliNotePreLoadLibrary:     // called just before LoadLibrary, can override w/ new HMODULE return val
            notify = "LoadLibrary     ";
            break;

        case dliNotePreGetProcAddress:  // called just before GetProcAddress, can override w/ new FARPROC return value
            notify = "GetProcAddress  ";
            break;

        case dliFailLoadLib:            // failed to load library, fix it by returning a valid HMODULE
            throw_mswin_error( pdli->dwLastError, "LoadLibrary (delayed loading)", pdli->szDll );
            break;

        case dliFailGetProc:            // failed to get proc address, fix it by returning a valid FARPROC
        {
            Sos_string name;
            if( pdli->dlp.fImportByName )  name = pdli->dlp.szProcName;
                                     else  name = as_string( pdli->dlp.dwOrdinal );

            throw_mswin_error( pdli->dwLastError, "GetProcAddress (delayed loading)", c_str(name) );
        }
        break;

        case dliNoteEndProcessing:      // called after all processing is done, no
          //notify = "End Processing  ";
          //break;
            return NULL;

        default: ;
    }

    //if( sos_static_ptr() && sos_static_ptr()->_valid )    // Wenn nach Programmende eine DLL geladen wird, knallt's in iostream (Microsoft) jz 21.5.03
    {
        LOG( "delayed_loading_dll_notify " << notify << ", " << *pdli << '\n' );
    }

    return NULL;
}

extern "C" PfnDliHook __pfnDliFailureHook = delayed_loading_dll_notify;
extern "C" PfnDliHook __pfnDliNotifyHook  = delayed_loading_dll_notify;

#endif
//-----------------------------------------------------------------------------version_info
#ifdef SYSTEM_WIN

    string version_info_detail( void* info, const char* field, bool with_field_name )
    {
        struct Lang_and_codepage 
        {
             WORD wLanguage;
             WORD wCodePage;
        }* trans;

        uint  len;
        BOOL  ok;
        char  buffer [100];
        char* p;

        ok = VerQueryValue( info, "\\VarFileInfo\\Translation", (void**)&trans, &len );
        if( !ok )  return "";

        sprintf( buffer, "\\StringFileInfo\\%04x%04x\\", trans->wLanguage, trans->wCodePage );
        string name = buffer;
        name += field;

        ok = VerQueryValue( info, (char*)name.c_str(), (void**)&p, &len ); 
        if( !ok )  return "";

        if( len > 0  &&  p[len-1] == 0 )  len--;

        string value ( p, len / sizeof *p );

        return with_field_name? string(field) + "=" + value
                              : value;
    }
#endif

//--------------------------------------------------------------------------file_version_info

string file_version_info( const string& filename )
{
#   ifdef SYSTEM_WIN

        BOOL ok;
        DWORD w = 0;
        int size = GetFileVersionInfoSize( (char*)filename.c_str(), &w );
        if( size > 0 )
        {
            void* info = malloc( size );

            ok = GetFileVersionInfo( (char*)filename.c_str(), NULL, size, info );
            if( !ok )  return "";

            string result = version_info_detail( info, "ProductName", true ) + " " +
                            version_info_detail( info, "ProductVersion", true ) + " " + 
                            version_info_detail( info, "OriginalFilename", true );

            free(info);

            return result;
        }
            
#   endif

    return "";
}

//------------------------------------------------------------------------filename_of_hinstance
#ifdef SYSTEM_WIN

string filename_of_hinstance( HINSTANCE hinstance )
{
    char path[ MAXPATH+1 ];

    int len = GetModuleFileName( hinstance, (LPSTR)path, sizeof path );
    if( !len )  throw_mswin_error( "GetModuleFileName" );

    return path;
}

#endif
//-----------------------------------------------------------------------------program_filename

Sos_string program_filename()
{
#   if defined SYSTEM_WIN
        return filename_of_hinstance( (HINSTANCE)0 );
#   elif 1
        return "";
#   else
        extern char** _argv;
        return _argv && _argv[0]? _argv[ 0 ] : "";
#   endif
}

//------------------------------------------------------------------------------module_filename

Sos_string module_filename()
{
#   if defined SYSTEM_WIN

        extern HINSTANCE _hinstance;
        return filename_of_hinstance( _hinstance );

#   else
#       if defined SYSTEM_HPUX || defined Z_AIX

            return "";

#        else

            Dl_info info;
            memset( &info, 0, sizeof info );

            int err = dladdr( (void*)&module_filename, &info );

            return !err && info.dli_fname? string(info.dli_fname) 
                                         : program_filename();

#       endif
#   endif
}

//----------------------------------------------------------------------------print_all_modules

void print_all_modules( ostream* s )
{
#   ifdef SYSTEM_WIN

        OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
        GetVersionEx( &v );
        if( v.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS  ||  v.dwMajorVersion >= 5 )     // Windows >= 95  oder Windows >= 2000? (Nicht NT bis 4.0)
        {
            try
            {
                HINSTANCE lib = LoadLibrary( "kernel32.dll" );
                if( lib )
                {
                    typedef HANDLE (WINAPI *Proc1)(DWORD,DWORD); 
                    typedef BOOL   (WINAPI *Proc )(HANDLE,LPMODULEENTRY32);

                    Proc1 createsnapshot = (Proc1)GetProcAddress( lib, "CreateToolhelp32Snapshot" );
                    Proc  module32first  = (Proc) GetProcAddress( lib, "Module32First" );
                    Proc  module32next   = (Proc) GetProcAddress( lib, "Module32Next" );

                    if( createsnapshot && module32first && module32next )
                    {
                        HANDLE        snapshot = createsnapshot( TH32CS_SNAPMODULE, 0 );
                        MODULEENTRY32 e;
                        char          buffer [ 2 * MAX_MODULE_NAME32 + 50 ];
                        memset( &e, 0, sizeof e );
                        e.dwSize = sizeof e;

                        BOOL ok = module32first( snapshot, &e );

                        while(1) {
                            e.szModule[ sizeof e.szModule - 1 ] = '\0';
                            sprintf( buffer, "%-12s HMODULE=%8X ", e.szModule, e.hModule );
                            *s << buffer << setw(40) << left << e.szExePath << setw(0) << "  " << file_version_info( e.szExePath ) << '\n';
                            ok = module32next( snapshot, &e );
                            if( !ok )  break;
                        }

                        CloseHandle( snapshot );
                    }

                    FreeLibrary( lib );
                }
            }
            catch( const Xc& x ) { *s << "print_all_modules: " << x << '\n'; }
        }
    
#   endif
}

//----------------------------------------------------------------------------directory_of_path
// Liefert alles außer dem Datennamen und Schrägstrich

Sos_string directory_of_path( const Sos_string& path )
{
    const char* p0 = c_str( path );
    const char* p  = p0 + length( path );

    if( p > p0 )
    {
#       if defined SYSTEM_WIN
            if( length(path) >= 2  &&  p0[1] == ':' )  p0 += 2;
            while( p > p0  &&  p[-1] != '/'  &&  p[-1] != '\\'  &&  p[-1] != ':' )  p--;
            if( p > p0+1  &&  ( p[-1] == '/' || p[-1] == '\\' ) )  p--;
#        else
            while( p > p0  &&  p[-1] != '/' )  p--;
            if( p > p0+1  &&  p[-1] == '/' )  p--;
#       endif
    }

    return path.substr( 0, p - c_str(path) );
}

//-----------------------------------------------------------------------------filename_of_path
// Liefert den Dateinamen von "pfad/dateiname"

string filename_of_path( const string& path )
{
    const char* p0 = path.c_str();
    const char* p  = p0 + path.length();

#   if defined SYSTEM_WIN
        while( p > p0  &&  p[-1] != ':'  &&  p[-1] != '/'  &&  p[-1] != '\\' )  p--;
#    else
        while( p > p0  &&  p[-1] != '/' )  p--;
#   endif

    return p;
}

//-----------------------------------------------------------------------------basename_of_path
// Liefert den Basisnamen  "pfad/basisname.ext"

Sos_string basename_of_path( const Sos_string& path )
{
    const char* p0 = c_str( path );
    const char* p2 = p0 + length( path );
    const char* p1;

#   if defined SYSTEM_WIN
        while( p2 > p0  &&  p2[-1] != '.'  &&  p2[-1] != '/'  &&  p2[-1] != '\\' )  p2--;
        while( p2 > p0  &&  p2[-1] == '.' )  p2--;
        p1 = p2;
        while( p1 > p0  &&  p1[-1] != '/'  &&  p1[-1] != '\\' )  p1--;
#   else
        while( p2 > p0  &&  p2[-1] != '.'  &&  p2[-1] != '/' )  p2--;
        while( p2 > p0  &&  p2[-1] == '.' )  p2--;
        p1 = p2;
        while( p1 > p0  &&  p1[-1] != '/' )  p1--;
#   endif

    return as_string( p1, p2 - p1 );
}

//----------------------------------------------------------------------------extension_of_path
// Liefert die Extension "pfad/basisname.ext"

Sos_string extension_of_path( const Sos_string& path )
{
    const char* p0 = c_str( path );
    const char* p2 = p0 + length( path );

    while( p2 > p0  &&  p2[-1] == ' '  )  p2--;
    const char* p1 = p2;

#   if defined SYSTEM_WIN
        while( p1 > p0  &&  p1[-1] != '.'  &&  p1[-1] != '/'  &&  p1[-1] != '\\' )  p1--;
#    else
        while( p1 > p0  &&  p1[-1] != '.'  &&  p1[-1] != '/' )  p1--;
#   endif

    if( p1[-1] != '.' )  p1 = p2;
    return as_string( p1, p2 - p1 );
}

//----------------------------------------------------------------------make_absolute_filename

string make_absolute_filename( const string& dir, const string& filename )
{
    if( is_absolute_filename( filename )  ||  dir.empty()  ||  filename.empty() )  return filename;

    string fn = dir;
    if( fn == "." )
    {
        char* working_dir = getcwd(NULL,0);
        fn = working_dir? working_dir : "";
        free( working_dir );
        if( fn.empty() )  return filename;
    }
    
    char last = fn[ fn.length() - 1 ];
    
    if( last != '/'  &&  last != '\\' ) 
    {
        if( strchr( fn.c_str(), '\\' ) )  fn += '\\';
                                    else  fn += '/';
    }

    return fn + filename;
}

//--------------------------------------------------------------------------------get_temp_path

//string get_temp_path()
//{
//#   ifdef SYSTEM_WIN
//
//        char buffer [_MAX_PATH+1];
//
//        int len = GetTempPath( sizeof buffer, buffer );
//        if( len == 0 )  throw_mswin_error( "GetTempPath" );
//
//        return string( buffer, len );
//
//#    else
//
//        char* tmp = getenv( "TMP" );
//        if( !tmp || !tmp[0] )  tmp = "/tmp";
//        return tmp;
//
//#   endif
//}
//----------------------------------------------------------------------------------sos_mkstemp

int sos_mkstemp( const string& name )
{
    int file;

#   ifdef SYSTEM_WIN

        char tmp_filename [1024+1];

        string path = zschimmer::file::get_temp_path();
        int ret = GetTempFileName( path.c_str(), name.c_str(), 0, tmp_filename );
        if( ret == 0 )
        {
            string p = path + "\\" + name + "????.tmp";
            throw_mswin_error( "GetTempFileName", p.c_str() );
        }

        LOG( "sos_mkstemp create " << tmp_filename << '\n' );

        int oflag = O_TEMPORARY | _O_SHORT_LIVED | O_SEQUENTIAL;
        int pmode = S_IREAD | S_IWRITE;

        file = ::sopen( tmp_filename, oflag | O_CREAT | O_RDWR | O_APPEND | O_BINARY, _SH_DENYRW, pmode );
        if( file == -1 )  throw_errno( errno, "open" );

#    else

        string filename;

        if( strchr( name.c_str(), '/' ) )  filename = name;
                                     else  filename = zschimmer::file::get_temp_path() + "/" + name;

        filename += ".XXXXXX";

        file = mkstemp( (char*)filename.c_str() );        // <D6>ffnet die Datei, aber ohne O_APPEND
        LOG( "sos_mkstemp create " << filename << '\n' );

        int ret = unlink( filename.c_str() );                    // Namen sofort wieder l<F6>schen
        if( ret == -1 )  throw_errno( errno, "unlink", filename.c_str() );

#   endif

    return file;
}

//------------------------------------------------------------------------------call_for_linker
// Damit *o eingebunden wird

void call_for_linker( const void* )
{
}

//-------------------------------------------------------------------------------_check_pointer

void _check_pointer( const void* ptr, uint length, const char* info )
{
    if( ptr == NULL )  return;
    if( length == 0 )  return;

#   if defined SYSTEM_WIN16
#       define CODE_OK
        uint segment_size;

        HGLOBAL handle = (HGLOBAL)GlobalHandle( (uint)( ((uint4)ptr) >> 16 ) );
        //HGLOBAL handle = GlobalHandle( ptr );
        if( !handle )  goto FEHLER;

        segment_size = GlobalSize( handle );
        if( (((uint4)ptr)  & 0xFFFF ) + length > segment_size )  goto FEHLER;

        return;
#   elif defined SYSTEM_WIN32
#       define CODE_OK
        if( IsBadWritePtr( (void*)ptr, length ) )  goto FEHLER;
#   endif

    return;


#   ifdef CODE_OK
      FEHLER:
        SHOW_ERR( "Programmfehler - Ungültige Adresse " << (void*)ptr << ' ' << info
                  << "\nBitte beenden Sie sofort die Anwendung!" );
        throw_xc( "SOS-INVALID-POINTER" );
#   endif
}

//-----------------------------------------------------------------------_check_string0_pointer

void _check_string0_pointer( const char* ptr, const char* info )
{
    uint len = 1;

#   if defined SYSTEM_WIN32
#       define CODE_OK
        if( IsBadStringPtr( ptr, INT_MAX ) )  goto FEHLER;
        len = strlen( ptr );
#   endif

    check_pointer( ptr, len, info );
    return;

#   ifdef CODE_OK
      FEHLER:
        SHOW_ERR( "Programmfehler - Ungültige String-Adresse " << (void*)ptr << ' ' << info
                  << "\nBitte beenden Sie sofort die Anwendung!" );
        throw_xc( "SOS-INVALID-POINTER" );
#   endif
}

//-------------------------------------------------------------------------------------sos_time

time_t sos_time( time_t *timer )
{
    static Bool     profile_read = false;
    static time_t   offset       = 0;
    time_t          tim          = time( timer );

    if( !profile_read )
    {
        profile_read = true;
        try {
            Sos_string  today_string;
            Sos_date    date;
            struct tm   t;

            today_string = read_profile_string( "", "debug", "today" );
            if( !empty( today_string ) )
            {
                date.assign( today_string );

                t.tm_year  = date.year();
                t.tm_mon   = date.month()- 1;
                t.tm_mday  = date.day();
                t.tm_hour  = 0;
                t.tm_min   = 0;
                t.tm_sec   = 0;
                t.tm_isdst = 0;

                offset = mktime( &t ) - tim / ( 24L*60L*60L ) * ( 24L*60L*60L );
            }
        }
        catch( const Xc& x )
        {
            SHOW_MSG( "Fehlerhafter Eintrag in der sos.ini: [debug] date=\n" << x );
        }
    }

    tim += offset;

    if( timer )  *timer = tim;
    return tim;
}

//---------------------------------------------------------------------------------get_cpu_time

double get_cpu_time()
{
#   ifdef SYSTEM_WIN    

        int64 CreationTime; // process creation time
        int64 ExitTime;     // process exit time
        int64 KernelTime;   // process kernel-mode time
        int64 UserTime;     // process user-mode time

        BOOL ok = GetProcessTimes( GetCurrentProcess(), (LPFILETIME)&CreationTime, (LPFILETIME)&ExitTime, 
                                                        (LPFILETIME)&KernelTime, (LPFILETIME)&UserTime );
        if( !ok )  return 0;
        return ( KernelTime + UserTime ) / 1e7;

#    else

/*
        struct tms  
        {
            clock_t tms_utime;  // user time 
            clock_t tms_stime;  // system time 
            clock_t tms_cutime; // user time of children
            clock_t tms_cstime; // system time of children
        };
*/

        // Note that the time can wrap around.   On  a  32bit  system
        // where  CLOCKS_PER_SEC  equals  1000000  this function will
        // return the same value approximately every 72 minutes.

        static double last_times   [4] = { 0.0, 0.0, 0.0, 0.0 };
        static double missing_times[4] = { 0.0, 0.0, 0.0, 0.0 };
        double        tm           [4];
        int           i;
        double        result = 0.0;

        struct tms t;
        times( &t );
        
        tm[0] = t.tms_utime;
        tm[1] = t.tms_stime;
        tm[2] = t.tms_cutime;
        tm[3] = t.tms_cstime;

        for( i = 0; i < 4; i++ )
        {
            if( tm[i] < last_times[4] )  missing_times[i] += ( ((double)UINT_MAX) + 1 ) / (double)CLOCKS_PER_SEC;
            last_times[i] = tm[i];
            result += tm[i] + missing_times[i];
        }

        return result;

#   endif
}
//-----------------------------------------------------------------------------operator << int64
#if defined SYSTEM_INT64

ostream& operator << ( ostream& s, const int64& i )
{
    char buffer [ 50 ];
    sprintf( buffer, "%" PRINTF_LONG_LONG "d", i );
    s << buffer;
    return s;
}

ostream& operator << ( ostream& s, const uint64& i )
{
    char buffer [ 50 ];
    sprintf( buffer, "%" PRINTF_LONG_LONG "u", i );
    s << buffer;
    return s;
}

#endif

}
