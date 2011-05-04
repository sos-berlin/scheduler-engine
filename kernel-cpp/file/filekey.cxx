#include "precomp.h"
//#define MODULE_NAME "objfile"
//#define COPYRIGHT   "© 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


#include "../kram/sysdep.h"
//#include <except.h>         // Borland spezial

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/xception.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"
#include "../file/filekey.h"

namespace sos {

//------------------------------------------------------------------------------------------statics

struct File_as_key_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "file_as_key"; }
  //virtual const char*         alias_name              () const { return ""; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<File_as_key_file> f = SOS_NEW_PTR( File_as_key_file );
        return +f;
    }
};

const File_as_key_file_type  _file_as_key_file_type;
const Abs_file_type&          file_as_key_file_type = _file_as_key_file_type;

//----------------------------------------------------------------------------File_as_key_file::open
/*
void File_as_key_file::open( const char* filename, Open_mode open_mode, const File_spec& file_spec )
{
}
*/
//---------------------------------------------------------------------------File_as_key_file::close
/*
void File_as_key_file::close( Close_mode close_mode )
{
}
*/
//------------------------------------------------------------------File_as_key_file::get_record_key

void File_as_key_file::get_record_key( Area& buffer, const Key& key )
{
    ASSERT_VIRTUAL( get_record_key )

    Any_file   file;
    Sos_string filename = as_string( key );

    file.open( c_str( filename ), File_base::Open_mode( File_base::in | File_base::seq ) );

    buffer.allocate_min( 32760 );
    Byte* p     = buffer.byte_ptr();
    Byte* p_end = p + buffer.size() - 2 /*für \r\n*/;

    //try {
        while(1) {
            Area rest ( p, p_end - p );

            try {
                file.get( &rest );
            }
            catch( const Eof_error& )      { break; }
            catch( const Too_long_error& ) { break; }

            p += rest.length();

            for( int i = 0; i < rest.length(); i++ )  if( rest.char_ptr()[ i ] != ' ' )  break;
            //jz 7.4.97 if( i == rest.length() ) {   // Leerzeile?
                if( p + 2 > p_end )  throw_too_long_error();
#               if defined SYSTEM_WIN
                    *p++ = '\r';
#               endif
                *p++ = '\n';
                continue;
            //jz 7.4.97 } else {
            //jz 7.4.97     *p++ = ' ';
            //jz 7.4.97 }
        }

        buffer.length( p - buffer.byte_ptr() );

/* Folgender Code wirkt bei Borland nicht (ohne oben stehende catchs):
    }
    catch( const Eof_error& x )
    {
        buffer.length( p - buffer.byte_ptr() );
    }
    catch( ... )
    {
        int BORLAND_SPEZIAL_EXCEPTION;  // ?? jz 29.8.95
        if( strcmp( __throwExceptionName, "D310" ) != 0 )  throw;
        buffer.length( p - buffer.byte_ptr() );
    }
*/
    file.close();
}

//----------------------------------------------------------------------File_as_key_file::put_record
/*
void File_as_key_file::put_record( const Const_area& record )
{

}
*/


} //namespace sos
