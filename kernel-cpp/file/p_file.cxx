// $Id$

#include "precomp.h"
#include "../kram/sysdep.h"
#include <limits.h>

#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#include <stdio.h>                  // fileno
#include <signal.h>
#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <windows.h>
#else
#   include <unistd.h>              // read(), write(), close()
#   include <sys/ioctl.h>          

#   if defined SYSTEM_LINUX
#       include <asm/ioctls.h>          // FIONBIO
#    else
#       if defined SYSTEM_HPUX || defined Z_AIX
#           include <sys/ioctl.h>
#        else
#           include <sys/filio.h>           // FIONBIO, Solaris
#       endif
#   endif

#   include <poll.h>

#   ifndef Z_COM
        typedef int HANDLE;
#   endif
#endif

#include <errno.h>
#include <time.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"
#include "../kram/env.h"
#include "../kram/log.h"


namespace sos {


const int    max_args            = 1+100;
const int    default_buffer_size = 4096;

//----------------------------------------------------------------------------------static

static double       default_timeout     = 60;
static string       directory;
static bool         initialized         = false;

//----------------------------------------------------------------------------------------

SOS_INIT( P_file )
{
#   ifdef SYSTEM_UNIX
        ::signal( SIGPIPE, SIG_IGN );
#   endif
}

//-----------------------------------------------------------------------------------P_file

struct P_file : Abs_file
{
                                    P_file              ();
                                   ~P_file              ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            get_record          ( Area& area );
    void                            put_record          ( const Const_area& record );

    void                            start_process       ();

    Fill_zero                      _zero_;
    double                         _timeout;
    string                         _program_path;
    string                         _command_line;
    HANDLE                         _pid;
    HANDLE                         _stdin_write;
    HANDLE                         _stdout_read;

#   ifdef SYSTEM_WIN
        ulong                       write_thread        ();
        ulong                       read_thread         ();

        HANDLE                     _write_thread_handle;
        HANDLE                     _read_thread_handle;

        Const_area                 _write_record;
        ulong                      _written_length;
        ulong                      _write_error;

        char                       _read_buffer[1];
        DWORD                      _read_length;
        ulong                      _read_error;
        bool                       _data_read;          // Daten stehen im _read_buffer

        HANDLE                     _write_event;
      //HANDLE                     _read_next_event;

        union {
            struct {
                HANDLE             _data_written_event;
                HANDLE             _data_read_event;
            };
            HANDLE                 _data_written_or_read_events[2];
        };
#    else
        int                        _argc;
        char*                      _argv[ max_args ];
#   endif
};

//------------------------------------------------------------------------------P_file_type

struct P_file_type : Abs_file_type
{
    virtual const char*         name                    () const        { return "prog"; }
  //virtual const char*         alias_name              () const        { return ""; };   

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<P_file> f = SOS_NEW( P_file );
        return +f;
    }
};

const P_file_type      _p_file_type;
const Abs_file_type&    p_file_type = _p_file_type;

//-------------------------------------------------------------------------- P_file::P_file

P_file::P_file()
:
    _zero_(this+1),
    _timeout( default_timeout ),
    _stdin_write( (HANDLE)-1 ),
    _stdout_read( (HANDLE)-1 )
{
    set_environment_from_sos_ini();
}

//--------------------------------------------------------------------------P_file::~P_file

P_file::~P_file()
{
#   ifdef SYSTEM_UNIX
        for( int i = NO_OF(_argv) - 1; i >= 0; i-- )  free( _argv[i] );
#   endif

#   ifdef SYSTEM_WIN
        if( _stdin_write != (HANDLE)-1 )  CloseHandle( _stdin_write ); 
        if( _stdout_read != (HANDLE)-1 )  CloseHandle( _stdout_read ); 

        CloseHandle( _write_event );
      //CloseHandle( _read_next_event );
        CloseHandle( _data_written_event );
        CloseHandle( _data_read_event );

        TerminateThread( _write_thread_handle, 2 );  CloseHandle( _write_thread_handle );
        TerminateThread( _read_thread_handle, 2 );   CloseHandle( _read_thread_handle );

        TerminateProcess( _pid, 127 );
        CloseHandle( _pid );
#   endif
}

//-----------------------------------------------------------------------------P_file::open

void P_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    if( !initialized ) 
    {
        directory = read_profile_string( "", "prog", "dir", "" );
        if( directory.empty() )  throw_xc( "SOS-1417" );

        default_timeout = read_profile_double( "", "prog", "timeout", default_timeout );
    }

    {
        for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
        {
            if( opt.with_value( "timeout" ) )   _timeout = opt.as_double();
            else
            if( opt.pipe() || opt.param() )  { _command_line = opt.rest(); break; }
            else throw_sos_option_error( opt );
        }
    }

#   ifdef SYSTEM_UNIX
    {
        for( Sos_option_iterator opt = _command_line; !opt.end(); opt.next() )
        {
            string arg;

            if( opt.param() )  arg = opt.value();
            else
            if( opt.flag( opt.option() ) )  arg = string("-") + opt.option() + ( opt.set()? "" : "-" );
            else 
            if( opt.with_value( opt.option() ) )  arg = string("-") + opt.option() + string("=") + opt.value();
            else
                throw_sos_option_error( opt );

            if( _argc == NO_OF(_argv) )  throw_xc( "SOS-1418", NO_OF(_argv)-1 );
            _argv[ _argc++ ] = strdup( arg.c_str() );
        }

        if( _argc == 0 )  throw_xc( "SOS-1419" );
        _program_path = directory + "/" + _argv[0];
    }
#   else
    {
        Sos_option_iterator opt = _command_line; 
        if( !opt.end()  &&  opt.param() )  _program_path = directory + "\\" + opt.value();
        _command_line = _program_path + " " + opt.rest();
    }
#   endif


    start_process();
}

//----------------------------------------------------------------------------P_file::close
#ifdef SYSTEM_UNIX

void P_file::close( Close_mode )
{
    if( _stdin_write != -1 )  ::close( _stdin_write ), _stdin_write = -1;
    if( _stdout_read != -1 )  ::close( _stdout_read ), _stdout_read = -1;
}

#endif
//-----------------------------------------------------------------------P_file::put_record
#ifdef SYSTEM_UNIX

void P_file::put_record( const Const_area& record )
{
    struct pollfd   pollfd[2];
    int             timeout_ms = int( _timeout * 1000.0 );
    const Byte*     p = record.byte_ptr();
    const Byte*     p_end = p + record.length();

    const unsigned long on = 1;
    int ret = ioctl( _stdout_read, FIONBIO, &on );   if( ret == -1 )  throw_errno( errno, "ioctl(FIONBIO)" );

    pollfd[0].fd = _stdin_write;  pollfd[0].events = POLLOUT;
    pollfd[1].fd = _stdout_read;  pollfd[1].events = POLLIN;

    while(1)
    {
        int ret = poll( pollfd, NO_OF(pollfd), timeout_ms );      // Timeout ist wirkungslos, denn es wird nicht geprüft. Noch weniger bei get_record()
        if( ret == -1 )  throw_errno( errno, "poll" );

        if( pollfd[1].fd & (POLLIN|POLLERR|POLLHUP) )  throw_xc( "SOS-1419" );      // Kollision: Programm schreibt und wir schreiben

        if( p == p_end )  break;

        int len = write( _stdin_write, p, p_end - p );
        if( len == -1 )  throw_errno( errno, "write" );

        p += len;
        if( p == p_end )  break;
    }
}

#endif
//-----------------------------------------------------------------------P_file::get_record
#ifdef SYSTEM_UNIX

void P_file::get_record( Area& area )
{
    if( _stdin_write != -1 )  ::close( _stdin_write ), _stdin_write = -1;      // Erster get()? Dann put() abschließen

    const unsigned long off = 0;
    int ret = ioctl( _stdout_read, FIONBIO, &off );   if( ret == -1 )  throw_errno( errno, "ioctl(FIONBIO)" );

    if( area.size() == 0 )  area.allocate_min( default_buffer_size );
    
    int len = read( _stdout_read, area.ptr(), area.size() );   if( len == -1 )  throw_errno( errno, "read" );
    if( len == 0 )  throw_eof_error();

    area.length( len );
}

#endif
//--------------------------------------------------------------------P_file::start_process
#ifdef SYSTEM_UNIX

void P_file::start_process()
{
    int     ret;
    int     stdin_pipe[2];
    int     stdout_pipe[2];

    stdin_pipe[0] = -1;
    stdin_pipe[1] = -1;
    stdout_pipe[0] = -1;
    stdout_pipe[1] = -1;

#   if defined SYSTEM_UNIX
        ret = access( _program_path.c_str(), X_OK );     // "Real uid", nicht "effective uid" wird verwendet. Bei NFS gibt es vielleicht Schwierigkeiten.
        if( ret < 0 )  throw_errno( errno, _argv[0] );
#   endif

    try
    {
        ret = pipe( stdin_pipe );    if( ret < 0 )  throw_errno( errno, "pipe" );
        ret = pipe( stdout_pipe );   if( ret < 0 )  throw_errno( errno, "pipe" );

        pid_t pid = fork();
    
        switch( pid )
        {
            case -1: throw_errno( errno, "fork" );

            case 0:  {  // child
                        dup2( stdin_pipe[0], 0 );
                        dup2( stdout_pipe[1], 1 );

                        // stdin, stdout und stderr offen lassen, alle anderen schließen:
                        int fdlimit = sysconf( _SC_OPEN_MAX );
                        for( int fd = 3; fd < fdlimit; fd++ )  ::close( fd++ );

                        execv( _program_path.c_str(), _argv );
                        _exit(127);
                     }

            default: {  // parent
                        ::close( stdin_pipe[0] );
                        ::close( stdout_pipe[1] );
                    
                        _stdin_write = stdin_pipe[1];
                        _stdout_read = stdout_pipe[0];

                        _pid = pid;
                     }
        }
    }
    catch( const Xc& )
    {
        ::close( stdin_pipe[0] );
        ::close( stdin_pipe[1] );
        ::close( stdout_pipe[0] );
        ::close( stdout_pipe[1] );

        throw;
    }
}

#endif
//----------------------------------------------------------------------------P_file::close
#ifdef SYSTEM_WIN

void P_file::close( Close_mode )
{
    if( _stdin_write != (HANDLE)-1 )  CloseHandle( _stdin_write ), _stdin_write = (HANDLE)-1;
    if( _stdout_read != (HANDLE)-1 )  CloseHandle( _stdout_read ), _stdout_read = (HANDLE)-1;
}

#endif
//-----------------------------------------------------------------------P_file::put_record
#ifdef SYSTEM_WIN

void P_file::put_record( const Const_area& record )
{
    BOOL   ok;
    DWORD  ret;

    // write_thread bescheid geben

    _write_record = record;
    
    ok = SetEvent( _write_event );   if(!ok) throw_mswin_error("SetEvent");


    // Warten, bis WriteFile() fertig oder ein Byte auf der Lese-Pipe eingetroffen ist:

    do ret = WaitForMultipleObjects( 2, _data_written_or_read_events, FALSE, 24*60*60*1000 );  while( ret == WAIT_TIMEOUT );
    
    if( ret == WAIT_OBJECT_0 + 1 ) 
    {
        TerminateThread( _read_thread_handle, 2 );
        _data_read = true;
        throw_xc( "SOS-1419" );         // Kollision: Programm schreibt und wir schreiben
    }
    if( ret != WAIT_OBJECT_0 )  throw_mswin_error("WaitForMultipleObjects");

    if( _write_error )  throw_mswin_error( _write_error, "WriteFile" );
}

#endif
//-----------------------------------------------------------------------P_file::get_record
#ifdef SYSTEM_WIN

void P_file::get_record( Area& area )
{
    BOOL    ok;
    DWORD   ret;
    DWORD   len;

    if( area.size() == 0 )  area.allocate_min( default_buffer_size );
    
    if( _stdin_write != (HANDLE)-1 )                                 // Erster get()? Dann put() abschließen
    {
        TerminateThread( _write_thread_handle, 2 );

        CloseHandle( _stdin_write ); 
        _stdin_write = (HANDLE)-1;

        if( !_data_read )
        {
            do ret = WaitForSingleObject( _data_read_event, 24*60*60*1000 );  while( ret == WAIT_TIMEOUT );
            if( ret != WAIT_OBJECT_0 )  throw_mswin_error( "WaitForSingleObject" );
        }
    
        if( _read_length == 0 )  throw_eof_error();
 
        area.assign( _read_buffer, _read_length );
        if( area.length() == area.size() )  return;
    }
    else
        _read_length = 0;


    ok = ReadFile( _stdout_read, area.byte_ptr() + _read_length, area.size() - _read_length, &len, NULL );   
    if(!ok) {
        if( GetLastError() == ERROR_HANDLE_EOF 
         || GetLastError() == ERROR_BROKEN_PIPE )  throw_eof_error();
        throw_mswin_error("ReadFile");
    }

    area.length( _read_length + len );
}

#endif
//----------------------------------------------------------------------------write_thread_function
#ifdef SYSTEM_WIN

static ulong __stdcall write_thread_function( void* par )
{
    return ((P_file*)par)->write_thread();
}

#endif
//------------------------------------------------------------------------------P_file::write_thread
#ifdef SYSTEM_WIN

ulong P_file::write_thread()
{
    BOOL    ok;
    DWORD   ret;

    while(1)
    {
        do ret = WaitForSingleObject( _write_event, 24*60*60*1000 );  while( ret == WAIT_TIMEOUT );
        if( ret != WAIT_OBJECT_0 )  return 1;
        
        ok = WriteFile( _stdin_write, _write_record.ptr(), _write_record.length(), &_written_length, NULL );
        _write_error = ok? 0 : GetLastError();
        
        ok = SetEvent( _data_written_event );  if(!ok) return 1;
    }

    return 0;
}

#endif
//------------------------------------------------------------------------------read_thread_function
#ifdef SYSTEM_WIN

static ulong __stdcall read_thread_function( void* par )
{
    return ((P_file*)par)->read_thread();
}

#endif
//-------------------------------------------------------------------------------P_file::read_thread
#ifdef SYSTEM_WIN

ulong P_file::read_thread()
{
    BOOL    ok;
  //DWORD   ret;
    
    //while(1)
    {
        ok = ReadFile( _stdout_read, _read_buffer, sizeof _read_buffer, &_read_length, NULL );
        _read_error = ok? 0 : GetLastError();

        ok = SetEvent( _data_read_event );   if(!ok) return 1;

      //do ret = WaitForSingleObject( _read_next_event, 24*60*60*1000 );  while( ret == WAIT_TIMEOUT );
      //if( ret != WAIT_OBJECT_0 )  return 1;
    }

    return 0;
}

#endif
//--------------------------------------------------------------------P_file::start_process
#ifdef SYSTEM_WIN

void P_file::start_process()
{
    BOOL    ok; 
    HANDLE  parents_stdin  = GetStdHandle(STD_INPUT_HANDLE); 
    HANDLE  parents_stdout = GetStdHandle(STD_OUTPUT_HANDLE); 
    HANDLE  stdin_read     = (HANDLE)0;
    HANDLE  stdin_write    = (HANDLE)0;
    HANDLE  stdout_read    = (HANDLE)0;
    HANDLE  stdout_write   = (HANDLE)0;
    DWORD   thread_id;

    try
    {
        {
            SECURITY_ATTRIBUTES security_attributes; 

            security_attributes.nLength              = sizeof security_attributes; 
            security_attributes.bInheritHandle       = TRUE;    // pipe handles are inherited. 
            security_attributes.lpSecurityDescriptor = NULL; 
 
            // The steps for redirecting child process's STDIN: 
            //     1.  Save current STDIN, to be restored later. 
            //     2.  Create anonymous pipe to be STDIN for child process. 
            //     3.  Set STDIN of the parent to be the read handle to the 
            //         pipe, so it is inherited by the child process. 
            //     4.  Create a noninheritable duplicate of the write handle, 
            //         and close the inheritable write handle. 
 
            ok = CreatePipe( &stdin_read, &stdin_write, &security_attributes, 0 );          if(!ok) throw_mswin_error("CreatePipe");
            ok = CreatePipe( &stdout_read, &stdout_write, &security_attributes, 0 );        if(!ok) throw_mswin_error("CreatePipe");

            ok = SetStdHandle( STD_INPUT_HANDLE, stdin_read );                              if(!ok) throw_mswin_error("SetStdHandle");
            ok = SetStdHandle( STD_OUTPUT_HANDLE, stdout_write );                           if(!ok) throw_mswin_error("SetStdHandle"); 

            ok = DuplicateHandle( GetCurrentProcess(), stdin_write, 
                                  GetCurrentProcess(), &_stdin_write, 0, 
                                  FALSE,   // not inherited 
                                  DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS );         if(!ok) throw_mswin_error("DuplicateHandle");

            ok = DuplicateHandle( GetCurrentProcess(), stdout_read,
                                  GetCurrentProcess(), &_stdout_read, 0,
                                  FALSE,   // not inherited 
                                  DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS );         if(!ok) throw_mswin_error("DuplicateHandle");
        }

        // Now create the child process. 
        {
            PROCESS_INFORMATION process_info; 
            STARTUPINFO         startup_info; 
 
            memset( &process_info, 0, sizeof process_info );
 
            memset( &startup_info, 0, sizeof startup_info );
            startup_info.cb = sizeof startup_info; 
 
            ok = CreateProcess( _program_path.c_str(),       // application name
                                (char*)_command_line.c_str(),       // command line 
                                NULL,          // process security attributes 
                                NULL,          // primary thread security attributes 
                                TRUE,          // handles are inherited 
                                0,             // creation flags 
                                NULL,          // use parent's environment 
                                NULL,          // use parent's current directory 
                                &startup_info, // STARTUPINFO pointer 
                                &process_info ); // receives PROCESS_INFORMATION 

            if( !ok )  throw_mswin_error( "CreateProcess", _program_path.c_str() );

            CloseHandle( process_info.hThread );
            _pid = process_info.hProcess;
        } 
     
        // After process creation, restore the saved STDIN and STDOUT. 
 
        ok = SetStdHandle( STD_INPUT_HANDLE , parents_stdin  );                             if(!ok) throw_mswin_error("SetStdHandle");
        ok = SetStdHandle( STD_OUTPUT_HANDLE, parents_stdout );                             if(!ok) throw_mswin_error("SetStdHandle");

        CloseHandle( stdin_read );
        CloseHandle( stdout_write );    

        // Vorbereiten der Schreib- und Lesefäden:

        _write_event        = CreateEvent( NULL, FALSE, FALSE, NULL );                      if(!_write_event       ) throw_mswin_error("CreateEvent");
        _data_written_event = CreateEvent( NULL, FALSE, FALSE, NULL );                      if(!_data_written_event) throw_mswin_error("CreateEvent");
        _data_read_event    = CreateEvent( NULL, FALSE, FALSE, NULL );                      if(!_data_read_event   ) throw_mswin_error("CreateEvent");

        _write_thread_handle = CreateThread( NULL, 0, write_thread_function, this, 0, &thread_id );  
        if(!_write_thread_handle) throw_mswin_error("CreateThread");

        _read_thread_handle  = CreateThread( NULL, 0, read_thread_function , this, 0, &thread_id );  
        if(!_read_thread_handle) throw_mswin_error("CreateThread");
    }
    catch( const Xc& )
    {
        CloseHandle( stdin_read  ); 
        CloseHandle( stdin_write ); 
        CloseHandle( stdout_read  ); 
        CloseHandle( stdout_write ); 
        throw;
    }
} 

#endif

} //namespace sos
