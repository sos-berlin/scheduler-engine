// $Id: spooler_log.cxx 15049 2011-08-26 09:08:36Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

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

const int log_buffer_max = 100*1000;

namespace sos {
namespace scheduler {

using namespace log;

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
        spooler->set_state( Spooler::s_paused );

        string error_code = "ERRNO-" + as_string( spooler->_waiting_errno );
        zschimmer::Xc x ( error_code.c_str(), filename.c_str() );

        Z_LOGI2( "scheduler", "\n*** SCHEDULER HAELT WEGEN PLATTENPLATZMANGEL AN. " << x.what() << ", Datei " << filename << "\n\n" );

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

        if (spooler->_connection_manager) {
            while (spooler->_state_cmd != Spooler::sc_continue) {
                int wait_seconds = 1;
                spooler->_connection_manager->async_continue_selected( is_allowed_operation_while_waiting, wait_seconds );   // Kann ins scheduler.log schreiben!
            }
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
        ret = ::write( file, t, int_cast(text + len - t) );

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
            ret = ::write( file, t, int_cast(text + len - t) );
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
    _spooler(spooler)
{
    _file = -1;
}

//----------------------------------------------------------------------------------------Log::~Log

Log::~Log()         
{
    if( _file != -1  &&  _file != fileno(stderr) )  ::close( _file ),  _file = -1;
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
    int       old_file     = _file;
    File_path old_filename = _filename;

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
        string line = "\nDas Protokoll wird fortgefuehrt in " + _filename + "\n";
        ::write( old_file, line.c_str(), int_cast(line.length()) );
        ::close( old_file );
    }

    if( _log_buffer.length() > 0  &&  _file != -1 )
    {
        int ret = my_write( _spooler, _filename, _file, _log_buffer.c_str(), int_cast(_log_buffer.length()) );
        if( ret != _log_buffer.length() )  
        {
            _err_no = errno;
            throw_errno( errno, "write", _filename.c_str() );
        }

        _log_buffer = "";
    }
}

//------------------------------------------------------------------------------Log::start_new_file

void Log::start_new_file()
{
    string old_filename = _filename;
    info( message_string( "SCHEDULER-967" ) );   // "start_new_file(): Die Protokolldatei wird geschlossen"

    string old_log_categories = current_log_categories();
    string old_log_filename = log_filename();
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
        if (!(extra_log && extra_log->_task) && !order_log) {
            if( _file != -1  && 
                ( !_spooler->_log_to_stderr ||  _file != fileno(stderr) ) )     // Nicht doppelt nach stderr schreiben
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
            if (_spooler->_log_to_stderr && level >= _spooler->_log_to_stderr_level) {
                my_write(_spooler, "(stderr)", fileno(stderr), text, len);
            }
        }

        if( extra_log )  extra_log->write( text, len );
        if( order_log )  order_log->write( text, len );

        if (_corresponding_prefix_log) _corresponding_prefix_log->on_logged();
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
        fprintf( stderr, "Error when writing log file: %s\n", x.what() );
        Z_LOG2( "scheduler", "Error when writing log file: " << x.what() << "\n" );
        
        if( level < log_error )  throw;     // Bei error() Exception ignorieren, denn die Funktion wird gerne in Exception-Handlern gerufen
    }
}

//----------------------------------------------------------------------------------------Log::log2

void Log::log2( Log_level level, bool log_to_files, const string& prefix, const string& line_, Prefix_log* extra_log, Prefix_log* order_log )
{
    if( this == NULL )  return;

    if (log_to_files || log_category_is_set("scheduler.mainlog")) {
        Log_set_console_colors console_colors ( _spooler );

        char time_buffer [50];   time_buffer[0] = '\0';
        char level_buffer[50];
        string line = line_;
        for( size_t i = line.find( '\r' ); i != string::npos; i = line.find( '\r', i+1 ) )  line[i] = ' ';     // Windows scheint sonst doppelte Zeilenwechsel zu schreiben. jz 25.11.03

        if( log_to_files )
        {
            if( _file != -1  &&  isatty( _file )  ||  _spooler->_log_to_stderr  &&  level >= _spooler->_log_to_stderr_level  &&  isatty( fileno( stderr ) ) )  console_colors.set_color_for_level( level );

            Time now = Time::now();
            _last_time = now;
            strcpy( time_buffer, now.as_string(_spooler->_time_zone_name).c_str() );
        }
        
        switch( level )
        {
            case log_error: strcpy ( level_buffer, " [ERROR]  " );  break;
            case log_warn : strcpy ( level_buffer, " [WARN]   " );  break;
            case log_info : strcpy ( level_buffer, " [info]   " );  break;
            case log_debug: strcpy ( level_buffer, " [debug]  " );  break;
            case log_none : strcpy ( level_buffer, " [none]   " );  break;          // Passiert, wenn nur das scheduler.log beschrieben werden soll (log_to_files==false)
            case log_unknown:strcpy( level_buffer, " [unknown]" );  break;          // Sollte nicht passieren
            default:        snprintf( level_buffer, sizeof level_buffer, " [debug%d] ", (int)-level );
        }


        size_t begin = 0;
        while(1) {
            int level_len = int_strlen(level_buffer);   // " [info]"
            string prefix_string;
            if (!prefix.empty()) {
                prefix_string.reserve(prefix.length() + 3); 
                prefix_string = "(";
                prefix_string += prefix;
                prefix_string += ") ";
            }
            size_t next = line.find('\n', begin);  
            string line_end;
            if (next == string::npos)  next = int_cast(line.length()), line_end = "\n"; 
                                 else  next++;
            size_t len = next - begin;
            while (len > 1  &&  line.c_str()[begin+len-1] == '\r')  len--;
            
            if (z::Log_ptr log = "scheduler.mainlog") {
                log->write(level_buffer + 1, level_len - 1);
                log << prefix_string;
                log->write(line.data() + begin, len);
                log << line_end;
            }

            if (log_to_files) {
                write(level, extra_log, order_log, time_buffer, int_strlen(time_buffer));   // Zeit
                write(level, extra_log, order_log, level_buffer, level_len);            // [info]
                if (!(extra_log && extra_log->_task)) {
                    write(level, NULL, order_log, prefix_string);                           // (Job ...)
                }
                write(level, extra_log, order_log, line.c_str() + begin, int_cast(len));    // Text
                write(level, extra_log, order_log, line_end.data(), int_cast(line_end.length())); // "\n"
            }

            begin = next;
            if( begin >= line.length() )  break;
        }

        if (log_to_files) {
            if( extra_log )  extra_log->signal_events();
            if( order_log )  order_log->signal_events();

            if( this == &_spooler->_base_log )  _spooler->log()->signal_events();   // Nicht schön, aber es gibt sowieso nur ein Log.
        }
    }
}

//---------------------------------------------------------------------------Prefix_log::Prefix_log

Prefix_log::Prefix_log( int )
:
    _zero_(this+1),
    _file(-1),
    _mail_defaults(NULL)
{
}

//---------------------------------------------------------------------------Prefix_log::Prefix_log

Prefix_log::Prefix_log(Scheduler_object* o)
:
    javabridge::has_proxy<Prefix_log>(o->spooler()),
    _zero_(this+1),
    _object(o),
    _spooler(o->spooler()),
    _log(&o->spooler()->_base_log),
    _prefix( o->obj_name() ),
    _file(-1),
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

void Prefix_log::init(Scheduler_object* o, const string& prefix)
{
    reset_highest_level();

    _mail_defaults = o->spooler()->_mail_defaults;
    _object  = o;
    _spooler = o->spooler();
    _log     = &o->spooler()->_base_log;
    _prefix  = prefix;

    _mail_on_warning = _spooler->_mail_on_warning;
    _mail_on_error   = _spooler->_mail_on_error;
    _mail_on_process = _spooler->_mail_on_process;
    _mail_on_success = _spooler->_mail_on_success;
    _mail_on_delay_after_error = _spooler->_mail_on_delay_after_error;
    _collect_within  = _spooler->_log_collect_within;
    _collect_max     = _spooler->_log_collect_max;

    _append_for_cache = false;
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
        _collect_within  = Duration(read_profile_uint( _spooler->_factory_ini, _section, "log_collect_within", (uint)_collect_within.as_double() ));
        _collect_max     = Duration(read_profile_uint( _spooler->_factory_ini, _section, "log_collect_max"   , (uint)_collect_max.as_double() ));
    }
}

//---------------------------------------------------------------------Prefix_log::set_dom_settings

void Prefix_log::set_dom_settings( xml::Element_ptr settings_element )
{
    if( settings_element )
    {
        if( xml::Element_ptr e = settings_element.select_node( "log_level"          ) )  _log_level = make_log_level( e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "mail_on_error"      ) )  _mail_on_error   = as_bool( e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "mail_on_warning"    ) )  _mail_on_warning = as_bool( e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "mail_on_success"    ) )  _mail_on_success = as_bool( e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "mail_on_process"    ) )  _mail_on_process = as_bool( e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "mail_on_delay_after_error" ) )  _mail_on_delay_after_error = make_yes_no_last_both( "mail_on_delay_after_error", e.text(), _mail_on_delay_after_error );
        if( xml::Element_ptr e = settings_element.select_node( "log_mail_to"        ) )  _mail_defaults.set( "to" , e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "log_mail_cc"        ) )  _mail_defaults.set( "cc" , e.text() );
        if( xml::Element_ptr e = settings_element.select_node( "log_mail_bcc"       ) )  _mail_defaults.set( "bcc", e.text() );

        if( _mail )  _mail->set_mail_defaults( _mail_defaults );
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
    _collect_within  = other._collect_within;
    _collect_max     = other._collect_max;

    _mail_defaults = other._mail_defaults;
    if( _mail )  _mail->set_mail_defaults( _mail_defaults );
}

//-------------------------------------------------------------------------Prefix_log::set_filename

void Prefix_log::set_filename( const File_path& filename )
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

//----------------------------------------------------------------------Prefix_log::open_dont_cache

void Prefix_log::open_dont_cache() {
    open();
    _inhibit_caching_request = _spooler->_log_file_cache->request(this);
}

//---------------------------------------------------------------------------------Prefix_log::open

void Prefix_log::open()
{
    if( !is_active() )
    {
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

            open_patiently_file();

            _instance_number++;
            _spooler->_log_file_cache->cache(this);
        }

        if (typed_java_sister()) typed_java_sister().onStarted(_spooler->java_sister());
        _started = true;
    }
}

//--------------------------------------------------------------------------------Prefix_log::close

void Prefix_log::close()
{
    _inhibit_caching_request = NULL;
    if (_started)  
        finish_log();
    if (_spooler && _spooler->_log_file_cache)  // Bei Programmende kann der Cache weg sein.
        _spooler->_log_file_cache->remove(this);
    close_file();
    try {
        if (!_closed && typed_java_sister()) typed_java_sister().onClosed();
    } catch (const exception& x) {
        Z_LOG2("scheduler", Z_FUNCTION << " " << x.what() << "\n");
    }
    _log = NULL;

    if( _remove_after_close )
    {
        _remove_after_close = false;
        remove_file();
    }

    _events.clear();
    _closed = true;
}

//---------------------------------------------------------------------------Prefix_log::finish_log

void Prefix_log::finish_log()
{
    if (!_is_finished) {
        _is_finished = true;

        if (_spooler && _spooler->_log_file_cache) {
            if (ptr<cache::Request> request = _spooler->_log_file_cache->request(this)) {
                try {
                    log( log_info, message_string( "SCHEDULER-962", _filename ) );      // "Protokol ends in " 
                }
                catch( const exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "()  ERROR " << x.what() << "\n" ); }
            }
        }

        close_file();
        _append_for_cache = false;

        if( !_new_filename.empty() )
        {
            log( log_info, message_string( "SCHEDULER-963", _new_filename ) );
            copy_file( _filename, _new_filename );
            _new_filename = "";
        }

        signal_events();
    }
}

//------------------------------------------------------------------Prefix_log::open_patiently_file

void Prefix_log::open_patiently_file()
{
    assert( _file == -1 );

    if( _file == -1 )
    {
        while(1)
        {
            open_file_without_error();
            if( _file != -1 )  break;
            
            if( !is_stop_errno( _spooler, errno ) )  check_open_errno();
            io_error( _spooler, _filename );
        }
    }
}

//----------------------------------------------------------------------Prefix_log::try_reopen_file

void Prefix_log::try_reopen_file() {
    if (is_active()  &&   _file == -1  &&   !_filename.empty()) {
        try {
            open_file();
        } catch (exception& x) {
            Message_string m ("SCHEDULER-477", x.what());
            if (_log) _log->error(m);
            else Z_LOG2("scheduler", "ERROR: " << m.as_string() << "\n");
        }
    }
}

//----------------------------------------------------------------------------Prefix_log::open_file

void Prefix_log::open_file() {
    assert(_file == -1);

    if (_file == -1) {
        open_file_without_error();
        check_open_errno();
    }
}

//--------------------------------------------------------------Prefix_log::open_file_without_error

void Prefix_log::open_file_without_error() {
    bool open_for_append = (_append || _append_for_cache);
    _file = ::open( _filename.c_str(), O_CREAT | ( open_for_append ? O_APPEND : O_TRUNC ) | O_WRONLY | O_NOINHERIT, 0666 );
    if(_file != -1 )
    {
       Z_LOG2("scheduler","logfile " << _filename << " opened (append=" << _append << ", append_for_cache=" << _append_for_cache << ")\n");
       if(!_log_buffer.empty() )
       {
          if( !_is_logging_continuing && ! open_for_append)
            {
               string msg = "\n";
               if( _title != "" )  msg += _title + " - ";
               log( log_info, msg + "Protocol starts in " + _filename );       // "SCHEDULER-961"
            }
            write( _log_buffer.c_str(), int_cast(_log_buffer.length()) );
            _log_buffer = "";
       }
    }
}

//---------------------------------------------------------------------Prefix_log::check_open_errno

void Prefix_log::check_open_errno() {
    if( _file == -1 )
        throw_errno(errno, _filename.c_str(), "protocol file");
}

//---------------------------------------------------------------------------Prefix_log::close_file

void Prefix_log::close_file()
{
    if( _file != -1 )
    {
        Z_DEBUG_ONLY(Z_LOG2("JS-611", Z_FUNCTION << " " << _filename << "\n"));  // JS611IT liest das
        try
        {
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
    catch( const exception&  x ) 
    { 
        _spooler->log()->error( message_string( "SCHEDULER-291", x ) );       // Kann bei "http://.../show_log?" passieren
    }
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
            if (!_log_buffer_is_full) {
                if (_log_buffer.length() + len > log_buffer_max) {
                    _log_buffer_is_full = true;
                    string msg = "Buffered log for deferred opened log file grows too big and is truncated";
                    _log_buffer.append("\n" "... (" + msg + ") ...\n");
                    Z_LOG2("scheduler", msg << "\n");  // Wird denn open_file() nicht aufgerufen?
                }
                else
                    _log_buffer.append( text, len );
            }
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

    on_logged();
}

void Prefix_log::on_logged() {
    if (typed_java_sister()) typed_java_sister().onLogged();
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
    if( !_mail )
    {
        if( !_mail_defaults_set )  set_mail_defaults();

        ptr<Com_mail> mail = new Com_mail( _spooler );

        mail->init();
        mail->use_queue_defaults( _mail_defaults );
        mail->use_smtp_default  ( _mail_defaults );
        mail->set_mail_defaults ( _mail_defaults );

        _mail = mail;   // Nur bei fehlerfreiem init() speichern
    }

    return _mail;
}

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

//---------------------------------------------------------------------------------Prefix_log::send

void Prefix_log::send( Scheduler_event* scheduler_event )
{
    if( !is_active()  &&  ( !_log || _log->filename() == "" ) )       // Nur senden, wenn die Log-Datei beschrieben worden ist
    {
        _first_send = Time(0);
        _mail = NULL;
    }
    else
    {
        Time now = Time::now();

        if( _last_send.is_zero()  ||  _last_send  > now )  _last_send  = now;
        if( _first_send.is_zero()  ||  _first_send > now )  _first_send = now;


        // Wenn die Protokolle in einer eMail gesammelt verschickt werden, wirken 
        // mail_on_error==false oder mail_on_process==false nicht wie gewünscht,
        // denn diese Bedingung wird erst festgestellt, wenn das Protokoll bereits geschrieben ist.

        // Datei kann offen bleiben   //finish_log();
        send_really( scheduler_event );

        _first_send = Time(0);
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

//-----------------------------------------------------------------------------Prefix_log::obj_name

string Prefix_log::obj_name() const { 
    return S() << "Prefix_log(" << _prefix << ")"; 
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
    if (level <= log_none) return;

    string line = remove_password( line_par );
    
    if( !_log )  
    {
        Z_LOG2( "scheduler", Z_FUNCTION << "  _log==NULL  " << line << "\n" );
        return;
    }

    if( _in_log )
    {
        Z_LOG2("scheduler", "Recursive log output: " << line << "\n");
        return;
    }

    Prefix_log_deny_recursion deny_recursion ( this );

    if( level == log_error  &&  _task  &&  !_task->has_error() )  _task->set_error_xc_only( Xc( "SCHEDULER-140", line.c_str() ) );

    if( _highest_level < level )  _highest_level = level, _highest_msg = line;

    _last_level = level;
    _last[ level ] = line;

    string my_prefix = _task? _task->obj_name() : _prefix;
    {
        ptr<cache::Request> request = _spooler->_log_file_cache->request(this);
        bool log_to_files = level >= log_level();
        _log->log2( level, log_to_files, my_prefix, line, this, _order_log );
    }

    if (_object && javabridge::Vm::is_active()) {
        if (Spooler* sp = _object->spooler())
            if (Event_subsystem* event_subsystem = sp->event_subsystem()) 
                if (_spooler->java_subsystem())
                    event_subsystem->report_logged(level, my_prefix, line);
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
            log_element.appendChild(document.createTextNode( _spooler->truncate_head(as_string()) ));
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

//--------------------------------------------------------------------Prefix_log::typed_java_sister

PrefixLogJ& Prefix_log::typed_java_sister() {
    if (!_typed_java_sister && javabridge::Vm::is_active())
        _typed_java_sister = java_sister();
    return _typed_java_sister;
}

//---------------------------------------------------------------Prefix_log::as_string_ignore_error

string Prefix_log::as_string_ignore_error()
{
    string result;

    try
    {
        result = as_string();
    }
    catch( exception& x )
    {
        _spooler->log()->warn( x.what() );
        result = x.what();
    }

    return result;
}

//----------------------------------------------------------------------------Prefix_log::as_string

string Prefix_log::as_string()
{
    string result;

    if( _started )
    {
        #ifdef Z_WINDOWS

            result = File( filename(), "r" ).read_all();        // Ersetzt \r\n zu \n

        #else

            result = string_from_file( filename() );

        #endif
    }

    result.append( _log_buffer );

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
