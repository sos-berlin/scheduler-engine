// $Id$

#include "zschimmer.h"
#include "file.h"

#ifdef Z_WINDOWS
#   include <direct.h>      // getcwd()
#else
#   include <sys/utsname.h> // uname()
#endif

namespace zschimmer {

//-----------------------------------------------------------------------------------check_compiler

bool check_compiler( ostream* log )
{
    bool ok = true;


#   ifdef __hpux
        *log << "HP-UX  ";
#   endif

#   ifdef sparc
        *log << "Sparc  ";
#   endif

#   ifdef __IA64__
        *log << "ia64 ";
#   endif

#   ifdef Z_WINDOWS
    {
        *log << "Microsoft Windows ";

        OSVERSIONINFO v;  memset( &v, 0, sizeof v ); v.dwOSVersionInfoSize = sizeof v;
        GetVersionEx( &v );

        if( v.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )  *log << "95/98/Me";
      //if( v.dwPlatformId == VER_PLATFORM_WIN32_NT      )  *log << "NT/2000/XP";
        *log << v.dwMajorVersion << '.' << v.dwMinorVersion;

        if( v.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            switch( v.dwMajorVersion ) 
            {
                case 3:  *log << "  Windows NT 3.5.1";  break;
                case 4:  *log << "  Windows NT 4.0"  ;  break;

                case 5: switch( v.dwMinorVersion ) 
                        {
                            case 0:  *log << "  Windows 2000";                 break;
                            case 1:  *log << "  Windows XP";                   break;
                            case 2:  *log << "  Windows 2003 Server family";   break;
                            default: *log << "  Windows nach 2003";
                        }
                        break;

                case 6: switch( v.dwMinorVersion ) 
                        {
                            case 0:  *log << "  Windows Vista";                 break;
                        }
                        break;

                default:;
            }
        }

        if( v.szCSDVersion )  *log << ", " << v.szCSDVersion;

        *log << ", Build " << ( v.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS? v.dwBuildNumber & 0xFFFF : v.dwBuildNumber ) << '\n';

        *log << "GetCommandLine(): " << GetCommandLine() << '\n';
    }
#   else
    {
        struct utsname u;
        int ret = uname( &u );
        if( ret != -1 )
        {
            *log << "sys="       << u.sysname 
                 << " node="     << u.nodename 
                 << " release="  << u.release 
                 << " version="  << u.version 
                 << " machine="  << u.machine 
                 << "\n";
        }
    }
#   endif


#   ifdef __GNUC__
        *log << "Compiler GNU gcc " << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__;
#   endif

#   ifdef _MSC_VER
        *log << "Compiler Microsoft Visual C++ " <<  ( _MSC_VER - 600 ) / 100 << '.' << ( _MSC_VER % 100 ) << "  (" <<_MSC_VER << ")";
#   endif

    *log << ", " __DATE__ << "\n";

    *log << "get_temp_path() => " << file::get_temp_path() << "\n";


    // Prüfen, ob string(NULL,0) exeception "attempt to create string with null pointer" auslöst.

    try
    {
//#     if _MSC_VER != 1400 || !defined( _DEBUG )  // Visual Studio 2005 bricht bei _DEBUG ab.
        // Microsoft Visual Studio 2005     (8.0.50727.42)  ändern: vc/include/xstring, Zeile 2013 auskommentieren: //_DEBUG_POINTER(_Ptr);
        // Microsoft Visual Studio 2005 SP1 (8.0.50727.762) ändern: vc/include/xstring, Zeile 2051 auskommentieren: //_DEBUG_POINTER(_Ptr);
        string a ( (const char*)NULL, (string::size_type)0 );
//#     endif
        //*log << "string(NULL,0) funktioniert!\n";
    }
    catch( const exception& x ) 
    { 
        *log << "Test: string(NULL,0) wirft Exception: " << x.what() << '\n';
        ok = false;
    }

    {
        char wd [ 1024+1 ];
        wd[0] = '\0';
        getcwd( wd, sizeof wd );
        *log << "working directory = " << wd << "\n";    
    }

    return ok;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
