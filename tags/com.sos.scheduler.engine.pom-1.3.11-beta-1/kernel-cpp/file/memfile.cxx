//#define MODULE_NAME "stdfile"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"

#if 0

#include <stdlib.h>
#include <string.h>

#include <sos.h>
#include <xception.h>
#include <memfile.h>




//----------------------------------------------------------------------statics

struct Memory_file_type : Abs_file_type
{
    Memory_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "memory"; }

    virtual Bool is_record_file()  const { return true; }
    virtual Bool is_indexed_file()  const { return true; }

    virtual Abs_file_base*    create_base_file() const { return new Memory_file(); }
    virtual Abs_record_file*  create_record_file() const {return new Memory_file();  }
    virtual Abs_indexed_file* create_indexed_file() const { return new Memory_file(); }

};

static Memory_file_type _Memory_file_type;

const Abs_file_type& Memory_file::static_file_type ()
{
    return _Memory_file_type;
}



//-----------------------------------------------------------Memory_file::Memory_file

Memory_file::Memory_file()
{
}

//----------------------------------------------------------Memory_file::~Memory_file

Memory_file::~Memory_file()
{
    close();
    delete _current_key_value_ptr;
}

//---------------------------------------------------------------Memory_file::open

void Memory_file::open( const char* filename, Open_mode, File_spec& file_spec )
{
    _key_position = file_spec.key_specs().key_position();
    _key_length   = file_spec.key_specs().key_length();

    if( _key_length ) {
        _current_key_value_ptr = new Byte [ _key_length ];
                          if( !_current_key_value_ptr )  raise( "NOMEMORY", "R101" );

        _current_key = Const_area( _current_key_value_ptr, _key_length );
        current_key_ptr( &_current_key );
    }

  exceptions
}

//--------------------------------------------------------------Memory_file::close

void Memory_file::close( Close_mode )
{

    if (!opened)  return;
    opened = false;

}

//-------------------------------------------------------------Memory_file::rewind

void Memory_file::rewind( Key::Number )
{
    if( !_f_in )  raise( "NOTOPEN", "NOTOPEN" );

    _f_in->clear(); // ios::badbit | ios::failbit | ios::eofbit ); //? jz 31.8.94
    _f_in->seekg( 0 );

  exceptions
}

//---------------------------------------------------------Memory_file::get_record_key

void Memory_file::get_record_key( Area& area, const Key& key )
{
    int  cmp;
    Bool truncated = true;

    if( !_f_in )  raise( "NOTOPEN", "NOTOPEN" );

    rewind();

    while(1) {
        get( area );  truncated = exception("TRUNCATE"); xc;
        if( area.length() >= key_position() + key_length() ) {
            cmp = memcmp( area.char_ptr() + key_position(), key.ptr(), key_length() );
            if( cmp >= 0 )  break;
        }
    }

    if( cmp >  0  )  raise( "NOFIND", "D311" );
    if( truncated )  raise( "TRUNCATE", "D320" );

  exceptions
    if( exception( "EOF" ) ) {
        raise( "NOFIND", "D311" );
    }
}

//-----------------------------------------------------------------Memory_file::set

void Memory_file::set( const Key& key )
{
}
/*
Record_position Memory_file::key_position (Key::Number )
{
    return _key_position;
}

Record_length Memory_file::key_length   (Key::Number )
{
    return _key_length;
}
*/
//---------------------------------------------------------Memory_file::get_record
/*
void Memory_file::get_record( Area& area )
{
  exceptions
}
*/
//----------------------------------------------------------------Memory_file::put
/*
void Memory_file::put_record( const Const_area& area )
BEGIN
  exceptions
END
*/



#endif

