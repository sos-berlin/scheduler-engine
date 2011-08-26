// $Id: z_io.h 13670 2008-09-26 15:09:47Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_Z_IO_H
#define __ZSCHIMMER_Z_IO_H

#include "md5.h"

namespace zschimmer {
namespace io {

//-------------------------------------------------------------------------------------------------

//template< typename SIMPLE_TYPE >
//struct sequence< SIMPLE_TYPE >
//{
//                                sequence                    ( const SIMPLE_TYPE* p, size_t length ) : _ptr(p), _length( length ) {}
//
//    int                         length                      () const                                { return _length; }
//    SIMPLE_TYPE                 operator[]                  ( size_t i ) const                      { assert( i < _length );  return _ptr[ i ]; }
//    sequence                    sub_sequence                ( size_t start, size_t end ) const      { assert( start < _length );  assert( end < _length );  assert( start <= end );
//                                                                                                      return sequence( _ptr + start, end - start ); }
//    string                      to_string                   () const                                { return string( _ptr, _length ); }
//
//  private:
//    const char* const          _ptr;
//    size_t const               _length;
//};

//-------------------------------------------------------------------------------------------------

extern const Byte               latin1_charset_table        [ 256 ];

inline bool                     is_valid_latin1             ( char c )                              { return latin1_charset_table[ (Byte)c ] != 0; }

//------------------------------------------------------------------------------------Char_sequence

struct Char_sequence
{
                                Char_sequence               ( const char* s )                       : _ptr(s), _length( strlen( s ) ) {}
                                Char_sequence               ( const char* p, size_t length )        : _ptr(p), _length( length ) {}
                                Char_sequence               ( const string* s )                     : _ptr( s->data() ), _length( s->length() ) {}
                                Char_sequence               ( const string& s )                     : _ptr( s.data() ), _length( s.length() ) {}

    const char*                 ptr                         () const                                { return _ptr; }
    size_t                      length                      () const                                { return _length; }
    bool                        is_empty                    () const                                { return _length == 0; }
    char                        operator[]                  ( size_t i ) const                      { Z_DEBUG_ONLY( assert( i < _length ) );  return _ptr[ i ]; }
    Char_sequence               sub_sequence                ( size_t start, size_t end ) const      { Z_DEBUG_ONLY( assert( start < _length );  assert( end < _length );  assert( start <= end ); )
                                                                                                      return Char_sequence( _ptr + start, end - start ); }
    bool                        is_valid_latin1             () const;
    string                      to_string                   () const                                { return string( _ptr, _length ); }

    friend ostream&             operator <<                 ( ostream& s, const Char_sequence& seq ){ s.write( seq.ptr(), seq.length() ); return s; }

  private:
    const char* const          _ptr;
    size_t const               _length;
};

//struct Char_sequence : IUnknown
//{
//    virtual                    ~Char_sequence               ()                                      = 0;
//
//    virtual int                 length                      () const                                = 0;
//    virtual char                operator[]                  ( int ) const                           = 0; 
//    virtual Char_sequence       sub_sequence                ( int start, int end ) const            = 0;
//    virtual string              to_string                   () const                                = 0;
//};

//-------------------------------------------------------------------------------------------------

struct Byte_sequence
{
                                Byte_sequence               ( const Byte* p, size_t length )        : _ptr(p), _length( length ) {}
                                Byte_sequence               ( const string* s )                     : _ptr( (const Byte*)s->data() ), _length( s->length() ) {}
                                Byte_sequence               ( const string& s )                     : _ptr( (const Byte*)s.data() ), _length( s.length() ) {}

    const Byte*                 ptr                         () const                                { return _ptr; }
    const char*                 char_ptr                    () const                                { return (const char*)_ptr; }
    size_t                      length                      () const                                { return _length; }
    Byte                        operator[]                  ( size_t i ) const                      { assert( i < _length );  return _ptr[ i ]; }
    Byte_sequence               sub_sequence                ( size_t start, size_t end ) const      { assert( start < _length );  assert( end < _length );  assert( start <= end );
                                                                                                      return Byte_sequence( _ptr + start, end - start ); }
    string                      to_string                   () const                                { return string( (const char*)_ptr, _length ); }

  private:
    const Byte* const          _ptr;
    size_t const               _length;
};

//-----------------------------------------------------------------------------String_char_sequence

//struct String_char_sequence : Char_sequence
//{
//                                String_char_sequence        ( const string* s )                     : Char_se_string(s) {}
//
//    int                         length                      () const                                { return s->length(); }
//    char                        operator[]                  ( int ) const                           { return s->length(); }
//    Char_sequence               sub_sequence                ( int start, int end ) const            { return _strings->length(); }
//    string                      to_string                   () const                                { return *_string; }
//
//  private:
//    string                     _string;
//};

//----------------------------------------------------------------------------------------Closeable
    
struct Closeable //: IUnknown
{
    virtual                    ~Closeable                   ()                                      {}
    virtual void                close                       ()                                      = 0;
};

//----------------------------------------------------------------------------------------Flushable
    
struct Flushable //: IUnknown
{
    virtual                    ~Flushable                   ()                                      {}
    virtual void                flush                       ()                                      = 0;
};

//-------------------------------------------------------------------------------------------Buffer

//struct Buffer : Object
//{
//    size_t                      capacity                    () const                                { return _capacity; }
//    void                        clear                       ();
//  //void                        flip                        ();
//  //bool                        is_read_only                ();
//    size_t                      length                      () const                                { return _length; }
//    void                    set_length                      ( size_t );
//    int                         read_position               () const                                { return _read_position; }
//    void                    set_read_position               ( size_t );
//    size_t                      remaining                   () const                                { return _length - _read_position; }
//    void                        rewind                      ()                                      { _read_position = 0; }
//
//  private:
//    Fill_zero                  _zero_;
//    size_t                     _capacity;
//    size_t                     _length;
//    size_t                     _read_position;
//    size_t                     _write_position;
//};

//--------------------------------------------------------------------------------------Byte_buffer

//struct Byte_buffer : Buffer
//{
//                                Byte_buffer                 ( size_t capacity );
//                               ~Byte_buffer                 ();
//
//    virtual Byte                get_byte                    () const                                = 0;
//    virtual void                get_bytes                   ( vector<Byte>*, size_t offset, size_t length ) = 0;
//
//    virtual void                put_byte                    ( Byte )                                = 0;
//    virtual void                put_bytes                   ( const vector<Byte>&, size_t offset, size_t length ) = 0;
//
//  private:
//    Byte*                      _bytes;
//};

//-------------------------------------------------------------------------------------------------

struct Char_set_encoder;
struct Char_set_decoder;

//-----------------------------------------------------------------------------------------Char_set

//struct Char_set : IUnknown
//{
//    static const Char_set&      default_char_set            ();
//
//
//    string                      name                        ();
//    int                         average_bytes_per_char      () const                                { return _average_bytes_per_char; }
//    string                      encoded_bytes               ( const string& ) const;
//    string                      decoded_string              ( const string& ) const;
//  //Char_buffer                 decode                      ( const Byte_buffer& ) const;
//  //Char_sequence               decode                      ( const Byte_sequence& ) const;
//    ptr<Char_set_encoder>       new_encoder                 ();
//    ptr<Char_set_decoder>       new_decoder                 ();
//
//  protected:
//                                Char_set                    ( const string& name, const int average_bytes_per_char );
//
//  private:
//    Fill_zero                  _zero_;
//    int                        _average_bytes_per_char;
//    string                     _name;
//};
//
////---------------------------------------------------------------------------------Char_set_encoder
//
//struct Char_set_encoder : Object
//{
//    const Char_set*             char_set                    () const                                { return _char_set; }
//
//    string                      encoded_bytes               ( const string& );
//    virtual void                encode_loop                 ( const Char_sequence&, string* )       = 0;
//    virtual void                flush                       ( string* )                             = 0;
//
//  private:
//    Fill_zero                  _zero_;
//    ptr<Char_set>              _char_set;
//};
//
////---------------------------------------------------------------------------------Char_set_decoder
//
//struct Char_set_decoder : Object
//{
//    const Char_set*             char_set                    () const                                { return _char_set; }
//
//    string                      decoded_string              ( const string& );
//    virtual void                decode_loop                 ( const Byte_sequence&, string* )       = 0;
//    virtual void                flush                       ( string* )                             = 0;
//
//  private:
//    Fill_zero                  _zero_;
//    ptr<Char_set>              _char_set;
//};
//
////---------------------------------------------------------------------Transparent_char_set_encoder 
//
//struct Transparent_char_set_encoder : Char_set_encoder
//{
//    string                      encode                      ( const string& decoded )               { return decoded; }
//    virtual void                encode_loop                 ( const Char_sequence& in, string* out ) { out->append( in.ptr(), in.length() ); }
//    virtual void                flush                       ( string* )                             {}
//};
//
////---------------------------------------------------------------------Transparent_char_set_decoder 
//
//struct Transparent_char_set_decoder : Char_set_decoder
//{
//    string                      decode_byte_string          ( const string& encoded )               { return encoded; }
//    virtual void                decode_loop                 ( const Byte_sequence& in, string* out ) { out->append( in.char_ptr(), in.length() ); }
//    virtual void                flush                       ( string* )                             {}
//};
//
////-----------------------------------------------------------------------------Transparent_char_set
//
//struct Transparent_char_set : simple_iunknown_implementation<Char_set>
//{
//    ptr<Char_set_encoder>       new_encoder                 ()                                      { return &static_encoder; }
//    ptr<Char_set_decoder>       new_decoder                 ()                                      { return &static_decoder; }
//
//  private:
//    static Transparent_char_set_encoder static_encoder;
//    static Transparent_char_set_decoder static_decoder;
//};
//
//
//extern Transparent_char_set     transparent_char_set;

//------------------------------------------------------------------------------------Output_stream

struct Output_stream : IUnknown, Closeable, Flushable
{
    virtual void                write_bytes                 ( const Byte_sequence& )                = 0;

    void                        write_bytes                 ( const string& s )                     { write_bytes( Byte_sequence( (const Byte*)s.data(), s.length() ) ); }
};

//-------------------------------------------------------------------------------------Input_stream

struct Input_stream : IUnknown, Closeable
{
    virtual                    ~Input_stream                ()                                      {}
    virtual string              read_bytes                  ( size_t maximum )                      = 0;    // sollten wir string nutzen?
};

//-----------------------------------------------------------------------------Filter_output_stream

struct Filter_output_stream : simple_iunknown_implementation< Output_stream >
{
                                Filter_output_stream        ( Output_stream* os )                   : _output_stream(os) { assert( os ); }
    virtual                    ~Filter_output_stream        ()                                      {}

    void                        close                       ()                                      { _output_stream->close();  _output_stream = NULL; }
    void                        flush                       ()                                      { _output_stream->flush(); }
    void                        write_bytes                 ( const Byte_sequence& b )              { _output_stream->write_bytes( b ); }
    string                      obj_name                    () const;

  protected:
    Output_stream*             _output_stream;
};

//-------------------------------------------------------------------------------------------Writer

struct Writer : IUnknown, Closeable, Flushable
{
    virtual void                write                       ( char c );
    virtual void                write                       ( const Char_sequence& )                = 0;  

  //void                        write                       ( const string& s )                     { write( Char_sequence( s.data(), s.length() ) ); }
};

//-------------------------------------------------------------------------------------------Reader

struct Reader : IUnknown, Closeable
{
  //virtual char                read_char                   ();
    virtual string              read_string                 ( size_t maximum )                      = 0;
  //virtual bool                eof                         ()                                      = 0;
};

//-------------------------------------------------------------------------------------------Writer

struct Filter_writer : simple_iunknown_implementation< Writer >
{
                                Filter_writer               ( Writer* w )                           : _writer(w) { assert( w ); }

    void                        write                       ( const Char_sequence& s )              { if( !_writer )  throw_xc( Z_FUNCTION );  _writer->write( s ); }
    void                        flush                       ()                                      {}
    void                        close                       ()                                      { if( _writer ) flush(), _writer = NULL; }
    //void                        flush                       ()                                      { _writer->flush(); }
    //void                        close                       ()                                      { ptr<Writer> w = _writer; _writer = NULL; w->close(); }

  protected:
                                Filter_writer               ()                                      : _writer(NULL) {}

    Writer*                    _writer;
};

//-----------------------------------------------------------------Transparent_output_stream_writer

struct Transparent_output_stream_writer : simple_iunknown_implementation< Writer >
{
                                Transparent_output_stream_writer( Output_stream* );

    void                        write                       ( const Char_sequence& );
    void                        flush                       ()                                      { _output_stream->flush(); }
    void                        close                       ()                                      { _output_stream->close(); }

  private:
    ptr<Output_stream>         _output_stream;
};

//------------------------------------------------------------------Transparent_input_stream_reader

struct Transparent_input_stream_reader : simple_iunknown_implementation< Reader >
{
                                Transparent_input_stream_reader( Input_stream* );

    string                      read_string                 ( size_t maximum );
    void                        close                       ()                                      { _input_stream->close(); }

  private:
    ptr<Input_stream>          _input_stream;
};

//------------------------------------------------------------------------------Input_stream_reader

//struct Input_stream_reader : simple_iunknown_implementation< Reader >
//{
//                                Input_stream_reader         ( Input_stream*, Char_set* );
//
//    char                        read_char                   ();
//    string                      read_string                 ( size_t maximum );
//    bool                        eof                         ()                                      { return _eof; }
//    void                        close                       ()                                      { _input_stream->close(); }
//
//  private:
//    ptr<Input_stream>          _input_stream;
//    ptr<Char_set>              _char_set;
//    ptr<Char_set_decoder>      _decoder;
//    string                     _buffer;
//    bool                       _eof;
//};

//----------------------------------------------------------------------------------Buffered_writer

struct Buffered_writer : Filter_writer
{
    static const size_t         default_buffer_byte_count;


                                Buffered_writer             ( Writer*, int default_buffer_byte_count );
                               ~Buffered_writer             ();

    void                        write                       ( const Char_sequence& );
    void                        flush                       ();

  private:
    string                     _buffer;
    size_t                     _buffer_size;
};

//------------------------------------------------------------------------------------String_writer

struct String_writer : simple_iunknown_implementation< Writer >
{
                                String_writer               ();
                               ~String_writer               ();

    void                        write                       ( const Char_sequence& cs )             { _string_stream.append( cs.ptr(), cs.length() ); }
    void                        flush                       ()                                      {}
    void                        close                       ()                                      {}
    string                      to_string                   ()                                      { return _string_stream.to_string(); }

  private:
    String_stream              _string_stream;
};

//-------------------------------------------------------------------------------------------------

struct Line_reader : Object
{
                                Line_reader                 ( Reader* );

    bool                        was_eof                     ()                                      { return _was_eof; }
    string                      read_line                   ();

  private:
    Fill_zero                  _zero_;
    ptr<Reader>                _reader;
    string                     _buffer;
    size_t                     _position;
    bool                       _was_eof;
};

//-------------------------------------------------------------------------------------------------

inline string                   lcase_hex                   ( const Byte_sequence& seq )            { return zschimmer::lcase_hex( seq.ptr(), seq.length() ); }            
inline string                   lcase_hex                   ( const Char_sequence& seq )            { return zschimmer::lcase_hex( seq.ptr(), seq.length() ); }            
inline bool                     is_valid_latin1             ( const Char_sequence& seq )            { return seq.is_valid_latin1(); }

//-------------------------------------------------------------------------------------------------

} //namespace io

inline void                     insert_into_message         ( Message_string* m, int index, const io::Char_sequence& seq ) { m->insert( index, string( seq.ptr(), seq.length() ) ); }

} //namespace zschimmer

#endif
