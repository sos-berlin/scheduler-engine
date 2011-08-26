// $Id: tail_file.cxx 11394 2005-04-03 08:30:29Z jz $


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

//-----------------------------------------------------------------------------------Tail_file

struct Tail_file : Abs_file
{
                                    Tail_file          ();

    void                            open                ( const char*, Open_mode, const File_spec& );
  //void                            close               ( Close_mode );

  //void                            rewind              ( Key::Number );

  protected:
    void                            get_record          ( Area& area );
    void                            read_tail           ();

  private:
    Fill_zero                      _zero_;
    Open_mode                      _open_mode;
    bool                           _reverse;
    int                            _head;
    int                            _head_count_down;
    int                            _tail;
    Any_file                       _file;
    std::vector<Dynamic_area>      _queue;
    int                            _begin;
    int                            _end;
    int                            _read_next;
    bool                           _tail_read;
};

//------------------------------------------------------------------------------Tail_file_type

struct Tail_file_type : Abs_file_type
{
    virtual const char*         name                    () const        { return "tail"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Tail_file> f = SOS_NEW( Tail_file );
        return +f;
    }
};

const Tail_file_type  _tail_file_type;
const Abs_file_type&   tail_file_type = _tail_file_type;

//---------------------------------------------------------------------- Tail_file::Tail_file

Tail_file::Tail_file()
:
    _zero_(this+1)
{
}

//-----------------------------------------------------------------------------Tail_file::open

void Tail_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string  filename;

    _open_mode = open_mode;
    _tail = 10;

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "reverse" ) )      _reverse = opt.set();
        else
        if( opt.with_value( "head"    ) )      _head = opt.as_uint();
        else
        if( isdigit( opt.option()[ 0 ] )  &&  opt.flag( opt.option() ) )   // -1234
        {
            _tail = as_int( opt.option() );
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

    _queue.resize( _tail+1 );

    _head_count_down = _head;
}

// --------------------------------------------------------------------- Tail_file::rewind
/*
void Tail_file::rewind( Key::Number )
{
    if( _tail_read )  
    {
        if( _open_mode & File_base::seq )  throw_xc( "SOS-1352" );

        _read_next = _reverse? _end : _begin;
    }
}
*/
// -------------------------------------------------------------------Tail_file::get_record

void Tail_file::get_record( Area& area )
{
    if( _head_count_down > 0 ) 
    {
        _file.get( area );
        _head_count_down--;
    }
    else
    {
        if( !_tail_read )
        {
            read_tail();
            _file.close();
            _read_next = _reverse? _end : _begin;
            _tail_read = true;
        }

        int idx = _read_next;

        if( _reverse )
        {
            if( _read_next == _begin )  throw_eof_error();
            _read_next = _read_next > 0? _read_next - 1 : _tail;

            area.assign( _queue[ _read_next ] );
        }
        else
        {
            if( _read_next == _end )  throw_eof_error();

            area.assign( _queue[ _read_next ] );
        
            _read_next = _read_next < _tail? _read_next+1 : 0;
        }

        //if( _open_mode & File_base::seq )   gibt sowieso kein rewind()
        _queue[idx].free();
    }
}

//----------------------------------------------------------------------Tail_file::read_tail

void Tail_file::read_tail()
{
    if( _tail == 0 )  return;

    while(1)
    {
        try 
        {
            _file.get( &_queue[_end] );

            _end = _end < _tail? _end+1 : 0;
            if( _begin == _end )   _begin = _begin < _tail? _begin+1 : 0;
        }
        catch( const Eof_error& ) { break; }
    }
}

} //namespace sos
