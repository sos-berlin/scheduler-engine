#include <precomp.h>
//#define MODULE_NAME "sosddeap"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

//#define PROBEJAHR 1995

#if defined PROBEJAHR
#   include <time.h>
#endif

#include <sos.h>

#if defined SYSTEM_STARVIEW  &&  defined SYSTEM_WIN 
#   include <svwin.h>
#   include <sv.hxx>
#   include <sysdep.hxx>
#endif

#include <xception.h>
#include <soswin.h>

#include <sosappl.h>
#include <sosstrng.h>
#include <sosopt.h>
#include <log.h>
#include <filedde.h>
#include <logwin.h>
#include <sosdde.hrc>

//#define SOSDDE_TRACE_WINDOW 0

HINSTANCE _hinstance;
extern Bool sossock_reschedule;



struct Sos_dde_application : Sos_application
{
  protected:
    void                       _main                    ( int, char*[] );
};


Sos_dde_application sos_application;

void Sos_dde_application::_main( int argc, char** argv )
{
    Sos_appl    appl            ( false );
    Sos_string  dde_server_name = "hostDDE";
    Bool        trace           = true;

    Trace_window* win_ptr = NULL;

    //sossock soll nicht DDE-Anforderungen durchlassen, dass wird rekursiv und führt zum Absturz. jz 21.8.97 (???)
    sossock_reschedule = true;  // Windows-Botschaften können uns nicht irritieren. Oder doch? jz 21.8.97

#   if defined PROBEJAHR
    {
        int ___PROBEVERSION_MIT_BEGRENZTER_LAUFZEIT___;
        time_t t;
        time( &t );
        const char* ct = ctime( &t );
        if( ct[ 23 ] != PROBEJAHR - 1990 + '0' )  {
            SHOW_ERR( "Die kostenlose Probezeit dieses Programms ist abgelaufen.\n"
                      "SOS GmbH Berlin, Telefon (030) 864 790-0" );
            return;
        }
    }
#   endif

try {
    appl.init();

    for ( Sos_option_iterator opt( argc, argv ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "log" ) )   log_start( c_str( opt.value() ) );
        else
        if( opt.flag( 't', "trace" ) )  trace = opt.set();
        else
        if( opt.param() ) { dde_server_name = opt.value(); opt.max_params(1); }
        else throw Sos_option_error( opt );
    }

    File_dde_server  file_dde_server ( c_str(dde_server_name) );

#   if defined LOGWIN_USE_STARVIEW
        win_ptr = new Trace_window( NULL, WB_APP|WB_STDWORK|WB_SIZEMOVE|WB_MINMAX );
        if ( !win_ptr ) throw_no_memory_error();

        win_ptr->ChangeIcon( Icon( ResId( ICON_SOSDDE )));
        win_ptr->SetText( String( "DDE-Server " ) + String( dde_server_name ) );
        win_ptr->Show(); // Fenster anzeigen
#   else
        win_ptr = new Trace_window;
        if ( !win_ptr ) throw_no_memory_error();
#   endif

    if ( trace )
    {
        file_dde_server.log_stream_ptr( ((Trace_window*)win_ptr)->ostream_ptr() );
        *(((Trace_window*)win_ptr)->ostream_ptr()) << "DDE-Server " << dde_server_name << " gestartet." << endl;
    } else {
        file_dde_server.log_stream_ptr( log_ptr );
        LOG( "DDE-Server " << dde_server_name << " gestartet." << endl );
    }


    execute();

    delete win_ptr;
    //sos_static_ptr()->~Sos_static();
}
catch( const Xc& x )
{
    SHOW_MSG( x );

    delete win_ptr;

    sos_static_ptr()->~Sos_static();
    throw;
}
}


