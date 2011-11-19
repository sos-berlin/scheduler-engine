// $Id: head_file.cxx 11394 2005-04-03 08:30:29Z jz $


#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"

#include "../kram/log.h"


namespace sos {




//-----------------------------------------------------------------------------------Head_file

struct Head_file : Abs_file
{
                                    Head_file          ();

    void                            open                ( const char*, Open_mode, const File_spec& );
  //void                            close               ( Close_mode );

    void                            rewind              ( Key::Number );
  //void                            set                 ( const Key& );
  //Record_position                 key_position        ( Key::Number ) { return _file.key_position(); }
  //Record_length                   key_length          ( Key::Number ) { return _file.key_length(); }

  protected:
    void                            get_record          ( Area& area );
  //void                            update_current_key  ( int64 );

  private:
    Fill_zero                      _zero_;
    int                            _head;
    int                            _count_down;
    Any_file                       _file;
};

//------------------------------------------------------------------------------Head_file_type

struct Head_file_type : Abs_file_type
{
    virtual const char*         name                    () const        { return "head"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Head_file> f = SOS_NEW( Head_file );
        return +f;
    }
};

const Head_file_type  _head_file_type;
const Abs_file_type&   head_file_type = _head_file_type;

//---------------------------------------------------------------------- Head_file::Head_file

Head_file::Head_file()
:
    _zero_(this+1)
  //_current_key( _current_key_buffer, sizeof _current_key_buffer )
{
}

//-----------------------------------------------------------------------------Head_file::open

void Head_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string  filename;

    _head = 10;

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( isdigit( opt.option()[ 0 ] )  &&  opt.flag( opt.option() ) )   // -1234
        {
            _head = as_int( opt.option() );
        }
        else
        if( opt.param() || opt.pipe() )  { filename = opt.rest(); break; }
        else 
            throw_sos_option_error( opt );
    }

    _file.open( filename, Abs_file::Open_mode( open_mode | in | seq ), file_spec );

    _any_file_ptr->_spec._field_type_ptr                       = _file.spec()._field_type_ptr;
    _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = _file.spec()._key_specs._key_spec._field_descr_ptr;

    _key_pos = _file.key_position();
    _key_len = _file.key_length();

    _count_down = _head;
}

// --------------------------------------------------------------------- Head_file::rewind

void Head_file::rewind( Key::Number )
{
    _file.rewind();
    _count_down = _head;
}

// -------------------------------------------------------------------Head_file::get_record

void Head_file::get_record( Area& area )
{
    if( _count_down == 0 )  throw_eof_error();

    _file.get( area );
    _count_down--;
}


} //namespace sos
