#include "precomp.h"
//#define MODULE_NAME "sosmain0"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sysdep.h"
#include "../kram/sysxcept.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosappl.h"    // Sos_static
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../kram/log.h"
#include "../zschimmer/log.h"

#ifdef _MSC_VER
#   include "../zschimmer/com.h"
#   include "com_simple_standards.h"
#endif

using namespace std;
namespace sos {


extern int sos_main( int, char** );    // Wird von der Applikation implementiert.


//---------------------------------------------------------------------------------DEBUG ReportHook
#if defined _DEBUG && defined SYSTEM_WIN

int DebugHook( int, char* szOutMessage, int* retval )
{
    DWORD written;

    //if( zschimmer::Log_ptr log = "memory_leak_check" )     // Falls bei Programmende, nach ~Log_categories 
    //{
    //    *log << szOutMessage << "\n";
    //}
    //else
        WriteFile( GetStdHandle(STD_ERROR_HANDLE), szOutMessage, strlen( szOutMessage ), &written, NULL );


    *retval = FALSE;
    return FALSE;
}

#endif
//---------------------------------------------------------------------process_sos_ini_in_argc_argv

int process_sos_ini_in_argc_argv(int argc, char** argv) {
    for( int i = 1; i < argc; i++ ) {  // -sos.ini=FILENAME ?
        const char* p = argv[ i ];

        if( strncmp( p, "-sos.ini=", 9 ) == 0 ) {
            //if( !strchr( p + 9, ' ' ) )                     // Nicht bei "-sos.ini=filename bla ...", Blanks im Dateinamen nicht erlaubt
            if( p[9] == '"'  &&  p[ strlen( p ) - 1 ] == '"' ) {
                set_sos_ini_filename( string( p + 10, strlen( p + 10 ) - 1 ) );
            } else {
                set_sos_ini_filename( p + 9 );
            }
        }
    }
    return argc;
}

//----------------------------------------------------------------------------------------sos_main0

int sos_main0( int argc, char** argv )
{
#   if defined SYSTEM_WIN && defined _DEBUG
        boolean check_memory_leak = false;
        
        for( int i = 1; i < argc; i++ )   // -check-memory-leak ?
        {
            const char* p = argv[ i ];

            if( strcmp( p, "-check-memory-leak" ) == 0 ) 
            {
                check_memory_leak = true;
                _CrtSetDbgFlag( _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );   //  | _CRTDBG_CHECK_ALWAYS_DF
                _CrtSetReportHook( DebugHook );

                while( ++i < argc )  argv[ i-1 ] = argv[ i ];
                argc--;
                break;
            }
        }

#   endif

    Sos_appl appl ( false );   // init im try-Block
    int rc = 0;

    try {
        appl.init();

        #ifdef Z_DEBUG
            set_log_category_default( "memory_leak_check", true );
        #endif

        argc = process_sos_ini_in_argc_argv(argc, argv);
        rc = sos_main( argc, argv );

        #if defined SYSTEM_WIN && defined _DEBUG
            if (check_memory_leak)
                _CrtDumpMemoryLeaks();
        #endif
    }

    catch( const Xc& x )
    {
        SHOW_ERR( "Fehler " << x );
        rc = 9999;
    }

    catch( const Xc_base& )
    {
        SHOW_ERR( "Fehler " /*<< x*/ );
        rc = 9999;
    }

    catch( const zschimmer::Xc& x )
    {
        SHOW_ERR( "Fehler " << x );
        rc = 9999;
    }

    catch( const exception& x )
    {
#       if defined __BORLANDC__
            SHOW_ERR( "Exception " << __throwExceptionName
                   << "( "         << x.why().c_str()
                   << " ) in "     << __throwFileName
                   << ", Zeile "   << __throwLineNumber );
//#    elif defined SYSTEM_SOLARIS
     //     SHOW_ERR( "Unbekannte Exception: " << x.why() );
#       else
            SHOW_ERR( "Exception exception(\"" << exception_text( x ) << "\")" );
#       endif

        rc = 9999;
    }

    catch( const char* text )
    {
#       if defined __BORLANDC__
            SHOW_ERR( "Exception " << __throwExceptionName
                   << text
                   << " in "       << __throwFileName
                   << ", Zeile "   << __throwLineNumber );
#        else
            SHOW_ERR( "Exception (char*) " << text );
#       endif
    }

#ifdef _MSC_VER
    catch( const _com_error& x )
    {
        SHOW_MSG( "Fehler HRESULT=" << as_hex_string( x.Error() ) << " source=" << bstr_as_string(x.Source()) << ", " << bstr_as_string(x.Description()) );
        rc = x.Error();
    }
#endif

#if !defined _MSC_VER       // Für MSVC++ ist auch ein Absturz eine Exception
    catch( ... )
    {
#       if defined __BORLANDC__
            SHOW_ERR( "Exception " << __throwExceptionName
                      << " in "       << __throwFileName
                      << ", Zeile "   << __throwLineNumber );
#		 elif defined SYSTEM_GNU
            SHOW_ERR( "Unbekannte Exception" );
#        elif defined SYSTEM_SOLARIS
            //SHOW_ERR( "Unbekannte Exception: " << exception_name() );
            SHOW_ERR( "Unbekannte Exception" );
#        endif
        rc = 9999;
    }
#endif

    return rc;
}

} //namespace sos
