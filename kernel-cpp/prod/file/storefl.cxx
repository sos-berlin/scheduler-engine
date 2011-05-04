//#define MODULE_NAME "storefl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"

#include "../kram/log.h"

namespace sos {
using namespace std;


//----------------------------------------------------------------------------------Store_file

struct Store_file : Abs_file
{
                                    Store_file         ();
                                   ~Store_file         ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            print_field_names   ( ostream*, const Field_type&, const char* = "" );

    Fill_zero                      _zero_;
    Any_file                       _file;
};

//----------------------------------------------------------------------Store_file_type

struct Store_file_type : Abs_file_type
{
                                Store_file_type         () : Abs_file_type() {}
    virtual const char*         name                    () const { return "store"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Store_file> f = SOS_NEW_PTR( Store_file() );
        return +f;
    }
};

const Store_file_type  _store_file_type;
const Abs_file_type&    store_file_type = _store_file_type;

// --------------------------------------------------------------------Store_file::Store_file

Store_file::Store_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Store_file::~Store_file

Store_file::~Store_file()
{
}

// -------------------------------------------------------------------------Store_file::open

void Store_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
    _file.obj_owner( this );
    if( !( open_mode & out ) )  throw_xc( "D126" );
    _file.open( filename, Open_mode( open_mode | inout ), file_spec );
}

// --------------------------------------------------------------------------Store_file::close

void Store_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// ---------------------------------------------------------------------Store_file::put

void Store_file::put_record( const Const_area& record )
{
    _file.store( record );
}

} //namespace sos

