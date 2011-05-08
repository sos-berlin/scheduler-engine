//#define MODULE_NAME "selall"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Jˆrg Schwiemann"

#include "precomp.h"
#include "../kram/sysdep.h"

#include "../kram/sos.h"
#include "../kram/sosstrng.h"
#include "../kram/sosfield.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/log.h"


namespace sos {
using namespace std;


// ----------------------------------------------------------------------------- Alias_file

struct Select_all_file : Abs_file
{
                              //~Select_all_file             () { if ( _file ) _file->_f = 0; } // Damit Any_file Datei nicht schlieﬂt

    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
  //void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        replace_select_all      ( const Sos_string& filename, Sos_string* result );

 private:
    Any_file                   _file;
    Sos_ptr<Record_type>       _record_type_ptr;
};


struct Select_all_file_type : Abs_file_type
{
    virtual const char* name      () const { return "select_all"; }

    virtual Sos_ptr<Abs_file> create_base_file   () const
    {
        Sos_ptr<Select_all_file> f = SOS_NEW_PTR( Select_all_file() );
        return +f;
    }
};

const Select_all_file_type  _select_all_file_type;
const Abs_file_type&        select_all_file_type = _select_all_file_type;

//---------------------------------------------------------------------Select_all_file::prepare_open

void Select_all_file::prepare_open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    //LOG( "Select_all_file::prepare_open: filename=" << filename << "\n" );
    Sos_string filename;
    Sos_string real_filename;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if ( opt.pipe() )                               { filename = opt.rest(); break; }
        else throw_sos_option_error( opt );
    }

    _record_type_ptr = (Record_type*)(file_spec.field_type_ptr());
    if( !_record_type_ptr ) throw_xc( "SOS-1171" );

    replace_select_all( filename, &real_filename );
    LOG( "Select_all_file::prepare_open: resolved=" << real_filename << "\n" );

    _file.prepare_open( real_filename, open_mode, file_spec );

    _any_file_ptr->new_file( &_file );
}

//--------------------------------------------------------------------------Alias_file::resolve

void Select_all_file::replace_select_all( const Sos_string& original, Sos_string* result_ptr )
{
    const char* p_start = c_str(original);
    const char* p       = p_start;

    while (*p)
    {
      if ( (*p == 's' || *p == 'S') && strncmpi( p, "select * ", 9 ) == 0  )
      {
            Dynamic_area buf(4096);
            ostrstream s( buf.char_ptr(), buf.size() );
            Text_format format; format.separator( ',' );
            _record_type_ptr->print_field_names( &s, format );
            s << ' ' << '\0' << flush;
            buf.char_ptr()[buf.size()-1] = '\0';

            *result_ptr  = sub_string( p_start, 0, p-p_start ); // Anfang schreiben
            *result_ptr += "SELECT ";
            *result_ptr += buf.char_ptr();
            *result_ptr += p+9; // Rest schreiben
            return;
      }
      p++;
    }
    *result_ptr = original;
}

} //namespace sos
