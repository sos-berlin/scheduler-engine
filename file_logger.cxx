// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#include "file_logger.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------------------------------const

const int                       max_line_length             = 10000;
const double                    read_interval_min           =   1.0;
const double                    read_interval_max           =  10.0;

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

void File_logger::add_file( const File_path& path, const string& prefix )
{
    Z_LOG2( "joacim", Z_FUNCTION << " " << path << " (" << prefix << ")\n" );

    if( path != "" )  
    {
        _file_line_reader_list.push_back( Z_NEW( File_line_reader( path, prefix ) ) );
    }
}

//-------------------------------------------------------------------------------File_logger::close

void File_logger::close()
{
    set_async_manager( NULL );

    if( _thread )  
    {
        _thread->terminate();
        _thread->thread_wait_for_termination();
        _thread->thread_close();
        _thread = NULL;
    }

    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;

        if( _remove_files )  file_line_reader->_file.unlink_later();
        file_line_reader->close();
    }

    _file_line_reader_list.clear();
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

bool File_logger::flush_lines()
{
    bool something_done = false;

    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;
        something_done |= log_lines( file_line_reader->read_lines() );
    }

    return something_done;
}

//------------------------------------------------------------------------------File_logger::finish

bool File_logger::flush()
{
    bool   something_done = false;
    string s;

    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;

        while(1)
        {
            s = file_line_reader->read_lines();
            if( s == "" )  break;
            something_done |= log_lines( s );
        }
    }

    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;

        while(1)
        {
            s = file_line_reader->read_remainder();
            if( s == "" )  break;
            something_done |= log_lines( s );
        }
    }

    // Nicht rufen, wenn wir in eigenem Thread sind:  close();

    return something_done;
}

//------------------------------------------------------------------------------File_logger::finish

void File_logger::finish()
{
    flush();

    Z_FOR_EACH( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;

        file_line_reader->close();
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
    // Kann von anderem Thread gerufen werden!

    S result;
    result << "File_logger(";
    result << _for_object;
    
    Z_FOR_EACH_CONST( File_line_reader_list, _file_line_reader_list, it )
    {
        File_line_reader* file_line_reader = *it;
        //if( it != _file_line_reader_list.begin() )  
        result << ", ";
        result << file_line_reader->_prefix << ": " << file_line_reader->_read_length << " bytes";
    }

    result << ")";

    return result;
}

//---------------------------------------------------------------------------File_logger::log_lines

bool File_logger::log_lines( const string& lines )
{
    bool something_done = false;

    if( lines != "" )  
    {
        _log->info( lines );
        something_done = true;
    }

    return something_done;
}

//--------------------------------------------------File_logger::File_line_reader::File_line_reader

File_logger::File_line_reader::File_line_reader( const File_path& path, const string& prefix )                                      
: 
    _zero_(this+1),
    _prefix(prefix)
{
    _file.open( path, "rb" );
}

//-------------------------------------------------------------File_logger::File_line_reader::close
    
void File_logger::File_line_reader::close()
{
    _file.close();
}

//--------------------------------------------------------File_logger::File_line_reader::read_lines

string File_logger::File_line_reader::read_lines()
{
    string lines;

    if( _file.opened()  &&  _file.length() > _read_length )
    {
        _file.seek( _read_length );
        lines = _file.read_string( max_line_length );
        size_t nl = lines.rfind( '\n' );

        if( nl == string::npos )    // Kein \n
        {
            if( lines.length() < max_line_length )  lines = "";    // Mehr Text als max_line_length wird umgebrochen
            nl = lines.length();
        }

        lines.erase( nl );
        _read_length += lines.length();
    }

    return lines;
}

//----------------------------------------------------File_logger::File_line_reader::read_remainder

string File_logger::File_line_reader::read_remainder()
{
    string result;

    if( _file.opened() )
    {
        _file.seek( _read_length );
        result = _file.read_string( max_line_length );
        _read_length += result.length();
    }

    return result;
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
// Anderer Thread!

void File_logger::File_logger_thread::terminate()
{
    _terminate_event.signal( Z_FUNCTION );
}

//-----------------------------------------------------File_logger::File_logger_thread::thread_main

int File_logger::File_logger_thread::thread_main()
{
    Z_LOG2( "joacim", Z_FUNCTION << "\n" );

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

    Z_LOG2( "joacim", Z_FUNCTION << "  terminates\n" );
    return 0;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
