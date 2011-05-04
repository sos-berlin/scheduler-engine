// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "zschimmer.h"
#include "z_io.h"
#include "log.h"

namespace zschimmer {
namespace io {

using namespace ::std;

//--------------------------------------------------------------------------------------------const

const size_t Buffered_writer::default_buffer_byte_count = 50000;

const Byte latin1_charset_table[ 256 ] =
{
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0,     // 00  nur \t \n \r
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 10
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 20
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 30
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 40
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 50
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 60
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,     // 70  \x7f nicht
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 90
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // a0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // b0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // c0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // d0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // e0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1      // f0
};

//----------------------------------------------------------------------------------is_valid_latin1

bool Char_sequence::is_valid_latin1() const
{
    const char* p_end = _ptr + _length;

    for( const char* p = _ptr; p < p_end; p++ )  if( !io::is_valid_latin1( *p ) )  return false;
    return true;
}

//-------------------------------------------------------------------Filter_output_stream::obj_name

string Filter_output_stream::obj_name() const
{
    return "Filter_output_stream";// + _output_stream->obj_name();    
}

//------------------------------------------------------------------------------------Writer::write
    
void Writer::write( char c )
{
    write( Char_sequence( &c, 1 ) );
}

//-------------------------------Transparent_output_stream_writer::Transparent_output_stream_writer

Transparent_output_stream_writer::Transparent_output_stream_writer( Output_stream* output_stream )
: 
    _output_stream(output_stream ) 
{
    assert( output_stream );
}

//----------------------------------------------------------Transparent_output_stream_writer::write
    
void Transparent_output_stream_writer::write( const Char_sequence& s )
{ 
    _output_stream->write_bytes( Byte_sequence( (const Byte*)s.ptr(), s.length() ) ); 
}

//---------------------------------Transparent_input_stream_reader::Transparent_input_stream_reader

Transparent_input_stream_reader::Transparent_input_stream_reader( Input_stream* input_stream )
: 
    _input_stream(input_stream ) 
{
    assert( input_stream );
}

//-----------------------------------------------------Transparent_input_stream_reader::read_string
    
string Transparent_input_stream_reader::read_string( size_t maximum )
{ 
    return _input_stream->read_bytes( maximum );
}

//-----------------------------------------------------------------Buffered_writer::Buffered_writer

Buffered_writer::Buffered_writer( Writer* writer, int byte_count ) 
: 
    Filter_writer(writer), 
    _buffer_size(byte_count)
{
    assert( writer );
}

//----------------------------------------------------------------Buffered_writer::~Buffered_writer
    
Buffered_writer::~Buffered_writer()
{
    try
    {
        flush();
    }
    catch( exception& x ) { Z_LOG( Z_FUNCTION << "  " << x.what() << '\n' ); }
}

//----------------------------------------------------------nput_stream_reader::Input_stream_reader

//Input_stream_reader::Input_stream_reader( Input_stream* is, Char_set* cs )
//: 
//    _input_stream(is), 
//    _char_set(cs), 
//    _decoder( cs->new_decoder() ),
//    _eof(false) 
//{
//}

//-----------------------------------------------------------------Input_stream_reader::read_string

//string Input_stream_reader::read_string( size_t maximum )
//{
//    string result = _buffer;
//
//    //_buffer.reserve( maximum );
//    result.append( _decoder->decoded_string( _input_stream->read_bytes( (int)( _char_set->average_bytes_per_char() * maximum + 0.99 ) ) ) );
//
//    if( result.length() > maximum )
//    {
//        _buffer.assign( result.data() + maximum, maximum - result.length() );
//        result.erase( maximum );
//    }
//
//    return result;
//}

//-------------------------------------------------------------------------Line_reader::Line_reader

Line_reader::Line_reader( Reader* reader )                      
: 
    _zero_(this+1), 
    _reader(reader),
    _was_eof(false)
{
    _buffer.reserve( 10000 );
}

//---------------------------------------------------------------------------Line_reader::read_line

string Line_reader::read_line()
{
    S result;

    while(1)
    {
        //Z_LOG2( "zschimmer", Z_FUNCTION << " position=" << _position << " length=" << _buffer.length() << "\n" );

        if( _position == _buffer.length() )     // Puffer ist leer?
        {
            _buffer = _reader->read_string( _buffer.capacity() );
            _position = 0;
            
            if( _buffer.length() == 0 )         // Dateiende?
            {
                if( result.empty() )  _was_eof = true;
                break;
            }
        }

        assert( _position < _buffer.length() );

        size_t n = _buffer.find( '\n', _position );
        if( n != string::npos )
        {
            size_t n0 = n;
            if( n0 > _position  &&  _buffer[ n0 ] == '\r' )  n0--;
            result.append( _buffer.data() + _position, n0 - _position );
            _position = n + 1;
            break;
        }
        else
        {
            result.append( _buffer.data() + _position, _buffer.length() - _position );
            _position = _buffer.length();
        }
    }

    return result;
}

//---------------------------------------------------------------------------Buffered_writer::write

void Buffered_writer::write( const Char_sequence& char_sequence )
{
    if( _buffer.capacity() < _buffer_size )  _buffer.reserve( _buffer_size );

    size_t length = min( char_sequence.length(), _buffer_size - _buffer.length() );
    _buffer.append( char_sequence.ptr(), char_sequence.length() );

    if( _buffer.length() == _buffer_size )  flush();

    size_t position = length;
    while( position < char_sequence.length() )
    {
        size_t length = min( char_sequence.length() - position, _buffer_size - _buffer.length() );
        _buffer.append( char_sequence.ptr() + position, length );
        position += length;
        if( _buffer.length() == _buffer_size )  flush();
    }
}

//---------------------------------------------------------------------------Buffered_writer::flush

void Buffered_writer::flush()
{
    if( _buffer.length() > 0 )  
    {
        _writer->write( _buffer );
        _buffer.erase();
    }
}

//---------------------------------------------------------------------String_writer::String_writer

String_writer::String_writer()
{
}

//--------------------------------------------------------------------String_writer::~String_writer

String_writer::~String_writer()
{
}

//-------------------------------------------------------------------------------------------------

} //namespace io
} //namespace zschimmer
