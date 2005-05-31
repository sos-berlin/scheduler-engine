// $Id$

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <string>


using namespace std;


int main( int argc, char** argv )
{
    int         err;
    struct stat directory_stat;
    struct stat setuid_stat;
    int         effective_user_id  = geteuid();
    int         effective_group_id = getegid();


    // Wir brauchen ein Verzeichnis in argv[0]
    if( argc <= 1  ||  !strchr( argv[0], '/' ) )  { fprintf( stderr, "usage: ...directory/setuid programpath arguments...\n" ); return 255; }



    //const char* login_name = getlogin();
    //if( !login_name )  { fprintf( stderr, "getlogin() returns errno=%d %s\n", errno, strerror( errno ) ); return 255; }


    // Effektive Userid zurücksetzen, damit wir Zugriff aufs Verzeichnis von setuid haben.

    seteuid( getuid() );


    // chmod-Bits von setuid prüfen: Darf für group und others nicht lesbar oder beschreibbar sein.

    err = stat( argv[0], &setuid_stat );
    if( err )  { fprintf( stderr, "stat(\"%s\") returns errno=%d %s\n", argv[0], errno, strerror( errno ) );  return 255; }
    if( setuid_stat.st_mode & 066 ) { fprintf( stderr, "Do chmod go-rw %s\n", argv[0] );  return 255; }


    // chmod-Bits des Verzeichnisses von setuid prüfen: Darf für group und others nicht lesbar, beschreibbar oder ausführbar sein.

    string directory = string( argv[0], strrchr( argv[0], '/' ) - argv[0] ) + "/";
    err = stat( directory.c_str(), &directory_stat );
    if( err )  { fprintf( stderr, "stat(\"%s\") returns errno=%d %s\n", directory.c_str(), errno, strerror( errno ) );  return 255; }
    if( directory_stat.st_mode & 077 ) { fprintf( stderr, "Do chmod go-rwx %s\n", directory.c_str() );  return 255; }
    if( directory_stat.st_uid != getuid() )  { fprintf( stderr, "Do chown %d %s\n", getuid(), directory.c_str() );  return 255; }


    // argv[1] wird argv[0], also das zu startende Programm.

    int    my_argc = argc - 1;
    char** my_argv = (char**)malloc( (my_argc+1) * sizeof (char*) );

    for( int i = 0; i < my_argc; i++ )  my_argv[ i ] = argv[ 1+i ];
    my_argv[ my_argc ] = NULL;


    err = setuid( effective_user_id );
    if( err )  { fprintf( stderr, "setuid(%d) returns errno=%d %s\n", effective_user_id, errno, strerror( errno ) ); return 255; }


    err = setgid( effective_group_id );
    if( err )  { fprintf( stderr, "setgid(%d) returns errno=%d %s\n", effective_group_id, errno, strerror( errno ) ); return 255; }


    execvp( my_argv[ 0 ], my_argv );
    fprintf( stderr, "execvp(\"%s\",argv) returns errno=%d %s\n", my_argv[0], errno, strerror( errno ) );
    return 255;
}
