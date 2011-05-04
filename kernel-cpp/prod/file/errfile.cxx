// errfile.cxx                                ©1997 SOS GmbH Berlin
//                                             Joacim Zschimmer

#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/sosfield.h"
#include "../kram/soslimtx.h"


namespace sos {   

//-----------------------------------------------------------------------------------Error_file

struct Error_file : Abs_file
{
                                Error_file              ();
                               ~Error_file              ();

    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
  //void                        close                   ( Close_mode );

 protected:
    void                        get_record              ( Area& area );

    Fill_zero                  _zero_;
    Any_file                   _file;
};


//-----------------------------------------------------------------------------Error_file_type

struct Error_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "error"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Error_file> f = SOS_NEW( Error_file );
        return +f;
    }
};

const Error_file_type          _error_file_type;
const Abs_file_type&            error_file_type = _error_file_type;

//-------------------------------------------------------------------Error_file::Error_file

Error_file::Error_file()
:
    _zero_ ( this+1 )
{
}

//------------------------------------------------------------------Error_file::~Error_file

Error_file::~Error_file()
{
}

//-----------------------------------------------------------------Error_file::prepare_open

void Error_file::prepare_open( const char* parameter, Open_mode open_mode, const File_spec& spec )
{
    Sos_string filename;

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if( opt.param() || opt.pipe() )  filename = opt.rest();
        else throw_sos_option_error( opt );
    }

    if( open_mode & out )  throw_xc( "D127" );

    _file.prepare( filename, open_mode, spec );
}

//---------------------------------------------------------------------------Error_file::open

void Error_file::open( const char*, Open_mode, const File_spec& )
{
    _file.open();
}

//---------------------------------------------------------------------Error_file::get_record

void Error_file::get_record( Area& buffer )
{
    Dynamic_area key;
    
    _file.get( &key );

    try {
        key.append( '\0' );
        char* p = strchr( key.char_ptr(), ' ' );
        if( p )  *p = '\0';  // Danach kommen Parameter, die ausgewertet werden sollten!
        throw_xc( key.char_ptr() );
    } 
    catch( const Xc& x )
    {
        x.get_text( &buffer );
        if( buffer.length() > 0  &&  buffer.char_ptr()[ buffer.length() - 1 ] == '\0' )  buffer.length( buffer.length() - 1 );
    }
}


} //namespace
