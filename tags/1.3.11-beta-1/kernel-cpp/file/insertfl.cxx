//#define MODULE_NAME "insertfl"
//#define AUTHOR      "Joacim Zschimmer"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"


namespace sos {
using namespace std;


//----------------------------------------------------------------------------------Insert_file

struct Insert_file : Abs_file
{
                                    Insert_file         ();
                                   ~Insert_file         ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:
    void                            print_field_names   ( ostream*, const Field_type&, const char* = "" );

    Fill_zero                      _zero_;
    Any_file                       _file;
};

//----------------------------------------------------------------------Insert_file_type

struct Insert_file_type : Abs_file_type
{
                                Insert_file_type        () : Abs_file_type() {}
    virtual const char*         name                    () const { return "insert"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Insert_file> f = SOS_NEW_PTR( Insert_file() );
        return +f;
    }
};

const Insert_file_type  _insert_file_type;
const Abs_file_type&     insert_file_type = _insert_file_type;

// --------------------------------------------------------------------Insert_file::Insert_file

Insert_file::Insert_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------Insert_file::~Insert_file

Insert_file::~Insert_file()
{
}

// -------------------------------------------------------------------------Insert_file::open

void Insert_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if ( opt.pipe() )                               filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    _file.obj_owner( this );

    if( !( open_mode & out ) )  throw_xc( "D126" );
    _file.open( filename, Open_mode( open_mode | inout ), file_spec );
}

// --------------------------------------------------------------------------Insert_file::close

void Insert_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

// ---------------------------------------------------------------------Insert_file::put

void Insert_file::put_record( const Const_area& record )
{
    _file.insert( record );
}


} //namespace sos
