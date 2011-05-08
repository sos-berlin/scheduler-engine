//#define MODULE_NAME "keyseqfl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include "precomp.h"

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"


namespace sos {
using namespace std;


//---------------------------------------------------------------------------Key_seq_file

struct Key_seq_file : Abs_file
{
                                Key_seq_file            ();
                               ~Key_seq_file            ();
 
    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

    

  protected:

    void                        set                     ( const Key& key );
    void                        put_record              ( const Const_area& );
    void                        get_record              ( Area& area );
    void                        get_record_key          ( Area& area, const Key& key );
    void                        rewind                  ( Key::Number );
    void                        update                  ( const Const_area& );
    void                        del                     ();

  private:
    Bool                       _set                     ( const Key& key );
    void                        assign_current_key      ( const Const_area& record );

    Fill_zero                  _zero_;
    Any_file                   _file;
    Dynamic_area               _record;                 // Zuletzt gelesener Satz (der nicht der gewünschte sein muss)
    Bool                       _record_is_next;         // _record enthält nächsten Satz für get_record()
    Dynamic_area               _key;                    // Zuletzt erfolgreich gelesener Satz
    Dynamic_area               _previous_key;           // Schlüssel des vorangehenden Satzes oder leer
    Bool                       _need_rewind;
};

//----------------------------------------------------------------------Key_seq_file_type

struct Key_seq_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "key/seq"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Key_seq_file> f = SOS_NEW( Key_seq_file );
        return +f;
    }
};

const Key_seq_file_type        _key_seq_file_type;
const Abs_file_type&            key_seq_file_type = _key_seq_file_type;

// ------------------------------------------------------------------Key_seq_file::Key_seq_file

Key_seq_file::Key_seq_file()
:
    _zero_(this+1)
{
}

//------------------------------------------------------------------Key_seq_file::~Key_seq_file

Key_seq_file::~Key_seq_file()
{
}

//-------------------------------------------------------------------Key_seq_file::prepare_open

void Key_seq_file::prepare_open( const char* parameter, Open_mode open_mode, const File_spec& file_spec )
{
    File_spec  spec = file_spec;
    Sos_string filename;

    for( Sos_option_iterator opt = parameter; !opt.end(); opt.next() )
    {
        if( opt.pipe()                    )   { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    _key_pos = spec._key_specs._key_spec._key_position;
    _key_len = spec._key_specs._key_spec._key_length;
    if( !_key_len )  throw_xc( "SOS-1319" );

    spec._key_specs._key_spec._key_length = 0;
    spec._key_specs._key_spec._field_descr_ptr = NULL;

    _file.prepare_open( filename, open_mode, spec );

    _any_file_ptr->_spec._field_type_ptr = _file.spec()._field_type_ptr;

    current_key_ptr( &_key );
}

//---------------------------------------------------------------------------Key_seq_file::open

void Key_seq_file::open( const char*, Open_mode, const File_spec& file_spec )
{
    _key_pos = file_spec._key_specs._key_spec.key_position();
    //_key_descr = _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = _file.spec()._key_specs._key_spec._field_descr_ptr;

  //File_spec spec = file_spec;
  //spec._key_specs._key_spec._key_length = 0;
  //spec._key_specs._key_spec._field_descr_ptr = NULL;


    _file.open();
}

//--------------------------------------------------------------------------Key_seq_file::close

void Key_seq_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//-------------------------------------------------------------------------Key_seq_file::rewind

void Key_seq_file::rewind( Key::Number )
{
    _record.length( 0 );
    _file.rewind();
    _key.length( 0 );
    _previous_key.length( 0 );
    _need_rewind = false;
    _record_is_next = false;
}

//---------------------------------------------------------------------------Key_seq_file::_set

Bool Key_seq_file::_set( const Key& key )
/*
    Positioniert auf den gewünschten Satz oder den Satz mit dem folgenden Schlüssel.
    Liefert true, wenn der Satz mit dem gewünschten Schlüssel vorhanden ist (für get_key).
*/
{
#if 1  // jz 18.3.97
    _record_is_next = false;

    int cmp = compare( key, _previous_key );

    if( cmp <= 0 ) {
        // Rückwärts lesen
        LOG( "*** key/seq: Schlüssel nicht aufsteigend. set=" << hex << key << ", prev=" << _previous_key << dec << '\n' );
        try { rewind(0); }
        catch( const Xc& ) { throw_xc( "SOS-1316", this ); }

        cmp = +1;
    }

    while(1)
    {
        cmp = compare( key, _key );
        //LOG( hex << key << ( cmp < 0? " < " : cmp > 0? " > " : " = " ) << _key << dec  << '\n' );
        if( cmp <= 0 )  break;          // Satz >= gesuchter Schlüssel gefunden

        _previous_key.assign( _key );

        try { _file.get( &_record ); }
        catch( const Eof_error& ) { return false; }

        assign_current_key( _record );
    }

    _record_is_next = true;

    return cmp == 0;

#else
    Bool first = true;

    while(1) {
        int kl = key.length();
        if( _record.length() < _key_pos )  kl = 0;
        else
        if( _record.length() < _key_pos + _key_len )  kl = _record.length() - _key_pos;

        Const_area current_key ( _record.byte_ptr() + _key_pos, kl );
        int cmp = compare( key, current_key );
        
        if( cmp == 0 ) {
            _key.assign( current_key );
            return true;
        }
        else 
        if( cmp < 0 ) {
            if( !first )  return false;
            if( key > _key )  return false;        // key zeigt hinter den zuletzt gelesenen Satz?
            try { 
                LOG( "key/seq: rewind()\n" );
                _key.length( 0 );
                _file.rewind(); 
            }
            catch( const Xc& ) { throw_xc( "SOS-1316", this ); }
        }

        first = false;
        
        try {
            _file.get( &_record );
        }
        catch( const Eof_error& ) { 
            return false; 
        }
    }
#endif
}

//-------------------------------------------------------------Key_seq_file::assign_current_key

void Key_seq_file::assign_current_key( const Const_area& record ) 
{
    int kl = _key_len;
    if( _record.length() < _key_pos )  kl = 0;
    else
    if( _record.length() < _key_pos + _key_len )  kl = _record.length() - _key_pos;

    _key.assign( record.byte_ptr() + _key_pos, kl );

    //LOG( "key/seq: current_key=" << hex << _key << dec << '\n' );
}

//---------------------------------------------------------------------Key_seq_file::get_record

void Key_seq_file::get_record( Area& buffer )
{
    if( _record_is_next ) {
        _record_is_next = false;
        buffer.assign( _record );
    } else {
        _previous_key.assign( _key );
        _file.get( &_record );
        assign_current_key( _record );
        buffer.assign( _record );
    }

    //LOG( "Key_seq_file::get_record ok\n" );
}

//----------------------------------------------------------------------------Key_seq_file::set

void Key_seq_file::set( const Key& key )
{
    _set( key );
}

//-----------------------------------------------------------------Key_seq_file::get_record_key

void Key_seq_file::get_record_key( Area& buffer, const Key& key )
{
    if( !_set( key ) )  throw_not_found_error( "D311", this );
    
    buffer.assign( _record );

    _record_is_next = false;
}

//-------------------------------------------------------------------------Key_seq_file::update

void Key_seq_file::update( const Const_area& record )
{
    _need_rewind = true;
    _record_is_next = false;
    // wird wohl nicht gehen:
    _file.update( record );
}

//----------------------------------------------------------------------------Key_seq_file::del

void Key_seq_file::del()
{
    _need_rewind = true;
    _record_is_next = false;
    // wird wohl nicht gehen:
    _file.del();
}

//---------------------------------------------------------------------Key_seq_file::put_record
// transparent, nicht mit get_record() mischen!
// Damit der selbe Dateiname zum Lesen und zum Schreiben verwendet werden kann.

void Key_seq_file::put_record( const Const_area& record )
{
    _file.put( record );
}


} //namespace sos

