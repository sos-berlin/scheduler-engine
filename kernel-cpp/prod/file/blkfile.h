// blkfile.h
// 14. 3.92                                             (c) Joacim Zschimmer

#ifndef __BLKFILE_H
#define __BLKFILE_H

#include "../file/absfile.h"


struct Blocked_file : public Abs_record_file
{
    Blocked_file () : _opened (false),
                      _opened_by_constructor (false) {}

    Blocked_file (
        const char         *filename,
        Open_mode           open_mode
    )
      : _opened (false),
        _opened_by_constructor (true)
    {
        open (filename, open_mode);
    }

    ~Blocked_file ()  {
         close (_opened_by_constructor ? close_normal : close_error);
    }

    void open (
        const char         *filename,
        Open_mode           open_mode,
        File_spec          &file_spec
    );

    void close (
        Close_mode = close_normal
    );

    void put (
        const void     *buffer,
        Record_length   size
    );

    void get (
        void           *buffer,
        Record_length   size,
        Record_length  *length
    );

    void update(
        const void     *buffer,
        Record_length   length
    );

    void set(long record_number);
    void del();

  private:
    struct Block {
        Block() : _f(0), _buffer(0) {}
        ~Block()  { delete _buffer; }

        void init( fstream*, int blksize );
        void exit();
        void create( long seek_value );
        void create_next()  { create( _seek_value + _blksize ); }
        void read( long seek_value );
        void read_next() { read( _seek_value + _blksize ); }
        void flush() { if (_is_modified)  _write(); }
        long seek_value() const { return _seek_value; }
        void set_append_mode();
        void set_block_length( uint l )  { *(uint2*)_buffer = l; }

        uint2 length() const { return *(uint2*)_buffer; }
        int end_of_block() const { return long2int( _ptr - _buffer ) == length(); };
        uint space_left() const { return long2int( _buffer + _blksize - _ptr ); }
        int fits_general(Record_length l) const { return 2 + 3 + l < _blksize; }
        int fits( Record_length l ) const { return space_left() >= 3 + l; }
        int is_append_mode() const { return _mode == append_mode; }
        void put( const void*, Record_length );
        void get( void*, Record_length, Record_length* );
        void update( const void*, Record_length );
        void del();

      private:
        void _write();
        Record_length record_length(Byte*) const;

        fstream    *_f;
        Byte       *_buffer;
        Byte       *_ptr;
        Byte       *_prev_ptr;
        int         _blksize;
        long        _seek_value;
        enum {read_mode=1, append_mode} _mode;
        Bit         _is_modified;

    };

    struct Header {
        char i_am_a_blkfile [16];   // "Blocked_file\n^Z"
        Byte version;
        Byte blksize512;

        void set_standards();
        void set_blksize(long blksize)   { blksize512 = (blksize+511) / 512; }
        long blksize() const { return blksize512 * 512; }
        int is_blocked() const { return blksize512 != 0; };
    };

    fstream     _f;
    Block       _block;
    Header      _hdr;
    enum {is_get = 1, is_set, is_put} _mode;
    Bool        _opened : 1;
    Bool        _opened_by_constructor : 1;

}; // struct Blocked_file

#endif

