// $Id: z_signals.cxx 13199 2007-12-06 14:15:42Z jz $
#include "zschimmer.h"
#include "z_signals.h"

#ifdef Z_UNIX
#   include <signal.h>
#endif


namespace zschimmer {


#ifndef SIGHUP
#   define SIGHUP 0
#endif

#ifndef SIGINT
#   define SIGINT 0
#endif

#ifndef SIGQUIT
#   define SIGQUIT 0
#endif

#ifndef SIGILL
#   define SIGILL 0
#endif

#ifndef SIGTRAP
#   define SIGTRAP 0
#endif

#ifndef SIGABRT
#   define SIGABRT 0
#endif

#ifndef SIGIOT
#   define SIGIOT 0
#endif

#ifndef SIGBUS
#   define SIGBUS 0
#endif

#ifndef SIGFPE
#   define SIGFPE 0
#endif

#ifndef SIGKILL
#   define SIGKILL 0
#endif

#ifndef SIGUSR1
#   define SIGUSR1 0
#endif

#ifndef SIGSEGV
#   define SIGSEGV 0
#endif

#ifndef SIGUSR2
#   define SIGUSR2 0
#endif

#ifndef SIGPIPE
#   define SIGPIPE 0
#endif

#ifndef SIGALRM
#   define SIGALRM 0
#endif

#ifndef SIGTERM
#   define SIGTERM 0
#endif

#ifndef SIGSTKFLT
#   define SIGSTKFLT 0
#endif

#ifndef SIGCHLD
#   define SIGCHLD 0
#endif

#ifndef SIGCONT
#   define SIGCONT 0
#endif

#ifndef SIGSTOP
#   define SIGSTOP 0
#endif

#ifndef SIGTSTP
#   define SIGTSTP 0
#endif

#ifndef SIGTTIN
#   define SIGTTIN 0
#endif

#ifndef SIGTTOU
#   define SIGTTOU 0
#endif

#ifndef SIGURG
#   define SIGURG 0
#endif

#ifndef SIGXCPU
#   define SIGXCPU 0
#endif

#ifndef SIGXFSZ
#   define SIGXFSZ 0
#endif

#ifndef SIGVTALRM
#   define SIGVTALRM 0
#endif

#ifndef SIGPROF
#   define SIGPROF 0
#endif

#ifndef SIGWINCH
#   define SIGWINCH 0
#endif

#ifndef SIGPOLL
#   define SIGPOLL 0
#endif

#ifndef SIGIO
#   define SIGIO 0
#endif

#ifndef SIGPWR
#   define SIGPWR 0
#endif

#ifndef SIGSYS
#   define SIGSYS 0
#endif


struct Signal_name
{
    int             code;
    const char*     name;
    const char*     title;
};


const static Signal_name signal_names[] =
{
    { SIGHUP   , "SIGHUP"   , "Hangup" },
    { SIGINT   , "SIGINT"   , "Interrupt" },
    { SIGQUIT  , "SIGQUIT"  , "Quit" },
    { SIGILL   , "SIGILL"   , "Illegal instruction" },
    { SIGTRAP  , "SIGTRAP"  , "Trace trap" },
    { SIGABRT  , "SIGABRT"  , "Abort" },
    { SIGIOT   , "SIGIOT"   , "IOT trap" },
    { SIGBUS   , "SIGBUS"   , "BUS error" },
    { SIGFPE   , "SIGFPE"   , "Floating-point exception" },
    { SIGKILL  , "SIGKILL"  , "Kill, unblockable" },
    { SIGUSR1  , "SIGUSR1"  , "User-defined signal 1" },
    { SIGSEGV  , "SIGSEGV"  , "Segmentation violation" },
    { SIGUSR2  , "SIGUSR2"  , "User-defined signal 2" },
    { SIGPIPE  , "SIGPIPE"  , "Broken pipe" },
    { SIGALRM  , "SIGALRM"  , "Alarm clock" },
    { SIGTERM  , "SIGTERM"  , "Termination" },
    { SIGSTKFLT, "SIGSTKFLT", "Stack fault" },
    { SIGCHLD  , "SIGCHLD"  , "Child status has changed" },
    { SIGCONT  , "SIGCONT"  , "Continue" },
    { SIGSTOP  , "SIGSTOP"  , "Stop, unblockable" },
    { SIGTSTP  , "SIGTSTP"  , "Keyboard stop" },
    { SIGTTIN  , "SIGTTIN"  , "Background read from tty" },
    { SIGTTOU  , "SIGTTOU"  , "Background write to tty" },
    { SIGURG   , "SIGURG"   , "Urgent condition on socket" },
    { SIGXCPU  , "SIGXCPU"  , "CPU limit exceeded" },
    { SIGXFSZ  , "SIGXFSZ"  , "File size limit exceeded" },
    { SIGVTALRM, "SIGVTALRM", "Virtual alarm clock" },
    { SIGPROF  , "SIGPROF"  , "Profiling alarm clock" },
    { SIGWINCH , "SIGWINCH" , "Window size change" },
    { SIGPOLL  , "SIGPOLL"  , "Pollable event occurred" },
    { SIGIO    , "SIGIO"    , "I/O now possible" },
    { SIGPWR   , "SIGPWR"   , "Power failure restart" },
    { SIGSYS   , "SIGSYS"   , "Bad system call" },
    { 0, NULL }
};

//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-SIGNALS-001", "Unknown signal name: $1" },
    { "Z-SIGNALS-002", "Unknown signal code: $1" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_signales )
{
    add_message_code_texts( error_codes ); 
}

//----------------------------------------------------------------------------signal_code_from_name

int signal_code_from_name( const string& name, bool* unknown_in_this_os_only )
{
    int result = signal_code_from_name_or_0( name, unknown_in_this_os_only );
    if( result == 0  &&  unknown_in_this_os_only  &&  !*unknown_in_this_os_only )  throw_xc( "Z-SIGNALS-001", name );
    return result;
}

//-----------------------------------------------------------------------signal_code_from_name_or_0

int signal_code_from_name_or_0( const string& name, bool* unknown_in_this_os_only )
{
    if( !name.empty()  &&  isdigit( (unsigned char)name[ 0 ] ) )
    {
        try
        {
            return as_int( name );
        }
        catch( exception& x ) { throw_xc( "Z-SIGNALS-001", x ); }
    }

    string uname = ucase( name );

    for( const Signal_name* s = signal_names; s->name; s++ )
    {
        if( uname == s->name )  
        {
            if( unknown_in_this_os_only )  *unknown_in_this_os_only = s->code == 0;
            return s->code;
        }
    }

    if( unknown_in_this_os_only )  unknown_in_this_os_only = false;
    return 0;
}

//----------------------------------------------------------------------------signal_name_from_code
/*
string signal_name_from_code( int code )
{
    string result = signal_name_from_code_or_default( code, "" );
    if( result == "" )  throw_xc( "Z-SIGNALES-002", code );
    return result;
}
*/
//----------------------------------------------------------------------------signal_name_from_code

string signal_name_from_code( int code )
{
    if( code != 0 ) 
    {
        for( const Signal_name* s = signal_names; s->name; s++ )
        {
            if( code == s->code )  return s->name;
        }
    }

    return printf_string( "SIG%d", code );
}

//---------------------------------------------------------------------------signal_title_from_code

string signal_title_from_code( int code )
{
    if( code != 0 ) 
    {
        for( const Signal_name* s = signal_names; s->name; s++ )
        {
            if( code == s->code )  return s->title;
        }
    }

    return "";
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
