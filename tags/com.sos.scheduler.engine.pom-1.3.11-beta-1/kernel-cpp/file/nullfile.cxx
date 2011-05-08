//#define MODULE_NAME "nullfile"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1997 SOS GmbH Berlin"


#include "precomp.h"

#include "../kram/sos.h"
#include "../file/absfile.h"

namespace sos {

//---------------------------------------------------------------------------Null_file

struct Null_file : Abs_file
{
    void                        open                    ( const char*, Open_mode, const File_spec& );
  //void                        close                   ( Close_mode );

    void                        update                  ( const Const_area& )       {}
    void                        update_direct           ( const Const_area& )       {}
    void                        insert                  ( const Const_area& )       {}
    void                        store                   ( const Const_area& )       {}

    void                        get_record_key          ( Area&, const Key& );
    void                        del                     ()                          {}
    void                        del_key                 ( const Key& )              {}
    void                        set                     ( const Key& )              {}
    void                        rewind                  ( Key::Number )             {}

  protected:
    void                        get_record              ( Area& );                   
    void                        put_record              ( const Const_area& )       {}
};

//----------------------------------------------------------------------Null_file_type

struct Null_file_type : Abs_file_type
{
    virtual const char*         name                    () const  { return "null"; }

    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Null_file> f = SOS_NEW_PTR( Null_file() );
        return +f;
    }
};

const Null_file_type           _null_file_type;
const Abs_file_type&            null_file_type = _null_file_type;

//---------------------------------------------------------------------Null_file::open

void Null_file::open( const char* parameter, Open_mode open_mode, const File_spec& file_spec )
{
    // Parameter werden ignoriert

    if( file_spec.key_specs().key_length() ) 
    {
        _key_len = file_spec.key_specs().key_length();
        _key_pos = file_spec.key_specs().key_position();
    }
}

//---------------------------------------------------------------Null_file::get_record

void Null_file::get_record( Area& buffer )
{
    throw_eof_error();
}

//-----------------------------------------------------------Null_file::get_record_key

void Null_file::get_record_key( Area& buffer, const Key& key )
{
    throw_not_found_error();
}

//--------------------------------------------------------------Null_file::put_record

} //namespace sos
