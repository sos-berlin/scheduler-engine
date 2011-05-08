//#define MODULE_NAME "absfile"
// absfile.cpp
//  2. 1.91                                             (c) Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"
#include <stdlib.h>
#include <string.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
//#include <stdfile.h>
#include "../kram/log.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"
#include "../file/absfile.h"
#include "../file/anyfile2.h"

namespace sos {
using namespace std;

const File_spec  std_file_spec_;
const File_spec& std_file_spec = std_file_spec_;

//----------------------------------------------------File_base::norm_open_mode

File_base::Open_mode File_base::norm_open_mode (File_base::Open_mode open_mode)
{
    if (! (open_mode & File_base::out)) {
        open_mode = (File_base::Open_mode) (open_mode | File_base::nocreate);
        open_mode = (File_base::Open_mode)
                    (open_mode & ~(File_base::app       |
                                   File_base::ate       |
                                   File_base::trunc     |
                                   File_base::noreplace |
                                   File_base::unsafe));
    }
    return open_mode;
}

//-----------------------------------------------------------Key_spec::Key_spec

Key_spec::Key_spec (
        Record_position   key_position,                // 0..
        Record_length     key_length,
        Attributes        key_attributes,
        const Field_descr* field_descr_ptr
) :
    _key_position  (key_position),
    _key_length    (key_length),
    _duplicate  ((key_attributes & Key_spec::ka_duplicate ) != 0),
    _modifiable ((key_attributes & Key_spec::ka_modifiable) != 0),
    _null       ((key_attributes & Key_spec::ka_null      ) != 0),
    _descending ((key_attributes & Key_spec::ka_descending) != 0),
    _segmented  ((key_attributes & Key_spec::ka_segmented ) != 0),
    _field_descr_ptr( (Field_descr*)field_descr_ptr )
{
}

//---------------------------------------------------------Key_specs::Key_specs

Key_specs::Key_specs (
    Record_position key_position,                // 0..
    Record_length   key_length,
    Key_attributes  key_attributes
) :
    _key_count (1)
{
    _key_spec /*[0]*/ = Key_spec (key_position, key_length, key_attributes);
}

//---------------------------------------------------------File_spec::File_spec

File_spec::File_spec ()
{
    *this = File_spec( 2048 );
}

//---------------------------------------------------------File_spec::File_spec

File_spec::File_spec (
        Record_length      max_record_length,
        const Key_specs&   key_specs,
        int                pad,
        int                first_allocation,
        const Record_type*  record_type_ptr
)
:
    _max_record_length (max_record_length),
    _key_specs         (key_specs),
    _pad               (pad),
    _first_allocation  (first_allocation),
    _field_type_ptr    ( (Record_type*)record_type_ptr ),
    _need_type         ( false )
{}

//---------------------------------------------------------File_spec::File_spec

File_spec::File_spec( const File_spec& f )
{
    assign( f );
}

//---------------------------------------------------------File_spec::~File_spec

File_spec::~File_spec ()
{
}

//---------------------------------------------------------File_spec::File_spec

File_spec& File_spec::operator=( const File_spec& f )
{
    assign( f );
    return *this;
}

//------------------------------------------------------------File_spec::assign

void File_spec::assign( const File_spec& f )
{
    _max_record_length  = f._max_record_length;
    _key_specs          = f._key_specs;
    _pad                = f._pad;
    _first_allocation   = f._first_allocation;
    _field_type_ptr     = f._field_type_ptr;
}

//-------------------------------------------------------------------------File_info::File_info

File_info::File_info()
{
}

//------------------------------------------------------------------------File_info::~File_info

File_info::~File_info()
{
}

//------------------------------------------------------------------------------------File_info

File_info::File_info( const File_info& info )
{
    _assign( info );
}

//---------------------------------------------------------------------------File_info::_assign

void File_info::_assign( const File_info& info )
{
    _typename  = info._typename;
    _filename  = info._filename;
    _key_specs = info._key_specs;
    _text      = info._text;
}

//---------------------------------------------------------------------------Abs_file::Abs_file

Abs_file::Abs_file()
:
    _zero_ (this+1),
    _key_pos( -1 )
{
}

//--------------------------------------------------------------------------Abs_file::~Abs_file

Abs_file::~Abs_file()
{
}

//----------------------------------------------------------------------Abs_file::insert_filter
/*
void Abs_file::insert_filter( const char* filename, Open_mode open_mode )
{
    // Dateityp ermitteln und Filter_file (erbt von Abs_file) anlegen.
    // prepare_open() und open() aufrufen, um Schalter auswerten zu können.
    // pipe darf nicht angegeben werden, denn die Datei ist ja schon geöffnet.

    Sos_ptr<Any_file_obj> f = SOS_NEW_PTR( Any_file_obj );
    f->filter_open( filename, open_mode, _any_file_ptr->_handle_ptr );
    _any_file_ptr->_handle_ptr->_file = f;
}
*/
//----------------------------------------------------------------------Abs_file::prepare_open

void Abs_file::prepare_open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    //LOG( "Abs_file::prepare_open(\""<<filename<<"\")\n" );
//jz 24.9.96 key_len wird gebraucht    if( file_spec._field_type_ptr  &&  file_spec.key_specs()._key_spec.field_descr_ptr() )  return;
    //LOG( "  file_spec._field_type_ptr == 0\n" );
    open( filename, open_mode, file_spec );
    _any_file_ptr->_opened = true;                        // s. Any_file::open()
    _any_file_ptr->_prepare_open_not_implemented = true;  // s. Any_file::open()
}

//--------------------------------------------------------------------Abs_file::bind_parameters

void Abs_file::bind_parameters( const Record_type*, const Byte* )
{
    throw_xc( "bind_parameters", this );
}

//--------------------------------------------------Abs_file::open

void Abs_file::open( const char*, Open_mode, const File_spec& )
{
}

//--------------------------------------------------Abs_file::execute

void Abs_file::execute()
{
    throw_xc( "Abs_file::execute", this );
}

//--------------------------------------------------Abs_file::close

void Abs_file::close( Close_mode )
{
}

//-------------------------------------------------------------Abs_file::store

void Abs_file::store( const Const_area& record )
{
    try {
        Dynamic_area get_buffer;
        get_record_key( get_buffer, Const_area( record.byte_ptr() + key_position(), key_length() ) );
        update( record );
    }
    catch( const Not_found_error& )
    {
        insert( record );
    }
    catch( const Record_not_found_error& )      // ??
    {
        insert( record );
    }
}

//--------------------------------------------------Abs_file::get_filename

void Abs_file::get_filename( Area& area )
{
    if( area.size() >= 1 ) {
        area.char_ptr()[ 0 ] = '\0';
        area.length( 1 );
    }
}

//--------------------------------------------------Abs_file::put_record

void Abs_file::put_record( const Const_area& )
{
    throw_xc( "SOS-1372", this );
}

//--------------------------------------------------Abs_file::get_record

void Abs_file::get_record( Area& )
{
    throw_xc( "SOS-1373", this );
}

//------------------------------------------------------Abs_file::get_until

void Abs_file::get_until( Area* buffer, const Const_area& until_key )
{
    get_record( *buffer );
}

//--------------------------------------------------Abs_file::get_position

void Abs_file::get_position( Area* pos, const Const_area* until_key )
{
    Dynamic_area buffer;

    if( until_key )  get_until( &buffer, *until_key );
              else   get_record( buffer );

    pos->assign( current_key() );
}

//--------------------------------------------------Abs_file::get_record_key

void Abs_file::get_record_key( Area&, const Key& )
{
    throw_xc( "SOS-1154", this );
}

//--------------------------------------------------Abs_file::get_record_lock

void Abs_file::get_record_lock( Area& area, Record_lock lock )
{
    if( lock == no_lock ) {
        get_record( area );
    } else {
        throw_xc( "SOS-1155", this );
    }
}

//--------------------------------------------------Abs_file::set

void Abs_file::set( const Key& )
{
    throw_xc( "SOS-1156", this ); 
}

//-----------------------------------------------------Abs_file::rewind

void Abs_file::rewind( Key::Number key_number )
{
    int key_len = key_length();

    if( key_len == 0 )  throw_xc( "SOS-1350" );

    char key [ 2 * file_max_key_length ];       // Für Leasy (Eichenauer) doppelt
    memset( key, 0, key_len );
    set( Key( Const_area( key, key_len ), key_number ));
}

//--------------------------------------------------Abs_file::insert

void Abs_file::insert( const Const_area& )
{
    throw_xc( "SOS-1157", this );
}

//--------------------------------------------------Abs_file::insert_key

void Abs_file::insert_key( const Const_area&, const Key& )
{
    throw_xc( "SOS-1158", this );
}

//--------------------------------------------------Abs_file::store
/*
void Abs_file::store( const Const_area& )
{
    raise( "STORE", "STORE" );
  exception_handler: ;
}
*/
//--------------------------------------------------Abs_file::store_key

void Abs_file::store_key( const Const_area&, const Key& )
{
    throw_xc( "SOS-1159", this );
}

//--------------------------------------------------Abs_file::update

void Abs_file::update( const Const_area& )
{
    throw_xc( "Abs_file::update", this );
    //store( area );
}

//--------------------------------------------------Abs_file::update_direct

void Abs_file::update_direct( const Const_area& )
{
/*
    Dynamic_area buffer;
    get_key( buffer, Const_area( buffer.key_pos, ... ) );
    update( buffer );
*/
    throw_xc( "SOS-1337", this );
}

//--------------------------------------------------Abs_file::del

void Abs_file::del( const Key& )
{
    throw_xc( "SOS-1371", this );
}

//--------------------------------------------------Abs_file::del

void Abs_file::del()
{
    Sos_limited_text<256> key;
    key = current_key();
    LOG( "Abs_file::del " << hex << key << dec << '\n' );
    del( key );
    //throw_xc( "SOS-1160", this );
}

//-----------------------------------------------------------------------Abs_file::current_key

const Const_area& Abs_file::current_key()
{
    if( !_current_key_ptr )  throw_xc( "SOS-1374", this );
    return *_current_key_ptr;
}

//-----------------------------------------------------------------------Abs_file::any_file_ptr

void Abs_file::any_file_ptr( Any_file_obj* )
{
}

//-------------------------------------------------------------------------------Abs_file::info

File_info Abs_file::info()
{
    File_info info;
    info._key_specs._key_spec._key_position = key_position();
    info._key_specs._key_spec._key_length  = key_length();

    return info;
}

//---------------------------------------------------------------------------------------------
/* 22.12.97
Record_position Abs_file::key_position( Key::Number )
{
    return _key_pos;
}

Record_length Abs_file::key_length( Key::Number )
{
    return _key_len;
}

*/
//---------------------------------------------------------------------------Abs_file::new_file

Abs_file* Abs_file::new_file()
{
    return 0;
}

//-------------------------------------------------------------------------Abs_file::_obj_print

void Abs_file::_obj_print( ostream* s ) const
{
    if( _any_file_ptr )  _any_file_ptr->_obj_print( s );
                   else  *s << "Abs_file";
}

//-------------------------------------------------------------Abs_file_type

Abs_file_type* Abs_file_type::_head = NULL;

//-----------------------------------------------------------------------Abs_file_type::_loopup

Abs_file_type* Abs_file_type::_lookup( const char* name ) 
{
    if( !_head ) {
        show_msg( "Abs_file_type::_lookup: Statische Variable benutzt Datei, bevor Dateitypen initialisiert sind. Programmfehler." );
        exit(9999);
    }

    for ( Abs_file_type* p=_head; p; p=p->_next ) 
    {
        const char* type_name = p->name();
        const char* alias_name = p->alias_name();
        if ( strcmp( name,  type_name ) == 0 
         ||  strcmp( name, alias_name ) == 0 ) 
        {
            return p;
        }
    }

    return NULL;
}

//---------------------------------------------------------Abs_file_type::erase

void Abs_file_type::erase( const char* filename )
{
    throw_xc( "SOS-1161", filename );
}

//--------------------------------------------------------Abs_file_type::rename

void Abs_file_type::rename( const char* filename , const char* )
{
    throw_xc( "SOS-1162", filename );
}

//---------------------------------------------------------Abs_file_type::dummy

void Abs_file_type::dummy()
{
}


} //namespace sos
