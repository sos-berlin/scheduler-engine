#define MODULE_NAME "fssta"
#define AUTHOR      "Joacim Zschimmer"
#define COPYRIGHT   "©1996 SOS GmbH Berlin"

#include <stdio.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosfiltr.h"
#include "../kram/sosopt.h"
#include "fs.h"

using namespace std;
namespace sos {

//-------------------------------------------------------------------------------Fs_status_file

struct Fs_status_file : Abs_file
{
                                Fs_status_file          ();
                               ~Fs_status_file          ();

    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

  private:
    void                        get_next_name           ( Area*, int* fs_index, int* client_index );

    Fill_zero                  _zero_;
    Any_file*                  _any_file_ptr;
    Bool                       _clients;
    Bool                       _files;
    int                        _fs_index;
    int                        _client_index;
    int                        _file_index;

  protected:
    void                        get_record              ( Area& area );
    void                        del                     ( const Key& );
};

//--------------------------------------------------------------------------Fs_status_file_type

struct Fs_status_file_type : Abs_file_type
{
    const char*                 name                    () const  { return "fs_status"; }
    Sos_ptr<Abs_file>           create_base_file        () const
    {
        Sos_ptr<Fs_status_file> f = SOS_NEW( Fs_status_file() );
        return +f;
    }
};


Fs_status_file_type            _fs_status_file_type;
const Abs_file_type&            fs_status_file_type = _fs_status_file_type;

//---------------------------------------------------------------Fs_status_file::Fs_status_file

Fs_status_file::Fs_status_file()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------Fs_status_file::~Fs_status_file

Fs_status_file::~Fs_status_file()
{
}

//-------------------------------------------------------------------------Fs_status_file::open

void Fs_status_file::open( const char* fn, Open_mode, const File_spec& )
{
    Sos_string filename = fn;

    // Dateityp-Optionen parsieren
    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if ( opt.flag( "clients" ) )           _clients = opt.set();
        else
        if ( opt.flag( "files" ) )             _files = opt.set();
        //if ( opt.param() || opt.pipe()      )   { filename = opt.rest(); break; }
        else throw                              Sos_option_error( opt );
    }
}

//------------------------------------------------------------------------Fs_status_file::close

void Fs_status_file::close( Close_mode )
{
}

//-------------------------------------------------------------------Fs_status_file::get_record

void Fs_status_file::get_record( Area& area )
{
    if( _clients ) {
        get_next_name( &area, &_fs_index, &_client_index );
    }
    else
    {
        time_t current_time;
        time( &current_time );

      NEXT_FS:
        _fs_index = max( _fs_index, Fileserver::_fs_array.first_index() );
        if( _fs_index > Fileserver::_fs_array.last_index() )  throw_eof_error();
        const Fileserver* fs = Fileserver::_fs_array[ _fs_index ];
        if( !fs ) { _fs_index++; goto NEXT_FS; }

      NEXT_CLIENT:
        _client_index = max( _client_index, fs->_client_array.first_index() );
        if( _client_index > fs->_client_array.last_index() )  {
            _fs_index++;
            _client_index = 0;
            _file_index = 0;
            goto NEXT_FS;
        }


        const Fs_client* client = fs->_client_array[ _client_index ];
        if( !client )  { _client_index++; goto NEXT_CLIENT; }

        if( _file_index == 0 ) {             // Zeile für Client allein einfügen
            _file_index++;
            area.assign( client->_sos_client._name );
            area.append( "\t" );
            char buffer [ 30 ];
            time_t t = current_time - client->_last_used_time;
            sprintf( buffer, "%ld", t );
            area.append( buffer );

            area.allocate_min( area.length() + 200 );

            ///// client name ////
            area.assign( client->_sos_client._name );

            {   ////// idle time /////
                area += '\t';
                char buffer [ 30 ];
                time_t t = current_time - client->_last_used_time;
                sprintf( buffer, "%ld", t );
                area.append( buffer );
            }

            area += '\t';

            ostrstream s ( area.char_ptr() + area.length(), area.size() - area.length() );
            //client->obj_print_status( &s );
            client->_sos_client.obj_print( &s );
            area.length( area.length() + s.pcount() );
            return;
        }

      NEXT_FILE:
        _file_index = max( _file_index, client->_file_array.first_index() );
        if( _file_index > client->_file_array.last_index() )  {
            _client_index++;
            _file_index = 0;
            goto NEXT_CLIENT;
        }

        ///// client name ////
        area.assign( client->_sos_client._name );


        Fs_client_file* file = client->_file_array[ _file_index ];
        if( !file )  { _file_index++; goto NEXT_FILE; }

        ////// idle time /////
        {
            area += '\t';
            char buffer [ 30 ];
            time_t t = current_time - file->_last_used_time;
            sprintf( buffer, "%ld", t );
            area.append( buffer );
        }

        ///// Dateiname /////
        area += '\t';
        Sos_string filename = file->_filename;
        area.append( c_str( filename ), length( filename ) );

        ///// info() /////
        File_info info = client->_file_array[ _file_index ]->_file.info();
        if( !empty( info._text ) ) {
            area += '\t';
            area.append( c_str( info._text ), length( info._text ) );
        }

        _file_index++;
    }
}

//----------------------------------------------------------------Fs_status_file::get_next_name

void Fs_status_file::get_next_name( Area* area, int* fs_index, int* client_index )
{
  NEXT_FS:
    _fs_index = max( _fs_index, Fileserver::_fs_array.first_index() );
    if( *fs_index > Fileserver::_fs_array.last_index() )  throw Eof_error();
    const Fileserver* fs = Fileserver::_fs_array[ *fs_index ];
    if( !fs ) { (*fs_index)++; goto NEXT_FS; }

  NEXT_CLIENT:
    *client_index = max( *client_index, fs->_client_array.first_index() );

    if( *client_index > fs->_client_array.last_index() )  {
        (*fs_index)++;
        *client_index = 0;
        goto NEXT_FS;
    }

    const Fs_client* client = fs->_client_array[ *client_index ];
    if( !client )  { (*client_index)++; goto NEXT_CLIENT; }

    area->assign( client->_sos_client._name );
    (*client_index)++;
}

//--------------------------------------------------------------------------Fs_status_file::del

void Fs_status_file::del( const Key& name )
{
 SHOW_MSG( "Fs_status_file::del( \"" << name << "\" )\n");
    int             fs_index     = 0;
    int             client_index = 0;
    Dynamic_area    record;

    while(1) {
        try {
            get_next_name( &record, &fs_index, &client_index );
        }
        catch( const Eof_error& ) { throw_not_found_error(); }

        if( name == record )  break;
    }

    client_index--;  // get_next_name hat ihn weitergezählt
    Fileserver::_fs_array[ fs_index ]->kill_client( client_index );
}


} //namespace sos