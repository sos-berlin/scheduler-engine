// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "spooler_mail.h"

#include "../kram/sosdate.h"
#include "../kram/com_simple_standards.h"
#include "../kram/com.h"
#include "../kram/com_server.h"
#include "../kram/sosprof.h"
#include "../kram/sleep.h"
#include "../file/anyfile.h"
#include "../zschimmer/olechar.h"
#include "../zschimmer/file.h"

#include <stdio.h>
#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY
#include <errno.h>

#if defined _MSC_VER
#    include <io.h>       // open(), read() etc.
#    include <direct.h>   // mkdir
# else
#    include <stdio.h>    // fileno
#    include <unistd.h>   // read(), write(), close()
#endif



namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------Log_set_console_colors

struct Log_set_console_colors_base
{
    Spooler*                    _spooler;
    bool                        _restore_console;

    enum Color { c_black, c_dark_red, c_pale_red, c_blue, c_pale_blue, c_pale_blue_green, c_dark_green, c_pale_brown, c_gray };


    Log_set_console_colors_base( Spooler* spooler )
    :
        _spooler(spooler),
        _restore_console(false)
    {
    }


    virtual ~Log_set_console_colors_base()
    {
    }


    virtual void set_color( Color ) = 0;


    void set_color_for_level( Log_level level )
    {
        //bool with_colors = _spooler && _spooler->_zschimmer_mode  Z_WINDOWS_ONLY( Z_DEBUG_ONLY( || true ) );
        bool with_colors = _spooler && _spooler->_log_to_stderr;

        if( with_colors ) 
        {
            _restore_console = true;

            switch( level )
            {
                case log_error:     set_color( c_dark_red   ); break;
                case log_warn:      set_color( c_dark_red   ); break;
                case log_info:      set_color( c_black      ); break;
                case log_debug1:    set_color( c_pale_blue  ); break;
                case log_debug2:    //set_color( c_dark_green ); break;
                case log_debug3:    set_color( c_pale_blue_green ); break;
                case log_debug4:
                case log_debug5:    
                case log_debug6:    set_color( c_pale_brown  ); break;
                case log_debug7:    
                case log_debug8:    
                case log_debug9:    
                default:            set_color( c_gray  ); break;
            }
        }
    }
};

#ifdef Z_WINDOWS

    struct Log_set_console_colors : Log_set_console_colors_base
    {
        CONSOLE_SCREEN_BUFFER_INFO  _console_screen_buffer_info; 


        Log_set_console_colors( Spooler* spooler ) 
        :     
            Log_set_console_colors_base( spooler )
        {
        }


        ~Log_set_console_colors()
        {
            if( _restore_console ) 
                SetConsoleTextAttribute( GetStdHandle(STD_ERROR_HANDLE), _console_screen_buffer_info.wAttributes );
        }


        void set_color( Color color )
        {
            GetConsoleScreenBufferInfo( GetStdHandle(STD_ERROR_HANDLE), &_console_screen_buffer_info );

            WORD attributes = _console_screen_buffer_info.wAttributes;   //BACKGROUND_BLUE | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;

            if( attributes & BACKGROUND_INTENSITY
             && attributes & BACKGROUND_RED 
             && attributes & BACKGROUND_GREEN )       // Hintergrund ist hell und weiß oder gelb
            {
                attributes &= ~( FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE );

                switch( color )
                {
                    case c_dark_red:         attributes |= FOREGROUND_INTENSITY | FOREGROUND_RED;               break;
                    case c_pale_red:         attributes |=                        FOREGROUND_RED;               break;
                    case c_blue:             attributes |= FOREGROUND_INTENSITY | FOREGROUND_BLUE;              break;
                    case c_pale_blue:        attributes |=                        FOREGROUND_BLUE;              break;
                    case c_pale_blue_green:  attributes |= FOREGROUND_GREEN | FOREGROUND_BLUE;                  break;
                    case c_dark_green:       attributes |= FOREGROUND_GREEN;                                    break;
                    case c_pale_brown:       attributes |= FOREGROUND_RED | FOREGROUND_GREEN;                   break;
                    case c_gray:             attributes |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
                    case c_black: 
                    default: ;

                }

                SetConsoleTextAttribute( GetStdHandle(STD_ERROR_HANDLE), attributes );
            }
        }
    };

#else

    struct Log_set_console_colors : Log_set_console_colors_base
    {
        Log_set_console_colors( Spooler* spooler ) 
        :     
            Log_set_console_colors_base( spooler )
        {
        }

        ~Log_set_console_colors()
        {
            if( _restore_console ) 
                set_color( c_black );
        }

        void set_color( Color color )
        {
            string seq;

            switch( color )
            {
                case c_dark_red:        seq = "\e[1;31m";   break;
                case c_pale_red:        seq = "\e[0;31m";   break;
                case c_blue:            seq = "\e[0;34m";   break;
                case c_pale_blue:       seq = "\e[1;34m";   break;
                case c_pale_blue_green: seq = "\e[0;36m";   break;
                case c_pale_brown:      seq = "\e[0;33m";   break;
                case c_gray:            seq = "\e[0;37m";   break;
                case c_black:           seq = "\e[0;30m";   break;
                default: ;

            }

            write( fileno(stderr), seq.data(), seq.length() );
        }
    };

#endif
//------------------------------------------------------------------------------------is_stop_errno
    
static bool is_stop_errno( Spooler* spooler, int err_no )
{
    if( spooler->state() == Spooler::s_starting )  return false;
    if( spooler->state() == Spooler::s_stopping )  return false;
    if( spooler->state() == Spooler::s_stopped  )  return false;

    if( err_no == ENOSPC )  return true;

    return false;
}

//-----------------------------------------------------------------------------------------io_error
    
static void io_error( Spooler* spooler, const string& filename )
{
    try
    {
        spooler->_waiting_errno          = errno;
        spooler->_waiting_errno_filename = filename;
      //spooler->_waiting_errno_continue = false;
        spooler->set_state( Spooler::s_paused );

        string error_code = "ERRNO-" + as_string( spooler->_waiting_errno );
        zschimmer::Xc x ( error_code.c_str(), filename.c_str() );
      //string error_text = S() << "ERRNO-" << spooler->_waiting_errno << "  " << strerror( spooler->_waiting_errno );

        Z_LOGI2( "scheduler", "\n*** SCHEDULER HÄLT WEGEN PLATTENPLATZMANGEL AN. " << x.what() << ", Datei " << filename << "\n\n" );

        Scheduler_event scheduler_event ( evt_disk_full, log_warn, spooler );
        scheduler_event.set_error( x );

        Mail_defaults mail_defaults ( spooler );

        mail_defaults.set( "subject", "SCHEDULER SUSPENDED:  " + string( x.what() ) );
        mail_defaults.set( "body"   , "Job Scheduler is suspended due to disk space shortage.\n"
                                      "\n" + 
                                      string( x.what() ) + "\n"
                                      "File " + filename + "\n"
                                      "\n"
                                      "You can continue the Job Scheduler as soon as there is enough disk space.\n"
                                      "Use this XML-command: <modify_spooler cmd=\"continue\"/>" );

        scheduler_event.send_mail( mail_defaults );
/*
        spooler->send_error_email( "SCHEDULER SUSPENDED:  " + error_text,
                                   "Job Scheduler is suspended due to disk space shortage.\n"
                                   "\n" + 
                                   error_text + "\n"
                                   "File " + filename + "\n"
                                   "\n"
                                   "You can continue the Job Scheduler as soon as there is enough disk space.\n"
                                   "Use this XML-command: <modify_spooler cmd=\"continue\"/>" );
*/

        //while( !spooler->_waiting_errno_continue )
        while( spooler->_state_cmd != Spooler::sc_continue )
        {
            int wait_seconds = 1;
            spooler->_connection_manager->async_continue_selected( is_allowed_operation_while_waiting, wait_seconds );   // Kann ins scheduler.log schreiben!
        }

        spooler->_waiting_errno = 0;
    }
    catch( exception& )
    {
        spooler->_waiting_errno = 0;
    }
}

//-----------------------------------------------------------------------------------------my_write

static int my_write( Spooler* spooler, const string& filename, int file, const char* text, int len )
{
    if( spooler->_waiting_errno )  return len;  // Im Fehlerzustand io_error()? Dann schreiben wir nix (kann in spooler_communication.cxx passieren)


    int         ret = 0;
    const char* t   = text;
   
    while( t < text + len )   // Solange write() etwas schreiben kann
    {
        ret = ::write( file, t, text + len - t );

        if( ret < 0 )
        {
            if( is_stop_errno( spooler, errno ) ) 
            {
                io_error( spooler, filename );
                continue;
            }
        }

        if( ret <= 0 )  break;
        t += ret;
        if( t < text + len )  sos_sleep( 0.001 );
    }

    if( ret <= 0  &&  len > 0 )
    {
        int err = errno;

        if( err == EAGAIN )     // Das kann passieren, wenn ein Thread gleichzeitig nach stderr schreibt.
        {
            //Z_LOG2( "scheduler", "Prefix_log::write ERRNO-" << err << " " << strerror(err) );
            sos_sleep( 0.01 );
            ::write( file, "<<errno=EAGAIN>>", 16 ); 
            ret = ::write( file, t, text + len - t );
            if( ret != text + len - t  &&  file != fileno(stderr) )  return -1;  // Nur bei stderr ignorieren wir den Fehler
        }

        throw_errno( err, "write", filename.c_str() );
    }

    return len;
}

//-----------------------------------------------------------------------------------------Log::Log

Log::Log( Spooler* spooler )         
: 
    _zero_(this+1),
    _spooler(spooler),
    _semaphore("Log")
{
    _file = -1;
}

//----------------------------------------------------------------------------------------Log::~Log

Log::~Log()         
{
    Z_MUTEX( _semaphore )
    {
        if( _file != -1  &&  _file != fileno(stderr) )  ::close( _file ),  _file = -1;
    }
}

//-------------------------------------------------------------------------------Log::set_directory
// Für allgemeines Spooler-Protokoll

void Log::set_directory( const string& directory )         
{
    if( directory.empty() )  _directory = get_temp_path();
                       else  _directory = directory;

    if( _directory.length() > 0  &&  ( _directory[_directory.length()-1] == '/'  ||  _directory[_directory.length()-1] == '\\' ) ) 
        _directory = _directory.substr( 0, _directory.length() - 1 );
}

//------------------------------------------------------------------------------------Log::open_new
// Für allgemeines Spooler-Protokoll

void Log::open_new()
{
    Z_MUTEX( _semaphore )
    {
        int    old_file     = _file;
        string old_filename = _filename;

        _file     = -1;
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

            filename += "/scheduler-";
            filename += time.formatted( "yyyy-mm-dd-HHMMSS" );
            if( !_spooler->id().empty() )  filename += "." + _spooler->id();
            if( _spooler->_cluster_configuration._is_backup_member )  filename += "_backup";
            filename += ".log";

            Z_LOG2( "scheduler.log", "open(\"" << filename << "\")\n" );
            _file = open( filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_NOINHERIT, 0666 );
            if( _file == -1 )  throw_errno( errno, filename.c_str() );
            Z_LOG2( "scheduler.log", "open() => " << _file << "\n" );

            _filename = filename;
        }

        if( old_file != -1  &&  old_file != fileno(stderr) )
        {
            string line = "\nDas Protokoll wird fortgeführt in " + _filename + "\n";
            ::write( old_file, line.c_str(), line.length() );
            ::close( old_file );
        }

        if( _log_buffer.length() > 0  &&  _file != -1 )
        {
            int ret = my_write( _spooler, _filename, _file, _log_buffer.c_str(), _log_buffer.length() );
            if( ret != _log_buffer.length() )  
            {
                _err_no = errno;
                throw_errno( errno, "write", _filename.c_str() );
            }

            _log_buffer = "";
        }
    }
}

//------------------------------------------------------------------------------Log::start_new_file

void Log::start_new_file()
{
    string old_filename = _filename;
    info( message_string( "SCHEDULER-967" ) );   // "start_new_file(): Die Protokolldatei wird geschlossen"

    string old_log_categories = current_log_categories();
    string old_log_filename = log_filename();
    log_stop();
    log_start( S() << old_log_categories << " >" << old_log_filename );

    open_new();

    info( message_string( "SCHEDULER-968", old_filename ) );   //"start_new_file(): Vorhergehende Protokolldatei ist "
}

//---------------------------------------------------------------------------------------Log::write

void Log::write( Log_level level, Prefix_log* extra_log, Prefix_log* order_log, const char* text, int len )
{
    if( _err_no )  return;       // Falls nach einer Exception noch was ins Log geschrieben wird, ignorieren wir das.


    if( len > 0  &&  text[len-1] == '\r' )  len--;

    if( len > 0 )
    {
        //if( log && log_ptr )  _log_line.append( text, len );

        if( _file != -1 )
        {
            int ret = my_write( _spooler, _filename, _file, text, len );
            if( ret != len )  
            {
                _err_no = errno;
                throw_errno( errno, "write", _filename.c_str() );
            }
        }
        else
        {
            _log_buffer.append( text, len );  // Das ist derselbe Mechanismus wie in Prefix_log. Das könnte man zusammenfassen.
        }

        if( extra_log )  extra_log->write( text, len );
        if( order_log )  order_log->write( text, len );

        if( _spooler->_log_to_stderr  &&  level >= _spooler->_log_to_stderr_level )  my_write( _spooler, "(stderr)", fileno(stderr), text, len );
    }
}

//-----------------------------------------------------------------------------------------Log::log

void Log::log( Log_level level, const string& prefix, const string& line )
{
    if( this == NULL )  return;

    bool log_to_files = level >= _spooler->_log_level;

    try
    {
        log2( level, log_to_files, prefix, line );
    }
    catch( const exception& x ) 
    {
        fprintf( stderr, "%s\n", line.c_str() );
        fprintf( stderr, "Fehler beim Schreiben des Protokolls: %s\n", x.what() );
        Z_LOG2( "scheduler", "Fehler beim Schreiben des Protokolls: " << x.what() << "\n" );
        
        if( level < log_error )  throw;     // Bei error() Exception ignorieren, denn die Funktion wird gerne in Exception-Handlern gerufen
    }
}

//----------------------------------------------------------------------------------------Log::log2

void Log::log2( Log_level level, bool log_to_files, const string& prefix, const string& line_, Prefix_log* extra_log, Prefix_log* order_log )
{
    if( this == NULL )  return;

    //if( _file == -1 )  return;
            

    if( !log_to_files )
    {
        if( !log_category_is_set( "scheduler" ) )  return;

#       ifndef Z_DEBUG
            if( !_spooler->_zschimmer_mode )  return;   // eMail von Uwe Risse 2007-07-12 15:18, Jira JS-50
#       endif
    }

    string line = line_;
    for( int i = line.find( '\r' ); i != string::npos; i = line.find( '\r', i+1 ) )  line[i] = ' ';     // Windows scheint sonst doppelte Zeilenwechsel zu schreiben. jz 25.11.03

    //assert( !line.empty() );
        
    
    THREAD_LOCK( _semaphore )
    {
        Log_set_console_colors console_colors ( _spooler );

        char time_buffer [50];   time_buffer[0] = '\0';
        char level_buffer[50];

        if( log_to_files )
        {
            if( _file != -1  &&  isatty( _file )  ||  _spooler->_log_to_stderr  &&  level >= _spooler->_log_to_stderr_level  &&  isatty( fileno( stderr ) ) )  console_colors.set_color_for_level( level );

            Time now = Time::now();
            _last_time = now;
            strcpy( time_buffer, now.as_string().c_str() );
        }
        
        switch( level )
        {
          //case log_fatal: strcpy ( level_buffer, " [FATAL]  " );  break;
            case log_error: strcpy ( level_buffer, " [ERROR]  " );  break;
            case log_warn : strcpy ( level_buffer, " [WARN]   " );  break;
            case log_info : strcpy ( level_buffer, " [info]   " );  break;
            case log_debug: strcpy ( level_buffer, " [debug]  " );  break;
            case log_none : strcpy ( level_buffer, " [none]   " );  break;          // Passiert, wenn nur das scheduler.log beschrieben werden soll (log_to_files==false)
            case log_unknown:strcpy( level_buffer, " [unknown]" );  break;          // Sollte nicht passieren
            default:        snprintf( level_buffer, sizeof level_buffer, " [debug%d] ", (int)-level );
        }


        int begin = 0;
        while(1)
        {
            z::Log_ptr log ( "scheduler" );

            int next = line.find( '\n', begin );  
            if( next == string::npos )  next = line.length(); 
                                  else  next++;

            if( log_to_files )
            {
                int buffer1_len = strlen( time_buffer );
                write( level, extra_log, order_log, time_buffer, buffer1_len );                        // Zeit
            }

            int buffer2_len = strlen( level_buffer );
            if( log          )  log->write( level_buffer + 1, buffer2_len - 1 );
            if( log_to_files )  write( level, extra_log, order_log, level_buffer, buffer2_len );       // [info]

            if( !prefix.empty() )
            {
                string s; s.reserve( prefix.length() + 3 ); s = "(",  s += prefix, s += ") ";   // (prefix)
                if( log          )  log << s;
                if( log_to_files )  write( level, NULL, order_log, s );     // (Job ...)
            }

            int len = next - begin;
            while( len > 1  &&  line.c_str()[begin+len-1] == '\r' )  len--;
            if( log          )  log->write( line.c_str() + begin, len );
            if( log_to_files )  write( level, extra_log, order_log, line.c_str() + begin, len );       // Text

            begin = next;
            if( begin >= line.length() )  break;
        }

        if( line.length() == 0 || line[line.length()-1] != '\n' )  
        {
            Z_LOG( "\n" );
            if( log_to_files )  write( level, extra_log, order_log, "\n", 1 );
        }

        //Z_LOG2( "scheduler", _log_line );  _log_line = "";

        
        if( log_to_files )  
        {
            if( extra_log )  extra_log->signal_events();
            if( order_log )  order_log->signal_events();

            if( this == &_spooler->_base_log )  _spooler->log()->signal_events();   // Nicht schön, aber es gibt sowieso nur ein Log.
        }
    }
}

//-------------------------------------------------------Prefix_log::Open_and_close::Open_and_close

Prefix_log::Open_and_close::Open_and_close( Prefix_log* log )
:
    _log(NULL)
{
    if( log->_open_and_close_every_line  &&  
        log->is_active()  &&  
        log->_file == -1  &&  
       !log->_filename.empty() )
    {
        _log = log;
        _log->open_file();
    }
}

//------------------------------------------------------Prefix_log::Open_and_close::~Open_and_close
    
Prefix_log::Open_and_close::~Open_and_close()
{
    if( _log  &&  _log->_open_and_close_every_line )
    {
        _log->close_file();
    }
}

//---------------------------------------------------------------------------Prefix_log::Prefix_log

Prefix_log::Prefix_log( int )
:
    _zero_(this+1),
    _file(-1),
  //_log_level(log_unknown),
    _mail_defaults(NULL)
{
}

//---------------------------------------------------------------------------Prefix_log::Prefix_log

Prefix_log::Prefix_log( Scheduler_object* o )
:
    _zero_(this+1),
    _object(o),
    _spooler(o->_spooler),
    _log(&o->_spooler->_base_log),
    _prefix( o->obj_name() ),
    _file(-1),
  //_log_level(log_unknown),
    _mail_defaults(NULL),
    _last_level( log_unknown )
{
    init( o );
}

//--------------------------------------------------------------------------Prefix_log::~Prefix_log

Prefix_log::~Prefix_log()
{
    close();
}

//---------------------------------------------------------------------------------Prefix_log::init

void Prefix_log::init( Scheduler_object* o, const string& prefix )
{
    reset_highest_level();

    _mail_defaults = o->_spooler->_mail_defaults;
    _object  = o;
    _spooler = o->_spooler;
    _log     = &o->_spooler->_base_log;
    _prefix  = prefix;

  //_log_level       = _spooler->_log_level;
    _mail_on_warning = _spooler->_mail_on_warning;
    _mail_on_error   = _spooler->_mail_on_error;
    _mail_on_process = _spooler->_mail_on_process;
    _mail_on_success = _spooler->_mail_on_success;
    _mail_on_delay_after_error = _spooler->_mail_on_delay_after_error;
  //_subject         = _spooler->_log_mail_subject;
    _collect_within  = _spooler->_log_collect_within;
    _collect_max     = _spooler->_log_collect_max;
}

//----------------------------------------------------------------------------Prefix_log::log_level

int Prefix_log::log_level()
{ 
    return _log_level == log_unknown && _spooler? _spooler->_log_level 
                                                : _log_level; 
}

//------------------------------------------------------------------Prefix_log::set_profile_section

void Prefix_log::set_profile_section( const string& section )
{ 
    _section = section; 

    if( !_section.empty() ) 
    {
        _log_level       = make_log_level( read_profile_string( _spooler->_factory_ini, _section, "log_level", sos::as_string(_log_level) ) );
        _mail_on_warning = read_profile_bool           ( _spooler->_factory_ini, _section, "mail_on_warning"   , _mail_on_warning );
        _mail_on_error   = read_profile_bool           ( _spooler->_factory_ini, _section, "mail_on_error"     , _mail_on_error   );
        _mail_on_process = read_profile_mail_on_process( _spooler->_factory_ini, _section, "mail_on_process"   , _mail_on_process );
        _mail_on_success =         read_profile_bool   ( _spooler->_factory_ini, _section, "mail_on_success"   , _mail_on_success );
        _mail_on_delay_after_error = read_profile_yes_no_last_both( _spooler->_factory_ini, _section, "mail_on_delay_after_error", _mail_on_delay_after_error );
      //_subject         =         read_profile_string ( _spooler->_factory_ini, _section, "log_mail_subject"  , _subject );
        _collect_within  = (double)read_profile_uint   ( _spooler->_factory_ini, _section, "log_collect_within", (uint)_collect_within );
        _collect_max     = (double)read_profile_uint   ( _spooler->_factory_ini, _section, "log_collect_max"   , (uint)_collect_max );
    }
}

//---------------------------------------------------------------------Prefix_log::inherit_settings

void Prefix_log::inherit_settings( const Prefix_log& other )
{
    _log_level       = other._log_level;

    _mail_on_warning = other._mail_on_warning;
    _mail_on_error   = other._mail_on_error;
    _mail_on_process = other._mail_on_process;
    _mail_on_success = other._mail_on_success;
    _mail_on_delay_after_error = other._mail_on_delay_after_error;
  //_subject         = other._subject;
    _collect_within  = other._collect_within;
    _collect_max     = other._collect_max;
  //_smtp_server     = other._smtp_server;
  //_queue_dir       = other._queue_dir;
  //_from            = other._from;
  //_to              = other._to;
  //_cc              = other._cc;
  //_bcc             = other._bcc;

    _mail_defaults = other._mail_defaults;
    if( _mail )  _mail->set_mail_defaults( _mail_defaults );
}

//-------------------------------------------------------------------------Prefix_log::set_filename

void Prefix_log::set_filename( const string& filename )
{
    if( _file != -1 )  throw_xc( "spooler_log::filename", filename, _filename );
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
    if( !is_active() )
    {
        //2005-09-22  reset_highest_level();
        //2005-09-22  _highest_msg = "";
        
        _is_finished = false;

        if( _file != -1 )  return; //z::throw_xc( "SCHEDULER-134", _filename );

        if( !_filename.empty() )
        {
            #ifdef Z_DEBUG
                if( string_begins_with( file::File_path( _filename ).name(), "order." )  &&  file::File_path( _filename ).file_exists() )
                {
                    File_path line_for_breakpoint;
                }
            #endif

            Z_LOG2( "scheduler.log", "\nopen " << _filename << '\n' );

            open_file();

            _instance_number++;

            if( !_log_buffer.empty() )
            {
                write( _log_buffer.c_str(), _log_buffer.length() );
                _log_buffer = "";

                if( !_is_logging_continuing )
                {
                    string msg = "\n";
                    if( _title != "" )  msg += _title + " - ";
                    log( log_info, msg + "Protocol starts in " + _filename );       // "SCHEDULER-961"
                }
            }

            if( _open_and_close_every_line )  
            {
                close_file();
                _append = true;
            }
        }

        _started = true;
    }
}

//--------------------------------------------------------------------------------Prefix_log::close

void Prefix_log::close()
{
    _is_finished = true;

    if( _file != -1 )  
    {
        finish_log();

        /*
        try
        {
            //if( !_subject.empty()  ||  !_body.empty() )     // 20.11.2002
            {
                Scheduler_event::Scheduler_event_type event_code = _object->scheduler_type_code() == Scheduler_object::type_task? evt_task_ended 
                                                                                                                      : evt_unknown;
                Scheduler_event scheduler_event ( event_code, highest_level(), _object );
                send_really( &scheduler_event );
            }
        }
        catch( const exception&  x ) { _spooler->log()->error(x.what());                         _remove_after_close = false; }
        catch( const _com_error& x ) { _spooler->log()->error(bstr_as_string(x.Description()));  _remove_after_close = false; }
        */
    }

    if( _remove_after_close )
    {
        _remove_after_close = false;
        remove_file();
    }

    //signal_events();
    _events.clear();
}

//---------------------------------------------------------------------------Prefix_log::finish_log

void Prefix_log::finish_log()
{
    if( _file != -1 )  
    {
        try {
            log( log_info, message_string( "SCHEDULER-962", _filename ) );      // "Protokol ends in " 
        }
        catch( const exception& ) {}

        close_file();

        if( !_new_filename.empty() )
        {
            log( log_info, message_string( "SCHEDULER-963", _new_filename ) );
            copy_file( _filename, _new_filename );
            //int ret = rename( _filename.c_str(), _new_filename.c_str() );
            //if( ret == -1 )  throw_errno( errno, "rename", _new_filename.c_str() );
            _new_filename = "";
        }

        signal_events();
    }
}

//----------------------------------------------------------------------------Prefix_log::open_file

void Prefix_log::open_file()
{
    assert( _file == -1 );

    if( _file == -1 )
    {
        while(1)
        {
            _file = ::open( _filename.c_str(), O_CREAT | ( _append? O_APPEND : O_TRUNC ) | O_WRONLY | O_NOINHERIT, 0666 );
            if( _file != -1 )  break;
            
            if( !is_stop_errno( _spooler, errno ) ) throw_errno( errno, _filename.c_str(), "protocol file" );
            io_error( _spooler, _filename );
        }
    }
}

//---------------------------------------------------------------------------Prefix_log::close_file

void Prefix_log::close_file()
{
    if( _file != -1 )
    {
        try
        {
            //Z_LOG2( "scheduler", "close(" << _file << ")\n" );

            int ret = ::close( _file );
            if( ret == -1 )  throw_errno( errno, "close", _filename.c_str() );
        }
        catch( const exception& x ) { _spooler->log()->error( string("Error when closing protocol file: ") + x.what() ); }
            
        _file = -1;
    }
}

//--------------------------------------------------------------------------Prefix_log::remove_file

void Prefix_log::remove_file()
{
    try
    {
        z_unlink( _filename );
    }
    catch( const exception&  x ) { _spooler->log()->error( message_string( "SCHEDULER-291", x ) ); }  // Kann bei "http://.../show_log?" passieren
}

//--------------------------------------------------------------------------------Prefix_log::write

void Prefix_log::write( const char* text, int len )
{
    if( _err_no )  return;       // Falls nach einer Exception noch was ins Log geschrieben wird, ignorieren wir das.


    if( len == 0 )  return;

    if( _file == -1 )
    {
        if( this == _spooler->_log  ||  _spooler->log_directory() != "*stderr" )        // Datei wird noch geöffnet?
        {
            _log_buffer.append( text, len );
        }
    }
    else
    {
        int ret = my_write( _spooler, _filename, _file, text, len );
        if( ret != len )
        {
            _err_no = errno;
            throw_errno( errno, "write", _filename.c_str() );
        }
    }
}

//--------------------------------------------------------------------Prefix_log::set_mail_defaults

void Prefix_log::set_mail_defaults()
{
    if( _section != "" )
    {
        _mail_defaults.set( "queue_dir", read_profile_string ( _spooler->_factory_ini, _section, "mail_queue_dir"    , _mail_defaults[ "queue_dir" ] ) );
        _mail_defaults.set( "smtp"     , read_profile_string ( _spooler->_factory_ini, _section, "smtp"              , _mail_defaults[ "smtp"      ] ) );
        _mail_defaults.set( "from"     , read_profile_string ( _spooler->_factory_ini, _section, "log_mail_from"     , _mail_defaults[ "from"      ] ) );
        _mail_defaults.set( "to"       , read_profile_string ( _spooler->_factory_ini, _section, "log_mail_to"       , _mail_defaults[ "to"        ] ) );
        _mail_defaults.set( "cc"       , read_profile_string ( _spooler->_factory_ini, _section, "log_mail_cc"       , _mail_defaults[ "cc"        ] ) );
        _mail_defaults.set( "bcc"      , read_profile_string ( _spooler->_factory_ini, _section, "log_mail_bcc"      , _mail_defaults[ "bcc"       ] ) );
        _mail_defaults.set( "subject"  , read_profile_string ( _spooler->_factory_ini, _section, "log_mail_subject"  , _mail_defaults[ "subject"   ] ) );
    
        if( _mail )  _mail->set_mail_defaults( _mail_defaults );
    }

    _mail_defaults_set = true;
}

//---------------------------------------------------------------------Prefix_log::set_mail_default

void Prefix_log::set_mail_default( const string& field_name, const string& value, bool overwrite )
{ 
    if( overwrite  ||  !_mail_defaults.has_value( field_name ) )   
    {
        _mail_defaults.set( field_name, value ); 
        if( _mail )  _mail->set_mail_defaults( _mail_defaults );
    }
}

//--------------------------------------------------------------------------------Prefix_log::imail

Com_mail* Prefix_log::imail()
{
    //HRESULT hr;

    if( !_mail )
    {
        if( !_mail_defaults_set )  set_mail_defaults();

        ptr<Com_mail> mail = new Com_mail( _spooler );

        mail->init();
        mail->use_queue_defaults( _mail_defaults );
        mail->use_smtp_default  ( _mail_defaults );

        _mail = mail;   // Nur bei fehlerfreiem init() speichern
/*
        if( _smtp_server != "-" )
        {
            hr = _mail->put_Smtp( Bstr(_smtp_server) );     if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::smtp_server", _smtp_server.c_str() );
        }

        if( _queue_dir != "-" )
        {
            hr = _mail->put_Queue_dir( Bstr(_queue_dir) );     if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::queue_dir", _smtp_server.c_str() );
        }
        if( _mail_defaults[ "queue_dir" ] != "-" )
        {
            hr = _mail->put_Queue_dir( Bstr(_mail_defaults[ "queue_dir" ]) );     if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::queue_dir", _queue_dir.c_str() );
        }
*/

        /*
        if( _smtp_server != "-" )  _mail->set_smtp( _smtp_server );
        if( _from        != "-" )  _mail->set_from( _from );
                                   _mail->set_to  ( _to   );
        if( _cc          != "-" )  _mail->set_cc  ( _cc   );
        if( _bcc         != "-" )  _mail->set_bcc ( _bcc  );
        */
        //_mail->set_defaults( _mail_defaults );
        //set_mail_header();

        // Vorbesetzungen von spooler_task.cxx:
        /*
        if( !_from_name.empty() )  set_mail_from_name( _from_name, true ),  _from_name = "";   
        if( !_subject  .empty() )  set_mail_subject  ( _subject         ),  _subject   = "";
        if( !_body     .empty() )  set_mail_body     ( _body            ),  _body      = "";
        */

        //_mail->Add_header_field( Bstr(L"X-SOS-Spooler-Job"), Bstr( _job_name ) );
    }

    return _mail;
}

//----------------------------------------------------------------------Prefix_log::set_mail_header
/*
void Prefix_log::set_mail_header()
{
    HRESULT hr = NOERROR;

    if( _from != "-" )  hr = _mail->put_From( Bstr( _from ) );    if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::from", _from.c_str() );
                        hr = _mail->put_To  ( Bstr( _to   ) );    if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::to"  , _to.c_str() );
    if( _cc   != "-" )  hr = _mail->put_Cc  ( Bstr( _cc   ) );    if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::cc"  , _cc.c_str() );
    if( _cc   != "-" )  hr = _mail->put_Cc  ( Bstr( _cc   ) );    if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::cc"  , _cc.c_str() );
    if( _bcc  != "-" )  hr = _mail->put_Bcc ( Bstr( _bcc  ) );    if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::bcc" , _bcc.c_str() );
}
*/
//-----------------------------------------------------------------------Prefix_log::start_new_file

void Prefix_log::start_new_file()
{
    if( _prefix != "" 
     || _file != -1 
     || _order_log )
    {
        z::throw_xc( "SCHEDULER-225" );
    }

    _log->start_new_file();
}

//-------------------------------------------------------------------Prefix_log::set_mail_from_name
/*
void Prefix_log::set_mail_from_name( const string& from_name, bool overwrite )
{
    HRESULT hr;

    if( _mail )
    {
        if( !overwrite )
        {
            Bstr from_bstr;
            hr = _mail->get_From( &from_bstr._bstr );     if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::from" );

            if( SysStringLen(from_bstr) > 0 )  return;
        }

        Bstr old_from;
        hr = _mail->get_From( &old_from._bstr );                        if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::from" );
        if( !wcschr( old_from, '<' )  &&  wcschr( old_from, '@' ) )
        {
            string from = '"' + from_name + "\" <" + bstr_as_string(old_from) + ">";
            Bstr from_bstr = from;
            hr = _mail->put_From( from_bstr );                          if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::from", from.c_str() );
        }
    }
    else
    {
        if( !_from_name.empty()  &&  !overwrite )  return;
        _from_name = from_name;
    }
}
*/
//---------------------------------------------------------------------Prefix_log::set_mail_subject
/*
void Prefix_log::set_mail_subject( const string& subject, bool overwrite )
{
    HRESULT hr;

    if( _mail )
    {
        if( !overwrite )
        {
            Bstr subject_bstr;
            hr = _mail->get_Subject( &subject_bstr._bstr );  if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::subject" );

            if( SysStringLen(subject_bstr) > 0 )  return;
        }

        hr = _mail->put_Subject( Bstr(subject) );     if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::subject", subject.c_str() );
    }
    else
    {
        if( !_subject.empty()  &&  !overwrite )  return;
        _subject = subject;
    }
}
*/
//------------------------------------------------------------------------Prefix_log::set_mail_body
/*
void Prefix_log::set_mail_body( const string& body, bool overwrite )
{
    HRESULT hr;

    if( _mail )
    {
        if( !overwrite )
        {
            Bstr body_bstr;
            hr = _mail->get_Body( &body_bstr._bstr );   if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::body" );

            if( SysStringLen(body_bstr) > 0 )  return;
        }

        hr = _mail->put_Body( Bstr(body) );             if( FAILED(hr) ) throw_ole( hr, "scheduler::Mail::body", body.c_str() );
    }
    else
    {
        if( !_body.empty()  &&  !overwrite )  return;
        _body = body;
    }
}
*/
//---------------------------------------------------------------------------------Prefix_log::send

void Prefix_log::send( Scheduler_event* scheduler_event )
{
    if( !is_active()  &&  ( !_log || _log->filename() == "" ) )       // Nur senden, wenn die Log-Datei beschrieben worden ist
  //if( _file == -1  &&  ( !_log || _log->filename() == "" ) )       // Nur senden, wenn die Log-Datei beschrieben worden ist
    {
        //Z_LOG2( "joacim", "Prefix_log::send()  _file == -1\n" );
        _first_send = 0;
        _mail = NULL;
    }
    else
    {
        //bool mail_it =  _mail_it
        //             || reason == -1  &&  ( _mail_on_error | _mail_on_warning )
        //             || reason ==  0  &&  _mail_on_success
        //             || reason  >  0  &&  ( _mail_on_success || _mail_on_process && reason >= _mail_on_process )
        //             || _mail_on_warning  &&  _last.find( log_warn ) != _last.end();

        Time now = Time::now();

        //if( _first_send == 0  &&  !mail_it )
        //{
        //    finish_log();    // Protokoll nicht senden
        //    _mail = NULL;
        //}
        //else
        {
            if( _last_send  == 0  ||  _last_send  > now )  _last_send  = now;
            if( _first_send == 0  ||  _first_send > now )  _first_send = now;

            //Z_LOG2( "scheduler", "Prefix_log::send now=" << now << " _last_send+collect_within=" << Time(_last_send + _collect_within) << " _first_send+collectmax=" << Time(_first_send + _collect_max) << "\n" );
            //Z_LOG2( "scheduler", "Prefix_log::send now=" << now << " collect_within=" << _collect_within << " collectmax=" << _collect_max << "\n" );
            //Z_LOG2( "scheduler", "now >= _last_send + _collect_within:  " << (now >= _last_send + _collect_within) << "\n" );
            //Z_LOG2( "scheduler", "now >= _first_send + _collect_max  :  " << (now >= _first_send + _collect_max) << "\n" );

          //if( reason == -1  &&  _mail_on_error                 // Fehler?
          // || now >= _last_send  + _collect_within - 0.001     // Nicht mehr sammeln?  (ohne -0.001 liefert der Ausdruck manchmal false).
          // || now >= _first_send + _collect_max    - 0.001 )   // Lange genug gesammelt?
            {
                // Wenn die Protokolle in einer eMail gesammelt verschickt werden, wirken 
                // mail_on_error==false oder mail_on_process==false nicht wie gewünscht,
                // denn diese Bedingung wird erst festgestellt, wenn das Protokoll bereits geschrieben ist.

                finish_log();
                //Z_LOG2( "joacim", "Prefix_log::send_really()\n" );
                send_really( scheduler_event );

                _first_send = 0;
            }
        }
    }
 
    _last_send = Time::now();
}

//--------------------------------------------------------------------------Prefix_log::send_really

void Prefix_log::send_really( Scheduler_event* scheduler_event )
{
    assert( scheduler_event );


    int ok;

    if( filename() != "*stderr" )
    {
        imail()->Add_file( Bstr( filename() ), NULL, Bstr(L"text/plain"), Bstr(_spooler->_mail_encoding) );

        if( scheduler_event )  scheduler_event->set_log_path( filename() );
    }

    scheduler_event->set_mail( imail() );
    ok = scheduler_event->send_mail( _mail_defaults );


    if( ok )
    {
        try
        {
            imail()->auto_dequeue();
        }
        catch( const Xc& x ) { warn( message_string( "SCHEDULER-321", x ) ); }   // "eMail versendet, aber Fehler beim Verarbeiten der eMail-Warteschlange: "
    }
    else
        warn( message_string( "SCHEDULER-320" ) );   //"eMail konnte nicht versendet werden" );

    _mail = NULL;
}

//-----------------------------------------------------------------Prefix_log::is_enabled_log_level

bool Prefix_log::is_enabled_log_level( Log_level level )
{ 
    return log_level() <= level;
}

//---------------------------------------------------------------------------------Prefix_log::log2

struct Prefix_log_deny_recursion
{
    Prefix_log_deny_recursion( Prefix_log* l )
    : 
        _prefix_log( l ) 
    { 
        _prefix_log->_in_log = true; 
    }

    ~Prefix_log_deny_recursion()
    { 
        _prefix_log->_in_log = false; 
    }

    Prefix_log* const  _prefix_log;
};


void Prefix_log::log2( Log_level level, const string& prefix, const string& line_par, Has_log* log )
{
    string line = remove_password( line_par );
    
    if( !_log )  
    {
        Z_LOG2( "scheduler", Z_FUNCTION << "  _log==NULL  " << line << "\n" );
        return;
    }

    Z_MUTEX( _log->_semaphore )
    {
        if( _in_log )
        {
            Z_LOG2( "scheduler", "Rekursiv: " << line );
            return;
        }

        Prefix_log_deny_recursion deny_recursion ( this );


 
        if( level == log_error  &&  _task  &&  !_task->has_error() )  _task->set_error_xc_only( Xc( "SCHEDULER-140", line.c_str() ) );

        if( _highest_level < level )  _highest_level = level, _highest_msg = line;
        //if( level < log_level() )  return;

        _last_level = level;
        _last[ level ] = line;

      //if( level >= log_debug9  &&  level <= log_fatal )  _counter[ level - log_debug9 ]++;

        bool log_to_files = level >= log_level();

        {
            Open_and_close open_and_close ( this );

            _log->log2( level, log_to_files, _task? _task->obj_name() : _prefix, line, this, _order_log );
        }
    }
}

//----------------------------------------------------------------------------Prefix_log::add_event

void Prefix_log::add_event( Event_base* event )
{ 
    _events.push_back( event ); 
}

//-------------------------------------------------------------------------Prefix_log::remove_event

void Prefix_log::remove_event( Event_base* event )
{
    Z_FOR_EACH( list<Event_base*>, _events, e )
    {
        if( *e == event )  { _events.erase( e );  return; }
    }
}

//------------------------------------------------------------------------Prefix_log::signal_events

void Prefix_log::signal_events()
{
    Z_FOR_EACH( list<Event_base*>, _events, e )
    {
        (*e)->signal( "Log" );
    }
}

//-------------------------------------------------------Prefix_log::get_reference_with_properties
/*
ptr<object_server::Reference_with_properties> Prefix_log::get_reference_with_properties()
{
    ptr<object_server::Reference_with_properties> ref = Z_NEW( object_server::Reference_with_properties( "Log_proxy", NULL ) );  // IDispatch* wird von Com_log eingesetzt.

    ref->add_property( "log_level", _log->log_level() );

    return ref;
}
*/
//--------------------------------------------------------------------------Prefix_log::dom_element

xml::Element_ptr Prefix_log::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr log_element = document.createElement( "log" );

    if( !show.is_set( show_for_database_only ) )
    {
        if( log_level()    >= log_min   )  log_element.setAttribute( "level"          , name_of_log_level( (Log_level)log_level() ) );
        if( _highest_level >= log_min   )  log_element.setAttribute( "highest_level"  , name_of_log_level( (Log_level)_highest_level ) );
        if( last( log_error ) != ""     )  log_element.setAttribute( "last_error"     , last( log_error ) );
        if( last( log_warn  ) != ""     )  log_element.setAttribute( "last_warning"   , last( log_warn ) );
        if( last( log_info  ) != ""     )  log_element.setAttribute( "last_info"      , last( log_info ) );

        if( _filename != "" )
        {
            if( _mail_on_error              )  log_element.setAttribute( "mail_on_error"  , "yes" );
            if( _mail_on_warning            )  log_element.setAttribute( "mail_on_warning", "yes" );
            if( _mail_on_success            )  log_element.setAttribute( "mail_on_success", "yes" );
            if( _mail_on_process            )  log_element.setAttribute( "mail_on_process", _mail_on_process );
          //if( _mail_on_delay_after_error  )  log_element.setAttribute( "mail_on_delay_after_error ", ...( _mail_on_delay_after_error  ) );

          //string queue_dir   = _mail_defaults[ "queue_dir" ];
            string smtp_server = _mail_defaults[ "smtp"    ];
            string from        = _mail_defaults[ "from"    ];
            string to          = _mail_defaults[ "to"      ];
            string cc          = _mail_defaults[ "cc"      ];
            string bcc         = _mail_defaults[ "bcc"     ];
            string subject     = _mail_defaults[ "subject" ];

            if( _mail )
            {
                HRESULT hr;

                //hr = _mail->get_Queue_dir( &bstr );if( !FAILED(hr) )  queue_dir   = string_from_bstr( bstr );
                { Bstr bstr; hr = _mail->get_Smtp   ( &bstr._bstr );  if( !FAILED(hr) )  smtp_server = string_from_bstr( bstr ); }
                { Bstr bstr; hr = _mail->get_From   ( &bstr._bstr );  if( !FAILED(hr) )  from        = string_from_bstr( bstr ); }
                { Bstr bstr; hr = _mail->get_To     ( &bstr._bstr );  if( !FAILED(hr) )  to          = string_from_bstr( bstr ); }
                { Bstr bstr; hr = _mail->get_Cc     ( &bstr._bstr );  if( !FAILED(hr) )  cc          = string_from_bstr( bstr ); }
                { Bstr bstr; hr = _mail->get_Bcc    ( &bstr._bstr );  if( !FAILED(hr) )  bcc         = string_from_bstr( bstr ); }
                { Bstr bstr; hr = _mail->get_Subject( &bstr._bstr );  if( !FAILED(hr) )  subject     = string_from_bstr( bstr ); }
            }

          //if( queue_dir   != "" )  log_element.setAttribute( "queue_dir"   , queue_dir );
            if( smtp_server != "" )  log_element.setAttribute( "smtp"        , smtp_server );
            if( from        != "" )  log_element.setAttribute( "mail_from"   , from );
            if( to          != "" )  log_element.setAttribute( "mail_to"     , to );
            if( cc          != "" )  log_element.setAttribute( "mail_cc"     , cc );
            if( bcc         != "" )  log_element.setAttribute( "mail_bcc"    , bcc );
            if( subject     != "" )  log_element.setAttribute( "mail_subject", subject );
        }
    }

    if( show.is_set( show_log ) )
    {
        try
        {
            log_element.appendChild( document.createTextNode( as_string() ) );
        }
        catch( exception& x ) 
        { 
            _spooler->log()->warn( x.what() ); 
            log_element.appendChild( document.createTextNode( x.what() ) );
        }
    }


    return log_element;
}

//-------------------------------------------------------------------Prefix_log::continue_with_text

void Prefix_log::continue_with_text( const string& text )
{
    if( is_active() )
    {
        z::throw_xc( Z_FUNCTION, "log shall not be active" );
    }
    else
    {
        string b = _log_buffer;     // Sollte leer sein
        _log_buffer = text;
        _log_buffer += b;
    }

    if( _log_buffer != "" )  _is_logging_continuing = true;
}

//----------------------------------------------------------------------------Prefix_log::as_string

string Prefix_log::as_string()
{
    string result;

    if( _started )
    {
#       ifdef Z_WINDOWS

            result = File( filename(), "r" ).read_all();        // Ersetzt \r\n zu \n

#        else

            result = string_from_file( filename() );

#       endif
    }

    result.append( _log_buffer );
    return result;
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

} //namespace scheduler
} //namespace sos
