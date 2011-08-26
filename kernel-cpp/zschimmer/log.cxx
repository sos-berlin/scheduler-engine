// $Id: log.cxx 13693 2008-10-02 14:15:45Z jz $

#include "zschimmer.h"
#include "log.h"
#include "log_categories.h"
#include "file.h"
#include "z_io.h"
#include "xml_libxml2.h"

using namespace std;
using namespace zschimmer::file;


namespace zschimmer {

//------------------------------------------------------------------------------------------extern

Log_categories          static_log_categories;
Log_categories_cache    static_log_categories_cache;

Log_context*            Log_ptr::static_no_log_context  = NULL;
Log_context**           Log_ptr::static_log_context_ptr = &static_no_log_context;
Log_ptr::Log_ostream    Log_ptr::static_stream;
ostream*                Log_ptr::static_stream_ptr      = &Log_ptr::static_stream;

//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "Z-LOG-001", "Ungültiger Log-Level $1: Nur error, warn, info und debug1 bis debug9 sind möglich" },
    { NULL       , NULL }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( log )
{
    add_message_code_texts( error_codes );
}

//------------------------------------------------------------------------------log_category_is_set
// Deklariert in base.h, damit log.h nicht eingezogen werden muss.

bool                        log_category_is_set         ( const char* name )                        { return static_log_categories.is_set( name ); }
bool                        log_category_is_set         ( const string& name )                      { return static_log_categories.is_set( name ); }
void                    set_log_category                ( const string& name, bool value )          { static_log_categories.set( name, value ); }
void                    set_log_category_default        ( const string& name, bool value )          { static_log_categories.set_default( name, value ); }
void                    set_log_category_implicit       ( const string& name, bool value )          { static_log_categories.set( name, value, Log_categories::Entry::e_implicit ); }
void                    set_log_category_explicit       ( const string& name )                      { static_log_categories.set( name, false, Log_categories::Entry::e_explicit ); }
string                      log_categories_as_string    ()                                          { return static_log_categories.to_string(); }

//-----------------------------------------------------------------------------------make_log_level

Log_level make_log_level( const string& name )
{
    Log_level log_level = log_debug9;

    if( name == "error"   )  log_level = log_error;
    else
    if( name == "warn"
     || name == "warning" )  log_level = log_warn;
    else                     
    if( name == "info"    )  log_level = log_info;
    else
    if( name == "debug"   )  log_level = log_debug;
    else
    if( name == "unknown" )  log_level = log_unknown;
    else
    if( name == "none"    )  log_level = log_none;
    else
    if( strncmp(name.c_str(),"debug",5) == 0 )
    {
        try {
            log_level = (Log_level)-(int)as_uint( name.c_str() + 5 );
        }
        catch( const Xc& ) { throw_xc( "Z-LOG-001", name ); }
    }
    else
    {
        try {
            log_level = (Log_level)as_int( name );
            if( log_level != log_unknown  
             && log_level != log_none
             && ( log_level < log_debug9  ||  log_level > log_error ) )  throw_xc( "Z-LOG-001", name );  //log_level = log_error;
        }
        catch( const Xc& ) { throw_xc( "Z-LOG-001", name ); }
    }

    return log_level;
}

//--------------------------------------------------------------------------------name_of_log_level

string name_of_log_level( Log_level level )
{
    if( level >= log_debug9  &&  level <= log_debug1 )  return "debug" + as_string( -level );
    
    switch( level )
    {
        case log_unknown: return "unknown";
        case log_none:    return "none";
        case log_info:    return "info";
        case log_warn:    return "warning";
        case log_error:   return "error";
        case log_fatal:   return "fatal";
        default:          return as_string( level );
    }
}

//------------------------------------------------------------------------------Delegated_log::log2

void Delegated_log::log2( Log_level level, const string& prefix, const string& line, Has_log* prefix_log )   
{ 
    if( this != NULL  &&  level < _log_level )  return;
    
    if( this != NULL  &&  _log )  
    {
        _log->log2( level, prefix, line, prefix_log? prefix_log : this ); 
    }
    else
    {
        // Möglicherweise ist this == null.

        Z_LOG( line << '\n' );
        string p = prefix == ""? "" : "(" + prefix + ") ";
        string l = name_of_log_level( level );
        fprintf( stderr, "[%s] %s%s\n", l.c_str(), p.c_str(), line.c_str() );
    }
}

//------------------------------------------------------------Log_ptr::Log_streambuf::Log_streambuf

Log_ptr::Log_streambuf::Log_streambuf()
{
    setp( _buffer, _buffer + NO_OF( _buffer ) );
}

//---------------------------------------------------------------------Log_ptr::Log_streambuf::sync

int Log_ptr::Log_streambuf::sync()
{
    int result = overflow( EOF );

    if( Log_context* log_context = *static_log_context_ptr )
    {
        result = log_context->_log_write( NULL, 1 );  // sync
    }

    return result;
}

//----------------------------------------------------------------Log_ptr::Log_streambuf::underflow

int Log_ptr::Log_streambuf::underflow()
{
    return EOF;
}

//-----------------------------------------------------------------Log_ptr::Log_streambuf::overflow

int Log_ptr::Log_streambuf::overflow( int character )
{
    if( Log_context* log_context = *static_log_context_ptr )
    {
        char* p      = pbase();
        int   length = p? pptr() - p : 0;

        if( length > 0 )  
        {
            log_context->_log_write( p, length );
        }

        setp( _buffer, _buffer + NO_OF( _buffer ) );

        if( character != EOF ) 
        {
            _buffer[ 0 ] = (char)( character & 0xFF );
            pbump( 1 );
        }
    }

    return 0;
}

//--------------------------------------------------------------------------Log_ptr::Indent::indent

void Log_ptr::Indent::indent()
{
    indent_( +1 );

    //?2006-03-03   Log_ptr().flush();
}

//-------------------------------------------------------------------------Log_ptr::Indent::~Indent

Log_ptr::Indent::~Indent()
{
    indent_( -1 );
}

//-------------------------------------------------------------------------Log_ptr::Indent::indent_

void Log_ptr::Indent::indent_( int plus_or_minus )
{
#   if defined Z_WINDOWS
        if( Log_context* log_context = *static_log_context_ptr )
        {
            if( int tls_index = log_context->_indent_tls_index )
            {

                size_t indent = (size_t)TlsGetValue( tls_index );
                indent += plus_or_minus;
                TlsSetValue( tls_index, (void*)indent ); 

            }
        }
#   endif
}

//---------------------------------------------------------------------------------Log_ptr::Log_ptr

Log_ptr::Log_ptr()
:
    _stream( *static_log_context_ptr? static_stream_ptr : NULL )
{ 
    if( _stream )  enter_mutex( (*static_log_context_ptr)->_mutex ); 
}

//---------------------------------------------------------------------------------Log_ptr::Log_ptr

Log_ptr::Log_ptr( const string& category, const char* function )
:
    _stream( *static_log_context_ptr  &&  log_category_is_set(category)? static_stream_ptr : NULL )
{ 
    if( _stream )
    {
        enter_mutex( (*static_log_context_ptr)->_mutex ); 

        if( !category.empty() )  *_stream << '{' << category << "} ";
        log_function( function );
    }
}

//---------------------------------------------------------------------------------Log_ptr::Log_ptr

Log_ptr::Log_ptr( const char* category, const char* function )
:
    _stream( *static_log_context_ptr  &&  log_category_is_set(category)? static_stream_ptr : NULL )
{ 
    if( _stream )
    {
        enter_mutex( (*static_log_context_ptr)->_mutex ); 

        if( category  &&  category[0] )  *_stream << '{' << category << "} ";
        log_function( function );
    }
}

//---------------------------------------------------------------------------------Log_ptr::Log_ptr

Log_ptr::Log_ptr( Cached_log_category& category, const char* function )
:
    _stream( *static_log_context_ptr  &&  static_log_categories.is_set( &category )? static_stream_ptr : NULL )
{ 
    if( _stream )
    {
        enter_mutex( (*static_log_context_ptr)->_mutex ); 

        *_stream << '{' << category << "} ";
        log_function( function );
    }
}

//--------------------------------------------------------------------------------Log_ptr::~Log_ptr

Log_ptr::~Log_ptr()
{ 
    if( _stream )
    {
        _stream->flush(); 
        leave_mutex( (*static_log_context_ptr)->_mutex ); 
    }
}

//----------------------------------------------------------------------------Log_ptr::log_function

void Log_ptr::log_function( const char* function )
{
    if( _stream  &&  function  &&  *function  &&  static_log_categories.is_set( &static_log_categories_cache._function ) )
    {
        *_stream << z_function( function ) << "()  ";
    }
}

//-------------------------------------------------------------------------Log_ptr::set_log_context

void Log_ptr::set_log_context( Log_context** c )
{ 
    static_log_context_ptr = c? c : &static_no_log_context;
}

//-------------------------------------------------------------Log_ptr::set_stream_and_system_mutex
/*
void Log_ptr::set_stream_and_system_mutex( ostream** s, System_mutex* m )
{ 
    static_system_mutex_ptr = m;  
    //static_hostware_stream_ptr = s; 
    static_stream_ptr = s;
}

//-------------------------------------------------------------Log_ptr::get_stream_and_system_mutex

void Log_ptr::get_stream_and_system_mutex( ostream*** s, System_mutex** m ) 
{ 
    *m = _system_mutex_ptr;  
    *s = _stream_ptr; 
}  
*/
//--------------------------------------------------------------------------------Has_log::log_file

void Has_log::log_file( const string& filename, const string& title )
{
    if( !filename.empty() ) 
    {
        try
        {
            Mapped_file file ( filename, "rS" );        // read sequential

            if( file.map_length() > 0 )
            {
                log_with_title( title == ""? filename + ":" : title, io::Char_sequence( (const char*)file.map(), file.map_length() ) );
            }
        }
        catch( exception& x ) 
        { 
            warn( filename + ": " + x.what() ); 
        }
    }
}

//--------------------------------------------------------------------------Has_log::log_with_title

void Has_log::log_with_title( const string& title, const io::Char_sequence& seq )
{
    if( !seq.is_empty() )
    {
        info( title );
        log_with_prefix( "    ", seq );
    }
}

//-------------------------------------------------------------------------Has_log::log_with_prefix

void Has_log::log_with_prefix( const string& prefix, const string& s )
{ 
    log_with_prefix( prefix, io::Char_sequence( &s ) ); 
}

//-------------------------------------------------------------------Has_log::log_with_extra_prefix

void Has_log::log_with_prefix( const string& prefix, const io::Char_sequence& seq )
{
    if( !seq.is_empty() )
    {
        const char* p     = seq.ptr();
        const char* p_end = p + seq.length();

        while( p < p_end )
        {
            const char* q = (const char*)memchr( p, '\n', p_end - p );
            if( !q )  q = p_end;

            info( prefix + string( p, q - p ) );
            p = q + 1;
        }
    }
}

//-------------------------------------------------------Log_categories_cache::Log_categories_cache

Log_categories_cache::Log_categories_cache()
:
    _async    ( "async"    ),
    _function ( "function" )
{
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
