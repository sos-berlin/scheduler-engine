// $Id: spooler_log.cxx,v 1.20 2002/03/02 23:17:06 jz Exp $

#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/sosdate.h"

#include <stdio.h>
#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#if defined _MSC_VER
#    include <io.h>       // open(), read() etc.
#    include <direct.h>   // mkdir
# else
#    include <stdio.h>    // fileno
#    include <unistd.h>   // read(), write(), close()
#endif
#include <errno.h>


namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------------Log::Log

Log::Log( Spooler* spooler )         
: 
    _zero_(this+1),
    _spooler(spooler)
{
    _file = -1;
}

//----------------------------------------------------------------------------------------Log::~Log

Log::~Log()         
{
    Thread_semaphore::Guard guard = &_semaphore;

    if( _file != -1  &&  _file != fileno(stderr) )  close( _file ),  _file = -1;
}

//-------------------------------------------------------------------------------Log::set_directory

void Log::set_directory( const string& directory )         
{
    if( directory.empty() )  _directory = get_temp_path();
                       else  _directory = directory;

    if( _directory.length() > 0  &&  ( _directory[_directory.length()-1] == '/'  ||  _directory[_directory.length()-1] == '\\' ) ) 
        _directory = _directory.substr( 0, _directory.length() - 1 );
}

//---------------------------------------------------------------------------------------Log::write

void Log::write( Prefix_log* extra_log, const char* text, int len, bool log )
{
    if( len > 0  &&  text[len-1] == '\r' )  len--;

    if( len > 0 )
    {
        if( log && log_ptr )  log_ptr->write( text, len );

        int ret = ::write( _file, text, len );
        if( ret != len )  throw_errno( errno, "write", _filename.c_str() );

        if( extra_log  &&  extra_log->_file != -1 )
        {
            int ret = ::write( extra_log->_file, text, len );
            if( ret != len )  throw_errno( errno, "write", extra_log->_filename.c_str() );
        }
    }
}

//------------------------------------------------------------------------------------Log::open_new

void Log::open_new()
{
    Thread_semaphore::Guard guard = &_semaphore;

    if( _file != -1  &&  _file != fileno(stderr) )  close( _file ),  _file = -1;
    _filename = "";

    if( _directory == "*stderr" )
    {
        _filename = "*stderr";
        _file = fileno(stderr);
    }
    else
    if( _directory == "*none" )
    {
        _filename = "*none";
    }
    else
    {
        Sos_optional_date_time time = Time::now();
        string filename = _directory;

        filename += "/spooler-";
        filename += time.formatted( "yyyy-mm-dd-HHMMSS" );
        if( !_spooler->id().empty() )  filename += "." + _spooler->id();
        filename += ".log";

        LOG( "\nopen " << filename << '\n' );
        _file = open( filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666 );
        if( _file == -1 )  throw_errno( errno, filename.c_str() );

        _filename = filename;
    }
}

//-----------------------------------------------------------------------------------------Log::log

void Log::log( Log_level level, const string& prefix, const string& line, Prefix_log* extra_log )
{
    if( level < _spooler->_log_level )  return;
    if( _file == -1 )  return;

    Thread_semaphore::Guard guard = &_semaphore;
    char buffer1[50];
    char buffer2[50];

    string now = Time::now().as_string();
    strcpy( buffer1, now.c_str() );

    switch( level )
    {
      //case log_fatal: strcpy ( buffer2, " [FATAL]  " );  break;
        case log_error: strcpy ( buffer2, " [ERROR]  " );  break;
        case log_warn : strcpy ( buffer2, " [WARN]   " );  break;
        case log_info : strcpy ( buffer2, " [info]   " );  break;
        case log_debug: strcpy ( buffer2, " [debug]  " );  break;
        default:        sprintf( buffer2, " [debug%d] ", (int)-level );
    }

    int begin = 0;
    while( begin < line.length() )
    {
        int nl = line.find( '\n', begin );  if( nl == string::npos )  nl = line.length();

        write( extra_log, buffer1, strlen(buffer1), false );           // Zeit
        
        write( extra_log, buffer2, strlen(buffer2) );                  // [info]

        if( !prefix.empty() )  write( NULL, "(" + prefix + ") " );     // (Job ...)

        write( extra_log, line.c_str() + begin, nl - begin );          // Text
        begin = nl + 1;
    }

    if( line.length() == 0 || line[line.length()-1] != '\n' )  write( extra_log, "\n", 1 );
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::Prefix_log( Log* log, const string& prefix )
:
    _zero_(this+1),
    _log(log),
    _prefix(prefix),
    _file(-1)
{
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::~Prefix_log()
{
    close();
}

//----------------------------------------------------------------------------------Prefix_log::log

void Prefix_log::close()
{
    if( _file != -1 )  
    {
        log( log_info, "Protokoll endet in " + _filename );

        ::close( _file ),  _file = -1;
    }
}

//---------------------------------------------------------------------------------Prefix_log::open

void Prefix_log::open( const string& filename )
{
    if( _file != -1 )  throw_xc( "SPOOLER-134", _filename );

    _filename = filename;

    LOG( "\nopen " << _filename << '\n' );
    _file = ::open( _filename.c_str(), O_CREAT | ( _append? 0 : O_TRUNC ) | O_WRONLY, 0666 );
    if( _file == -1 )  throw_errno( errno, _filename.c_str() );

    log( log_info, "Protokoll beginnt in " + _filename );
}

//----------------------------------------------------------------------------------Prefix_log::log

void Prefix_log::log( Log_level level, const string& line )
{
    //if( _file == -1  &&  !_filename.empty() )
    //{
    //}

    _log->log( level, _prefix, line, this );
}

//---------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------Stdout_collector
/*
struct Stdout_collector
{
    void                        close                       ();
  //void                        collect_stdout              ();
    void                        collect_stderr              ();
  //virtual void                write_stdout                ( const Const_area& data );
    virtual void                write_stderr                ( const Const_area& data );

    HANDLE                     _original_stderr;
    HANDLE                     _stdout_write;
    HANDLE                     _stdout_read;
};

//------------------------------------------------------------------Stdout_collector::collect_stderr

void Stdout_collector::collect_stderr()
{
    BOOL    ok; 

    _original_stderr = GetStdHandle( STD_ERROR_HANDLE ); 

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

            if( !ok )  throw_mswin_error("CreateProcess");

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
*/
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
