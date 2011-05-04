#define MODULE_NAME "blkfile"
// blkfile.cpp
//                                                      (c) Joacim Zschimmer

//#pragma hdrfile "headers.sym"
#include "headers.h"
#pragma hdrstop

#include <stdlib.h>
#include <string.h>
#include <stdio.h>      //sprintf
#include <errno.h>

#include <jzincl.h>

#include <blkfile.h>

//-----------------------------------------------------------------------------


//----------------------------------------------------Blocked_file::Block::init

void Blocked_file::Block::init( fstream *f, int blksize )
{
    memset( this, 0, sizeof *this );

    _f = f;
    _buffer = new Byte[ blksize ];
    if (!_buffer)  raise ("NOMEMORY", "R101");

  exceptions
}

//----------------------------------------------------Blocked_file::Block::exit

void Blocked_file::Block::exit()
{
    delete _buffer;
    _buffer = 0;
    _f = 0;
}

//--------------------------------------------------Blocked_file::Block::create

inline void Blocked_file::Block::create( long seek_value )
{
    _seek_value = seek_value;
    _mode = append_mode;
    _ptr = _buffer + 2;
    _is_modified = false;
}

//----------------------------------------------------Blocked_file::Block::read

void Blocked_file::Block::read( long seek_value )
{
    flush();
    _seek_value = seek_value;
    _f->seekg( _seek_value );
    _f->read( _buffer, _blksize );
    if (_f->gcount() != _blksize )  raise ("EOF", "D310");
    _ptr = _buffer + 2;
    _is_modified = false;
    _mode = read_mode;

  exceptions
}

//--------------------------------------------------Blocked_file::Block::_write

void Blocked_file::Block::_write()
{
    if (_mode == append_mode) {
        set_block_length( long2int( _ptr - _buffer ));
    }
    _f->seekp( _seek_value );
    _f->write( _buffer, _blksize );
    if (!_f->good())  raise_errno( errno );
    _is_modified = false;
    _mode = 0;

  exceptions
}

//-----------------------------------------------------Blocked_file::Block::get

void Blocked_file::Block::get(
        void                  *record_buffer,
        Record_length          record_buffer_size,
	Record_length         *record_length
) {
    
    Record_length   len;
    int             c;
    
    _prev_ptr = _ptr;

    len = 0;
    do {
        c = *_ptr++;
        if (c == EOF)  break;
        len = (len << 7) + (c & 0x7F);
    } while (c & 0x80);

    memcpy ((char*) record_buffer, _ptr, min (len, record_buffer_size));
    _ptr += len;
    
    if (len > record_buffer_size)  raise ("TRUNCATE", "D320");
    
    *record_length = len;
    
  exceptions

} // Blocked_file::Block::get

//-------------------------------------------Blocked_file::Block::record_length

Record_length Blocked_file::Block::record_length(Byte *p) const
{
    Byte          c;
    Record_length len = 0;

    do {
        c = *p++;
        if (c == EOF)  break;
        len = (len << 7) + (c & 0x7F);
    } while (c & 0x80);

    return len;
}

//--------------------------------------------------Blocked_file::Block::update

void Blocked_file::Block::update(
        const void            *rec_buffer,
	Record_length          rec_length
) {
    
    if (record_length(_prev_ptr) == rec_length) {
        memcpy (_ptr - rec_length, rec_buffer, rec_length);
        _is_modified = true;
    } else
                raise("UPDATE", "D4??");

  exceptions

} // Blocked_file::Block::update

//-----------------------------------------------------Blocked_file::Block::del

void Blocked_file::Block::del () 
{
    if (_mode != read_mode
     || _prev_ptr == _ptr)  raise("DELETE", "D???"); //??????????????????

    memmove( _prev_ptr, _ptr, long2int( _buffer + length() - _ptr ));
    (uint2*)_buffer -= long2int( _ptr - _prev_ptr );

    _is_modified = true;
    _mode = read_mode;

  exceptions

} // Blocked_file::Block::del

//-----------------------------------------Blocked_file::Block::set_append_mode

void Blocked_file::Block::set_append_mode()
{
    _ptr = _buffer + length();
    _mode = append_mode;
}

//-----------------------------------------------------Blocked_file::Block::put

void Blocked_file::Block::put (
        const void            *record,
        Record_length          record_length
) {
    Record_length l;

    if (_mode != append_mode)  return;

    l = record_length;
    do {
        int c = l & 0x7F;
        *_ptr++ = c | ((l >>= 7) ? 0x80 : 0);
    } while (l);

    memcpy (_ptr, record, record_length);
    _ptr += record_length;

    _is_modified = true;

} // Blocked_file::Block::put

//------------------------------------------Blocked_file::Header::set_standards

void Blocked_file::Header::set_standards()
{
    memset ( this, 0, sizeof *this );

    strcpy (i_am_a_blkfile, "Blocked_file\n\x1A\0");
}

//----------------------------------------------------------Blocked_file::open

void Blocked_file::open (
    const char     *filename,
    Open_mode       open_mode,
    File_spec      &file_spec
) {

    _f.open( filename, (open_mode & standard_mask) | binary);
    if (_f.fail())  raise_errno (errno);

    _f.read ((char*)&_hdr, sizeof _hdr);
    if (_f.gcount() != sizeof _hdr) {
        if ((open_mode & nocreate)
         || _f.gcount() != 0) {
            raise ("WRONGTYP", "D150");
        }
        {                               // Kopf schreiben
            Header h;
            h.set_standards();
            h.set_blksize( file_spec.max_record_length() + 3 + 3 );
            _f.write( (char*)&h, sizeof h );
            if (_f.fail())  raise_errno( errno );
            for (int i = sizeof h; i < h.blksize(); i++)  _f.put(0);
        }
    }

    _block.init( &_f, _hdr.blksize() );  xc;
    _opened = true;

  exceptions
    _f.close();
    _block.exit();

} // Blocked_file::open

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴Blocked_file::close

void Blocked_file::close (
        Close_mode close_mode
) {

    if (!_opened)  return;
    _opened = false;

    _block.flush();
    _block.exit();
    _f.close();
    if (_f.fail())  raise_errno (errno);

  exceptions

} // Blocked_file::close

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴Blocked_file::get

void Blocked_file::get (
        void                  *record_buffer,
        Record_length          record_buffer_size,
	Record_length         *record_length
) {

    if (_mode != 0
     && _mode != is_get
     && _mode != is_set)  raise("EOF", "D310");

    if (_mode == 0) {
        _block.read( _hdr.blksize() );
    }

    while (_block.end_of_block()) {
        _block.read_next();  xc;
    }

    _block.get( record_buffer, record_buffer_size, record_length );  xc;
    _mode = is_get;

  exceptions

} // Blocked_file::get

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴Blocked_file::del

void Blocked_file::del ()
{
    if (_mode != is_get)  raise("DELETE", "D???"); //??????????????????

    _block.del();
    _mode = is_set;

  exceptions

} // Blocked_file::del

//------------------------------------------------------------Blocked_file::put

void Blocked_file::put (
        const void            * record,
        Record_length           record_length
) {
    
    if (record_length < 0
     || !_block.fits_general (record_length))  raise ("LENGTH", "D420");

    if (!_block.is_append_mode()) {
        _block.flush();
        _f.seekp( -_hdr.blksize(), ios::end );
        long s = _f.tellp();
        if (s == 0) {                   // Datei ist leer (nur Header ist da)?
            _block.create(s + _hdr.blksize() );
        } else {
            _block.read( s );           // Letzten Block lesen
        }
        _block.set_append_mode();
    }

    if (!_block.fits( record_length )) {
        _block.flush();
        _block.create_next();
    }

    _block.put( record, record_length );

  exceptions

} // Blocked_file::put

//--------------------------------------------------------Blocked_file::update

void Blocked_file::update (
        const void            * record,
        Record_length           record_length
) {
    
    if (record_length < 0
     || !_block.fits_general (record_length))  raise ("LENGTH", "D420");

    if (_mode != is_get)        raise("UPDATE","D4??");

    _block.update( record, record_length );

  exceptions

} // Blocked_file::update

//-----------------------------------------------------------Blocked_file::set

void Blocked_file::set( long record_number )
{
    _block.flush();
    _block.read( _hdr.blksize() );
}
