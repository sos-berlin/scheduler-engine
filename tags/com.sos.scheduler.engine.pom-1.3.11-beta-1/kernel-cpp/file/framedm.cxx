// $Id$
#include "precomp.h"
//#define MODULE_NAME "frmfield"
//#define COPYRIGHT   "©1998 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"
#include "../kram/log.h"
#include "../kram/soslimtx.h"
#include "../kram/sosfield.h"
#include "../kram/stdfield.h"
#include "../file/anyfile.h"

namespace sos {

//-----------------------------------------------------------------------------------Frame_dm_file

struct Frame_dm_file : Abs_file
{

                                Frame_dm_file           ();
                               ~Frame_dm_file           ();

    void                        open                    ( const char*, Open_mode, const File_spec& );

 protected:
    void                        get_record              ( Area& area );

    Fill_zero                  _zero_;
    Sos_ptr<Record_type>       _column_type;
    Sos_ptr<Record_type>       _record_type;
    Any_file                   _file;
    Dynamic_area               _line;
    int                        _x;
    int                        _y;
    const char*                _p;
    Bool                       _header_read;

    struct Field
    {
                                    Field               () : _zero_(this+1) {}

        Fill_zero                  _zero_;
        int                        _y;
        int                        _x;
        int                        _length;
        char                       _typ     [ 50+1 ];
        char                       _text    [ 80+1 ];
        char                       _brackets [ 2+1 ];
    };
};

//-----------------------------------------------------------------------------Frame_dm_file_type

struct Frame_dm_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "frame_dm"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Frame_dm_file> f = SOS_NEW( Frame_dm_file );
        return +f;
    }
};

const Frame_dm_file_type       _frame_dm_file_type;
const Abs_file_type&            frame_dm_file_type = _frame_dm_file_type;

//-------------------------------------------------------------------Frame_dm_file::Frame_dm_file

Frame_dm_file::Frame_dm_file()
:
    _zero_(this+1)
{
}

//------------------------------------------------------------------Frame_dm_file::~Frame_dm_file

Frame_dm_file::~Frame_dm_file()
{
}

//----------------------------------------------------------------------------Frame_dm_file::open

void Frame_dm_file::open( const char* param, Open_mode open_mode, const File_spec& )
{
    Sos_string filename;

    for( Sos_option_iterator opt = param; !opt.end(); opt.next() )
    {
        if( opt.pipe()  ||  opt.param() )  filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _line.allocate_min( 80+1 );

    _file.open( filename, Any_file::in_seq );

    _column_type = Record_type::create();
    Record_type* t = _column_type;
    Field*       o = 0;

    t->name( "Frame_dm_field" );
    t->allocate_fields( 6 );

    RECORD_TYPE_ADD_FIELD       ( y             , 0 );
    RECORD_TYPE_ADD_FIELD       ( x             , 0 );
    RECORD_TYPE_ADD_FIELD       ( length        , 0 );
    RECORD_TYPE_ADD_CHAR        ( typ           , 0 );
    RECORD_TYPE_ADD_CHAR        ( text          , 0 );
    RECORD_TYPE_ADD_CHAR        ( brackets      , 0 );

    _any_file_ptr->_spec._field_type_ptr = +_column_type;
}

//----------------------------------------------------------------------Frame_dm_file::get_record

void Frame_dm_file::get_record( Area& buffer )
{
    if( !_header_read ) 
    {
        while(1) {
            _file.get( &_line );
            if( _line.length() >= 2  &&  _line.char_ptr()[0] == '.' ) {
                if( _line.char_ptr()[1] != '*' )  break;
            }
        }

        if( _line.length() < 3 
         || strnicmp( _line.char_ptr(), ".dm", 3 ) != 0 )  throw_xc( "SOS-FRAME-DM-1" );

        _file.get( &_line );
        _line.append( '\0' );
        _p = _line.char_ptr();

        _header_read = true;
    }

    while( *_p == ' ' )  _p++;

    while( !*_p ) 
    {
        _y++;
        _file.get( &_line );
        _line.append( '\0' );
        if( strnicmp( _line.char_ptr(), ".dm ", 4 ) == 0 )  throw_eof_error();
        _p = _line.char_ptr();
        while( *_p == ' ' )  _p++;
    }

    const char* p = _p;
    Field       f;

    switch( *p ) 
    {
        case '@': 
        case '§': 
        {
            strcpy( f._typ, "output" );
            while( *p == *_p )  p++;
            f._length = p - _p;
            break;
        }

        case '!':
        {
            strcpy( f._typ, "inout" );
            while( *p == '!' )  p++;
            f._length = p - _p;

            f._brackets[0] = _p == _line.char_ptr()? ' ' : _p[-1];
            f._brackets[1] = *p? *p : ' ';
            f._brackets[2] = '\0';
            break;
        }

        default:
        {
            strcpy( f._typ, "constant" );
            while( *p  &&  *p != '@'  &&  *p != '§'  &&  *p != '!' )  p++;
            p = _p + length_without_trailing_spaces( _p, p - _p );
            memcpy( f._text, _p, min( p - _p, 80 ) );
        }
    }

    f._x      = _p - _line.char_ptr();
    f._y      = _y;
    f._length = p - _p;

    _p = p;

    buffer.assign( &f, sizeof f );
}


} //namespace sos
