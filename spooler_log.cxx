// $Id: spooler_log.cxx,v 1.11 2001/01/24 12:35:51 jz Exp $

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
        if( !_spooler->_spooler_id.empty() )  filename += "." + _spooler->_spooler_id;
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

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
