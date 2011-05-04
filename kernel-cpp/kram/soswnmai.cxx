// $Id$

#include "precomp.h"
//#define MODULE_NAME "soswnmai"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/* Applikation, die main() ruft.
   Mit Fenster.

   Nicht in einer Bibliothek halten. Explizit einbinden!
*/

#include "sysdep.h"

//#if defined __BORLANDC__
//#   include <cstring.h>
//#   include <except.h>
//#elif defined SYSTEM_SOLARIS
//#   include <exception.h>
//#endif
#include "sysxcept.h"

#include "sosstrng.h"
#include "sos.h"
#include "sosappl.h"    // Sos_static
#include "sosprog.h"    // MFC
#include "log.h"

#if defined SYSTEM_STARVIEW
#   include "soswin.h"
#endif


namespace sos 
{
    extern int sos_main0( int argc, char** argv );
    extern Bool sos_gui;
}


#if defined SYSTEM_WIN

namespace sos 
{
    HINSTANCE _hinstance = 0;     //jz 25.7.96   wird von sossv.cxx oder WinMain() gesetzt

#   if defined SYSTEM_STARVIEW

        struct Sos_standard_application : Sos_application
        {
            virtual void _main( int, char*[] );
        };


        void Sos_standard_application::_main( int argc, char* argv[] )
        {
            sos_main0( argc, argv );
        }

        Sos_standard_application sos_application;
#   else
        //int       _argc;
        //char**    _argv;

//--------------------------------------------------------------------Sos_program::InitInstance

BOOL Sos_program::InitInstance()
{
    _hinstance = AfxGetInstanceHandle();

    char  progname [ _MAX_PATH ];
    char* cmd_line = strdup( c_str( m_lpCmdLine ) );
    char* p        = cmd_line;

    _argv = (char**) malloc( sizeof(char**) * 1000 );

    progname[ 0 ] = '\0';
    progname[ sizeof progname - 1 ] = '\0';
    GetModuleFileName( _hinstance, progname, sizeof progname );
    _argv[ 0 ] = progname;
    _argc = 1;

    while( *p  &&  _argc < 1000 )
    {
        _argv[ _argc++ ] = p;
        p = strchr( p, ' ' );
        if( !p )  break;
        while( *p == ' ' )  p++;
        p[-1] = '\0';
    }

    //?ohne catch jz 8.3.97: _sos_appl.init();

    sos_main0( _argc, _argv );   // Ruft sos_main()

    return FALSE;  // MFC soll nicht Run() rufen, sondern Anwendung beenden
}

# endif

} //namespace sos

#else
/*
    namespace sos
    {
        int    _argc;
        char** _argv;
    }
*/
    extern "C" int main( int argc, char** argv )
    {
        using namespace sos;

        _argc = argc;
        _argv = argv;

        int rc = sos_main0( argc, argv );

        return rc;
    }

#endif



/*
    int       _argc;
    char**    _argv;


    int WINAPI WinMain( HINSTANCE hinstance, HINSTANCE, LPSTR lpszCmdLine, int )
    {
        _hinstance = hinstance;

        char  progname [ _MAX_PATH ];
        char* cmd_line = strdup( lpszCmdLine );
        char* p        = cmd_line;

        _argv = (char**) malloc( sizeof(char**) * 1000 );

        progname[ 0 ] = '\0';
        progname[ sizeof progname - 1 ] = '\0';
        GetModuleFileName( hinstance, progname, sizeof progname );
        _argv[ 0 ] = progname;
        _argc = 1;

        while( *p  &&  _argc < 1000 ) {
            _argv[ _argc++ ] = p;
            p = strchr( p, ' ' );
            if( !p )  break;
            while( *p == ' ' )  p++;
            p[-1] = '\0';
        }

        return sos_main0( argc, argv );
    }
*/


