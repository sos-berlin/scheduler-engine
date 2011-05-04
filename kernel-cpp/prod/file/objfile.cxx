//#define MODULE_NAME "objfile"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/xception.h"
#include "../file/absfile.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfact.h"
#include "../kram/sosdumcl.h"
#include "../file/fileobj.h"
#include "../file/objfile.h"


namespace sos {

//------------------------------------------------------------------------------------------statics

struct Object_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "object"; }
  //virtual const char*         alias_name              () const { return ""; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Object_file> f = SOS_NEW_PTR( Object_file );
        return +f;
    }
};

const Object_file_type  _object_file_type;
const Abs_file_type&     object_file_type = _object_file_type;

//----------------------------------------------------------------------------Object_file::open

void Object_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    _key_len = file_spec.key_length();
    _key_pos = file_spec.key_position();

    //if( _key_length == 0 )  _key_length = 4;   int KEY_LENGTH_RICHTIG_SETZEN;

    _object_ptr = sos_factory_ptr()->create( filename, this/*, &file_spec*/ );
    //_object_ptr->obj_owner( this );  zu spät!
    current_key_ptr( &_current_key );
}

//---------------------------------------------------------------------------Object_file::close

void Object_file::close( Close_mode close_mode )
{
    _object_ptr->obj_end( close_mode );
}

//----------------------------------------------------------------------Object_file::get_record

void Object_file::get_record( Area& area )
{
    Const_area_handle record;

    record = _object_ptr->obj_get( &_current_key );
    area.assign( record );

    //int SATZPOSITION_IN_DATA_REPLY_MSG_UEBERGEBEN;
}

//------------------------------------------------------------------Object_file::get_record_key

void Object_file::get_record_key( Area& area, const Key& key )
{
    ASSERT_VIRTUAL( get_record_key )

    Sos_dummy_client dummy_client;
    Get_direct_msg   m             ( _object_ptr, &dummy_client, key );

    Const_area_handle record;
    record = dummy_client.send_and_await_data( &m );
    area.assign( record );
}

//-----------------------------------------------------------------------------Object_file::set

void Object_file::set( const Key& key )
{
    ASSERT_VIRTUAL( set )

    Dynamic_area k;
    k.assign( key );
    _current_key = k;
}

//----------------------------------------------------------------------Object_file::put_record

void Object_file::put_record( const Const_area& record )
{
    _object_ptr->obj_put( record );
}



} //namespace sos
