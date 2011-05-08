// $Id$
// © 2004 Joacim Zschimmer

#include "zschimmer.h"
#include "log.h"
#include "system_command.h"
#include "file.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef Z_WINDOWS
#   include <windows.h>
#   include <io.h>
#else
#   include <unistd.h>
#   include <locale.h>
#   include <sys/time.h>
#   include <signal.h>
#endif

using namespace std;
using namespace zschimmer::file;


namespace zschimmer {

//--------------------------------------------------------------------------System_command::execute

/* Neuer Code für Windows, der das Kommando direkt (nicht über cmd.exe) ruft und deshalb die Anführungszeichen korrekt übergibt: 
   2007-11-14

void System_command::execute( const string& cmd )
{
    File   stderr_file;
    File   stdout_file;
    string my_cmd = cmd;

    _stderr_text = "";

    //if( cmd.find( "2>" ) == string::npos )
    //{
          stderr_file.create_temporary( File::open_unlink_later );
    //    my_cmd.append( " 2>" + stderr_file.path() );
    //}

    //
    //if( !has_regex( cmd, "[^2]>" ) )
    //{
          stdout_file.create_temporary( File::open_unlink_later );
    //    my_cmd.append( " >" + stdout_file.path() );
    //}



#   ifdef Z_WINDOWS

        // Wir nehmen nicht system(), weil das in Anführungszeichen gesetzte Parameter nicht korrekt übergibt.

        Process process;
        process.set_stdout_path( stdout_file.path() );
        process.set_stderr_path( stderr_file.path() );

        process.start( my_cmd );
        process.wait();
        _exit_code = process.exit_code();

#    else
        my_cmd.append( " 2>" + stderr_file.path() );
        my_cmd.append( " >"  + stdout_file.path() );

        Z_LOG( "signal(SIGCHLD,SIG_DFL)\n" );
        ::signal( SIGCHLD, SIG_DFL );                 // 2006-03-04 Für Suse 9.2  (Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.)

        Z_LOG( "system " << my_cmd << "\n" );

        errno = 0;
        int ret = system( my_cmd.c_str() );
        int system_errno = errno;

        _exit_code = ret == -1? -1 : WEXITSTATUS(ret);

#   endif


    if( stderr_file.path() != "" )
    {
        try {
            _stderr_text = string_from_file( stderr_file.path() );
        }
        catch( const exception& ) {}
    }

    if( stdout_file.path() != "" )
    {
        try {
            _stdout_text = string_from_file( stdout_file.path() );
        }
        catch( const exception& ) {}
    }

    try
    {
#       ifdef Z_WINDOWS
            if( _exit_code != 0 )  throw_xc( "Z-4005", as_string(_exit_code), cmd );
#        else
            if( ret == -1 )  throw_errno( system_errno, "system" );
            if( WIFSIGNALED(ret) )                      throw_xc( "Z-4006", as_string(WTERMSIG(ret)), cmd );
            if( WIFEXITED(ret)  &&  WEXITSTATUS(ret) )  throw_xc( "Z-4005", as_string(WEXITSTATUS(ret)), cmd );
#       endif
    }
    catch( const Xc& x )
    {
        _error = true;
        if( _throw_xc )  throw;
        _xc = x;
    }
}
*/

void System_command::execute( const string& cmd )
{
    File stderr_file;
    File stdout_file;
    string my_cmd = cmd;

    _stderr_text = "";

    if( cmd.find( "2>" ) == string::npos )
    {
        stderr_file.create_temporary( File::open_unlink_later );
        my_cmd.append( " 2>" + stderr_file.path() );
    }

    
    if( !has_regex( cmd, "[^2]>" ) )
    {
        stdout_file.create_temporary( File::open_unlink_later );
        my_cmd.append( " >" + stdout_file.path() );
    }


#   ifdef Z_UNIX
        Z_LOG( "signal(SIGCHLD,SIG_DFL)\n" );
        ::signal( SIGCHLD, SIG_DFL );                 // 2006-03-04 Für Suse 9.2  (Java verändert das Signal-Verhalten, so dass waitpid() ohne diesen Aufruf versagte.)
#   endif


    Z_LOG( "system " << my_cmd << "\n" );
    errno = 0;
    int ret = system( my_cmd.c_str() );
    int system_errno = errno;

#   ifdef Z_WINDOWS
        _exit_code = ret;
#    else
        _exit_code = ret == -1? -1 : WEXITSTATUS(ret);
#   endif


    if( stderr_file.path() != "" )
    {
        try {
            _stderr_text = string_from_file( stderr_file.path() );
        }
        catch( const exception& ) {}

        //unlink( stderr_file.filename().c_str() );
    }

    if( stdout_file.path() != "" )
    {
        try {
            _stdout_text = string_from_file( stdout_file.path() );
        }
        catch( const exception& ) {}

        //unlink( stdout_file.filename().c_str() );
    }

    try
    {
        if( ret == -1 )  throw_errno( system_errno, "system" );

#       ifdef Z_WINDOWS
            if( ret != 0 )  throw_xc( "Z-4005", as_string(ret), cmd );
#        else
            if( WIFSIGNALED(ret) )                      throw_xc( "Z-4006", as_string(WTERMSIG(ret)), cmd );
            if( WIFEXITED(ret)  &&  WEXITSTATUS(ret) )  throw_xc( "Z-4005", as_string(WEXITSTATUS(ret)), cmd );
#       endif
    }
    catch( const Xc& x )
    {
        _error = true;
        if( _throw_xc )  throw;
        _xc = x;
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
