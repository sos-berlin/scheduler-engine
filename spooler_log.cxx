// $Id: spooler_log.cxx,v 1.5 2001/01/13 10:45:52 jz Exp $

#include "../kram/sos.h"
#include "../kram/sosdate.h"
#include "spooler.h"

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------Typbibliothek

Typelib_descr   spooler_typelib ( LIBID_spooler, "Spooler", "1.0" );
DESCRIBE_CLASS( &spooler_typelib, Com_log, spooler_log, CLSID_Spooler_log, "Spooler.Log", "1.0", 0 );

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

    if( _file  &&  _file != stderr )  fclose( _file );
    _filename = "";

    if( _directory == "*stderr" )
    {
        _filename = "*stderr";
        _file = stderr;
    }
    else
    {
        Sos_optional_date_time time = now();
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

void Log::log( Kind kind, const string& prefix, const string& line )
{
    Thread_semaphore::Guard guard = &_semaphore;
    char buffer[100];

    Time nw = now();
    Sos_optional_date_time time = nw;
    sprintf( buffer, "%s.%03d", time.as_string().c_str(), int( Big_int(nw * 1000) % 1000 ) );

    switch( kind )
    {
        case k_msg  : strcat( buffer, " msg   " );  break;
        case k_warn : strcat( buffer, " WARN  " );  break;
        case k_error: strcat( buffer, " ERROR " );  break;
        default: ;
    }

    write( buffer );
    if( !prefix.empty() )  write( "(" + prefix + ") " );
    write( line );
    write( "\n" );
    fflush( _file );
}

//------------------------------------------------------------------------------------Task_log::log

Task_log::Task_log( Log* log, Task* task )
:
    _log(log),
    _task(task)
{
    if( task )  _prefix = "Job " + task->_job->_name;
}

//------------------------------------------------------------------------------------Task_log::log

void Task_log::log( Log::Kind kind, const string& line )
{
    _log->log( kind, _prefix, line );
}

//---------------------------------------------------------------------------------Com_log::Com_log
#ifdef SYSTEM_WIN

Com_log::Com_log( Log* log )
:
    Sos_ole_object( spooler_log_class_ptr, this, NULL ),
    _zero_(this+1),
    _log(log)
{ 
}

#endif
//---------------------------------------------------------------------------------Com_log::Com_log
#ifdef SYSTEM_WIN

Com_log::Com_log( Task* task )
:
    Sos_ole_object( spooler_log_class_ptr, this, NULL ),
    _zero_(this+1),
    _task(task)
{ 
}

#endif
//---------------------------------------------------------------------------------Com_log::Com_log
#ifdef SYSTEM_WIN
/*
Com_log::~Com_log()
{ 
}
*/
#endif
//-------------------------------------------------------------------------------------Com_log::log
#ifdef SYSTEM_WIN

STDMETHODIMP Com_log::log( Log::Kind kind, BSTR line )
{ 
    HRESULT hr = NOERROR;

    try 
    {
        if( _task )  _task->_log.log( kind, bstr_as_string( line ) ); 
        else
        if( _log )  _log->log( kind, empty_string, bstr_as_string( line ) ); 
        else
            hr = E_POINTER;
    }
    catch( const Xc&   x )  { hr = _set_excepinfo(x); }
    catch( const xmsg& x )  { hr = _set_excepinfo(x); }

    return hr;
}

#endif
//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
