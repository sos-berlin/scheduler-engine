// $Id: file_logger.cxx 15047 2011-08-26 08:23:12Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "file_logger.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

const int                       max_line_length             = 10000;
const double                    read_interval_min           =   1.0;
const double                    read_interval_max           =  10.0;

//-------------------------------------------------------------------------------------------------

struct File_logger_opener {
    private: File _file;
    public: string _error_message;

    public: File_logger_opener(const File_path& path, const string& name) {
        try {
            _file.open(path, "rb");
        } catch (exception& x) {
            string m = message_string("SCHEDULER-478", name, x.what());
            _error_message = "*** " + m + " ***";
            Z_LOG2("scheduler", m << "\n");
        }
    }

    public: File* operator->() { return &_file; }
};

//-------------------------------------------------------------------------File_logger::File_logger

File_logger::File_logger( Has_log* log )
:
    _zero_(this+1),
    _log(log)
{
}

//------------------------------------------------------------------------File_logger::~File_logger
    
File_logger::~File_logger()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << "  " << x.what() << "\n" ); }
}

//----------------------------------------------------------------------------File_logger::add_file

void File_logger::add_file( const File_path& path, Log_level level, const string& name )
{
    if( path != "" )  
    {
        Z_LOG2( "zschimmer", Z_FUNCTION << "(\"" << path << "\",\"" << name << "\")\n" );
        _file_line_reader_list.push_back(Z_NEW(File_line_reader(path, _log, level, name)));
    }
}

//-------------------------------------------------------------------------------File_logger::close

void File_logger::close()
{
    Z_MUTEX(_mutex) {
        set_async_manager( NULL );
    }

    if( _thread )  
    {
        _thread->terminate();
        _thread->thread_wait_for_termination();
        _thread->thread_close();
        _thread = NULL;
    }

    Z_MUTEX(_mutex) {
        Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
        {
            File_line_reader* file_line_reader = *it;

            //if( _remove_files )  file_line_reader->_file.unlink_later();
            file_line_reader->close();
        }

        _file_line_reader_list.clear();
    }
}

//-------------------------------------------------------------------------------File_logger::start

void File_logger::start()
{
    if( !_file_line_reader_list.empty() )
    {
        set_async_delay( read_interval_min );
    }
}

//------------------------------------------------------------------------File_logger::start_thread

void File_logger::start_thread()
{
    assert( !_thread );

    _thread = Z_NEW( File_logger_thread( this ) );
    _thread->thread_start();
}

//-------------------------------------------------------------------------File_logger::flush_lines

void File_logger::flush_lines()
{
    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;
        file_line_reader->log_lines();
    }
}

//-------------------------------------------------------------------------------File_logger::flush

void File_logger::flush()
{
    Z_MUTEX(_mutex) {
        string s;

        if( has_files() )
        {
            fflush( stdout );
            fflush( stderr );

            //#ifdef Z_WINDOWS
            //    _commit( fileno( stdout ) );        // Debug-assert(), wenn Datei nicht geöffnet
            //    _commit( fileno( stderr ) );        // Debug-assert(), wenn Datei nicht geöffnet
            //#else
            //    fsync( fileno( stdout ) );
            //    fsync( fileno( stderr ) );
            //#endif

            // Die Ausgaben von PerlScript gehen verloren. In welchem Puffer stehen die?
        }


        Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
        {
            File_line_reader* file_line_reader = *it;
            while (file_line_reader->log_lines());
        }

        Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
        {
            File_line_reader* file_line_reader = *it;
            file_line_reader->log_remainder();
        }
    }
}

//------------------------------------------------------------------------------File_logger::finish

void File_logger::finish()
{
    Z_MUTEX(_mutex) {
        flush();

        Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
        {
            File_line_reader* file_line_reader = *it;

            file_line_reader->close();
        }
    }
}

//---------------------------------------------------------------------File_logger::async_continue_

bool File_logger::async_continue_( Async_operation::Continue_flags )
{
    bool something_done = false;

    Z_MUTEX( _mutex )
    {
        flush_lines();
    }

    set_async_delay( something_done? read_interval_min : read_interval_max );
    return true;
}

//-------------------------------------------------------------------File_logger::async_state_text_

string File_logger::async_state_text_() const
{
    S result;
    result << "File_logger(";
    Z_MUTEX(_mutex) {
        result << _for_object;
    
        Z_FOR_EACH_CONST( File_line_reader_list, _file_line_reader_list, it )
        {
            File_line_reader* file_line_reader = *it;
            result << ", ";
            result << file_line_reader->_name << ": " << file_line_reader->_read_length << " bytes";
        }

    }
    result << ")";
    return result;
}

//--------------------------------------------------File_logger::File_line_reader::File_line_reader

File_logger::File_line_reader::File_line_reader(const File_path& path, Has_log* log, Log_level level, const string& name)
: 
    _zero_(this+1),
    _path(path),
    _log(log),
    _log_level(level),
    _name(name)
{
}

//-------------------------------------------------------------File_logger::File_line_reader::close
    
void File_logger::File_line_reader::close()
{
}


bool File_logger::File_line_reader::log_lines() {
    string lines = read_lines();
    if (lines == "")
        return false;
    else {
        _log->log(_log_level, lines);
        return true;
    }
}

void File_logger::File_line_reader::log_remainder() {
    string lines = read_lines();
    if ( lines != "") {
        _log->log(_log_level, lines);
    }
}

//--------------------------------------------------------File_logger::File_line_reader::read_lines

string File_logger::File_line_reader::read_lines()
{
    string lines;

    if (!_error) {  // Damit Fehler nicht wiederholt gemeldet wird
        File_logger_opener file (_path, _name); 
        if (file->opened()) {
            if (file->length() > _read_length) {
                file->seek( _read_length );

                lines = file->read_string( max_line_length );

                size_t nl  = lines.rfind( '\n' );
                size_t end;

                if( nl == string::npos )    // Kein \n
                {
                    if( lines.length() < max_line_length )  lines = "";    // Mehr Text als max_line_length wird umgebrochen
                    nl = lines.length();
                    end = nl;
                }
                else
                    end = nl + 1;

                size_t before_end = nl;
                if( before_end > 0  &&  lines[ before_end - 1 ] == '\r' )  --before_end;

                lines.erase( before_end );
                _read_length += end;
            }
        } else {
            _error = true;
            lines = file._error_message;
        }
    }

    return prefix_with_name(lines);
}

//----------------------------------------------------File_logger::File_line_reader::read_remainder

string File_logger::File_line_reader::read_remainder()
{
    string result;
    
    if (!_error) {
        File_logger_opener file (_path, _name);
        if (file->opened()) {
            file->seek( _read_length );
            result = file->read_string( max_line_length );
            _read_length += result.length();
        } else {
            _error = true;
            result = file._error_message;
        }
    }

    return prefix_with_name(result);
}

string File_logger::File_line_reader::prefix_with_name(const string& lines) const {
    if (_name == "") {
        return lines;
    } else {
        std::vector<string> result;
        for (size_t i = 0; i < lines.length();) {
            result.push_back("[" + _name + "] ");
            size_t line_end = lines.find('\n', i);
            line_end = line_end == string::npos? lines.length() : line_end + 1;
            result.push_back(lines.substr(i, line_end - i));
            i = line_end;
        }
        return join("", result);
    }
}

//----------------------------------------------File_logger::File_logger_thread::File_logger_thread

File_logger::File_logger_thread::File_logger_thread( File_logger* file_logger )
:
    _zero_(this+1),
    _file_logger( file_logger )
{
    set_thread_name( "File_logger_thread" );

    _terminate_event.set_name( Z_FUNCTION );
    _terminate_event.create();
}

//---------------------------------------------File_logger::File_logger_thread::~File_logger_thread
    
File_logger::File_logger_thread::~File_logger_thread()
{
    if( _file_logger )  _file_logger->set_async_manager( NULL );
}

//-------------------------------------------------------File_logger::File_logger_thread::terminate

void File_logger::File_logger_thread::terminate()
{
    _terminate_event.signal( Z_FUNCTION );
}

//-----------------------------------------------------File_logger::File_logger_thread::thread_main

int File_logger::File_logger_thread::thread_main()
{
    Z_LOG2( "zschimmer", Z_FUNCTION << "\n" );

    ptr<File_logger> file_logger = _file_logger;    // Halten

    Z_MUTEX( file_logger->_mutex )
    {
        file_logger->set_async_manager( &_async_manager );
        file_logger->start();
    }

    while( !_terminate_event.signaled() )
    {
        double now = double_from_gmtime();
        _terminate_event.wait( _async_manager.async_next_gmtime() - now );
        _async_manager.async_continue();
    }

    Z_MUTEX( file_logger->_mutex )
    {
        file_logger->finish();
        file_logger->set_async_manager( NULL );
    }

    _file_logger = NULL;
    file_logger = NULL;

    Z_LOG2( "zschimmer", Z_FUNCTION << "  terminates\n" );
    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
