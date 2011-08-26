// $Id: z_gzip.h 13199 2007-12-06 14:15:42Z jz $

#ifndef __ZSCHIMMER_Z_GZIP_H
#define __ZSCHIMMER_Z_GZIP_H

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

string                          string_gzip_deflate     ( const Byte*, int length );
inline string                   string_gzip_deflate     ( const char* p, int length )               { return string_gzip_deflate( reinterpret_cast<const unsigned char*>( p ), length ); }
inline string                   string_gzip_deflate     ( const string& s )                         { return string_gzip_deflate( s.data(), s.length() ); }

//-------------------------------------------------------------------------------------------------

//void                            gzip_file               ( const File_path& source, const File_path& destination );

//--------------------------------------------------------------------------------Simple_byte_queue

struct Simple_byte_queue
{
                                Simple_byte_queue       ( int buffer_size = default_chunk_size )    : _zero_(this+1), _chunk_size( buffer_size ) {}
                               ~Simple_byte_queue       ()                                          { clear(); }

    void                        clear                   ();

    Byte*                       write_buffer            ( int size )                                { return request_write_buffer_2( size, 0 ); } // 1. Pointer lesen
    int                         write_buffer_size       ();                                                                                // 2. dann Größe des Puffers
    void                        on_write_buffer_written ( int count )                               { _write_position += count; }          // 3. Schließlich die Länge setzen

    Byte*                       request_write_buffer    ( int length )                              { return request_write_buffer_2( length, length ); }

    string                      string_dequeue_to_end   ();
    size_t                      length_to_end           ();


  private:
    Byte*                       request_write_buffer_2  ( int size, int used_length );


    struct Buffer
    {
                                Buffer                  ()                                          : _ptr(NULL), _size(0) {}
                                Buffer                  ( const Buffer& )                           : _ptr(NULL), _size(0) {}
                               ~Buffer                  ();

        void                    allocate                ( int size );

        Byte*                  _ptr;
        int                    _size;
    };


    Fill_zero                  _zero_;
    size_t                     _write_position;
    int const                  _chunk_size;

    typedef std::list<Buffer>   Buffer_list;
    Buffer_list                _buffer_list;

    Buffer_list::iterator      _read_iterator;
    size_t                     _read_position;

  public:
    static const int            default_chunk_size;
};

//------------------------------------------------------------------------------------Gzip_deflator

struct Gzip;

struct Gzip_deflator : Object
{
                                Gzip_deflator           ( Simple_byte_queue* );
                               ~Gzip_deflator           ();

    void                        write                   ( const Byte*, int length );

  private:
    Fill_zero                  _zero_;
    ptr<Gzip>                  _gzip;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
