//#define MODULE_NAME "alias"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Jörg Schwiemann"

#include "precomp.h"

#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../kram/sysxcept.h"
#include "../kram/sosfield.h"
#include "../file/absfile.h"
#include "../file/anyfile.h"
#include "../file/anyfile2.h"
#include "../kram/sosopt.h"
#include "../kram/sosprof.h"
#include "../kram/sosctype.h"
#include "../kram/log.h"
#include "../kram/thread_semaphore.h"

#include <errno.h>
#include <fcntl.h>                  // O_RDONLY

#if defined __BORLANDC__  ||  defined _MSC_VER
#    include <io.h>       // open(), read() etc.
#else
#    include <stdio.h>    // fileno
#    include <unistd.h>   // read(), write(), close()
#endif


namespace sos {


extern Sos_string module_filename();
extern Sos_string directory_of_path( const Sos_string& );


struct Alias_file_param
{
                                Alias_file_param        () : _used( false ) {}

    Sos_string                 _name;
    Sos_string                 _value;
    Bool                       _used;
};

// ----------------------------------------------------------------------------- Alias_file

struct Alias_file : Abs_file
{
    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        resolve                 ( const Sos_string& alias, Sos_string* );

 private:
    Any_file                   _file;

    static string              _path;
    static bool                _path_read;
};

//-------------------------------------------------------------------------------------------static

string                          Alias_file::_path;
bool                            Alias_file::_path_read = false;

//-------------------------------------------------------------------------------------------------

struct Alias_file_type : Abs_file_type
{
    virtual const char* name      () const { return "alias"; }

    virtual Sos_ptr<Abs_file> create_base_file   () const
    {
        Sos_ptr<Alias_file> f = SOS_NEW_PTR( Alias_file() );
        return +f;
    }
};

const Alias_file_type  _alias_file_type;
const Abs_file_type&    alias_file_type = _alias_file_type;

//---------------------------------------------------------------------Alias_file::prepare_open

void Alias_file::prepare_open( const char* parameter, Open_mode open_mode, const File_spec& file_spec )
{
    //LOG( "Alias_file::prepare_open: alias=" << filename << "\n" );

    Sos_string          ini_file;           // Default: "", also sos.ini
    Sos_string          section = "alias";
    Sos_string          filename;
    Sos_option_iterator opt = parameter;
    Bool                alias_ok = false;
    Sos_simple_array<Alias_file_param> params;

    params.obj_const_name( "Alias_file.params" );

    for(; !opt.end() && !alias_ok; opt.next() )
    {
        if( opt.with_value( "ini" ) )      { ini_file = opt.value(); }
        else
        if( opt.with_value( "section" ) )      { section = opt.value(); }
        else
        if( opt.option()[ 0 ] == '$'  &&  opt.with_value( opt.option() ) )   // -$parameter=wert
        {
            Alias_file_param* p = params.add_empty();
            p->_name  = opt.option();
            p->_value = opt.value();
        }
        else
        if( opt.with_value( "file" ) )         // Der Dateiname steht in einer Datei
        {
            Sos_string   name;
            Dynamic_area fn   ( 1024 );  fn.length( 0 );

            if( !_path_read ) 
            {
                THREAD_LOCK( hostware_mutex )
                {
                    _path_read = true;
                    _path = read_profile_string( "", c_str( section ), "_path" );
                    if( length( _path )
                     &&  _path[ (int)length( _path ) - 1 ] != '/'
                     &&  _path[ (int)length( _path ) - 1 ] != '\\' )  _path += "/";
                }
            }

            name = _path;
            name += opt.value();

            int f = ::open( c_str( name ), O_RDONLY | O_BINARY );
            if( f == EOF )  throw_errno( errno, c_str( opt.value() ), this );

            try {
                while(1) {
                    int read_size = fn.size() - fn.length();
                    int len = ::read( f, fn.char_ptr() + fn.length(), read_size );
                    if( len == EOF )  throw_errno( errno, "read", this );
                    fn.length( fn.length() + len );
                    if( len < read_size ) break;
                    fn.resize_min( fn.size() * 2 );
                }
                ::close( f );
            }
            catch( const Xc&        ) { ::close( f ); throw; }
            catch( const exception& ) { ::close( f ); throw; }

            filename = as_string( fn.char_ptr(), fn.length() );
            alias_ok = true;
        }
        else
        if( opt.param( 1 ) ) {          // Der Dateiname steht im Abschnitt [alias]
            filename = read_profile_string( c_str( ini_file ), c_str( section ), c_str( opt.value() ) );
            if( empty( filename ) )  {
                if( opt.value() == "soserror.txt" ) {       // Besonderer Name für xception.cxx
                    filename = directory_of_path( module_filename() );
                    if( length( filename ) )  filename += "/";
                    filename += "soserror.txt";
                }
                else throw_not_exist_error( "SOS-1163", c_str( opt.value() ) );
            }
            alias_ok = true;
        }
        else throw_sos_option_error( opt );
    }


    if( params.count() )           //// Parametersubstitution
    {
        Sos_string orig = filename;
        Sos_string var;

        filename = "";

        // Der Erkennung der Variablen kann gerne verbessert werden.
        // Z.B. könnte das Verfahren von sh, bash oder perl übernommen werden.

        const char* p = c_str( orig );
        while( *p ) {
            if( *p == '$' )
            {
                if( p[1] == '$' )  {   // $$VAR ==> $VAR (keine Variable, sondern Text)
                    p++;
                    filename += *p++;
                } else {
                    Alias_file_param* par = NULL;
                    int               i;

                    var = "$"; p++;
                    if( *p == '\''  ||  *p == '"' ) {
                        char quote = *p++;
                        while( *p == '$'  || sos_isalnum( *p ) )  var += *p++;
                        if( *p != quote )  throw_xc( "SOS-1321" );
                        p++;
                    } else {
                        while( *p == '$'  || sos_isalnum( *p ) )  var += *p++;
                    }

                    for( i = 0; i <= params.last_index(); i++ ) {
                        par = &params[ i ];
                        if( par->_name == var )  break;
                    }
                    if( i <= params.last_index() ) {
                        filename += par->_value;
                        par->_used = true;
                    }
                }
            } else {
                const char* q = strchr( p, '$' );
                if( !q )  q = p + strlen( p );
                append( &filename, p, q - p );
                p = q;
            }
        }

        // Sicherstellen, dass kein Parameter umsonst angegeben worden ist
        for( int i = 0; i <= params.last_index(); i++ ) {
            Alias_file_param* par = &params[ i ];
            if( !par->_used )  throw_xc( "SOS-1315", c_str( par->_name ) );
        }
    }

    filename += " ";
    filename += opt.rest();

    _file.obj_owner( this );
    _file.prepare_open( filename, open_mode, file_spec );

    _any_file_ptr->new_file( &_file );
}


} //namespace sos
