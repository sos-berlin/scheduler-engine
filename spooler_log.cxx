// $Id: spooler_log.cxx,v 1.14 2001/07/02 11:13:44 jz Exp $

#include "../kram/sos.h"
#include "spooler.h"

#include "../kram/sosdate.h"

namespace sos {
namespace spooler {

//-----------------------------------------------------------------------------------------Log::Log

Log::Log( Spooler* spooler )         
: 
    _zero_(this+1),
    _spooler(spooler)
{
}

//----------------------------------------------------------------------------------------Log::~Log

Log::~Log()         
{
    Thread_semaphore::Guard guard = &_semaphore;

    if( _file  &&  _file != stderr )  fclose( _file );
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

void Log::write( const string& text )
{
    if( !_file )  return;

    int ret = fwrite( text.c_str(), text.length(), 1, _file );
    if( ret != 1 )  throw_errno( errno, "fwrite" );
}

//------------------------------------------------------------------------------------Log::open_new

void Log::open_new( )
{
    Thread_semaphore::Guard guard = &_semaphore;

    if( _file  &&  _file != stderr )  fclose( _file ),  _file = NULL;
    _filename = "";

    if( _directory == "*stderr" )
    {
        _filename = "*stderr";
        _file = stderr;
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
    
        _file = fopen( filename.c_str(), "w" );
        if( !_file )  throw_errno( errno, filename.c_str() );

        _filename = filename;
    }
}

//-----------------------------------------------------------------------------------------Log::log

void Log::log( Log_kind kind, const string& prefix, const string& line )
{
    Thread_semaphore::Guard guard = &_semaphore;
    char buffer[100];

    string now = Time::now().as_string();
    strcpy( buffer, now.c_str() );

    switch( kind )
    {
        case log_msg  : strcat( buffer, " msg   " );  break;
        case log_warn : strcat( buffer, " WARN  " );  break;
        case log_error: strcat( buffer, " ERROR " );  break;
        default: ;
    }

    write( buffer );
    if( !prefix.empty() )  write( "(" + prefix + ") " );
    write( line );
    if( line.length() == 0 || line[line.length()-1] != '\n' )  write( "\n" );
    fflush( _file );
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::Prefix_log( Log* log, const string& prefix )
:
    _log(log),
    _prefix(prefix)
{
}

//----------------------------------------------------------------------------------Prefix_log::log

void Prefix_log::log( Log_kind kind, const string& line )
{
    _log->log( kind, _prefix, line );
}

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
