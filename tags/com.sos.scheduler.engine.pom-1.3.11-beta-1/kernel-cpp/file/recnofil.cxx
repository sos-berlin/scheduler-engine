//#define MODULE_NAME "recnofil"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

namespace sos {

//-----------------------------------------------------------------------------------Key_buffer

struct Key_buffer
{
                                    Key_buffer          ()              : _zero_(this+1) {}
    Byte*                           operator []         ( int );
    void                            add                 ( const Byte* );
    void                            last_index          ( int );

    Fill_zero                      _zero_;
    DECLARE_PRIVATE_MEMBER( int,    first_index )
    DECLARE_PRIVATE_MEMBER( int,    entry_size )
    Dynamic_area                   _buffer;
};

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

//-----------------------------------------------------------------------Record_number_key_file

struct Record_number_key_file : Abs_file
{
                                    Record_number_key_file  ();
                                   ~Record_number_key_file  ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );
  //void                            insert              ( const Const_area& );
    void                            update              ( const Const_area& );
  //void                            store               ( const Const_area& );
    static const Abs_file_type&     static_file_type    ();

  protected:
    void                            get_record          ( Area& );
    void                            get_recno           ( Area*, int );
    void                            put_record          ( const Const_area& );

  private:
    Fill_zero                      _zero_;
    Any_file                       _file;
    Sos_ptr<Field_descr>           _key_field_descr;
    int                            _key_length;
    int                            _key_position;
    Key_buffer                     _key_buffer;
    int                            _highest_recno;
    int                            _next_recno;
};

//------------------------------------------------------------------Record_number_key_file_type

struct Record_number_key_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "recno/key"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Record_number_key_file> f = SOS_NEW_PTR( Record_number_key_file() );
        return +f;
    }
};

const Record_number_key_file_type  _record_number_key_file_type;
const Abs_file_type&                record_number_key_file_type = _record_number_key_file_type;

//-----------------------------------------------Record_number_key_file::Record_number_key_file

Record_number_key_file::Record_number_key_file()
:
    _zero_(this+1)
{
}

//----------------------------------------------Record_number_key_file::~Record_number_key_file

Record_number_key_file::~Record_number_key_file()
{
    if( _key_field_descr ) {
        for( int i = _highest_recno; i >= _key_buffer.first_index(); i-- ) {
            _key_field_descr->type().destruct( _key_buffer[ i ] );
        }
    }
}

//-----------------------------------------------------------------Record_number_key_file::open

void Record_number_key_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_option_iterator opt ( filename );

    // Dateityp-Optionen parsieren
    for(; !opt.end(); opt.next() )
    {
        if ( opt.with_value( "first" ) )        _key_buffer.first_index( as_int( opt.value() ) );
        else
        if ( opt.param() || opt.pipe()      )   break;
        else throw                              Sos_option_error( opt );
    }

    _file.open( opt.rest(), open_mode, file_spec );


    _any_file_ptr->_spec._field_type_ptr = _file.spec()._field_type_ptr;

    _key_field_descr = +_file.spec()._key_specs[0]._field_descr_ptr;
    if( !_key_field_descr )  throw_xc( "recno/key", "Feldbeschreibung für Satzschlüssel fehlt" );
    _key_length = _key_field_descr->type().field_size();
    _key_buffer.entry_size( _key_length );
}

//----------------------------------------------------------------Record_number_key_file::close

void Record_number_key_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//-----------------------------------------------------------Record_number_key_file::get_record

void Record_number_key_file::get_record( Area& area )
{
    get_recno( &area, _next_recno );
    _next_recno++;
}

//------------------------------------------------------------Record_number_key_file::get_recno

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

//------------------------------------------------------------------Record_number_key_file::put

void Record_number_key_file::put_record( const Const_area& record )
{
    _file.put( record );
}

//---------------------------------------------------------------Record_number_key_file::insert
/*
void Record_number_key_file::insert( const Const_area& area )
{
}
*/
//----------------------------------------------------------------Record_number_key_file::store
/*
void Record_number_key_file::store( const Const_area& area )
{
}
*/
//---------------------------------------------------------------Record_number_key_file::update

void Record_number_key_file::update( const Const_area& record )
{
    _file.update( record );
}


} //namespace sos