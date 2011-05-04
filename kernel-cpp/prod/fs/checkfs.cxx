#define MODULE_NAME "checkfs"
#define COPYRIGHT   "©1996 SOS GmbH Berlin"
#define AUTHOR      "Jörg Schwiemann"

#include <stdlib.h>
#include <errno.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/anyfile.h"
#include "../kram/log.h"

#define EX_TRUE         0
#define EX_FALSE        1
#define EX_FSUNEXPECTED 2
#define EX_USAGE        3


#define USAGE \
    SHOW_ERR( "usage: checkfs -server=<server> [-running|-notrunning] [-log=<logfile]" ); \
    return EX_USAGE

int sos_main( int argc, char** argv )
{
    Sos_string fileserver;
    Bool check_running = true; // Default ist Prüfen, ob Filserver läuft

    //sos_static_ptr()->_licence->set_par( licence_fs );                 // FS erlauben
    Sos_option_iterator opt ( argc, argv );
    for(; !opt.end(); opt.next() ) {
        if( opt.with_value( 's', "server" ))  fileserver = opt.value();
        else
        if( opt.with_value( "log"         ))  log_start( opt.value() );
        else
        if( opt.flag(       "running"     ))  check_running = opt.set();
        else
        if( opt.flag(       "notrunning"  ))  check_running = !opt.set();
        else {
            USAGE;
        }
    }

    if( empty( fileserver ) ) {
        USAGE;
    }

    Any_file f;
    try {
        // Nur für Unix-Fileserver
        Sos_string filename = as_string( "fs -s=" ) + fileserver + as_string( " /dev/null" );
        f.open( filename, File_base::out );
        f.put( "DUMMY-RECORD" );
        f.close();
    }
    catch ( const Xc& x ) {
        f.close(); // keine Exception, oder?
        if ( strcmpi( x.code(), "SOCKET-61" ) == 0 ) {
            return check_running ? EX_FALSE : EX_TRUE;
        }
        SHOW_ERR( "checkfs: unexpected error " << x );
        return EX_FSUNEXPECTED; // Unerwarteter Fehler ...
    }

    return check_running ? EX_TRUE : EX_FALSE;
}

