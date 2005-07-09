// $Id$


#include "../zschimmer/zschimmer.h"
#include "../zschimmer/log.h"
#include "../zschimmer/argv.h"
#include "../zschimmer/async_socket.h"


namespace zschimmer
{

//typedef Byte_string base_string<Byte>;


struct Writer : Object
{
    virtual void write( char c )
    {
        write( string( &c, 1 ) );
    }

    virtual void write( const string& utf8 ) = 0;
    virtual void flush() = 0;
};


struct Output_stream : Object
{
    virtual void write_bytes( const string& bytes ) = 0;
    virtual void flush() = 0;
};


struct Input_stream : Object
{
    virtual string read_bytes() = 0;
};


struct Output_stream_writer : Writer
{
    Output_stream_writer( Output_stream* output_stream ) : _output_stream(output_stream ) {}
    
    virtual void write( const string& utf8 ) { _output_stream->write_bytes( utf8 ); }
    virtual void flush() { _output_stream->flush(); }

    ptr<Output_stream> _output_stream;
};



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
        catch( exception& x ) { Z_LOG( "~Buffered_writer: " << x << '\n' ); }
    }


    void write( const string& text )
    {
        if( _buffer.capacity() < _buffer_size )  _buffer.reserve( _buffer_size );

        size_t length = std::min( text.length(), _buffer_size - _buffer.length() );
        _buffer.append( text.data(), length );

        if( _buffer.length() == _buffer_size )  flush();

        int position = length;
        while( position < (int)text.length() )
        {
            int length = std::min( text.length() - position, _buffer_size - _buffer.length() );
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


using namespace zschimmer;
using namespace std;


int main( int argc, char** argv )
{
    int ret = 0;

    try
    {
        string scheduler_address;
        string job_chain;
        string order_id;

        typedef list< pair<string,string> >  Variables_list;
        Variables_list variables;

        for( int i = 1; i < argc; i++ )
        {
            const char* arg = argv[i];

            if( get_argv_option( arg, "-scheduler=", &scheduler_address ) );
            else
            if( get_argv_option( arg, "-job-chain=", &job_chain ) );
            else
            if( get_argv_option( arg, "-order-id=", &order_id ) );
            else
            if( arg[0] != '-' )
            {
                const char* equal = strchr( arg, '=' );
                if( !equal )  throw_xc( "= fehlt" );

                variables.push_back( pair<string,string>( string( arg, equal - arg ), equal + 1 ) );
            }
            else
                throw_xc( "Ungültiger Parameter" );
        }



        Socket_manager socket_manager;      // Für WSAStartup
        Socket_stream socket_stream;

        socket_stream.add_to_socket_manager( &socket_manager );
        socket_stream.connect_tcp( scheduler_address );
        
        
        Xml_writer xml_writer ( Z_NEW( Buffered_writer( Z_NEW( Output_stream_writer( &socket_stream ) ), 10000 ) ) );

        xml_writer.write_through( "<?xml version='1.0'?>\n" );
        xml_writer.begin_element( "add_order" );
        xml_writer.set_attribute( "job_chain", job_chain );
        xml_writer.set_attribute( "id"       , order_id );

        if( variables.size() > 0 )
        {
            xml_writer.begin_element( "params" );

            Z_FOR_EACH( Variables_list, variables, v )
            {
                xml_writer.begin_element( "param" );
                    xml_writer.set_attribute( "name" , v->first  ); 
                    xml_writer.set_attribute( "value", v->second ); 
                xml_writer.end_element( "param" );
            }

            xml_writer.end_element( "params" );
        }

        xml_writer.end_element( "add_order" );
        xml_writer.flush();

        string answer;

        while(1)
        {
            string text = socket_stream.read_bytes();
            answer += text;
            if( *text.rbegin() == '\0' )  break;
        }

        size_t pos = answer.find( "<ERROR " );
        if( pos != string::npos )
        {
            pos = answer.find( " text=", pos );
            if( pos == string::npos )
            {
                cerr << answer << "\n";
            }
            else
            {
                const char* p = answer.c_str() + pos;
                while( *p  &&  *p != '"'  &&  *p != '\'' )  p++;
                if( char quote = *p++ )
                {
                    const char* end = strchr( p, quote );
                    if( !end )  end = answer.c_str() + answer.length();
                    cerr << string( p, end - p ) << "\n";               // Die Entitäten sollten ersetzt werden!
                }
            }

            ret = 1;
        }

        socket_stream.close();
        return 0;
    }
    catch( exception& x )
    {
        cerr << "FEHLER  " << x << "\n";
        return 1;
    }
}
