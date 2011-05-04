#define MODULE_NAME "testfs"

#include <sysdep.h>

#if defined SYSTEM_UNIX
#   include <errno.h>
#   include <sys/resource.h>
#endif

#include <sos.h>
//#include <filedde.h>
#include <anyfile.h>
#include <log.h>
#include <msec.h>
#include <sosopt.h>
#include <sosarray.h>


int sos_main( int argc, char** argv )
{
    int4 count = 1;
    int4 first = 1;
    Sos_string server = "tcp:bs2/4002";
    int4 i;

    Sos_option_iterator opt ( argc, argv );
    for(; !opt.end(); opt.next() ) {
        if( opt.param( 1 ) )  count = as_int4( c_str( opt.value() ) );
        else
        if( opt.param( 2 ) )  first = as_int4( c_str( opt.value() ) );
        else
        if( opt.with_value( "server"   ))  server = opt.value();
        else
        if( opt.with_value( "log"   ))  log_start( opt.value() );
        else
        if( opt.with_value( "erase"  ))  { Sos_string fn = opt.value(); Any_file::erase( c_str( fn ) ); }
        else {
            SHOW_ERR( "usage: testfs N" );
            return 9999;
        }
    }

    //File_dde_server  file_dde_server ( "sos_fileserver" );
/*
#   if defined SYSTEM_UNIX
        struct rlimit limit;
        int    rc;
        rc = getrlimit( RLIMIT_NOFILE, &limit );
        if( rc == -1 )  throw_errno( errno );
        limit.rlim_cur = limit.rlim_max;
        rc = setrlimit( RLIMIT_NOFILE, &limit );
        if( rc == -1 )  throw_errno( errno );
#   endif

    int4 start_time = elapsed_msec();
    Sos_simple_array<Any_file> file_array;

    file_array.first_index( 1 );
    file_array.last_index( count );

    try {
        for( i = 1; i <= count; i++ )
        {
#           if defined SYSTEM_UNIX
                cerr << ( first + i ) << ' ';
#           endif
            file_array[ i ].open( "fs: -server=" + server + " -user=X" + as_string( first + i ) + " | a",
                                  Any_file::Open_mode( Any_file::in ) );
        }
    }
    catch( const Xc& x ) {
        SHOW_MSG( x );
    }

    SHOW_MSG( '\n' << (i-1) << " Verbindungen geöffnet.\n" );

#   if defined SYSTEM_UNIX
        cerr << "[mit Enter geht's weiter]";
        cin.get();
        cerr << "\n";
#   endif;
*/
    Any_file f;
  //f.open( "fs -server=bs2/4002 | com:$rzframe.film.adressen.datei in share", Any_file::in );
    f.open( "fs -server=bs2/4002 | com:external:$e.rapid.lib(testextr in kl=12)", Any_file::in );
    int4 start_time = elapsed_msec();
    int4 len = 0;
    for( i = 1; i < count; i++ ) {
        Dynamic_area record;
        f.get_key( &record, Const_area( "SOS         " ) );
        len = record.length();
#       if defined SYSTEM_UNIX
            //cerr << '.';
#       endif
    }

    int4 t = elapsed_msec() - start_time;
    SHOW_MSG( i << " Getkeys auf einen Satz zu " << len << " Bytes in " << t << "ms, also " << ( t / i ) << "ms pro Operation." );

    return 0;
}

