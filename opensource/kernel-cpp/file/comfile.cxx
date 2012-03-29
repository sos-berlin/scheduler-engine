//#define MODULE_NAME "comfile"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosfield.h"
//#include <sosdate.h> // wg. std_date_format_ingres
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"

#include "../kram/log.h"

namespace sos {


struct Com_file;

//-------------------------------------------------------------------------------------Com_main

struct Com_main : Sos_self_deleting
{
                                    Com_main            () : _zero_(this+1), _index(-1)  {}
                                   ~Com_main            ();

    Fill_zero                      _zero_;
    Sos_string                     _filename;
  //Dynamic_area                   _key;
    Any_file                       _file;
    Com_file*                      _last_user;
    Bool                           _prepared;
    Bool                           _opened;
    int                            _key_pos;
    int                            _key_len;
    int                            _index;              // Für Comfile_static::_com_main_array[]
    Bool                           _single_use;         // com ist unwirksam, nur ein Benutzer
};

//-------------------------------------------------------------------------------Comfile_static

DEFINE_SOS_DELETE_ARRAY( Com_main* )

struct Comfile_static : Sos_self_deleting
{
    Sos_simple_array<Com_main*>     _com_main_array;
};

DEFINE_SOS_STATIC_PTR( Comfile_static )

//-----------------------------------------------------------------------------------Com_file

struct Com_file : Abs_file
{
                                    Com_file();
                                   ~Com_file();

    void                            prepare_open        ( const char*, Open_mode, const File_spec& );
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            insert              ( const Const_area& area );
    void                            update              ( const Const_area& area );
    void                            store               ( const Const_area& area );

    void                            set                 ( const Key& );
    void                            del                 ( const Key& key );
    void                            del                 ();

  // jz 22.12.97  Record_position                 key_position        (Key::Number ) { return _com_main->_key_pos; };
  // jz 22.12.97  Record_length                   key_length          (Key::Number ) { return _com_main->_key_len; };

  protected:
    void                            get_record          ( Area& area );
    void                            get_until           ( Area*, const Const_area& until_key );
    void                            get_record_key      ( Area& area, const Key& key );
    void                            get_position        ( Area* );
    void                            put_record          ( const Const_area& area );
    void                            rewind              ( Key::Number );

  private:
    void                           _get_record          ( Area& area, const Const_area* until_key  );

    Fill_zero                      _zero_;
    Dynamic_area                   _key;
    Bool                           _set_key;            // letzte Operation war set_key
    Bool                           _rewind;
    Sos_ptr<Com_main>              _com_main;
};

//------------------------------------------------------------------------------Com_file_type

struct Com_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "com"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
    	Sos_ptr<Com_file> f = SOS_NEW_PTR( Com_file );
    	return +f;
    }
};

const Com_file_type   _com_file_type;
const Abs_file_type&   com_file_type = _com_file_type;

//----------------------------------------------------------------------Com_main::~Com_main

Com_main::~Com_main()
{
    if( _index >= 0 ) {
        int i = _index;  _index = -1;
        Comfile_static* st = sos_static_ptr()->_comfile;  // Falls falsche Reihenfolge beim shutdown
        if( st )  st->_com_main_array[ i ] = 0;
    }
}

// --------------------------------------------------------------------- Com_file::Com_file

Com_file::Com_file()
:
    _zero_ ( this+1 )
{
}

//----------------------------------------------------------------------Com_file::~Com_file

Com_file::~Com_file()
{
}

//---------------------------------------------------------------------Com_file::prepare_open

void Com_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    int                     i;
    Sos_string              filename;
  //Comfile_static*         st = auto_new_ptr( &sos_static_ptr()->_comfile );
    Comfile_static*         st = sos_static_ptr()->_comfile;
    Sos_ptr<Record_type>    callers_record_type = file_spec._field_type_ptr;

    if( !st ) {
        Sos_ptr<Comfile_static> p = SOS_NEW( Comfile_static );
        sos_static_ptr()->_comfile = +p;
        p->_com_main_array.obj_const_name( "Com_file::_com_main_array" );
        st = p;
    }

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if ( opt.pipe() )                               filename = opt.rest();
        else throw_sos_option_error( opt );
    }



    for( i = st->_com_main_array.first_index(); i <= st->_com_main_array.last_index(); i++ )
    {
        Com_main* m = st->_com_main_array[ i ];
        if( m ) {
            LOG( "Com_file: \"" << filename << "\" == \"" << m->_filename << "\" ?\n" );

            if( !m->_single_use
             && m->_filename == filename
             && ( m->_key_pos >= 0
                  || ( ( (open_mode & (out|app) ) == (out|app) )  &&  !(open_mode & in) ) ) )
            {
                _com_main = st->_com_main_array[ i ];
                LOG( "FOUND!\n" );
                _any_file_ptr->_spec = _com_main->_file.spec();
                if( _any_file_ptr->_spec._field_type_is_from_caller )  _any_file_ptr->_spec._field_type_ptr = +callers_record_type;     // jz 18.2.00
                goto FOUND;
            }
        }
    }
    {
        // Com_main anlegen:

        for( i = st->_com_main_array.first_index(); i <= st->_com_main_array.last_index(); i++ ) {
            if( st->_com_main_array[ i ] == NULL )  break;
        }

        if( i > st->_com_main_array.last_index() )  st->_com_main_array.last_index( i );

        _com_main = SOS_NEW( Com_main );
        Com_main* m = _com_main;

        st->_com_main_array[ i ] = m;
        m->_index = i;

        m->_file.prepare_open( filename, open_mode, file_spec );

        m->_key_pos = m->_file.key_position();
        m->_key_len = m->_file.key_length();
        m->_prepared = true;
        m->_filename = filename;
        m->_last_user = this;

        // Bei _single_use ist rewind() auf keylose Datei möglich
        m->_single_use = ( ( open_mode & inout ) == in )  &&  m->_key_pos < 0;

/*
        if( _com_main->_key_pos >= 0
         || ( ( open_mode & ( Any_file::out | Any_file::app )) == ( Any_file::out | Any_file::app ) )
            && !( open_mode & Any_file::in ) )
        {
          //m->_key.allocate_min( m->_key_len );
          //m->_key.length( m->_key_len );
          //memset( m->_key.ptr(), 0, m->_key_len );

            // Nur Key-Dateien sind gemeinsam nutzbar
            _com_main->_index = i;
            st->_com_main_array[ i ] = _com_main;
        }
*/
    }

    _any_file_ptr->_spec = _com_main->_file.spec();

  FOUND:
    _key_pos = _com_main->_key_pos;  // jz 22.12.97
    _key_len = _com_main->_key_len;  // jz 22.12.97
}

//-----------------------------------------------------------------------------Com_file::open

void Com_file::open( const char*, Open_mode, const File_spec& )
{
    if( !_com_main->_opened ) {
        _com_main->_file.open();
        // key_position() ist erst jetzt gültig:
        //_com_main->_key_pos = _com_main->_file.key_position();
        //_com_main->_key_len = _com_main->_file.key_length();
        _com_main->_opened = true;
    }

    _key.allocate_min( _com_main->_key_len );
    _key.length( _com_main->_key_len );
    memset( _key.ptr(), 0, _com_main->_key_len );
    current_key_ptr( &_key );
    _set_key = true;
    _rewind = true;
}

// -------------------------------------------------------------------------- Com_file::close

void Com_file::close( Close_mode )
{
    if( _com_main->_index == -1 ) {     // Keine gemeinsame Datei?
        _com_main->_opened = false;
        _com_main->_file.close();
    }
}

// ----------------------------------------------------------------------------Com_file::rewind

void Com_file::rewind( Key::Number )
{
    LOG( "Com_file::rewind\n" );

    if( _com_main->_single_use
     && _key.length() == 0     )  throw_xc( "SOS-1256", c_str( _any_file_ptr->_filename ) );

    if( _com_main->_last_user == this ) {
        _com_main->_last_user = 0;
    }

    memset( _key.ptr(), 0, _key.length() );
    _set_key = true;
    _rewind = true;
}

// --------------------------------------------------------------------- Com_file::put_record

void Com_file::put_record( const Const_area& record )
{
    _com_main->_last_user = this;
    _com_main->_file.put( record );
}

// --------------------------------------------------------------------- Com_file::get_record

void Com_file::get_record( Area& area )
{
    _get_record( area, NULL );
}

// --------------------------------------------------------------------- Com_file::get_until

void Com_file::get_until( Area* buffer, const Const_area& until_key )
{
    _get_record( *buffer, &until_key );
}

// --------------------------------------------------------------------- Com_file::get_record

void Com_file::_get_record( Area& area, const Const_area* until_key )
{
    //LOGI( "Com_file::get_record\n" );

    if( _com_main->_last_user != this ) {
        if( !_set_key ) {
            try {
                incr( &_key );
            }
            catch( const Overflow_error& ) {
                throw_eof_error();
            }
        }

        _com_main->_last_user = this;

        if( _rewind ) {
            _rewind = false;
            _com_main->_file.rewind();
        } else {
            _com_main->_file.set( _key );
        }
    }

    //LOG( "_com_main->_file.get( &area );\n" );
    if( until_key )  _com_main->_file.get_until( &area, *until_key );
               else  _com_main->_file.get( &area );

    if( _com_main->_key_pos >= 0 ) {
        if( area.length() >= _com_main->_key_pos + _com_main->_key_len ) {
            //LOG( "_key.assign( area.byte_ptr() + _com_main->_key_pos, _com_main->_key_len );\n");
            _key.assign( area.byte_ptr() + _com_main->_key_pos, _com_main->_key_len );
            _set_key = false;
            _rewind = false;
        }
    }
}

// ----------------------------------------------------------------------Com_file::get_position

void Com_file::get_position( Area* buffer )
{
    if( _com_main->_last_user != this ) {
        if( !_set_key ) {
            try {
                incr( &_key );
            }
            catch( const Overflow_error& ) {
                throw_eof_error();
            }
        }

        _com_main->_last_user = this;

        if( _rewind ) {
            _rewind = false;
            _com_main->_file.rewind();
        } else {
            _com_main->_file.set( _key );
        }
    }

    _com_main->_file.get_position( buffer );

    _key.assign( *buffer );
    _set_key = false;
    _rewind = false;
//LOG( "Com_file::get_position=" << hex << _key << dec << "\n" );
}

// ------------------------------------------------------------------Com_file::get_record_key

void Com_file::get_record_key( Area& area, const Key& key )
{
    _com_main->_last_user = this;

    _com_main->_file.get_key( &area, key );

    _key.assign( key );
    _set_key = false;
    _rewind = false;

//LOG( "Com_file::get_record_key _key=" << hex << _key << dec << "\n" );
    //_com_main->_key.assign( _key );
}

// ------------------------------------------------------------------------- Com_file::insert

void Com_file::insert( const Const_area& record )
{
    _com_main->_last_user = this;
    _com_main->_file.insert( record );
}

// --------------------------------------------------------------------------Com_file::update

void Com_file::update( const Const_area& record )
{
    if( _com_main->_key_pos >= 0 ) {
        int l = record.length() - _com_main->_key_pos;
        if( l < _com_main->_key_len
         || memcmp( record.byte_ptr() + _com_main->_key_pos, _key.ptr(), _com_main->_key_len ) != 0 )
        {
            //LOG( "Com_file::update current_key=" << hex << _key <<", geändert="
            //     << Const_area( record.byte_ptr() + _com_main->_key_pos, max( 0, l ) ) << dec << '\n' );
            throw_xc( "SOS-1229" );
        }
    }

    if( _com_main->_last_user == this ) {
        _com_main->_file.update( record );
    } else {
        _com_main->_last_user = this;
        _com_main->_file.store( record );
    }
}

// ---------------------------------------------------------------------------Com_file::store

void Com_file::store( const Const_area& record )
{
    _com_main->_last_user = this;
    _com_main->_file.store( record );
}

// ---------------------------------------------------------------------------- Com_file::set

void Com_file::set( const Key& key )
{
    if( _com_main->_last_user == this ) {
        _com_main->_last_user = 0;
    }

    _key = key;
    _rewind = false;
    _set_key = true;
//LOG( "Com_file::set _key=" << hex << _key << dec << "\n" );
}

// ---------------------------------------------------------------------------- Com_file::del

void Com_file::del( const Key& key )
{
    _com_main->_last_user = this;

    _com_main->_file.del( key );

  //_key.assign( key );
  //_com_main->_key.assign( key );
}

//------------------------------------------------------------------------------Com_file::del

void Com_file::del()
{
    if( _com_main->_last_user == this ) {
        _com_main->_last_user = this;
        _com_main->_file.del();       // jz 9.10.97
    } else {
        _com_main->_last_user = this;
        _com_main->_file.del( _key );
    }
}


} //namespace sos
