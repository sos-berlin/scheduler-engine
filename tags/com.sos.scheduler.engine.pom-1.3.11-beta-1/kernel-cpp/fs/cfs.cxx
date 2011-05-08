#define MODULE_NAME "cfs"

#define PORT "tcp 4001"

#include "../kram/sysdep.h"


#include <stdlib.h>     // exit()
#include <limits.h>

#if defined SYSTEM_UNIX
#   include <sys/resource.h>
#   include <errno.h>
#   include <signal.h>
#   include <unistd.h>      // exit()
#   include <stdio.h>
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/anyfile.h"
//#include "../kram/pointer.h"
#include "../kram/sosfact.h"
#include "../kram/sosarray.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../kram/soswin.h"
#include "../kram/licence.h"
#include "../kram/sosprog.h"
#include "../kram/log.h"
#include "fsman.h"

#if defined SYSTEM_STARVIEW  &&  !defined SYSTEM_UNIX
#   define BOOL SV_BOOL
#   include "../kram/logwin.h"
#   undef BOOL
#endif


using namespace std;
namespace sos {

struct Sosfs_app : Sos_program
{
    int                         main                    ( int, char** );
};


extern Bool     sossock_in_select;      // sossock.cxx   select() aktiv?
Sosfs_app       app;
Sos_program*    app_ptr = &app;

//---------------------------------------------------------------------------------------extern

extern Bool sossock_dispatch_self;  // sossock.cxx
//extern Bool sossock_reschedule;     // sossock.cxx

//------------------------------------------------------------------------------fs_run_callback
#if defined SYSTEM_WIN
/*
static void fs_run_callback( ULONG event, void* port_string )
{
    fs_run( *(Sos_string*)port_string );
}
*/
#endif
//----------------------------------------------------------------------------------------usage

static void usage()
{

#if defined SYSTEM_WIN32
    SHOW_MSG( "usage: sosfs [-port=TCPPORTNUMBER] [-log=LOGFILE] [-prot=PROTFILE]" );
#else
    SHOW_MSG( "usage: sosfs [-port=TCPPORTNUMBER] [-log=LOGFILE]" );
#endif        

}

//-------------------------------------------------------------------------------signal_handler
#if defined SYSTEM_SOLARIS

const int max_sig = 100;
static SIG_FUNC_TYP* prev_signal_handler [ max_sig + 1 ];

static void signal_handler( int sig )
{
    fprintf( stderr, "\n\n*** Signal %i ***\n", sig );

    if( !sossock_in_select )  {
        fprintf( stderr, "Der Fileserver ist gerade NICHT im Ruhezustand. Deshalb kann es bei der\n"
                         "folgenden Ausgabe zu einem Absturz kommen.\n" );
    }

    fprintf( stderr, "Angemeldete Clients:\n" );
    copy_file( "fs_status:", "file *stderr" );
    fprintf( stderr, "Ende.\n" );

    SIG_FUNC_TYP* f = prev_signal_handler[ sig ];
    //if( f == SIG_DFL )
        exit( 999 );
    //(*f)( sig );
}

#endif
//---------------------------------------------------------------------------set_signal_handler
#if defined SYSTEM_SOLARIS

void set_signal_handler( int sig, SIG_FUNC_TYP* f = signal_handler )
{
    if( signal( sig, SIG_IGN ) != SIG_IGN )  {
        if( sig >= NO_OF( prev_signal_handler ) )  throw_xc( "set_signal_handler", "Tabelle zu klein" );
        prev_signal_handler[ sig ] = signal( sig, signal_handler );
    }
}

#endif
//-------------------------------------------------------------------------------------sos_main

int Sosfs_app::main( int argc, char** argv )
{
    Sos_string port = PORT;
    Sos_string errlog;

    try {
        for ( Sos_option_iterator opt ( argc, argv ); !opt.end(); opt.next() )
        {
            if( opt.with_value( "log" ) )  log_start( c_str( opt.value() ) );
            else
#if defined SYSTEM_WIN32
            if( opt.with_value( "prot" ) )  errlog = opt.value();
            else
#endif
            if( opt.with_value( "port" ) )  port = opt.value();
            else
            if( opt.flag( "?" ) || opt.flag( "h" ) )  { usage(); return 0; }
            else throw_sos_option_error( opt );
        }
    }
    catch( const Sos_option_error& x )
    {
        SHOW_ERR( x );
        usage();
        return 1;
    }

    if( length( port ) < 4  ||  memcmp( c_str( port ), "tcp", 3 ) ) {
        port = "tcp " + port;
    }

#   if defined SYSTEM_WIN16
#       if defined SYSTEM_STARVIEW
            Auto_ptr<Trace_window> win_ptr;
            win_ptr = new Trace_window( NULL, WB_APP|WB_STDWORK|WB_SIZEMOVE|WB_MINMAX );
            if( !win_ptr )  throw_no_memory_error();
            win_ptr->SetText( "Fileserver" );
            win_ptr->Show();
            fs_log = win_ptr->ostream_ptr();
#       else
            fs_log = new ofstream( "c:/tmp/sosfs.log" );  // wird nicht freigegeben!
#       endif
     #else
            fs_log = errlog == "" ? &cerr : (ostream*) new ofstream( c_str( errlog ) );
    #endif

    sos_static_ptr()->_multiple_clients = true;

/*
    #if defined SYSTEM_WIN
        sos_application_ptr->user_event_function( fs_run_callback );
        pApp->PostUserEvent( 0, &port );
        sos_application_ptr->execute();
     #else
        fs_run( 0, 0 );
    #endif
*/

#if defined SYSTEM_SOLARIS
        set_signal_handler( SIGHUP );       //  1    Exit     Hangup (see termio(7))
        set_signal_handler( SIGINT );       //  2    Exit     Interrupt (see termio(7))
      //set_signal_handler( SIGQUIT );      //  3    Core     Quit (see termio(7))
      //set_signal_handler( SIGILL  );      //  4    Core     Illegal Instruction
      //set_signal_handler( SIGTRAP );      //  5    Core     Trace/Breakpoint Trap
      //set_signal_handler( SIGABRT );      //  6    Core     Abort
      //set_signal_handler( SIGEMT  );      //  7    Core     Emulation Trap
      //set_signal_handler( SIGFPE  );      //  8    Core     Arithmetic Exception
      //set_signal_handler( SIGKILL );      //  9    Exit     Killed
      //set_signal_handler( SIGBUS  );      //  10   Core     Bus Error
      //set_signal_handler( SIGSEGV );      //  11   Core     Segmentation Fault
      //set_signal_handler( SIGSYS  );      //  12   Core     Bad System Call
        set_signal_handler( SIGPIPE );      //  13   Exit     Broken Pipe
        set_signal_handler( SIGALRM );      //  14   Exit     Alarm Clock
        set_signal_handler( SIGTERM );      //  15   Exit     Terminated
        set_signal_handler( SIGUSR1 );      //  16   Exit     User Signal 1
        set_signal_handler( SIGUSR2 );      //  17   Exit     User Signal 2
      //set_signal_handler( SIGCHLD );      //  18   Ignore   Child Status Changed
      //set_signal_handler( SIGPWR  );      //  19   Ignore   Power Fail/Restart
      //set_signal_handler( SIGWINCH);      //  20   Ignore   Window Size Change
      //set_signal_handler( SIGURG  );      //  21   Ignore   Urgent Socket Condition
        set_signal_handler( SIGPOLL );      //  22   Exit     Pollable Event (see streamio(7))
      //set_signal_handler( SIGSTOP );      //  23   Stop     Stopped (signal)
      //set_signal_handler( SIGTSTP );      //  24   Stop     Stopped (user) (see termio(7))
      //set_signal_handler( SIGCONT );      //  25   Ignore   Continued
      //set_signal_handler( SIGTTIN );      //  26   Stop     Stopped (tty input) (see termio(7))
      //set_signal_handler( SIGTTOU );      //  27   Stop     Stopped (tty output) (see termio(7))
        set_signal_handler( SIGVTALRM);     //  28   Exit     Virtual Timer Expired
        set_signal_handler( SIGPROF );      //  29   Exit     Profiling Timer Expired
      //set_signal_handler( SIGXCPU );      //  30   Core     CPU time limit exceeded (see getrlimit(2))
      //set_signal_handler( SIGXFSZ );      //  31   Core     File size limit exceeded (see getrlimit(2))
      //set_signal_handler( SIGWAITING);    //  32   Ignore   Process's LWPs are blocked
      //set_signal_handler( SIGLWP  );      //  33   Ignore   Special signal used by thread library
#   endif

    {
        Fs_manager fs_manager( port );

        Sos_licence licence;
        licence.check();
        if( licence[ licence_fs      ] )  fs_manager._allowed_connections_count = INT32_MAX;
        else
        if( licence[ licence_fs_demo ] )  fs_manager._allowed_connections_count = max_demo_connections;
        else throw_xc( "SOS-1000", "Fileserver" );

        fs_manager.start();

#       if defined SYSTEM_WIN16
            dispatch_waiting_msg();
            sossock_dispatch_self = true;

            Sos_object dummy_object;
            Run_msg run_msg( &fs_manager, &dummy_object );
            send( &run_msg );
#           if defined SYSTEM_STARVIEW
                sos_application_ptr->execute();
#            else
                MSG msg;
                while( GetMessage( &msg, 0, 0, 0 ) ) {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
#           endif

            //if( Sos_socket::open_event_count() > 0 ) {   // Auch andere Botschaften prüfen!!! (z.B. DDE)
            //}

            //Geht nicht, wegen listening socket: fs_manager.obj_end();
#        else
            fs_manager.obj_run();
#       endif
    }

    return 0;
}

//-------------------------------------------------------------------------------------sos_main

int sos_main( int argc, char** argv )
{
    return app.main( argc, argv );
}

} //namespace sos