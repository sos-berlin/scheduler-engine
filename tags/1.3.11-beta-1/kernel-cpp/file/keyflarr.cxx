//#define MODULE_NAME "keyflarr"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
#include "precomp.h"

#if 0

#include <sos.h>
#include <anyfile.h>
//#include <keyflarr.h>


//-------------------------------------------------------------------------------Key_file_array
/*
struct Key_file_array
{
                                    Key_file_array      ( Any_file* open_file );
                                   ~Key_file_array      ();

    void                            first_index         ( int i )           { _first_index = i; }
    int                             first_index         () const            { return _first_index; }
    Const_area                      key                 ( int i );
    int                             index               ( const Const_area& key );
    void                            get                 ( Area* );

  private:
    Any_file*                      _file;
    Key_buffer                     _key_buffer;
};

*/


//-----------------------------------------------------------------------Key_buffer::operator[]

inline Byte* Key_buffer::operator[] ( int i )
{
    return _buffer.byte_ptr() + ( i - _first_index ) * _entry_size;
}

//-----------------------------------------------------------------------Key_buffer::last_index

void Key_buffer::last_index( int last )
{
    _buffer.resize_min( ( last - _first_index ) * _entry_size );
}

//------------------------------------------------------------------------------Key_buffer::add

void Key_buffer::add( const Byte* p )
{
    if( _buffer.length() + _entry_size > _buffer.size() ) {
        _buffer.resize_min( _buffer.length() + 4096 );
    }

    _buffer.append( p, _entry_size );
}

//---------------------------------------------------------------Key_file_array::Key_file_array
/*
Key_file_array::Key_file_array( Any_file* f )
:
    _file( f )
{
}

//--------------------------------------------------------------Key_file_array::~Key_file_array

Key_file_array::~Key_file_array()
{
}

//--------------------------------------------------------------------------Key_file_array::key

Const_area Key_file_array::key( int i )
{

}

//------------------------------------------------------------------------Key_file_array::index

int Key_file_array::index( const Const_area& key )
{
}

//--------------------------------------------------------------------------Key_file_array::get

void Key_file_array::get( Area* buffer )
{
}

//-----------------------------------------------------------------Key_file_array::get_by_index

void Record_number_key_file::get_recno( Area* buffer, int recno )
{
    if( recno <= _highest_recno ) {
        _file.get_key( buffer, Const_area( _key_buffer[ recno ], _key_length ) );
    }
    else
    {
        _key_buffer.last_index( recno );

        if( _next_recno < _highest_recno )  {
            _file.set( Const_area( _key_buffer[ _highest_recno ], _key_length ) );
            _next_recno = _highest_recno;
        }

        while( _next_recno < recno ) {
            //Area key ( _key_buffer[ _highest_recno + 1 ], _key_length );
            //_file.get_key_only( &key );
            _file.get( buffer );
            _key_field_descr->type().field_copy( _key_buffer[ _highest_recno + 1 ],
                                                 buffer->byte_ptr() + _key_field_descr->offset() );
            _next_recno++;
            if( _next_recno > _highest_recno )  _highest_recno = _next_recno - 1;
        }

        //_file.get( buffer );
        //_key_field_descr->type().field_copy( _key_buffer[ _highest_recno + 1 ],
        //                                     buffer.byte_ptr() + _key_field_descr->offset() );
    }
}

*/

#endif
