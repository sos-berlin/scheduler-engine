//$Id: cachefl.cxx 13579 2008-06-09 08:09:33Z jz $

//#include "../kram/optimize.h"
#include "precomp.h"
#include "../kram/sysdep.h"
#include <limits.h>
#include <time.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include <soslist.h>
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"

#include "../kram/log.h"

/*
    Zwei Anwendungen:

    1)  Basisdatei ist streng sequentiell. Alle Sätze werden gespeichert.

    2)  Basisdatei hat Schlüssel. Alle Satzschlüssel werden gespeichert.

    Wenn -emulate-key angegegeben ist, wird ein Schlüssel emuliert. 
*/

namespace sos {

//-----------------------------------------------------------------------------Cache_key_file_entry

struct Cache_key_file_entry
{
                                    Cache_key_file_entry    () : _younger(-1),_older(-1) {}

  //Bool                           _next_is_in_sequence;
    time_t                         _time_stamp;
  //Oder Big_int für ms. uint4 hält nur 40 Tage
  //Dynamic_area                   _record;             // WIRD BEI CACHE-ERWEITERUNG KOPIERT!!!!!!!
    Const_area_handle              _record;
    int                            _younger;            // oder nächster freier Eintrag
    int                            _older;
};

//typedef Sos_simple_list_node<Cache_key_file_entry> Cache_file_entry_node;

//-----------------------------------------------------------------------------------Cache_key_file

const int very_youngest = 0;     // Dummy-Eintrag für den Allerjüngsten
const int very_oldest   = 1;     // Dummy-Eintrag für den allerältesten
const int very_free     = 2;     // Dummy-Eintrag für den ersten freien
const int cache_min     = 2;     // Erste Index für die Sätze

struct Cache_key_file : Abs_file
{
                                    Cache_key_file();
                                   ~Cache_key_file();

    void                            prepare_open        ( const char*, Open_mode, const File_spec& );
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            insert              ( const Const_area& area );
    void                            update              ( const Const_area& area );
    void                            store               ( const Const_area& area );

    void                            set                 ( const Key& );
    void                            del                 ( const Key& key );
  //void                            del                 ();

  //Record_position                 key_position        (Key::Number ) { return _key_pos; }
  //Record_length                   key_length          (Key::Number ) { return _key_len; }

  protected:
    void                            get_record          ( Area& area );
    void                            get_record_key      ( Area& area, const Key& key );
    void                            put_record          ( const Const_area& area );

  private:
    void                           _cache_update        ( const Const_area_handle& );
    void                           _cache_insert        ( const Const_area_handle& );
    Bool                           _cache_get           ( Area*, const Const_area& key );
    void                           _cache_delete_key    ( const Const_area& key );

    int                            _cache_find_record   ( const Const_area& key );
    int                            _cache_find_key      ( const Const_area& key );
    void                           _cache_change_record ( int, const Const_area_handle& );
    void                           _cache_delete        ( int );

    void                           _lru_insert          ( int index );
    void                           _lru_delete          ( Cache_key_file_entry* );

    Fill_zero                      _zero_;
    Any_file                       _file;
    Dynamic_area                   _key;
    Bool                           _emulate_key;
    Bool                           _sort_key;
    Bool                           _set_key;
    int4                           _max_records;
    int4                           _max_time;
    int                            _file_key_pos;       // == _file.key_position()
    int                            _file_key_len;       // == _file.key_length()
    int                            _get_key_count;
    int                            _get_key_hits;
    Bool                           _file_key_in_record; // == _file.key_in_record()
  //Cache_file_entry_node*         _list;
    Sos_simple_array<Cache_key_file_entry> _cache;
    Sos_simple_array<void*>        _emulated_key_array; // Emulierter Schlüssel (nicht sortiert!)
    Sos_simple_array<void*>        _record_array;       // Satz oder Schlüssel
};

//------------------------------------------------------------------------------Cache_key_file_type

struct Cache_key_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "cache_key"; }    // Umbenannt, jz 12.11.00
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
    	Sos_ptr<Cache_key_file> f = SOS_NEW_PTR( Cache_key_file );
    	return +f;
    }
};

const Cache_key_file_type  _cache_key_file_type;
const Abs_file_type&    cache_key_file_type = _cache_key_file_type;

// --------------------------------------------------------------------- Cache_key_file::Cache_key_file

Cache_key_file::Cache_key_file()
:
    _zero_ ( this+1 )
{
    _cache             .obj_const_name( "Cache_key_file::_cache" );
    _emulated_key_array.obj_const_name( "Cache_key_file::_emulated_key_array" );
    _record_array      .obj_const_name( "Cache_key_file::_record_array" );

    _cache.increment( MIN( 10000, INT_MAX / sizeof _cache[0] ) );
}

//----------------------------------------------------------------------Cache_key_file::~Cache_key_file

Cache_key_file::~Cache_key_file()
{
}

//---------------------------------------------------------------------Cache_key_file::prepare_open

void Cache_key_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    _max_records = 1000;  // Default

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
	    if( opt.flag( "cache-records" ) )       /*_cache_records =*/ opt.set();
	    else
	  //if( opt.flag( "cache-keys" ) )          _cache_keys = opt.set();
	  //else
	    if( opt.flag( "emulate-key" ) )         _emulate_key = opt.set();
	    else
	  //if( opt.with_value( "emulate-sorted-key" ) )  _emulate_key = _sort_key = opt.set();
	  //else
	    if( opt.with_value( "records" ) )       _max_records = as_int4( c_str( opt.value() ) );
	    else
	    if( opt.with_value( "time" ) )          _max_time = as_int4( c_str( opt.value() ) );
	    else
	    if( opt.pipe() )                      { filename = opt.rest(); break; }
	    else throw_sos_option_error( opt );
    }

    _file.prepare_open( filename, open_mode, file_spec );

    _file_key_pos       = _file.key_position();
    _file_key_len       = _file.key_length();
    _file_key_in_record = _file.key_in_record();

    if( !_emulate_key ) {
        _key_pos = _file_key_pos;
        _key_len = _file_key_len;
    }

    _any_file_ptr->_spec._field_type_ptr = _file.spec()._field_type_ptr;
    _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = _file.spec()._key_specs._key_spec._field_descr_ptr;

    if( _max_records < _cache.increment() )  _cache.size( cache_min + _max_records );

    _cache.last_index( 2 );
    _cache[ very_youngest ]._older   = very_oldest;
    _cache[ very_oldest   ]._younger = very_youngest;
    _cache[ very_free     ]._younger = -1;        // Verkettung der freien Einträge
}

//-----------------------------------------------------------------------------Cache_key_file::open

void Cache_key_file::open( const char*, Open_mode, const File_spec& file_spec )
{
    if( _emulate_key ) {
        if( file_spec.key_specs().key_length() ) {
            _key_len = file_spec.key_specs().key_length();
            _key_pos = _key_len? file_spec.key_specs().key_position() : (uint)-1;
        } else {
            Field_descr* f = file_spec.key_specs()._key_spec._field_descr_ptr;
            if( f ) {
                _key_len = f->type().field_size();
                _key_pos = f->offset();
            }
        }
    }
     
    _file.open();
}

// -------------------------------------------------------------------------- Cache_key_file::close

void Cache_key_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
    LOG( "Cache_key_file: " << ( _cache.count() - 3 ) << " Einträge, " << _get_key_count << " get_keys, " << _get_key_hits << " Treffer\n ");
}

// --------------------------------------------------------------------- Cache_key_file::put_record

void Cache_key_file::put_record( const Const_area& record )
{
    _file.put( record );
}

// --------------------------------------------------------------------- Cache_key_file::get_record

void Cache_key_file::get_record( Area& area )
{
    /* Damit der Cache auf fürs sequentielle Lesen verwendet werden kann, muss er
       sich die Reihenfolge, Beginn und Ende der Sätze eines bereits gelesenen
       Abschnitts merken.
    */

    if( _set_key ) {
        _file.set( _key );
        _set_key = false;
    }

    _file.get( &area );
    _cache_update( area );
}

// ------------------------------------------------------------------Cache_key_file::get_record_key

void Cache_key_file::get_record_key( Area& area, const Key& key )
{
    _get_key_count++;
    Bool ok = _cache_get( &area, key );
    if( ok ) {
        _key = key;
        incr( &_key );
        _set_key = true;
        _get_key_hits++;
    } else {
        _file.get_key( &area, key );
        _cache_insert( area );
    }
}

// ------------------------------------------------------------------------- Cache_key_file::insert

void Cache_key_file::insert( const Const_area& record )
{
    _file.insert( record );
    _cache_update( record );
}

// --------------------------------------------------------------------------Cache_key_file::update

void Cache_key_file::update( const Const_area& record )
{
    _file.update( record );
    _cache_update( record );
}

// ---------------------------------------------------------------------------Cache_key_file::store

void Cache_key_file::store( const Const_area& record )
{
    _file.store( record );
    _cache_update( record );
}

// ---------------------------------------------------------------------------- Cache_key_file::set

void Cache_key_file::set( const Key& key )
{
    _set_key = false;
    _file.set( key );
}

// ---------------------------------------------------------------------------- Cache_key_file::del

void Cache_key_file::del( const Key& key )
{
    _cache_delete_key( key );
    _file.set( key );
}

//------------------------------------------------------------------------------Cache_key_file::del
/*
void Cache_key_file::del()
{
    _file.del();
}
*/
// ------------------------------------------------------------------------- Cache_key_file::commit
/*
void Cache_key_file::commit()
{
    _file.commit();
}

//------------------------------------------------------------------------ Cache_key_file::rollback

void Cache_key_file::rollback()
{
    _file.rollback();
}
*/

//---------------------------------------------------------------------Cache_key_file::_cache_update

void Cache_key_file::_cache_update( const Const_area_handle& record )
{
    if( !_key_len || _key_pos < 0 )  return;
    if( record.length() < _key_pos + _key_len )  return;

    int index = _cache_find_record( Const_area( record.byte_ptr() + _key_pos, _key_len ) );
    if( index < 0 ) {
        _cache_insert( record );
    } else {
        _cache_change_record( index, record );
    }
}

//----------------------------------------------------------------Cache_key_file::_cache_delete_key

void Cache_key_file::_cache_delete_key( const Const_area& key )
{
    int index = _cache_find_record( key );
    if( index >= 0 ) {
        _cache_delete( index );
    }
}

//-----------------------------------------------------------------------Cache_key_file::_cache_get

Bool Cache_key_file::_cache_get( Area* buffer, const Const_area& key )
{
    int index = _cache_find_record( key );
    if( index < 0 )  return false;

    buffer->assign( _cache[ index ]._record );
    return true;
}

//---------------------------------------------------------------Cache_key_file::_cache_find_record

int Cache_key_file::_cache_find_record( const Const_area& key )
{
    if( key.length() != _key_len )  throw_xc( "cache: invalid keylength", key.length(), _key_len );

    int               i = very_youngest;
    Cache_key_file_entry* e = &_cache[ i ];

    //for( i = _cache.first_index(); i <= _cache.last_index(); i++ ) {
    while(1) {
        i = e->_older;
        if( i == very_oldest )  break;
        e = &_cache[ i ];
        Const_area* c = &e->_record;
        if( c->length() && memcmp( c->byte_ptr() + _key_pos, key.ptr(), _key_len ) == 0 )  goto FOUND;
    }
    return -1;

  FOUND:
    /*if( time(0) - e->_time_stamp < _max_time )*/  return i;

    //_cache_delete( i );
    return -1;
}

//--------------------------------------------------------------------Cache_key_file::_cache_insert

void Cache_key_file::_cache_insert( const Const_area_handle& record )
{
    int index = _cache[ very_free ]._younger;
    if( index == -1 ) {   // Kein freier Eintrag?
        if( _cache.last_index() < cache_min + _max_records )  {
            // Array erweitern:
            _cache.add_empty();
            index = _cache.last_index();
        } else {
            // Den Ältesten nehmen:
            index = _cache[ very_oldest ]._younger;
            _lru_delete( &_cache[ index ] );
        }
    }

    Cache_key_file_entry* e = &_cache[ index ];
    e->_record = record;

    _lru_insert( index );
}

//-------------------------------------------------------------Cache_key_file::_cache_change_record

void Cache_key_file::_cache_change_record( int index, const Const_area_handle& record )
{
    Cache_key_file_entry* e = &_cache[ index ];
    e->_record = record;
    _lru_delete( e );
    _lru_insert( index );
}

//---------------------------------------------------------------------Cache_key_file::cache_delete

void Cache_key_file::_cache_delete( int index )
{
    Cache_key_file_entry* e = &_cache[ index ];
    e->_record = ""; //??     e->_record.free();
    _lru_delete( e );

    // In die Liste der freien Einträge: 
    Cache_key_file_entry* v = &_cache[ very_free ];
    e->_younger = v->_younger;
    v->_younger = index;
}

//-----------------------------------------------------------------------Cache_key_file::lru_insert

void Cache_key_file::_lru_insert( int index )
{
    Cache_key_file_entry* e = &_cache[ index ];
    Cache_key_file_entry* v = &_cache[ very_youngest ];

    e->_time_stamp = time(0);

    e->_younger = very_youngest;
    e->_older = v->_older;
    _cache[ v->_older ]._younger = index;
    v->_older = index;
}

//-----------------------------------------------------------------------Cache_key_file::lru_delete

void Cache_key_file::_lru_delete( Cache_key_file_entry* e )
{
    if( e->_younger != -1 )  _cache[ e->_younger ]._older   = e->_older;
    if( e->_older   != -1 )  _cache[ e->_older   ]._younger = e->_younger;
}

} //namespace sos
