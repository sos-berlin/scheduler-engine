//#define MODULE_NAME "deletefl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"

namespace sos {
using namespace std;

//----------------------------------------------------------------------------------Delete_file

struct Delete_file : Abs_file
{
                                    Delete_file         ();
                                   ~Delete_file         ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            print_field_names   ( ostream*, const Field_type&, const char* = "" );

    Fill_zero                      _zero_;
    Any_file                       _file;
};

//----------------------------------------------------------------------Delete_file_type

struct Delete_file_type : Abs_file_type
{
                                Delete_file_type         () : Abs_file_type() {}
    virtual const char*         name                    () const { return "delete"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Delete_file> f = SOS_NEW_PTR( Delete_file() );
        return +f;
    }
};

const Delete_file_type  _delete_file_type;
const Abs_file_type&    delete_file_type = _delete_file_type;

// --------------------------------------------------------------------Delete_file::Delete_file

Delete_file::Delete_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Delete_file::~Delete_file

Delete_file::~Delete_file()
{
}

// -------------------------------------------------------------------------Delete_file::open

void Delete_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.pipe() )                               filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _file.obj_owner( this );

    if( !( open_mode & out ) )  throw_xc( "D127" );
    _file.open( filename, Open_mode( open_mode | inout ), file_spec );
}

// --------------------------------------------------------------------------Delete_file::close

void Delete_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// ---------------------------------------------------------------------Delete_file::put

void Delete_file::put_record( const Const_area& record )
{
    _file.del( record );  // record ist der key
}

} //namespace sos
