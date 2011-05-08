//#define MODULE_NAME "updatefl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"

namespace sos {
using namespace std;


//----------------------------------------------------------------------------------Update_file

struct Update_file : Abs_file
{
                                    Update_file         ();
                                   ~Update_file         ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            print_field_names   ( ostream*, const Field_type&, const char* = "" );

    Fill_zero                      _zero_;
    Any_file                       _file;
};

//----------------------------------------------------------------------Update_file_type

struct Update_file_type : Abs_file_type
{
                                Update_file_type        () : Abs_file_type() {}
    virtual const char*         name                    () const { return "update"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Update_file> f = SOS_NEW_PTR( Update_file() );
        return +f;
    }
};

const Update_file_type  _update_file_type;
const Abs_file_type&     update_file_type = _update_file_type;

// --------------------------------------------------------------------Update_file::Update_file

Update_file::Update_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Update_file::~Update_file

Update_file::~Update_file()
{
}

// -------------------------------------------------------------------------Update_file::open

void Update_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    _file.obj_owner( this );

    Sos_string filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.pipe() )                               filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    if( !( open_mode & out ) )  throw_xc( "D126" );
    _file.open( filename, Open_mode( open_mode | inout ), file_spec );
}

// --------------------------------------------------------------------------Update_file::close

void Update_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// ---------------------------------------------------------------------Update_file::put

void Update_file::put_record( const Const_area& record )
{
    _file.update( record );
}

} //namespace sos

