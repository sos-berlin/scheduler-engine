// $Id$

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>



int main( int argc, char** argv )
{
    int         err;
    struct stat directory_stat;
    struct stat setuid_stat;
    int         effective_user_id  = geteuid();
    int         effective_group_id = getegid();


    // argv[0] soll absolut sein, damit's keinen Zweifel über das Verzeichnis unseres Programms gibt.
    if( argc <= 1  ||  argv[0][0] != '/' )  { fprintf( stderr, "usage: /.../setuid programpath arguments...\n" ); return 255; }



    //const char* login_name = getlogin();
    //if( !login_name )  { fprintf( stderr, "getlogin() returns errno=%d %s\n", errno, strerror( errno ) ); return 255; }

    const char* setuid_filename = strrchr( argv[ 0 ], '/' ) + 1;
    char*       directory       = strdup( argv[0] );
    strrchr( directory, '/' )[ 1 ] = '\0';


    // Effektive User-Id und Group-Id zurücksetzen, damit wir Zugriff aufs Verzeichnis von setuid haben.
#   ifdef sparc
        seteuid( getuid() );
        setegid( getgid() );
#   else
        setresuid( -1, getuid(), -1 );
        setresgid( -1, getgid(), -1 );
#   endif


    // chmod-Bits von setuid prüfen: Darf für group und others nicht kopierbar oder beschreibbar sein.
    err = stat( argv[0], &setuid_stat );
    if( err )  { fprintf( stderr, "stat(\"%s\") returns errno=%d %s\n", argv[0], errno, strerror( errno ) );  return 255; }
    if( setuid_stat.st_mode & 066 ) { fprintf( stderr, "%s must not be readable for group or other. Do: chmod go-rw %s\n", setuid_filename, argv[0] );  return 255; }


    // chmod-Bits des Verzeichnisses von setuid prüfen: Darf für group und others nicht lesbar, beschreibbar oder ausführbar sein.

    err = stat( directory, &directory_stat );
    if( err )  { fprintf( stderr, "stat(\"%s\") returns errno=%d %s\n", directory, errno, strerror( errno ) );  return 255; }

    if( directory_stat.st_uid != getuid() )  { fprintf( stderr, "%s's directory is not yours\n", setuid_filename );  return 255; }
    if( directory_stat.st_mode & 077 )       { fprintf( stderr, "%s's directory must not be readable for group or others. Do: chmod go-rwx %s\n", setuid_filename, directory );  return 255; }


    // Effektive User-Id und Group-Id von Programmstart setzen

    err = setuid( effective_user_id );
    if( err )  { fprintf( stderr, "setuid(%d) returns errno=%d %s\n", effective_user_id, errno, strerror( errno ) ); return 255; }

    err = setgid( effective_group_id );
    if( err )  { fprintf( stderr, "setgid(%d) returns errno=%d %s\n", effective_group_id, errno, strerror( errno ) ); return 255; }


    // argv[1] wird argv[0], also das zu startende Programm.

    int    my_argc = argc - 1;
    char** my_argv = (char**)malloc( (my_argc+1) * sizeof (char*) );

    int i;
    for( i = 0; i < my_argc; i++ )  my_argv[ i ] = argv[ 1+i ];
    my_argv[ my_argc ] = NULL;


    execv( my_argv[ 0 ], my_argv );


    fprintf( stderr, "execv(\"%s\",argv) returns errno=%d %s\n", my_argv[0], errno, strerror( errno ) );
    return 255;
}
