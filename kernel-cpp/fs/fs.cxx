#define MODULE_NAME "fs"
/* fs.cpp                                       (c) SOS GmbH Berlin
                                                    Joacim Zschimmer
*/

#define __USELOCALES__      // strftime()

#include "precomp.h"
#include "../kram/optimize.h"
#include "../kram/sysdep.h"

//#include <values.h>         // MAXINT
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#if defined SYSTEM_MICROSOFT
#   include <sys/types.h>
#   include <sys/timeb.h>
#endif

#include "../kram/sysxcept.h"
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/xception.h"
#include "../kram/sosarray.h"
#include "../kram/sosstrea.h"
#include "../kram/soslimtx.h"
#include "../kram/log.h"
#include "../kram/sosobj.h"
#include "../kram/sosmsg.h"
#include "../kram/sosfiltr.h"
#include "../kram/sosfact.h"
#include "../file/anyfile.h"
#include "../kram/sosopt.h"
//#include "../kram/init.h"
#include "../fs/rapid.h"
#include "fs.h"

#if defined SYSTEM_LINUX
//#   define USE_GETTIMEOFDAY
#endif


using namespace std;
namespace sos {

extern ostream* fs_log;     // definiert in cfs.cxx

#define FS_LOG( x )  (*fs_log) << x

const char fs_version [ 6+1 ]     = "2.0D01";
const uint max_block_size         = MIN( 256+2+32767+256, (uint)INT_MAX - 50 );
const uint max_answer_buffer_size = max_block_size;
const int  fs_max_key_length      = 256;
const int fs_max_client_name_length = 8 + 1 + 8;  // 8;

Sos_simple_array<Fileserver*> Fileserver::_fs_array;

inline DEFINE_SOS_STATIC_PTR( Fs_client )
DEFINE_SOS_DELETE_ARRAY( Fileserver* )

//struct Fs_status_file_type;
//extern Fs_status_file_type      _fs_status_file_type;      // damit's eingebunden wird
//static const Fs_status_file_type&     fs_status_file_type = _fs_status_file_type;


struct Fs_job : Sos_self_deleting, Fs_base
/*
    Ein Auftrag für den Fileserver.
    Bei einem asynchronen Fileserver können mehrere Aufträge gleichzeitig abgewickelt werden.
*/
{
                                Fs_job                  ( Fileserver*, Data_msg* );

    Const_area                  answer                  ();

    void                        enable                  ();
    void                        disable                 ();
    void                        open                    ();
    void                        close                   ();
    void                        putburst                ();
    void                        getburst                ();
    void                        putkey                  ();
    void                        delkey                  ();
    void                        getkey                  ();

    void                        close_file              ( Sos_ptr<Fs_client_file>* );

  private:
    void                        reply_header            ( Rapid_task_connection_cmd = taxcret );

    Fileserver* const          _fs_ptr;
    uint4                      _client_word;
    uint4                      _server_word;
    Id                         _application_id;
    Sos_binary_istream         _in;
    Sos_binary_ostream         _out;
  //Byte                       _answer_buffer [ max_answer_buffer_size ];
    Dynamic_area               _answer;
};

//-------------------------------------------------------------------------------------Fs_descr

struct Fs_descr : Sos_object_descr
{
    virtual const char*         name                    () const  { return "fileserver"; }
    virtual Sos_object_ptr      create                  ( Subtype_code ) const
    {
        Sos_ptr<Fileserver> p = SOS_NEW( Fileserver() );
        return +p;
    }
};

//----------------------------------------------------------------------------------------const

const Fs_descr fs_descr;

//----------------------------------------------------------Sos_static_ptr<Fs_job>::sos_pointer

DEFINE_SOS_STATIC_PTR( Fs_job )

//------------------------------------------------------------------------------------time_stamp

static const char* time_stamp()
{
/*
    static char text [ 20 ];
    time_t      t;

    //?tzset();
    time( &t );

    strftime( text, sizeof text, "%d %H:%M:%S ", localtime( &t ) );

    return text;
*/
    static char buffer [ 20 ];
#   if defined USE_GETTIMEOFDAY
        struct timeval t;
        strftime( buffer, 11+1, "%d %T", localtime( &t.tv_sec ) );
        sprintf( buffer + 11, ".%04d", (int)_time_stamp.tv_usec / 100 );
        buffer[ 31 + 5 ] = '\0';
#    elif defined SYSTEM_MICROSOFT
        _timeb t;
        _ftime( &t );
        strftime( buffer, 11+1, "%d %H:%M:%S", localtime( &t.time ) );
        sprintf( buffer + 11, ".%03d", (int)t.millitm );
#    else
        time_t t;
        time( &t );
        strftime( buffer, 11+1, "%d %T", localtime( &t ) );
#   endif

    return buffer;
}

//----------------------------------------------------------------------------ostream << ( Id )

ostream& operator<<( ostream& s, const Fs_base::Id& id )
{
    s << hex << setfill('0') << setw(4)
      << id._index << '/' << setw(4) << id._check_value._value
      << setfill(' ') << dec;
    return s;
}

//--------------------------------------------------------Fs_base::Check_value::Check_value

/*inline*/ Fs_base::Check_value::Check_value()
:
    _value( 0 )
{
}

//--------------------------------------------------------Fs_base::Check_value::Check_value

inline void Fs_base::Check_value::generate()
{
    _value = rand();   // Zufallszahl aus stdlib.h
    //int GENERATE_AUSSER_BETRIEB;
}

//---------------------------------------------------------Fs_base::Check_value::operator==

inline Bool Fs_base::Check_value::operator==( const Check_value& v )
{
    return Bool( _value == v._value );
}

//--------------------------------------------------------------Fs_base::Check_value::write

inline void Fs_base::Check_value::write( Sos_binary_ostream* s )
{
    s->write_uint2( _value );
}

//---------------------------------------------------------------Fs_base::Check_value::read

inline void Fs_base::Check_value::read( Sos_binary_istream* s )
{
    s->read_uint2( &_value );
}

//------------------------------------------------------------------------------Fs_base::Id::Id

inline Fs_base::Id::Id()
:
    _index( 0 )
{
}

//------------------------------------------------------------------------------Fs_base::Id::Id

inline Fs_base::Id::Id( int index )
:
    _index( index )
{
    _check_value.generate();
}

//------------------------------------------------------------------------------Fs_base::Id::Id

inline Fs_base::Id::Id( int index, const Check_value& check_value )
:
    _check_value( check_value ),
    _index( index )
{
}

//---------------------------------------------------------------------Fs_base::::check_value

inline Fs_base::Check_value Fs_base::Id::check_value() const
{
    return _check_value;
}

//---------------------------------------------------------------------------Fs_base::Id::index

inline int Fs_base::Id::index() const
{
    return _index;
}

//---------------------------------------------------------------------------Fs_base::Id::write

/*inline*/ void Fs_base::Id::write( Sos_binary_ostream* s )
{
    s->write_uint2( _index );
    _check_value.write( s );
}

//---------------------------------------------------------------------------Fs_base::Id::write

inline void Fs_base::Id::read( Sos_binary_istream* s )
{
    s->read_uint2( &_index );
    _check_value.read( s );
}

//-------------------------------------------------------------------------Fs_client::Fs_client

Fs_client::Fs_client( Fileserver* fs, const Sos_string& user_name )
:
    _zero_(this+1),
    _fileserver ( fs )
{
    _check_value.generate();
    _file_array.obj_const_name( "Fs_client::_file_array" );
    _file_array.first_index( 1 );
    _sos_client._name = user_name;  // tcp:192.0.0.20/32600.1234sosc.jz
    //strncpy( _name, user_name, fs_max_client_name_length );
    //_name[ fs_max_client_name_length ] = 0;
}

//----------------------------------------------------------------------Fs_client::~Fs_client

Fs_client::~Fs_client()
{
  //FS_LOG( "~Fs_client\n" );  // Dateien werden bei Disable nicht geschlossen?
    LOGI( "~Fs_client(\"" << _sos_client._name << "\")\n" );

    close_all_files();
}

//-------------------------------------------------------------------Fs_client::close_all_files

void Fs_client::close_all_files()
{
  //FS_LOG( "Fs_client::close_all_files\n" );  // Dateien werden bei Disable nicht geschlossen?
    LOGI( "~Fs_client(\"" << _sos_client._name << "\").close_all_files()\n" );

    for( int i = _file_array.last_index(); i >= _file_array.first_index(); i-- )
    {
        if( _file_array[ i ] ) {
            try {
                FS_LOG( time_stamp() << "   Fs_client: schließe Datei "
                        << _file_array[ i ]->_filename << '\n' );
                // Fileid ausgeben!
                LOGI( "   Fs_client: schließe Datei " << _file_array[ i ]->_filename << '\n' );
                SOS_DELETE( _file_array[ i ] );
            }
            catch( const Xc& ) {}
            catch( const exception& ) {}
        }
    }
}

//-----------------------------------------------------------------------------Fs_client::valid

inline Bool Fs_client::valid() const
{
    return _valid;
}

//------------------------------------------------------------------------------Fs_client::name

inline const char* Fs_client::name() const
{
    return c_str( _sos_client._name );
}

//-----------------------------------------------------------------------------Fs_client::check

void Fs_client::check( Id user_id ) const
{
    if( ! (user_id.check_value() == _check_value ))  throw_connection_lost_error( "RU02"  ); //"ID"
}

//--------------------------------------------------------------------------Fs_client::file_ptr

Sos_ptr<Fs_client_file>& Fs_client::file_ptr( Id file_id )
{
    int index = file_id.index();
    if( index < _file_array.first_index()  ||  index > _file_array.last_index() )  throw_xc( "RU03" );

    Sos_ptr<Fs_client_file>& f = _file_array[ index ];
    if( !f ||  f->check_value() != file_id.check_value() )  throw_xc( "RU03" );

    f->_last_used_time = _last_used_time;       // client_ptr() muß vorher gerufen worden sein

    return f;
}

//-----------------------------------------------------------------------Fs_client::check_value

inline Fs_base::Check_value Fs_client::check_value() const
{
    return _check_value;
}

//-------------------------------------------------------------------------------Fs_client::add

Fs_base::Id Fs_client::add( const Sos_ptr<Fs_client_file>& client_file_ptr )
{
    int i = _file_array.first_index();

    while( i <= _file_array.last_index()  &&  _file_array[ i ] )  i++;

    if( i > _file_array.last_index() ) {
        _file_array.last_index( i );
    }

    time( &client_file_ptr->_last_used_time );
    _file_array[ i ] = client_file_ptr;

    //client_file_ptr->obj_owner( this );

    return Id( i, client_file_ptr->check_value() );
}

//---------------------------------------------------------------Fs_client_file::Fs_client_file

inline Fs_client_file::Fs_client_file( Fs_client* client )
:
    _zero_(this+1),
    _client      ( client ),
    _text        ( true  ),
    _close_at_eof( false ),
    _file_closed ( false ),
    _next_record_valid ( false )
{
    _check_value.generate();
}

//--------------------------------------------------------------Fs_client_file::~Fs_client_file

Fs_client_file::~Fs_client_file()
{
    try {
        close( close_error );
    }
    catch( const Xc& x ) {
        FS_LOG( time_stamp() << "   " << x << '\n' );
    }
//#ifndef __GNU__  // Internal compiler error
    catch( const exception& ) {
        FS_LOG( time_stamp() << "   Unbekannte Exception aufgetreten\n" );
    }
//#endif
}

//-------------------------------------------------------------------------Fs_client_file::open

void Fs_client_file::open( Fd* fd, Openpar* openpar, const Sos_string& filename_ )
{
    //_filename = sub_string( filename_, 0, min( 100u, length( filename_ ) ) );
    _filename = as_string( c_str( filename_ ), min( 100u, length( filename_ ) ) );

    Sos_string filename = filename_;

    const char* p = c_str( filename );     // Spezial-Flag -binary ???
    while( *p == ' ' ) p++;
    if( memcmp( p, "-binary ", 8 ) == 0 ) {
        _text = false;
        filename = as_string( p + 8 );
    }

    File_base::Open_mode    open_mode = File_base::Open_mode( 0 );

    _close_at_eof = fd->_fdattr.fdseq() && ( fd->_fdattr.fdin() | fd->_fdattr.fdinout() );  // s.a. fsfile.cxx

    if(  fd->_fdattr.fdin()          )  open_mode = File_base::Open_mode( open_mode | File_base::in );
    if(  fd->_fdattr.fdout()         )  open_mode = File_base::Open_mode( open_mode | File_base::out );
    if(  fd->_fdattr.fdinout()       )  open_mode = File_base::Open_mode( open_mode | File_base::in | File_base::out );
    if(  openpar->_opflags.opnew()   )  open_mode = File_base::Open_mode( open_mode | File_base::trunc );
    if(  fd->_fdattr.fdseq()         )  open_mode = File_base::Open_mode( open_mode | File_base::seq );
    if(  openpar->_opflags.opexist() )  open_mode = File_base::Open_mode( open_mode | File_base::nocreate );
    if( !openpar->_opflags.opcreat() )  open_mode = File_base::Open_mode( open_mode | File_base::nocreate );
    if( !openpar->_opflags.opow()    )  open_mode = File_base::Open_mode( open_mode | File_base::noreplace );
    //                                   open_mode |= binary;

    _file.obj_owner( this );
    _file.open( _client->_fileserver->_filename_prefix + filename, open_mode );

    _key_position = _file.key_position();  fd->fdkp( _key_position );
    _key_length   = _file.key_length();    fd->fdkl( _key_length );
}

//------------------------------------------------------------------------Fs_client_file::close

void Fs_client_file::close( Close_mode close_mode )
{
    _file.close( close_mode );
}

//--------------------------------------------------------------------Fs_client_file::get_burst

void Fs_client_file::get_burst( Sos_binary_ostream* out, uint recommended_block_size,
                                const Byte* key, const Byte* key_end, Bool in_open )
{
    int   a = _key_length + 2;
    Bool  no_record_read = true;

    if( recommended_block_size > out->space_left() )  recommended_block_size = out->space_left();

    if( out->space_left() < a )  throw_too_long_error();

    if( _next_key.length()  &&  memcmp( _next_key.ptr(), key, _next_key.length() ) != 0 ) {
        _file.set( Const_area( key, _key_length ) );
        _next_record_valid = false;
    }

    try {
        while(1)
        {
            if( !_next_record_valid ) {
                _file.get( &_next_record );
                if( _key_length )  _next_key = _file.current_key(); //int KEY_LENGTH_MUSS_BEIM_OPEN_GESETZT_WERDEN;
                _next_record_valid = true;
            }

            if( _next_key.length() != _key_length )  { Xc x ("SOS-FILE-1100"); x.insert( _key_length ); x.insert( _next_key.length() ); x.insert( this ); throw x; }
            out->write_fixed_area( _next_key );

            int4 needed_space = 2 + _next_record.length() + _key_length + 1;
            
            // recommended_block_size gilt nicht für den ersten Satz (der darf größer sein)
            if( ( no_record_read? (int)out->space_left()
                                : (int)recommended_block_size - (int)out->length() ) < needed_space )
            {
                if( _key_length == 0 )  out->write_byte( 0xFF );    // Noch nicht Dateiende
                break;
            }

            out->write_uint2( _next_record.length() );
            if( _text ) {
                xlat( out->space( _next_record.length() ), _next_record.char_ptr(), _next_record.length(), iso2ebc );
            } else {
                out->write_fixed_area( _next_record );
            }

            _next_record_valid = false;
            no_record_read = false;
        }

        if( no_record_read  &&  !in_open )  throw_too_long_error( /*"D3??"*/ );
    }
    catch( const Eof_error& )
    {
        if( _close_at_eof ) {
            if( !in_open )  FS_LOG( time_stamp() << " FS autoclose\n" );
            _file_closed = true;
            _file.close();
        }
    }
}

//--------------------------------------------------------------------Fs_client_file::put_burst

void Fs_client_file::put_burst( Sos_binary_istream* in )
{
    Dynamic_area area;

    while( in->rest_length() >= sizeof (uint2) )
    {
        uint2 length = in->read_uint2();

        if( length == (uint2)-1 ) {
            _file_closed = true;
            _file.close(); 
            break;
        }

        if( in->rest_length() < length )  break;

        if( _text ) {
            area.allocate_min( length );
            xlat( area.ptr(), in->read_bytes( length ), length, ebc2iso );
            area.length( length );
            _file.put( area );
        } else {
            _file.put( Const_area( in->read_bytes( length ), length ) );
        }
    }

    if( in->rest_length() )  throw_xc( "SOS-1143", "putburst" );     // Datenfehler
}

//----------------------------------------------------------------------Fs_client_file::put_key

void Fs_client_file::put_key( Sos_binary_istream* in )
{
    Parflput::Flags flags;

    read_parflput_flags( in, &flags );

    Const_area   record ( in->ptr(), in->rest_length() );
    Const_area*  r = &record;
    Dynamic_area buffer;

    if( _text ) {
        buffer.allocate_min( record.length() );
        xlat( buffer.ptr(), record.ptr(), record.length(), ebc2iso );
        buffer.length( record.length() );
        r = &buffer;
    }

    if( flags.putupd() ) {
        _file.update( *r );
    }
    else
    if( flags.putins() ) {
        _file.insert( *r );
    }
    else
    if( flags.putkey() ) {
        _file.store( *r );
    }
    else {
        _file.put( *r );
    }
}

//-------------------------------------------------------------------------------Fs_job::Fs_job

Fs_job::Fs_job( Fileserver* fs_ptr, Data_msg* m )
:
    _fs_ptr ( fs_ptr ),
    _in     ( m->data() )
{
  try {
    //try {
        _answer.allocate_min( max_answer_buffer_size );
        _out.reset( _answer );

        Task_connection::Cmd    cmd;
        char                    entry_name [ 8+1 ];

        if( _in.rest_length() < 1 + 3 + 4 + 4 + 4 + 8 )  throw_xc( "SOS-1143" );

        cmd = Task_connection::Cmd( _in.read_byte() );
        _in.skip_bytes( 3 );
        _in.read_fixed( &_server_word, sizeof _server_word );
        _in.read_fixed( &_client_word, sizeof _client_word );

        if( cmd != taxccal )  throw_xc( "SOS-1143" );

        _in.skip_bytes( 4 + 4 );    // A(0), Task-Id
        _in.read_fixed( entry_name, 8 );

        xlat( entry_name, entry_name, sizeof entry_name, ebc2iso );

        if( memcmp( entry_name, "GETBRST2", 8 ) == 0 )  getburst();
        else
        if( memcmp( entry_name, "PUTBURST", 8 ) == 0 )  putburst();
        else
        if( memcmp( entry_name, "PUTKEY  ", 8 ) == 0 )  putkey();
        else
      //if( memcmp( entry_name, "GETKEY  ", 8 ) == 0 )  getkey();
      //else
        if( memcmp( entry_name, "DELKEY  ", 8 ) == 0 )  delkey();
        else
        if( memcmp( entry_name, "OPEN2   ", 8 ) == 0 )  open();
        else
        if( memcmp( entry_name, "CLOSE   ", 8 ) == 0 )  close();
        else
      //if( memcmp( entry_name, "ERASE   ", 8 ) == 0 )  erase();
      //else
        if( memcmp( entry_name, "ENABLE  ", 8 ) == 0 )  enable();
        else
        if( memcmp( entry_name, "DISABLE ", 8 ) == 0 )  disable();
        else
        {
            entry_name[ sizeof entry_name - 1 ] = '\0';
            throw_xc( "SOS-1144", entry_name );
        }

        if( _out.length() == 0 ) {     // Noch keine Antwort gegeben?
            reply_header();             //    Eine leere Antwort
        }
    //}
    //CATCH_AND_THROW_XC
  }
  catch( Xc x )
  {
      //Xc x = x_;        // jz 16.4.96 Auf Solaris liegt x_ hinter dem Stack und wird vom nächsten Upro überschrieben

      FS_LOG( time_stamp() << "            Exception " << x << endl );

      _out.reset( _answer );

      reply_header( taxcxcp );

      char error_code [ 16+1 ];
      strncpy( error_code, x.code(), 16 );  error_code[16] = '\0';

      write_string_ebcdic( &_out, x.name()        ,  8 );
      write_string_ebcdic( &_out, error_code      , 16 );
  }
  catch( const exception& x )
  {
      FS_LOG( time_stamp() << "            Unbekannte Exception" << x << endl );
      _out.reset( _answer );
      reply_header( taxcxcp );

      write_string_ebcdic( &_out, "FSERROR"         ,  8 );
      write_string_ebcdic( &_out, "FS-UNKNOWN-ERROR", 16 );
  }
}

//-------------------------------------------------------------------------------Fs_job::answer

inline Const_area Fs_job::answer()
{
    return _out.area();
}

//-------------------------------------------------------------------------------Fs_job::enable

void Fs_job::enable()
{
    char        appl_name      [ 8 + 1 ];
    char        user_name      [ 8 + 1 ];
    char        client_version [ 6 + 1 ];
    char        terminal_name  [ 8 + 1 ];
    Sos_ptr<Fs_client>  client_ptr;
    int         user_index;


    if( _in.rest_length() != 8 + 8 + 6 + 8 )  throw_xc( "SOS-1143", "enable" );

    read_string_ebcdic( &_in, appl_name, 8 );
    read_string_ebcdic( &_in, user_name, 8 );
    read_string_ebcdic( &_in, client_version, 6 );
    read_string_ebcdic( &_in, terminal_name, 8 );

    Sos_string client_name = _fs_ptr->_client_name;
    client_name += '/';
    client_name += appl_name;
    client_name += '.';
    client_name += user_name;

    if( sub_string( client_name, 0, 4 ) == "tcp:" ) {
        client_name = sub_string( client_name, 4 );
    }

    FS_LOG( time_stamp() << " FS ENABLE            " << client_name << ", Version " << client_version << '\n' );

    if( memcmp( client_version, "2.0C06", sizeof client_version ) < 0 )  throw_xc( "SOS-1145" );

    client_ptr = SOS_NEW( Fs_client( _fs_ptr, client_name ) );
    user_index = _fs_ptr->add( client_ptr );

    reply_header();

    _application_id.write( &_out );
    Id( user_index, client_ptr->check_value() ).write( &_out );

    xlat( _out.space( sizeof fs_version - 1 ), fs_version, sizeof fs_version - 1, iso2ebc );
}

//------------------------------------------------------------------------------Fs_job::disable

void Fs_job::disable()
{
    Id                    application_id;
    Id                    user_id;
    Fs_client*            client_ptr;

    if( _in.rest_length() != 4 + 4 )  throw_xc( "SOS-1143", "disable" );

    application_id.read( &_in );
    user_id       .read( &_in );

    client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS DISABLE " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << '\n' );

    // applikation_id wird nicht berücksichtigt

    Fs_client* c = _fs_ptr->_client_array[ user_id.index() ];
    if( c->obj_ref_count() != 1 )  LOG( "   obj_ref_count=" << c->obj_ref_count() << '\n' );
    c->close_all_files();
    c->_sos_client.close();
    SOS_DELETE( _fs_ptr->_client_array[ user_id.index() ] );
}

//---------------------------------------------------------------------------------Fs_job::open

void Fs_job::open()
{
    Id                    application_id;
    Id                    user_id;
    Id                    file_id;
    Fd                    fd;
    Openpar               openpar;
    int4                  block_size      = 0;
    Bool                  read            = false;
    Bool                  closed          = false;
    Sos_ptr<Fs_client_file>  client_file_ptr;
    Fs_client*            client_ptr;


    if( _in.rest_length() < 4 + 4 + fd.binary_size() + openpar.binary_size() + 4 )  throw_xc( "SOS-1143", "open" );

    application_id.read( &_in );
    user_id       .read( &_in );

    read_fd( &_in, &fd  );
    read_openpar( &_in, &openpar );
    read_int4 ( &_in, &block_size );

    client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS OPEN    " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << openpar._opname_string0 << '\n' );

    Sos_string filename;
    int l = strlen( openpar._opname_string0 );
    if( l > 0 && openpar._opname_string0[ l - 1 ] == 0x01 ) {   // Dateinamesverlängerung?
        int4  dummy_int4;
        uint2 len;
        read_int4 ( &_in, &dummy_int4 );   // Reserve
        read_uint2( &_in, &len        );   // Länge der Dateinamensverlängerung
        if( _in.rest_length() != len )  throw_xc( "SOS-1143", "open" );
        filename = as_string( openpar._opname_string0, l - 1 )
                 + as_string( (const char*)_in.read_bytes( len ), len );
    } else {
        filename = openpar._opname_string0;
        if( _in.rest_length() != 0 )  throw_xc( "SOS-1143", "open" );
    }

    // Datei-Objekt anlegen und Client zuordnen
    client_file_ptr = SOS_NEW( Fs_client_file ( client_ptr ) );
    client_file_ptr->obj_owner( client_ptr );
    client_file_ptr->open( &fd, &openpar, filename );

    file_id = client_ptr->add( client_file_ptr );


    reply_header();
    write_fd     ( &_out, fd );
    write_openpar( &_out, openpar );
    file_id.write( &_out );

    block_size = max( 0, (int4) ( min( block_size, max_block_size ) - _out.length() ) );

    if( ( fd._fdattr.fdin() | fd._fdattr.fdinout() )  &&  block_size )
    {
        read = true;

        _out.rest_size( max_block_size );

        Byte key_0  [ fs_max_key_length ];  memset( key_0 , 0x00, sizeof key_0 );
        Byte key_FF [ fs_max_key_length ];  memset( key_FF, 0xFF, sizeof key_0 );

        client_file_ptr->get_burst( &_out, block_size, key_0, key_FF, true/*open*/ );

        if( client_file_ptr->file_closed() ) {
            closed = true;
            close_file( &client_ptr->file_ptr( file_id ) );
        }
    }

    FS_LOG( time_stamp() << " FS opened" << ( closed? ", read and closed " : read? " and read " : "  " ) << file_id << '\n' );
}

//--------------------------------------------------------------------------------Fs_job::close

void Fs_job::close()
{
    Id         application_id;
    Id         user_id;
    Id         file_id;

    if( _in.rest_length() != 4 + 4 + 4 )  throw_xc( "SOS-1143", "close" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    Fs_client* client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS CLOSE   " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    close_file( &client_ptr->file_ptr( file_id ) );
}

//---------------------------------------------------------------------------Fs_job::close_file

void Fs_job::close_file( Sos_ptr<Fs_client_file>* file_ptr_ptr )
{
    (*file_ptr_ptr)->close();
    SOS_DELETE( *file_ptr_ptr );
    //*file_ptr_ptr = 0;
}

//-------------------------------------------------------------------------Fs_job::reply_header

void Fs_job::reply_header( Task_connection::Cmd cmd )
{
    _out.write_byte ( (Byte) cmd );
    _out.write_fixed( "\0\0\0", 3 );
    _out.write_fixed( &_server_word, sizeof _server_word );
    _out.write_fixed( &_client_word, sizeof _client_word );
    _out.write_fixed( "\0\0\0\0\0\0\0\0", 8 );

    if( cmd == taxcret ) {
        _out.write_fixed( "\0\0\0\0\0\0\0\0", 8 );
    }
}

//-----------------------------------------------------------------------------Fs_job::putburst

void Fs_job::putburst()
{
    Id               application_id;
    Id               user_id;
    Id               file_id;
    Sos_ptr<Fs_client_file>* file_ptr_ptr = 0;


    if( _in.rest_length() < 4 + 4 + 4 )  throw_xc( "SOS-1143", "putburst" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    Fs_client* client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS PUT     " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    file_ptr_ptr = &client_ptr->file_ptr( file_id );

    try {
        (*file_ptr_ptr)->put_burst( &_in );
    }
    catch( const Xc& ) {
        if( (*file_ptr_ptr)->file_closed() )  close_file( file_ptr_ptr );
        throw;
    }
    catch( exception& ) {
        if( (*file_ptr_ptr)->file_closed() )  close_file( file_ptr_ptr );
        throw;
    }

    if( (*file_ptr_ptr)->file_closed() )  close_file( file_ptr_ptr );
}

//-----------------------------------------------------------------------------Fs_job::getburst

void Fs_job::getburst()
{
    Id                  application_id;
    Id                  user_id;
    Id                  file_id;
    int4                block_size      = 0;
    int4                count           = 0;
    Fs_client*          client_ptr;
    Fs_client_file*     file_ptr        = 0;
    Byte                key     [ fs_max_key_length ];
    Byte                key_end [ fs_max_key_length ];

    if( _in.rest_length() < 4 + 4 + 4 + 4 )  throw_xc( "SOS-1143", "getburst" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    client_ptr = _fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS GET     " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    _in.read_int4( &block_size );
    _in.read_int4( &count );


    file_ptr   = client_ptr->file_ptr( file_id );

    if( _in.rest_length() != 2 * file_ptr->key_length() )  throw_xc( "SOS-1143", "getburst" );

    _in.read_fixed( key    , file_ptr->key_length() );
    _in.read_fixed( key_end, file_ptr->key_length() );

    //_out.rest_size( min( block_size, max_block_size ) );
    _out.rest_size( max_block_size );

    reply_header();

    try {
        file_ptr->get_burst( &_out, block_size, key, key_end );
    }
    catch( const Xc& )
    {
        if( file_ptr && file_ptr->file_closed() ) {
            close_file( &client_ptr->file_ptr( file_id ) );
        }
        throw;
    }
    catch( const exception& )
    {
        if( file_ptr && file_ptr->file_closed() ) {
            close_file( &client_ptr->file_ptr( file_id ) );
        }
        throw;
    }

    if( file_ptr->file_closed() ) {
        close_file( &client_ptr->file_ptr( file_id ) );
    }
}

//-------------------------------------------------------------------------------Fs_job::putkey

void Fs_job::putkey()
{
    Id               application_id;
    Id               user_id;
    Id               file_id;
    Fs_client_file*  file = 0;


    if( _in.rest_length() < 4 + 4 + 4 + 4 )  throw_xc( "SOS-1143", "putkey" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    Fs_client* client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS PUTKEY  " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    file = client_ptr->file_ptr( file_id );

    file->put_key( &_in );
}

//-------------------------------------------------------------------------------Fs_job::getkey

void Fs_job::getkey()
{
    Id               application_id;
    Id               user_id;
    Id               file_id;
    Fs_client_file*  file = 0;


    if( _in.rest_length() < 4 + 4 + 4 )  throw_xc( "SOS-1143", "getkey" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    Fs_client* client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS GETKEY  " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    file = client_ptr->file_ptr( file_id );

    Const_area   key ( _in.ptr(), _in.rest_length() );
    Const_area*  k = &key;
    Dynamic_area buffer;

    if( file->_text ) {
        buffer.allocate_min( key.length() );
        xlat( buffer.ptr(), key.ptr(), key.length(), ebc2iso );
        buffer.length( key.length() );
        k = &buffer;
    }


    Dynamic_area record;
    file->_file.get_key( &record, *k );


    reply_header();
    _out.write_fixed_area( record );
}

//-------------------------------------------------------------------------------Fs_job::delkey

void Fs_job::delkey()
{
    Id               application_id;
    Id               user_id;
    Id               file_id;
    Fs_client_file*  file = 0;


    if( _in.rest_length() < 4 + 4 + 4 )  throw_xc( "SOS-1143", "delkey" );

    application_id.read( &_in );
    user_id       .read( &_in );
    file_id       .read( &_in );

    Fs_client* client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS DELKEY  " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << ' ' << file_id << '\n' );

    file = client_ptr->file_ptr( file_id );

    Const_area   key ( _in.ptr(), _in.rest_length() );
    Const_area*  k = &key;
    Dynamic_area buffer;

    if( file->_text ) {
        buffer.allocate_min( key.length() );
        xlat( buffer.ptr(), key.ptr(), key.length(), ebc2iso );
        buffer.length( key.length() );
        k = &buffer;
    }

    file->_file.del( *k );
}

//-------------------------------------------------------------------------------Fs_job::erase
/* Baustelle! jz 1.10.98 (für Weichelt)
void Fs_job::erase()
{
    Id                    application_id;
    Id                    user_id;
    Fs_client*            client_ptr;
    Erasepar              erasepar;

    if( _in.rest_length() != 4 + 4 + erasepar.binary_size() )  throw_xc( "SOS-1143", "erase" );

    application_id.read( &_in );
    user_id       .read( &_in );

    read_erasepar( &_in, &erasepar );

    client_ptr = +_fs_ptr->client_ptr( application_id, user_id );

    FS_LOG( time_stamp() << " FS ERASE   " << user_id << ' ' );
    FS_LOG( client_ptr->_sos_client._name << '\n' );

    // applikation_id wird nicht berücksichtigt

    Fs_client* c = _fs_ptr->_client_array[ user_id.index() ];
    //? if( c->obj_ref_count() != 1 )  LOG( "   obj_ref_count=" << c->obj_ref_count() << '\n' );

    remove_file( erasepar->_eraname_string0 );
}
*/
//-----------------------------------------------------------------------Fileserver::Fileserver

Fileserver::Fileserver()
:
    _zero_(this+1)
{
    _fs_array.obj_const_name( "Fileserver::_fs_array" );

    _client_array.first_index( 1 );

    int i;
    for( i = _fs_array.first_index(); i <= _fs_array.last_index(); i++ ) {
        if( !_fs_array[ i ] )  break;
    }
    if( i > _fs_array.last_index() )  _fs_array.add( (Fileserver*)NULL );

    _fs_array[ i ] = this;
}

//----------------------------------------------------------------------Fileserver::~Fileserver

Fileserver::~Fileserver()
{
    LOGI( "~Fileserver\n" );

    try {
        kill();
    }
    catch( const Xc& ) {}
    catch( const exception& ) {}

    for( int i = _fs_array.first_index(); i <= _fs_array.last_index(); i++ ) {
        if( _fs_array[ i ] == this ) {
            _fs_array[ i ] = 0;
            break;
        }
    }
}

//-----------------------------------------------------------------------------Fileserver::kill

void Fileserver::kill()
{
    for( int i = _client_array.last_index(); i >= _client_array.first_index(); i-- )
    {
        kill_client( i );
    }

    _killed = true;
}

//----------------------------------------------------------------------Fileserver::kill_client

void Fileserver::kill_client( int i )
{
    if( _client_array[ i ] )
    {
        try {
            FS_LOG( time_stamp() << " Fileserver::kill_client " );
            FS_LOG( *_client_array[ i ] << '\n' );
            SOS_DELETE( _client_array[ i ] );
        }
        catch( Xc x ) { FS_LOG( time_stamp() << ' ' << x << '\n' ); }
    }
}

//--------------------------------------------------------------------Fileserver::_obj_open_msg

void Fileserver::_obj_open_msg( Open_msg* m )
{
    LOG( "Fileserver::_obj_create_msg( " << m->_name << " )\n" );
    for( Sos_option_iterator opt = m->_name; !opt.end(); opt.next() ) {
        if( opt.with_value( "filename-prefix" ) )  _filename_prefix = opt.value();
        else
        throw_sos_option_error( opt );
    }

    reply_ack_msg( m->source_ptr(), this );
}

//---------------------------------------------------------------------Fileserver::_obj_run_msg

void Fileserver::_obj_run_msg( Run_msg* m )
{
    // Diese Funktion wird ja nicht durchlaufen!
    reply_ack_msg( m->source_ptr(), this );  // ?sollte erst zum Ende des Fileservers abgesendet werden
}

//--------------------------------------------------------------------Fileserver::_obj_data_msg

void Fileserver::_obj_data_msg( Data_msg* m )
{
    if( !_initialized ) {
        _initialized = true;
        Sos_string client_name = as_string( m->source_ptr()->obj_client_name() );      // Hostname
/*
        for( int i = _fs_array.first_index(); i <= _fs_array.last_index(); i++ ) {
            if( _fs_array[ i ]
             && _fs_array[ i ]->_client_name == client_name
             && !_fs_array[ i ]->_killed )
            {
                _fs_array[ i ]->kill();
            }
        }
*/
        _client_name = client_name;
    }

    if( _job_ptr )  { obj_busy(); return; }

    Sos_ptr<Fs_job> job_ptr = SOS_NEW( Fs_job ( this, m ) );

    reply( Ack_msg( m->source_ptr(), this ) );

    _job_ptr = +job_ptr;
    post_request( Data_msg( SOS_CAST( Sos_msg_filter, m->source_ptr() )->obj_reverse_filter(),
                            this, _job_ptr->answer() ) );
}

//---------------------------------------------------------------------Fileserver::_obj_ack_msg

inline void Fileserver::_obj_ack_msg( Ack_msg* m )
{
    if( !_ending ) {
        SOS_DELETE( _job_ptr );
    } else {
        if( _job_ptr ) {
            SOS_DELETE( _job_ptr );
            post_request( End_msg( SOS_CAST( Sos_msg_filter, m->source_ptr() )->obj_reverse_filter(), this ) );
        } else {
            obj_reply_ack();
        }
    }
}

//---------------------------------------------------------------------Fileserver::_obj_end_msg

inline void Fileserver::_obj_end_msg( End_msg* m )
{
    _obj_client_ptr = m->source_ptr();

    if( _job_ptr ) {
        LOG( "Fileserver: Der Job ist noch nicht fertig\n" );
        _ending = true;
    } else {
        post_request( End_msg( SOS_CAST( Sos_msg_filter, m->source_ptr() )->obj_reverse_filter(), this ) );
        _ending = true;
    }
}

//-------------------------------------------------------------------Fileserver::_obj_error_msg

void Fileserver::_obj_error_msg( Error_msg* m )
{
    // Fehler von Sos_reverse_socket.
    // Sollte zum Abbruch der Verbindung führen. End_msg an Sos_reverse_socket senden?

    FS_LOG( "Fileserver: Fehler " << *m << " (wird ignoriert)\n" );
}

//-------------------------------------------------------------------------Fileserver::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Fileserver )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( ack  )
    SOS_DISPATCH_MSG( run  )
    SOS_DISPATCH_MSG( end  )
    SOS_DISPATCH_MSG( open )
    SOS_DISPATCH_MSG( error )
SOS_END_MSG_DISPATCHER

//------------------------------------------------------------------------------Fileserver::add

int Fileserver::add( Fs_client* client )
{
    int i;

    // Alle Fileserver (d.h. alle Verbindungen) nach dem Klientennamen durchsuchen.
    // Jeden gefundenen rauswerden (es ist wohl eine vom PC vergessene Verbindung)

    //FS_LOG( "Fileserver::add( \"" << client->_name << "\" )\n" );
    for( i = _fs_array.first_index(); i <= _fs_array.last_index(); i++ ) {
        Fileserver* fs = _fs_array[ i ];
        if( fs ) {
            for( int j = fs->_client_array.first_index(); j <= fs->_client_array.last_index(); j++ ) {
                Fs_client* c = fs->_client_array[ j ];
                //FS_LOG( "Fs " << i << " Client " << j << ": " << ( c? c_str( c->_name ) : "-" ) << '\n' );
                if( c && c->_sos_client._name == client->_sos_client._name ) {
                    FS_LOG( time_stamp() << " Klient " << c->_sos_client._name << " wird ungültig\n" );
                    if( c->obj_ref_count() != 1 )  LOG( "   obj_ref_count=" << c->obj_ref_count() << '\n' );
                    fs->_client_array[ j ]->close_all_files();
                    SOS_DELETE( fs->_client_array[ j ] );
                    goto kill_ok;
                }
            }
        }
    }
  kill_ok:

    for( i = _client_array.first_index(); i <= _client_array.last_index(); i++ ) {
        if( !_client_array[ i ] )  break;
    }
    if( i > _client_array.last_index() )  _client_array.add_empty();

    _client_array[ i ] = client;

    time( &client->_last_used_time );

    return i;
}

//--------------------------------------------------------------------------Fs_base::client_ptr

Sos_static_ptr<Fs_client> Fileserver::client_ptr( Id application_id, Id user_id )
{
    int index = user_id.index();

    if( application_id.index() != 0
     || index < _client_array.first_index()
     || index > _client_array.last_index() )
    {
        throw_connection_lost_error( "RU02" );  // "ID"
    }

    Fs_client* c = _client_array[ index ];
    if( !c )  throw_connection_lost_error( "RU02" );  // "ID"

    if( user_id.check_value() != c->_check_value )  throw_connection_lost_error( "RU02" );

    time( &c->_last_used_time );

    return c;
}


} //namespace sos
