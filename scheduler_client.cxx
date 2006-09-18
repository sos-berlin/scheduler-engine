// $Id$

#include "spooler.h"        // Das ist für vorkompilierte Header, sonst sind wir unabhängig von spooler.h
#include "../zschimmer/zschimmer.h"
#include "../zschimmer/log.h"
#include "../zschimmer/argv.h"
#include "../zschimmer/async_socket.h"
#include "../zschimmer/xml_libxml2.h"
#include "../zschimmer/file.h"

using namespace std;
using namespace zschimmer::xml_libxml2;


namespace zschimmer
{

const int block_size = 4000;

//typedef basic_string<Byte>  string;

//-------------------------------------------------------------------------------------------------

struct Writer : Object
{
    virtual void write( char c )
    {
        write( string( &c, 1 ) );
    }

    virtual void write( const string& utf8 ) = 0;
    virtual void flush() = 0;
};

//-------------------------------------------------------------------------------------------------

struct Output_stream : Object
{
    virtual void write_bytes( const string& bytes ) = 0;
    virtual void flush() = 0;
};

//-------------------------------------------------------------------------------------------------

struct Input_stream : Object
{
    virtual string read_bytes() = 0;
};

//-------------------------------------------------------------------------------------------------

struct Output_stream_writer : Writer
{
    Output_stream_writer( Output_stream* output_stream ) : _output_stream(output_stream ) {}
    
    virtual void write( const string& utf8 ) { _output_stream->write_bytes( utf8 ); }
    virtual void flush() { _output_stream->flush(); }

    ptr<Output_stream> _output_stream;
};

//-------------------------------------------------------------------------------------------------

struct Xml_writer : Writer
{
    Xml_writer( Writer* writer )
    : 
        _writer(writer),
        _is_tag_open(false)
    {

    }

    void close_tag()
    {
        if( _is_tag_open )
        {
            _writer->write( '>' );
            _is_tag_open = false;
        }
    }


    void begin_element( const string& element_name )
    {
        if( _is_tag_open )  close_tag();

        _writer->write( '<' );
        _writer->write( element_name );
        _is_tag_open = true;
    }


    void set_attribute( const string& name, const string& value )
    {
        _writer->write( ' ' );
        _writer->write( name );
        _writer->write( '=' );
        _writer->write( '"' );
        
        for( const char* p = value.c_str(); *p; p++ )
        {
            if( *p == '<' )  _writer->write( "&lt;" );
            else
            if( *p == '>' )  _writer->write( "&gt;" );
            else
            if( *p == '&' )  _writer->write( "&amp;" );
            else
            if( *p == '"' )  _writer->write( "&quot;" );
            else
                _writer->write( *p );
        }

        _writer->write( '"' );
    }


    void set_attribute_optional( const string& name, const string& value )
    {
        if( value != "" )  set_attribute( name, value );
    }

    
    void end_element( const string& element_name )
    {
        if( _is_tag_open )  
        {
            _writer->write( '/' );
            _writer->write( '>' );
            _is_tag_open = false;
        }
        else
        {
            _writer->write( '<' );
            _writer->write( '/' );
            _writer->write( element_name );
            _writer->write( '>' );
        }
    }


    void write( const string& utf8_text )
    {
        if( utf8_text.length() > 0 )
        {
            if( _is_tag_open )  close_tag();

            for( const char* p = utf8_text.c_str(); *p; p++ )
            {
                if( *p == '<' )  _writer->write( "&lt;" );
                else
                if( *p == '&' )  _writer->write( "&amp;" );
                else
                    _writer->write( *p );
            }
        }
    }

    void write_through( const string& text )
    {
        _writer->write( text );
    }


    void flush()
    {
        _writer->flush();
    }


    ptr<Writer>             _writer;
    bool                    _is_tag_open;
};

//-------------------------------------------------------------------------------------------------

struct Buffered_writer : Writer
{
    Buffered_writer( Writer* writer, int byte_count ) 
    : 
        _writer(writer), 
        _buffer_size(byte_count)
    {
    }


    ~Buffered_writer()
    {
        try
        {
            flush();
        }
        catch( exception& x ) { Z_LOG2( "scheduler", "~Buffered_writer: " << x << '\n' ); }
    }


    void write( const string& text )
    {
        if( _buffer.capacity() < _buffer_size )  _buffer.reserve( _buffer_size );

        size_t length = min( text.length(), _buffer_size - _buffer.length() );
        _buffer.append( text.data(), length );

        if( _buffer.length() == _buffer_size )  flush();

        size_t position = length;
        while( position < text.length() )
        {
            size_t length = min( text.length() - position, _buffer_size - _buffer.length() );
            _buffer.append( text.data() + position, length );
            position += length;
            if( _buffer.length() == _buffer_size )  flush();
        }
    }


    void flush()
    {
        if( _buffer.length() > 0 )  
        {
            _writer->write( _buffer );
            _buffer.erase();
        }
    }


    ptr<Writer>         _writer;
    string              _buffer;
    size_t              _buffer_size;
};

//-------------------------------------------------------------------------------------------------

struct Socket_stream : Buffered_socket_operation, Input_stream, Output_stream
{
    void connect_tcp( const string& address )
    {
        set_blocking( true );
        connect__start( address );
        async_finish();
    }


    void write_bytes( const string& bytes )
    {
        send__start( bytes );
        async_finish();
    }

    string read_bytes()
    {
        char buffer [ 10000 ];
        int length = call_recv( buffer, NO_OF( buffer ) );
        return string( buffer, length );
    }

    void flush() {}
};

} //namespace zschimmer

//-------------------------------------------------------------------------------------------------

using namespace std;
using namespace zschimmer;

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

struct Client
{
    Client()
    :
        _stdin( 0 )
    {
        _script_language = "shell";
    }


    void write_run_time()
    {
        _xml_writer->begin_element( "run_time" );

            if( _at == "" ) 
            {
                _xml_writer->set_attribute( "once", "yes" );
            }
            else
            {
                _xml_writer->begin_element( "at" );
                    _xml_writer->set_attribute( "at", _at == ""? "now" : _at );
                _xml_writer->end_element( "at" );
            }

        _xml_writer->end_element( "run_time" );
    }


    void start_job_script()
    {
        _xml_writer->begin_element( "job" );
            _xml_writer->set_attribute( "temporary", "true" );
            _xml_writer->set_attribute( "name", "temporary_" + hex_from_int( (int)( ::time(NULL) ) ^ rand() ) );
            _xml_writer->set_attribute_optional( "process_class", _process_class );

            if( _variables.size() > 0 )
            {
                _xml_writer->begin_element( "params" );

                Z_FOR_EACH( Variables_list, _variables, v )
                {
                    _xml_writer->begin_element( "param" );
                        _xml_writer->set_attribute( "name" , v->first  ); 
                        _xml_writer->set_attribute( "value", v->second ); 
                    _xml_writer->end_element( "param" );
                }

                _xml_writer->end_element( "params" );
            }

            _xml_writer->begin_element( "script" );
                _xml_writer->set_attribute_optional( "language", _script_language );
                while( !_stdin.eof() )  _xml_writer->write( _stdin.read_string( block_size ) );
            _xml_writer->end_element( "script" );


            write_run_time();

        _xml_writer->end_element( "job" );
    }


    void add_order()
    {
        _xml_writer->begin_element( "add_order" );
        _xml_writer->set_attribute( "job_chain", _job_chain );
        _xml_writer->set_attribute_optional( "id"   , _order_id );
        _xml_writer->set_attribute_optional( "title", _title );

        if( _variables.size() > 0 )
        {
            _xml_writer->begin_element( "params" );

            Z_FOR_EACH( Variables_list, _variables, v )
            {
                _xml_writer->begin_element( "param" );
                    _xml_writer->set_attribute( "name" , v->first  ); 
                    _xml_writer->set_attribute( "value", v->second ); 
                _xml_writer->end_element( "param" );
            }

            _xml_writer->end_element( "params" );
        }

        _xml_writer->end_element( "add_order" );
    }


    void read_response()
    {
        string response;

        while(1)
        {
            string text = _socket_stream.read_bytes();
            response += text;
            if( *text.rbegin() == '\0' )  break;
        }

        xml::Document_ptr response_document ( response );

        if( xml::Element_ptr error_element = response_document.select_node( "/spooler/answer/ERROR" ) )
        {
            zschimmer::Xc x;
            x.set_code( error_element.getAttribute( "code" ) );
            x.set_name( error_element.getAttribute( "class" ) );
            x.set_what( error_element.getAttribute( "text" ) );
            throw_xc( x );
        }
    }

    /*
    void read_response()
    {
        string response;

        while(1)
        {
            string text = _socket_stream.read_bytes();
            response += text;
            if( *text.rbegin() == '\0' )  break;
        }

        size_t pos = response.find( "<ERROR " );
        if( pos != string::npos )
        {
            pos = response.find( " text=", pos );
            if( pos == string::npos )
            {
                cerr << response << "\n";
            }
            else
            {
                const char* p = response.c_str() + pos;
                while( *p  &&  *p != '"'  &&  *p != '\'' )  p++;
                if( char quote = *p++ )
                {
                    const char* end = strchr( p, quote );
                    if( !end )  end = response.c_str() + response.length();
                    cerr << string( p, end - p ) << "\n";               // Die Entitäten sollten ersetzt werden!
                }
            }

            //ret = 1;
        }
    }
    */


    void get_argv( int argc, char** argv )
    {
        for( int i = 1; i < argc; i++ )
        {
            const char* arg         = argv[i];
            string      log_dummy;

            // Die Optionen hier müssen auch von spooler_main() in spooler.cxx geprüft werden:

            if( get_argv_option( arg, "-scheduler=", &_scheduler_address ) );
            else
            if( get_argv_option( arg, "-language=", &_script_language ) );
            else
            if( get_argv_option( arg, "-process-class=", &_process_class ) );
            else
            if( get_argv_option( arg, "-job-chain=", &_job_chain ) );
            else
            if( get_argv_option( arg, "-order-id=", &_order_id ) );
            else
            if( get_argv_option( arg, "-title=", &_title ) );
            else
            //if( get_argv_option( arg, "-job-name=", &_job_name ) );
            //else
            if( get_argv_option( arg, "-at=", &_at ) );
            else
            if( get_argv_option( arg, "-log=", &log_dummy ) );      // Bereits von spooler_main() ausgewertet
            else
            if( arg[0] != '-' )
            {
                const char* equal = strchr( arg, '=' );
                if( !equal )  throw_xc( "= fehlt" );

                _variables.push_back( pair<string,string>( string( arg, equal - arg ), equal + 1 ) );
            }
            else
                throw_xc( "Ungültiger Parameter" );
        }
    }


    void run()
    {
        _socket_stream.add_to_socket_manager( &_socket_manager );
        _socket_stream.connect_tcp( _scheduler_address );
        
        
        _xml_writer = Z_NEW( Xml_writer( Z_NEW( Buffered_writer( Z_NEW( Output_stream_writer( &_socket_stream ) ), 10000 ) ) ) );

        _xml_writer->write_through( "<?xml version='1.0'?>\n" );

        
        if( _job_chain != "" ) add_order();     
                          else start_job_script();

        _xml_writer->flush();

        read_response();

        _socket_stream.close();
    }


  private:
    string                     _scheduler_address;
    string                     _script_language;
    string                     _process_class;
    string                     _job_chain;
    string                     _order_id;
    string                     _title;
    string                     _at;

    typedef list< pair<string,string> >  Variables_list;
    Variables_list _variables;

    File                       _stdin;
    Socket_manager             _socket_manager;      // Für WSAStartup
    Socket_stream              _socket_stream;
    ptr<Xml_writer>            _xml_writer;
};

//----------------------------------------------------------------------------scheduler_client_main

int scheduler_client_main( int argc, char** argv )
{
    int ret = 0;

    try
    {
        Client client;
        client.get_argv( argc, argv );
        client.run();
    }
    catch( exception& x )
    {
        cerr << "ERROR " << x.what() << "\n";
        ret = 1;
    }

    return ret;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

//---------------------------------------------------------------------------------------------main
/*
int main( int argc, char** argv )
{
    return sos::spooler::scheduler_client_main( argc, argv );
}
*/
