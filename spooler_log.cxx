// $Id: spooler_log.cxx,v 1.48 2002/11/24 15:12:49 jz Exp $

#include "spooler.h"
#include "spooler_mail.h"

#include "../kram/sosdate.h"
#include "../kram/com_simple_standards.h"
#include "../kram/com.h"
#include "../kram/com_server.h"
#include "../kram/sosprof.h"
#include "../file/anyfile.h"

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

//-----------------------------------------------------------------------------------make_log_level

Log_level make_log_level( const string& name )
{
    Log_level log_level = log_debug9;

    if( name == "error" )  log_level = log_error;
    else
    if( name == "warn"  )  log_level = log_warn;
    else                     
    if( name == "info"  )  log_level = log_info;
    else
    if( name == "debug" )  log_level = log_debug;
    else
    if( strncmp(name.c_str(),"debug",5) == 0 )
    {
        try {
            log_level = (Log_level)-as_uint( name.c_str() + 5 );
        }
        catch( const Xc& ) { throw_xc( "SPOOLER-133", name ); }
    }
    else
    {
        try {
            log_level = (Log_level)as_int( name );
            if( log_level > log_error )  log_level = log_error;
        }
        catch( const Xc& ) { throw_xc( "SPOOLER-133", name ); }
    }

    return log_level;
}

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
        if( log && log_ptr )  _log_line.append( text, len );

        int ret = ::write( _file, text, len );
        if( ret != len )  throw_errno( errno, "write", _filename.c_str() );

        if( extra_log )  extra_log->write( text, len );
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
        Sos_optional_date_time time = Time::now().as_time_t();
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

void Log::log( Log_level level, const string& prefix, const string& line )
{
    if( level < _spooler->_log_level )  return;
    log2( level, prefix, line );
}

//----------------------------------------------------------------------------------------Log::log2

void Log::log2( Log_level level, const string& prefix, const string& line, Prefix_log* extra_log )
{
    if( _file == -1 )  return;

    THREAD_LOCK( _semaphore )
    {
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
            int next = line.find( '\n', begin );  
            if( next == string::npos )  next = line.length(); 
                                  else  next++;

            write( extra_log, buffer1, strlen(buffer1), false );           // Zeit
        
            write( extra_log, buffer2, strlen(buffer2) );                  // [info]

            if( !prefix.empty() )  write( NULL, "(" + prefix + ") " );     // (Job ...)

            int len = next - begin;
            while( len > 1  &&  line.c_str()[begin+len-1] == '\r' )  len--;
            write( extra_log, line.c_str() + begin, len );                 // Text

            begin = next;
        }

        if( line.length() == 0 || line[line.length()-1] != '\n' )  write( extra_log, "\n", 1 );

        LOG( _log_line );  _log_line = "";
    }
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::Prefix_log( int )
:
    _zero_(this+1),
    _file(-1)
{
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::Prefix_log( Spooler* spooler, const string& prefix )
:
    _zero_(this+1),
    _spooler(spooler),
    _log(&spooler->_log),
    _prefix(prefix),
    _file(-1)
{
    init( spooler, prefix );
}

//----------------------------------------------------------------------------------Prefix_log::log

Prefix_log::~Prefix_log()
{
    close();
}

//---------------------------------------------------------------------------------Prefix_log::init

void Prefix_log::init( Spooler* spooler, const string& prefix )
{
    _spooler = spooler;
    _log = &spooler->_log;
    _prefix = prefix;

    _log_level = _spooler->_log_level;
}

//------------------------------------------------------------------Prefix_log::set_profile_section

void Prefix_log::set_profile_section( const string& section )
{ 
    _section = section; 

    _mail_on_error   = _spooler->_mail_on_error;
    _mail_on_process = _spooler->_mail_on_process;
    _mail_on_success = _spooler->_mail_on_success;
    _subject         = _spooler->_log_mail_subject;
    _collect_within  = _spooler->_log_collect_within;
    _collect_max     = _spooler->_log_collect_max;

    if( !_section.empty() ) 
    {
        _log_level       = make_log_level( read_profile_string( _spooler->_factory_ini, _section, "log_level", as_string(_log_level) ) );
        _mail_on_error   = read_profile_bool           ( _spooler->_factory_ini, _section, "mail_on_error"     , _mail_on_error );
        _mail_on_process = read_profile_mail_on_process( _spooler->_factory_ini, _section, "mail_on_process"   , _mail_on_process );
        _mail_on_success =         read_profile_bool   ( _spooler->_factory_ini, _section, "mail_on_success"   , _mail_on_success );
        _subject         =         read_profile_string ( _spooler->_factory_ini, _section, "log_mail_subject"  , _subject );
        _collect_within  = (double)read_profile_uint   ( _spooler->_factory_ini, _section, "log_collect_within", _collect_within );
        _collect_max     = (double)read_profile_uint   ( _spooler->_factory_ini, _section, "log_collect_max"   , _collect_max );

        _smtp_server = read_profile_string( _spooler->_factory_ini, _section, "smtp"          , _spooler->_smtp_server );
        _queue_dir   = read_profile_string( _spooler->_factory_ini, _section, "mail_queue_dir", _spooler->_mail_queue_dir );

        _from    = read_profile_string( _spooler->_factory_ini, _section, "log_mail_from"   , _spooler->_log_mail_from );
        _to      = read_profile_string( _spooler->_factory_ini, _section, "log_mail_to"     );
        _cc      = read_profile_string( _spooler->_factory_ini, _section, "log_mail_cc"     );
        _bcc     = read_profile_string( _spooler->_factory_ini, _section, "log_mail_bcc"    );

        if( _to.empty() && _cc.empty() && _bcc.empty() )
        {       
            _to  = _spooler->_log_mail_to;
            _cc  = _spooler->_log_mail_cc;
            _bcc = _spooler->_log_mail_bcc;
        }
    }
}

//-------------------------------------------------------------------------Prefix_log::set_filename

void Prefix_log::set_filename( const string& filename )
{
    if( _file != -1 )  throw_xc( "spooler_log::filename" );
    _filename = filename;
    _new_filename = "";
}

//-------------------------------------------------------------------------Prefix_log::set_filename

void Prefix_log::set_new_filename( const string& filename )
{
    _new_filename = make_absolute_filename( _spooler->log_directory(), filename );
}

//---------------------------------------------------------------------------------Prefix_log::open

void Prefix_log::open()
{
    reset_highest_level();
    _highest_msg = "";
    
    if( _file != -1 )  return; //throw_xc( "SPOOLER-134", _filename );

    if( !_filename.empty() )
    {
        LOG( "\nopen " << _filename << '\n' );
        _file = ::open( _filename.c_str(), O_CREAT | ( _append? O_APPEND : O_TRUNC ) | O_WRONLY, 0666 );
        if( _file == -1 )  throw_errno( errno, _filename.c_str(), "Protokolldatei" );

        if( !_log_buffer.empty() )
        {
            write( _log_buffer.c_str(), _log_buffer.length() );
            _log_buffer = "";
        }

        log( log_info, "\nProtokoll beginnt in " + _filename );
    }
}

//--------------------------------------------------------------------------------Prefix_log::close

void Prefix_log::close()
{
    if( _file != -1 )  
    {
        close2();

        try
        {
            if( !_subject.empty()  ||  !_body.empty() )     // 20.11.2002
            {
                send_really();
            }
        }
        catch( const Xc& x         ) { _spooler->_log.error(x.what()); }
        catch( const exception&  x ) { _spooler->_log.error(x.what()); }
        catch( const _com_error& x ) { _spooler->_log.error(bstr_as_string(x.Description())); }
    }
}

//-------------------------------------------------------------------------------Prefix_log::close2

void Prefix_log::close2()
{
    if( _file != -1 )  
    {
        log( log_info, "Protokoll endet in " + _filename );

        ::close( _file ),  _file = -1;

        if( !_new_filename.empty() )
        {
            log( log_info, "Protokolldatei wird kopiert in " + _new_filename );
            copy_file( _filename, _new_filename );
            //int ret = rename( _filename.c_str(), _new_filename.c_str() );
            //if( ret == -1 )  throw_errno( errno, "rename", _new_filename.c_str() );
            _new_filename = "";
        }
    }
}

//--------------------------------------------------------------------------------Prefix_log::write

void Prefix_log::write( const char* text, int len )
{
    if( len == 0 )  return;

    if( _file == -1 )
    {
        if( !_filename.empty() )                // Datei wird noch geöffnet?
        {
            _log_buffer.append( text, len );
        }
    }
    else
    {
        int ret = ::write( _file, text, len );
        if( ret != len )  throw_errno( errno, "write", _filename.c_str() );
    }
}

//--------------------------------------------------------------------------------Prefix_log::imail

Com_mail* Prefix_log::imail()
{
    HRESULT hr;

    if( !_mail )
    {
        ptr<Com_mail> mail = new Com_mail( _spooler );
        mail->init();

        _mail = mail;   // Nur bei fehlerfreiem init() speichern

        if( _smtp_server != "-" )
        {
            hr = _mail->put_smtp( Bstr(_smtp_server) );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::smtp_server", _smtp_server.c_str() );
        }

        if( _queue_dir != "-" )
        {
            hr = _mail->put_queue_dir( Bstr(_queue_dir) );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::queue_dir", _smtp_server.c_str() );
        }

        set_mail_header();

        // Vorbesetzungen von spooler_task.cxx:
        if( !_from_name.empty() )  set_mail_from_name( _from_name ),  _from_name = "";   
        if( !_subject  .empty() )  set_mail_subject  ( _subject ),    _subject   = "";
        if( !_body     .empty() )  set_mail_body     ( _body ),       _body      = "";

        if( _job )
        {
            Bstr jobname_bstr = _job->name();
            _mail->add_header_field( Bstr(L"X-SOS-Spooler-Job"), jobname_bstr );
        }
    }

    return _mail;
}

//----------------------------------------------------------------------Prefix_log::set_mail_header

void Prefix_log::set_mail_header()
{
    HRESULT hr = NOERROR;

    if( _from != "-" )  hr = _mail->put_from( Bstr( _from ) );    if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::from", _from.c_str() );
                        hr = _mail->put_to  ( Bstr( _to   ) );    if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::to"  , _to.c_str() );
    if( _cc   != "-" )  hr = _mail->put_cc  ( Bstr( _cc   ) );    if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::cc"  , _cc.c_str() );
    if( _bcc  != "-" )  hr = _mail->put_bcc ( Bstr( _bcc  ) );    if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::bcc" , _bcc.c_str() );
}

//-------------------------------------------------------------------Prefix_log::set_mail_from_name

void Prefix_log::set_mail_from_name( const string& from_name )
{
    HRESULT hr;

    if( _mail )
    {
        Bstr old_from;
        hr = _mail->get_from( &old_from );                              if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::from" );
        if( !wcschr( old_from, '<' )  &&  wcschr( old_from, '@' ) )
        {
            string from = from_name + " <" + bstr_as_string(old_from) + ">";
            Bstr from_bstr = from;
            hr = _mail->put_from( from_bstr );                          if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::from", from.c_str() );
        }
    }
    else
    {
        _from_name = from_name;
    }
}

//---------------------------------------------------------------------Prefix_log::set_mail_subject

void Prefix_log::set_mail_subject( const string& subject, bool overwrite )
{
    HRESULT hr;

    if( _mail )
    {
        if( !overwrite )
        {
            Bstr subject_bstr;
            hr = _mail->get_subject( &subject_bstr );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::subject" );

            if( SysStringLen(subject_bstr) > 0 )  return;
        }

        hr = _mail->put_subject( Bstr(subject) );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::subject", subject.c_str() );
    }
    else
    {
        if( !_subject.empty()  &&  !overwrite )  return;
        _subject = subject;
    }
}

//------------------------------------------------------------------------Prefix_log::set_mail_body

void Prefix_log::set_mail_body( const string& body, bool overwrite )
{
    HRESULT hr;

    if( _mail )
    {
        if( !overwrite )
        {
            Bstr body_bstr;
            hr = _mail->get_body( &body_bstr );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::body" );

            if( SysStringLen(body_bstr) > 0 )  return;
        }

        hr = _mail->put_body( Bstr(body) );     if( FAILED(hr) ) throw_ole( hr, "spooler::Mail::body", body.c_str() );
    }
    else
    {
        if( !_body.empty()  &&  !overwrite )  return;
        _body = body;
    }
}

//---------------------------------------------------------------------------------Prefix_log::send

void Prefix_log::send( int reason )
{
    // reason == -2  =>  Gelegentlicher Aufruf, um Fristen zu prüfen und ggfs. eMail zu versenden
    // reason == -1  =>  Job mit Fehler beendet
    // reason >=  0  =>  Anzahl spooler_process()

    if( _file == -1 )       // Nur senden, wenn die Log-Datei beschrieben worden ist
    {
        _first_send = 0;
        _mail = NULL;
    }
    else
    {
        bool mail_it =  reason == -1  &&  _mail_on_error
                     || reason ==  0  &&  _mail_on_success
                     || reason  >  0  &&  ( _mail_on_success || _mail_on_process && reason >= _mail_on_process );

        Time now = Time::now();

        if( _first_send == 0  &&  !mail_it )
        {
            close2();    // Protokoll nicht senden
            _mail = NULL;
        }
        else
        {
            if( _last_send  == 0  ||  _last_send  > now )  _last_send  = now;
            if( _first_send == 0  ||  _first_send > now )  _first_send = now;

            if( reason == -1  &&  _mail_on_error        // Fehler?
             || now >= _last_send + _collect_within     // Nicht mehr sammeln?
             || now >= _first_send + _collect_max )     // Lange genug gesammelt?
            {
                // Wenn die Protokolle in einer eMail gesammelt verschickt werden, wirken 
                // mail_on_error==false oder mail_on_process==false nicht wie gewünscht,
                // denn diese Bedingung wird erst festgestellt, wenn das Protokoll bereits geschrieben ist.

                close2();
                send_really();

                _first_send = 0;
            }
        }
    }
 
    _last_send = Time::now();
}

//--------------------------------------------------------------------------Prefix_log::send_really

void Prefix_log::send_really()
{
    int ok;

    imail()->add_file( Bstr(_filename), NULL, Bstr(L"plain/text"), Bstr(_spooler->_mail_encoding) );

    ok = imail()->send();

    if( ok )
    {
        try
        {
            imail()->auto_dequeue();
        }
        catch( const Xc& x ) { warn( string("eMail versendet, aber Fehler beim Verarbeiten der eMail-Warteschlange: ") + x.what() ); }
    }
    else
        warn( "eMail konnte nicht versendet werden" );

    _mail = NULL;
}

//----------------------------------------------------------------------------------Prefix_log::log

void Prefix_log::log( Log_level level, const string& line )
{
    //if( _file == -1  &&  !_filename.empty() )
    //{
    //}

    if( level == log_error  &&  _job  &&  !_job->has_error() )  _job->set_error_xc_only( Xc( "SPOOLER-140", line.c_str() ) );

    if( _highest_level < level )  _highest_level = level, _highest_msg = line;
    if( level < _log_level )  return;


    string prefix = _prefix;
    _log->log2( level, _job && _job->current_task()? "Task " + as_string(_job->current_task()->id()) + " " + _job->name() : _prefix, line, this );
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
