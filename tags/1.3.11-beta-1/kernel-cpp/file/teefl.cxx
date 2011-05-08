//#define MODULE_NAME "teefl"
//#define AUTHOR      "Jörg Schwiemann"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include "precomp.h"
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../kram/sosstrng.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosarray.h"

namespace sos {

//----------------------------------------------------------------------------------Tee_file

struct Tee_file : Abs_file
{
                                    Tee_file            ();
                                   ~Tee_file            ();
    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            put_record          ( const Const_area& );

  private:

    Fill_zero                      _zero_;
    Sos_simple_array<Any_file>     _tee_files;
    Bool                           _left_to_right;
    Bool                           _ignore_error;
};

//----------------------------------------------------------------------Tee_file_type

struct Tee_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "tee"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Tee_file> f = SOS_NEW_PTR( Tee_file() );
        return +f;
    }
};

const Tee_file_type     _tee_file_type;
const Abs_file_type&     tee_file_type = _tee_file_type;

// --------------------------------------------------------------------Tee_file::Tee_file

Tee_file::Tee_file()
:
    _zero_(this+1),
    _left_to_right( true ),
    _ignore_error( false )
{
    _tee_files.obj_const_name( "Tee_file::_tee_files" );
}

// -------------------------------------------------------------------Tee_file::~Tee_file

Tee_file::~Tee_file()
{
}

// -------------------------------------------------------------------------Tee_file::open

void Tee_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    if( !( open_mode & out ) )  throw_xc( "D127" );

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.flag( "right-to-left" ) )  _left_to_right = !opt.set();
        else
        if( opt.param() || opt.pipe() )
        {
            Any_file* file = _tee_files.add_empty();
            file->obj_owner( this );
            file->open( opt.value(), open_mode, file_spec );
        }
        else throw_sos_option_error( opt );
    }
}

// --------------------------------------------------------------------------Tee_file::close

void Tee_file::close( Close_mode close_mode )
{
    try {
        if( _left_to_right ) {
            for ( int i=_tee_files.first_index(); i <= _tee_files.last_index(); i++ ) {
                _tee_files[i].close( close_mode );
            }
        } else {
            for ( int i=_tee_files.last_index(); i >= _tee_files.first_index(); i-- ) {
                _tee_files[i].close( close_mode );
            }
        }
    }
    catch ( const Xc& ) {
        if ( !_ignore_error ) throw;
    }
}

// ---------------------------------------------------------------------Tee_file::put

void Tee_file::put_record( const Const_area& record )
{
    try {
        if( _left_to_right ) {
            for ( int i=_tee_files.first_index(); i <= _tee_files.last_index(); i++ ) {
                _tee_files[i].put( record );
            }
        } else {
            for ( int i=_tee_files.last_index(); i >= _tee_files.first_index(); i-- ) {
                _tee_files[i].put( record );
            }
        }
    }
    catch ( const Xc& ) {
        if ( !_ignore_error ) throw;
    }
}

} //namespace sos
