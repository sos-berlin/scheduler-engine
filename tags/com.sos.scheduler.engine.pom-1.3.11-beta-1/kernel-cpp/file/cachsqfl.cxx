//#define MODULE_NAME "cachsqfl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

//#include "../kram/optimize.h"
#include "precomp.h"
#include "../kram/sysdep.h"
#include <limits.h>
#include <time.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include <sosdate.h> // wg. std_date_format_ingres
//#include <soslist.h>
#include "../kram/sosfield.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"

#include "../kram/log.h"

/*
    Basisdatei ist streng sequentiell. Alle Sätze werden gespeichert.
    rewind() wird im Cache durchgeführt.
    Keine Alterung.
*/

namespace sos {

struct Cache_seq_file : Abs_file
{
                                    Cache_seq_file      ();
                                   ~Cache_seq_file      ();

    void                            prepare_open        ( const char*, Open_mode, const File_spec& );
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

    void                            rewind              ( Key::Number );
  //Record_position                 key_position        ( Key::Number ) { return _file.key_position(); }
  //Record_length                   key_length          ( Key::Number ) { return _file.key_length(); }

  protected:
    void                            get_record          ( Area& area );

  private:
    Fill_zero                      _zero_;
    Any_file                       _file;
    int                            _next_record_no;
    int                            _rewind_count;
    Sos_simple_array<Byte*>        _record_array;
    Bool                           _read_all_and_close;
    int                            _last_record_no;
};

//------------------------------------------------------------------------------Cache_seq_file_type

struct Cache_seq_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "cache_seq"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Cache_seq_file> f = SOS_NEW_PTR( Cache_seq_file );
        return +f;
    }
};

const Cache_seq_file_type  _cache_seq_file_type;
const Abs_file_type&        cache_seq_file_type = _cache_seq_file_type;


DEFINE_SOS_DELETE_ARRAY( Byte* )  // Gehört nach sosarray.h

// --------------------------------------------------------------------- Cache_seq_file::Cache_seq_file

Cache_seq_file::Cache_seq_file()
:
    _zero_ ( this+1 ),
    _last_record_no ( INT_MAX )
{
    _record_array.obj_const_name( "cache_seq._record_array" );
    _record_array.increment( MIN( 10000, INT_MAX / sizeof _record_array[0] ) );
}

//----------------------------------------------------------------------Cache_seq_file::~Cache_seq_file

Cache_seq_file::~Cache_seq_file()
{
    for( int i = _record_array.last_index(); i >= 0; i-- )
    {
        Byte*& p = _record_array[ i ];
        sos_free( p );
        p = NULL;
    }
}

//---------------------------------------------------------------------Cache_seq_file::prepare_open

void Cache_seq_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt( param ); !opt.end(); opt.next() )
    {
        if( opt.flag( "read-all-and-close" ) )  _read_all_and_close = opt.set();
/*
        else
        if( opt.flag( "-file" ) ) {
#           ifdef SYSTEM_WIN
                char temp_filename [ 144+1 ];
                GetTempFileName( 0, "SOS", 0, temp_filename );
                _read_all_and_close = opt.set();
#            else
                throw_xc( "cache_seq -file", "Nur in Windows implementiert" );
#           endif
        }
*/
        else
        if( opt.pipe() )                      { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    _file.prepare_open( filename, Open_mode( open_mode | seq ), file_spec );

    _any_file_ptr->_spec._field_type_ptr = _file.spec()._field_type_ptr;
    _any_file_ptr->_spec._key_specs._key_spec._field_descr_ptr = _file.spec()._key_specs._key_spec._field_descr_ptr;

    _key_pos = _file.key_position();
    _key_len = _file.key_length();
}

//-----------------------------------------------------------------------------Cache_seq_file::open

void Cache_seq_file::open( const char*, Open_mode, const File_spec& )
{
    _file.open();

    if( _read_all_and_close )
    {
        Dynamic_area buffer;

        try {
            while(1) { get_record( buffer ); }
        }
        catch( const Eof_error& ) {}

        _file.close();
        rewind(0);
        LOG( "Cache_seq_file: Datei wieder geschlossen\n" );
    }
}

// -------------------------------------------------------------------------- Cache_seq_file::close

void Cache_seq_file::close( Close_mode close_mode )
{
    LOG( "Cache_seq_file: " << _record_array.count() << " Sätze, " << _rewind_count << " rewinds\n ");

    _file.close( close_mode );
}

// --------------------------------------------------------------------- Cache_seq_file::rewind

void Cache_seq_file::rewind( Key::Number )
{
    _next_record_no = 0;
    _rewind_count++;        // Statistik
}

// ------------------------------------------------------------------Cache_seq_file::get_record

void Cache_seq_file::get_record( Area& area )
{
    if( _next_record_no <= _record_array.last_index() )
    {
        Byte* p = _record_array[ _next_record_no ];
        area.assign( p + sizeof (int), *(int*)p );
    }
    else
    if( _next_record_no > _last_record_no ) {
        throw_eof_error();
    }
    else
    {
        try {
            _file.get( &area );
        }
        catch( const Eof_error& ) {
            _last_record_no = _record_array.last_index();
            throw;
        }

        _record_array.add( (Byte*)NULL );

        Byte* p = (Byte*)sos_alloc( sizeof (int) + area.length(), "Cache_seq_file" );

        *(int*)p = area.length();
        memcpy( p + sizeof (int), area.ptr(), area.length() );

        _record_array[ _record_array.last_index() ] = p;
    }

    _next_record_no++;
}


} //namespace sos
