#define MODULE_NAME "fsfile"
#define COPYRIGHT   "©1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"


#include <limits.h>
#include <stdlib.h>
#include <stdio.h>            // sprintf

#include "../kram/sysdep.h"

#if defined SYSTEM_UNIX
#   include <unistd.h>       // getpid()
#   include <sys/utsname.h>  // uname()
#   include <errno.h>
#endif


#if defined SYSTEM_WIN
#   if !defined SYSTEM_MICROSOFT
#       include <dir.h> // wg. fnsplit
#   endif

#   if defined SYSTEM_WIN16 && !defined SYSTEM_WINDLL
#       include <svwin.h>    // wg. GetModuleFileName
#       include <sysdep.hxx> // wg. Sysdepen::GethInst()
#       ifdef min
#           undef min
#           undef max
#       endif
#   else
//#       if defined SYSTEM_MICROSOFT
//#           include <afx.h>
//#       else
#           include <windows.h>
//#           if defined SYSTEM_WIN16
//#               include <toolhelp.h>
//#           endif
//#       endif
#   endif
#endif

#include "../kram/sosstrng.h"

#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosarray.h"
#include "../kram/sosfact.h"
#include "../kram/sosprof.h"
#include "../kram/sosopt.h"
#include "../file/absfile.h"
//#include "../kram/sossock.h"        // Sos_socket::my_host_name()

#include "../kram/sosstrea.h"
#include "../fs/rapid.h"

#include "fsfile.h"

#if !defined __POINTER_H
//#   include <pointer.h>
#endif

#if !defined __SOSSTREA_H
#   include "../kram/sosstrea.h"
#endif

// Was zum Teufel sind enable und disable?
#if defined enable
#   undef enable
#endif

#if defined disable
#   undef disable
#endif


namespace sos {

extern char mvs2iso        [256];  // in ebc2iso.cxx
extern char mvs2iso_german [256];  // in ebc2iso.cxx
extern Byte iso2mvs        [256];  // in iso2ebc.cxx
extern Byte iso2mvs_german [256];  // in iso2ebc.cxx

//-----------------------------------------------------------------------consts

#if defined SYSTEM_WIN  &&  INT_MAX == 32767
    const uint max_block_size =  32000; //recv() funktioniert nicht: 256u + 2u + 32767u + 256u;
#else
    const uint max_block_size =  256u + 2u + 32767u + 256u;
#endif

const int max_fs_key_length = 256;

//----------------------------------------------------------------------Fs_file

struct Fs_file : Abs_file, Has_static_ptr
{
    BASE_CLASS( Abs_file )

    struct Fs_id
    {
        Fs_id() : _id( 0 )  {}
        operator Area()     { return AREA( _id ); }

      private:
        uint4    _id;
    };

    struct Application_id : Fs_id {};
    struct User_id        : Fs_id {};
    struct Fs_file_id     : Fs_id {};


//#  if defined(SYSTEM_WINDLL)
//jz 19.10.96    friend                     _exit_dll               ();     // hostapi.dll, Modul dllmain.cpp
//  friend                      sos_exit               ();     // hostapi.dll, Modul sosfile.cpp
//#  endif
                                Fs_file                ();
                               ~Fs_file                ();

    static const Abs_file_type& static_file_type       ();

    virtual void                open                   ( const char*, Open_mode, const File_spec& );
    virtual void                close                  ( Close_mode = close_normal );

    virtual void                insert                 ( const Const_area& );
    virtual void                store                  ( const Const_area& );
    virtual void                update                 ( const Const_area& );
    virtual void                set                    ( const Key& );
  //virtual void                del                    ();
    virtual void                del                    ( const Key& );
    virtual void                del                    ()   { Base_class::del(); }
    void                        invoke                 ( const Sos_string& proc_name, uint4 pass,
                                                         const Const_area& input, Area* output );

  //virtual File_info           info                   ();
    static void                 erase                  ( const char* filename );

    static Bool                 is_connected           ();
    static void                 enable                 (); // js 3.8.
    static void                 disable                (); // js 4.2.94
    static Bool                 get_msg_text           ( ostream*, const Key& );
    static Bool                 get_msg_text           ( Area*,    const Key& );

  protected:
    virtual void                get_record             ( Area& );
    void                        get_until              ( Area* buffer, const Const_area& until_key );
    virtual void                get_position           ( Area*, const Const_area* until_key );
    virtual void                put_record             ( const Const_area& );
    virtual void                get_record_lock        ( Area&, Record_lock );
    virtual void                get_record_key         ( Area&, const Key& );

  private:
    enum Burst_mode { no_burst_mode, getburst_mode, putburst_mode };

                                Fs_file                ( const Fs_file& );         // Keine Kopie möglich

    void                       _get_record             ( Area*, Bool read_pos, const Const_area* until_key );
    void                        error_in_burstbuffer   ();

    static void                 init                   ();
    static void                 exit                   ();


    Fill_zero                  _zero_;
    Bool                       _german_text;

    void                       _get_burst              ( const Const_area* until_key );
    void                       _read_position_from_burst_buffer();
    void                       _put_key                ( const Const_area&, const Rapid::Parflput::Flags& );
    void                       _clear_burst_buffer     ( Burst_mode );
    void                       _read_burst_buffer      ();
    void                       _del                    ( const Key& );

    Fs_connection*             _fs_connection_ptr;
    Rapid::Task_connection*    _fs_task_ptr;
    Byte*                      _burst_buffer_ptr;
    Sos_binary_istream         _burst_input_stream;
    Sos_binary_ostream         _burst_output_stream;
    Bool                       _reading_pos;           // getburst liest nur Positionen
    int4                       _get_block_size;        // Dynamische Blockgröße beim Lesen

    Bool                       _opened;                // False, wenn Fileserver bei EOF und _closed_at_eof Datei selbstständig geschlossen hat.
    Bool                       _closed_at_eof;         // in seq: FS schließt Datei bei EOF selbstständig.
    Bool                       _eof;
    Bool                       _opened_for_input;
    Bool                       _get_seq_only;          // Nur sequentielles Lesen erlaubt.
    Bool                       _get_burst_with_key;    // Getburst-Antwort hat für jeden Satz seinen Schlüssel (so nicht bei mehrdeutigen Schlüssel)
    Bool                       _unique_key;            // Schlüssel ist eindeutig
    Bool                       _set_key;               // set() aufgerufen, für !_get_burst_with_key
    Bool                       _is_text; // automatische Umsetzung
  //Fs_file*                   _lib_ptr;
    Fs_file_id                 _fs_file_id;
    Rapid::Openpar::Opflags    _opflags;
    Record_length              _record_length;
    Dynamic_area               _position;

    Burst_mode                 _burst_mode;
    Bool                       _dont_block;
    Bool                       _holder;
    int4                       _read_block_size;
};

//----------------------------------------------------------------Fs_file_type

struct Fs_file_type : Abs_file_type
{
    virtual const char*         name            () const { return "fs"; }
    virtual const char*         alias_name      () const { return "nuc"; }
    virtual void                erase           ( const char* filename ) {
        Fs_file::erase( filename );
    }

    virtual Sos_ptr<Abs_file>   create_base_file() const
    {
        Sos_ptr<Fs_file> o = SOS_NEW( Fs_file );
        return +o;
    }
};

const Fs_file_type    _fs_file_type;
const Abs_file_type&   fs_file_type = _fs_file_type;

//--------------------------------------------------------------------Fs_file::get_msg_text
/*
Bool Fs_file::get_msg_text( Area* area_ptr, const Key& key )
{
try {
    if ( !Fs_file::is_connected() ) return false;

    try {
        area_ptr->allocate_min( 256 );
    }
    catch(...) {
        try {
            area_ptr->allocate_min( 80 );
        }
        catch(...) {}
    }

    if ( key.length() > 32 ) return false;

    char buf[32];
    memset( buf, ' ', sizeof buf );
    Area _key( buf, sizeof buf );
    _key.assign( key );

    Fs_file msg;
    msg.open( "msg: ", File_base::Open_mode(File_base::in), File_spec() );
    _key.length( msg.key_length() );
    msg.get( *area_ptr, _key );
    msg.close();

    return true;
}
catch(...)
{
    return false; // ignorieren
}
}
*/
//--------------------------------------------------------------------Fs_connection

struct Fs_connection : Sos_object
{
                                Fs_connection           ( const Sos_string& server, const Sos_string& user, const Sos_string& pass );
                               ~Fs_connection           ();

    void                        enable                  ();
    void                        disable                 ();
    void                        close_connection        ();
    void                        close_holder            ();
    void                        erase                   ( const Sos_string& filename );
  //void                        rename                  ( const char* old, const char* );

    Rapid::Task_connection*     fs_task_ptr             ();
    void                        read_answer             ();

    friend ostream&             operator <<             ( ostream&, const Fs_connection& );

    Fill_zero                  _zero_;
    Bool                       _enabled;
    Sos_object_ptr             _conn_ptr;
    Fs_file::Application_id    _application_id;
    Fs_file::User_id           _user_id;
    char                       _fs_version[ 6 ];
    Bool                       _io_error;               // Socket-Fehler, Verbindung ist unbrauchbar

    void                       _obj_print               ( ostream* s ) const  { *s << "Fs_connection( \"" << _server << ", " << _user_name << "\" )"; }

    Sos_string                 _server;
    Sos_string                 _user_name;
    Sos_string                 _password;
    Sos_ptr<Rapid_task_connection> _fs_task_ptr;
    int                        _ref_count;

    char*                      _ebc2iso;
    char*                      _ebc2iso_german;
    Byte*                      _iso2ebc;
    Byte*                      _iso2ebc_german;
};

//------------------------------------------------------------------------------------Fs_common

struct Fs_common : Sos_self_deleting
{
                                Fs_common               ();
                               ~Fs_common               ();

    void                        init                    ();

    Sos_string                  normalized_server_name  ( const Sos_string& );
    static Sos_string           normalized_user_name    ( const Sos_string& );

    Fs_connection*              exists_connection       ( const Sos_string& server,
                                                          const Sos_string& user,
                                                          const Sos_string& password );
    Fs_connection*              connection              ( const Sos_string& server, const Sos_string& user,
                                                          const Sos_string& password );

    Fill_zero                  _zero_;
  //Bool                       _german_text;
    Sos_string                 _default_server;
    int4                       _block_size;
    int4                       _read_block_size;
    Sos_simple_array< Sos_ptr<Fs_connection> >  _conn_array;
};

DEFINE_SOS_STATIC_PTR( Fs_common )

//----------------------------------------------------------------------------fs_connection_ptr
/*
inline Fs_connection* fs_connection_ptr()
{
    return (Fs_connection*)+sos_static_ptr()->_fs_conn_ptr;
}
*/
//---------------------------------------------------------AppName-Fktn------

#if defined SYSTEM_WIN
    char _app_name[_MAX_FNAME];
#endif

//----------------------------------------------------------------------------GetInstanceOfTask
#if defined SYSTEM_WIN16DLL

HINSTANCE GetInstanceOfTask() {
  HTASK task = GetCurrentTask();
  TASKENTRY te;
  te.dwSize = sizeof(te);
  BOOL bResult = TaskFirst(&te);
  while (bResult) {
    if (te.hTask == task) return te.hInst;
    bResult = TaskNext(&te);
  };
  // Task nicht gefunden
  return 0;
}

#endif
//-------------------------------------------------------------------------------------app_name

static Sos_string app_name()
{
#   if 0
        return Sos_socket::my_host_name();
#    else
#       if defined SYSTEM_WIN
#           if defined SYSTEM_WIN16  &&  defined SYSTEM_STARVIEW
                HINSTANCE hInst = Sysdepen::GethInst(); // StarView abhaengig !!!
#            else
#               if defined SYSTEM_WIN16
                    HINSTANCE hInst = GetInstanceOfTask();
#                else
                    extern HINSTANCE _hinstance;
                    HINSTANCE hInst = _hinstance;
#               endif
#           endif

            if( !hInst )  return "WIN";

            Dynamic_area buffer ( 260 );
            buffer.length( GetModuleFileName( hInst, buffer.char_ptr(), buffer.size() ) );
#           if defined __BORLANDC__
                fnsplit( buffer.char_ptr(), 0, 0, _app_name, 0 );
                return _app_name;
#           else
                const char* p = buffer.char_ptr() + buffer.length();
                const char* q = p;
                while( p > buffer.char_ptr()  &&  p[-1] != '/'  &&  p[-1] != '\\'  &&  p[-1] != ':' )  p--;
                while( q > p  &&  q[-1] != '.' )  q--;
                if( q > p  &&  q[-1] == '.' ) q--;
                int l = min( sizeof _app_name - 1, uint( q - p ) );
                memcpy( _app_name, p, l );
                _app_name[ l ] = '\0';
                return _app_name;
#           endif
#       elif defined SYSTEM_UNIX
            static /*!!!!*/ struct utsname utsname;
            int rc = uname( &utsname );
            if( rc == -1 )  throw_errno( errno, "uname" );//return "unknown";

            utsname.nodename[ 8 ] = '\0';
            return utsname.nodename;
#        else
            return "PC "; // der alte String
#       endif
#   endif
}

//------------------------------------------------------------------------------------user_name

static const char* user_name()
{
#   if defined SYSTEM_DOS || defined SYSTEM_WIN || defined SYSTEM_WINDLL
        static char  user_name [ 100+1 ];  // Lang genug für den längsten Namen, z.b. "Herr Fehrmann" (Dia Nielsen). jz 7.6.99
        char*        p = "unknown";

#       if defined SYSTEM_WIN32
            DWORD size = sizeof user_name;
            int rc = GetUserName( user_name, &size );
            if( rc ) {
                // LOGs für Fehrmann, Dia-Nielsen 7.6.99
                LOG( "fs: GetUserName() liefert \"" << user_name << "\"\n" );
                p = user_name;
            } else {
                try {
                    throw_mswin_error( GetLastError(), "GetUserName" );
                }
                catch( const Xc& ) {}   // Nur fürs Log
                p = "unknown";
            }
#       else
            int GETENV_USERNAME; // Für Warnung
            p = getenv( "USERNAME" );
            if( !p )  p = getenv( "USER" );
            if( !p )  p = "UNKNOWN";
#       endif

        strncpy( user_name, p, 8 ); // abgeschnitten !!!
        //sprintf( user_name, "%04X", (uint)sos_static_ptr()->_htask );
        //strncat( user_name, p, 4 );
        user_name[ 8 ] = '\0';
        LOG( "fs: user_name()=\"" << user_name << "\"\n" );
        return user_name;
#    else
        static char user_name [ 17 + 1 ];
        itoa( getpid(), user_name, 10 );
        user_name[ 8 ] = '\0';
        return user_name;
#   endif
}

//-----------------------------------------------------------------Fs_connection::Fs_connection

Fs_connection::Fs_connection( const Sos_string& server, const Sos_string& user, const Sos_string& pass )
:
    _zero_(this+1)
{
    _server = server;
    _user_name = user;    //LOG( "Fs_connection _user_name=\"" << _user_name << "\"\n" );
    _password = pass;

    try {
        _conn_ptr = sos_factory_ptr()->create( _server );
    }
    catch( const Not_exist_error& )
    {
        throw_not_exist_error( "SOS-1175" );
    }

    _fs_task_ptr = SOS_NEW( Rapid_task_connection( _conn_ptr ) );
}

//----------------------------------------------------------------Fs_connection::~Fs_connection

Fs_connection::~Fs_connection()
{
    LOGI( '~' << *this << '\n' );

    try {
        disable();
        SOS_DELETE( _fs_task_ptr );
    }
    catch( const Xc& x )
    {
        LOG_ERR( "~Fs_connection: " << x );
        return;   // TCP-Verbindung wird nicht abgebaut!
    }

    try {
        _conn_ptr->obj_end();
        SOS_DELETE( _conn_ptr );
    }
    catch( const Xc& x )
    {
        LOG_ERR( "~Fs_connection: " << x );
    }
}

//--------------------------------------------------------------------operator << Fs_connection

ostream& operator << ( ostream& s, const Fs_connection& c )
{
    s << "Fs_connection(" << c._server << ',' << c._user_name;
    if( c._io_error )  s << ",io_error";
    if( c._fs_task_ptr  &&  c._fs_task_ptr->connection_lost() )  s << ",connection lost";
    s << ')';

    return s;
}

//-------------------------------------------------------------------Fs_connection::fs_task_ptr

Rapid::Task_connection* Fs_connection::fs_task_ptr()
{
    if( !this )  throw_xc( "Fs_connection::fs_task_ptr", "this == NULL (jz 12.11.96)" );
    if( _io_error || _fs_task_ptr->connection_lost() )  throw_connection_lost_error( "SOS-1172" );
    return _fs_task_ptr;
}

//-------------------------------------------------------------------Fs_connection::read_answer

void Fs_connection::read_answer()
{
    try {
        fs_task_ptr()->read_answer();
    }
    catch( const Data_error& )
    {
        _io_error = true;
        throw;
    }
}

//-------------------------------------------------------------------------Fs_connection::erase

void Fs_connection::erase( const Sos_string& filename )
{
    Rapid::Erasepar erasepar;

    if( !_enabled )  enable();

    if( length( filename ) + 1 > sizeof erasepar._eraname_string0 )  throw_syntax_error( "D104" );

    strcpy( erasepar._eraname_string0, c_str( filename ) );

    Sos_binary_ostream* os = fs_task_ptr()->ostream_ptr();
    fs_task_ptr()->call( "ERASE" );
    write_bytes   ( os, &_application_id, sizeof _application_id );
    write_bytes   ( os, &_user_id       , sizeof _user_id        );
    write_erasepar( os, erasepar );
    _fs_task_ptr->write_record();

    read_answer();
    _fs_task_ptr->istream_ptr()->read_end();
}

//------------------------------------------------------------------------Fs_connection::enable

void Fs_connection::enable()
{
    Sos_string appname = app_name();

    fs_task_ptr()->call( "ENABLE" );

    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    write_string_ebcdic( os, c_str( appname ), 8 );
    write_string_ebcdic( os, c_str( _user_name ) , 8 );     //LOG( "fs enable: _user_name=\"" << _user_name << "\"  ebcdic=" << hex << Const_area( os->space(0) - 8, 8 ) << dec << '\n' );
    write_string_ebcdic( os, "2.0C06"   , 6 );
    write_string_ebcdic( os, "PC"       , 8 );
    _fs_task_ptr->write_record();

    read_answer();

    Sos_binary_istream* is = _fs_task_ptr->istream_ptr();
    is->read_fixed( &_application_id, sizeof _application_id );
    is->read_fixed( &_user_id       , sizeof _user_id );
    is->read_fixed( _fs_version     , sizeof _fs_version );
    is->read_end();

    xlat( _fs_version, _fs_version, sizeof _fs_version, ebc2iso );

    if( _fs_version[ sizeof _fs_version - 1 ] == 'M' ) {      // MVS-Fileserver?
        _ebc2iso = mvs2iso;  _ebc2iso_german = mvs2iso_german;
        _iso2ebc = iso2mvs;  _iso2ebc_german = iso2mvs_german;
    } else {
        _ebc2iso = ebc2iso;  _ebc2iso_german = ebc2iso_german;
        _iso2ebc = iso2ebc;  _iso2ebc_german = iso2ebc_german;
    }

    _enabled = true;
}

//-----------------------------------------------------------------------Fs_connection::disable

void Fs_connection::disable()
{
    if( ! _enabled )  return;

    if( _io_error )  return;
    if( _fs_task_ptr->connection_lost() )  return;

    try {
        fs_task_ptr()->call( "DISABLE" );
        Sos_binary_ostream* os = fs_task_ptr()->ostream_ptr();
        os->write_fixed( &_application_id, sizeof _application_id );
        os->write_fixed( &_user_id       , sizeof _user_id );
        _fs_task_ptr->write_record();

        _enabled = false;

        read_answer();
        _fs_task_ptr->istream_ptr()->read_end();
    }
    catch( const Connection_lost_error& x )
    {
        LOG_ERR( *this << ": " << x << " wird bei DISABLE ignoriert\n" );
    }
}

//--------------------------------------------------------------Fs_connection::close_connection

void Fs_connection::close_connection()
{
    try {
        disable();
    }
    catch( const Xc& x ) 
    {
        LOG_ERR( "Fs_connection::close_connection() Fehler bei disable ignoriert: " << x << '\n' );
    }

    Fs_common* com = sos_static_ptr()->_fs_common;
    for( int i = com->_conn_array.first_index(); i <= com->_conn_array.last_index(); i++ ) {
        Fs_connection* conn = this; // wg. internal compiler error (msvc++6sp2)
        if( conn == +com->_conn_array[ i ] )  {
            SOS_DELETE( com->_conn_array[ i ] );  // == delete this!
            // this ist ungültig!
            com->_conn_array[ i ] = com->_conn_array[ com->_conn_array.last_index() ];
            com->_conn_array.last_index( com->_conn_array.last_index() - 1 );
            break;
        }
    }
}

//------------------------------------------------------------------Fs_connection::close_holder

void Fs_connection::close_holder()
{
    LOG( "Fs_connection::close_holder()  ref_count=" << _ref_count << '\n' );

    if( _ref_count == 0 )
    {
        close_connection();   // ruft ~Fs_connection!
    }
}

//-------------------------------------------------------------------------Fs_common::Fs_common

Fs_common::Fs_common()
:
    _zero_(this+1)
{
    _conn_array.obj_const_name( "Fs_common::_conn_array" );
}

//------------------------------------------------------------------------------Fs_common::init

void Fs_common::init()
{
    _default_server = read_profile_string( "", "fs", "server" );
    if( empty( _default_server ) )  _default_server = read_profile_string( "", "fileclient", "server" );

    //_german_text = read_profile_bool( "", "fs", "german", false );

    //_block_size      = read_profile_int( "", "fs", "block-size", 0 );
    _read_block_size = read_profile_uint( "", "fs", "read-block-size", 1300 ); //default: < TCP-Blocksize //jz 18.5.97max_block_size );
}

//------------------------------------------------------------------------Fs_common::~Fs_common

Fs_common::~Fs_common()
{
    LOG( "~Fs_common: Fileserver-Verbindungen werden abgebaut\n" );
}

//------------------------------------------------------------Fs_common::normalized_server_name

Sos_string Fs_common::normalized_server_name( const Sos_string& server_ )
{
    Sos_string server = server_;

    if( empty( server ) )
    {
        if( empty( _default_server ) )  throw_xc( "SOS-1142" );
        server = _default_server;
    }

    if( !strchr( c_str( server ), ':' )
     && !strchr( c_str( server ), '/' ) ) {
        server = read_existing_profile_string( "", "fs alias", c_str( server ) );
    }

    if( !strchr( c_str( server ), ':' )
     && !strchr( c_str( server ), ' ' ) )
    {
        server = "tcp " + server;   // tcp ist default
    }

    if( strnicmp( c_str( server ), "tcp ", 4 ) == 0
     || strnicmp( c_str( server ), "tcp:", 4 ) == 0 )
    {
        server = Sos_string( "tcp " ) + ( c_str( server ) + 4 );
        if( !strstr( c_str( server ), " -sam3" ) )  server += " -sam3";
    }

    return server;
}

//--------------------------------------------------------------Fs_common::normalized_user_name

Sos_string Fs_common::normalized_user_name( const Sos_string& user )
{
    if( empty( user ) )  return user_name();
                   else  return user;

}

//-----------------------------------------------------------------Fs_common::exists_connection

Fs_connection* fs_exists_connection( const Sos_string& server_, const Sos_string& user_,
                                     const Sos_string& password )
{
    if( !sos_static_ptr()->_fs_common ) return NULL;

    return sos_static_ptr()->_fs_common->exists_connection( server_, user_, password );
}

//-----------------------------------------------------------------Fs_common::exists_connection

Fs_connection* Fs_common::exists_connection( const Sos_string& server_, const Sos_string& user_,
                                             const Sos_string& /*password*/ )
{
    // Liefer eine eine neue Verbindung oder eine vorhandene, 
    // wenn die Angaben übereinstimmen und sie fehlerlos ist.

    Sos_string      server;
    Sos_string      user;
    Fs_connection*  c = NULL;
	int             i;

    server = normalized_server_name( server_ );
    user   = normalized_user_name( user_ );

    for( i = _conn_array.first_index(); i <= _conn_array.last_index(); i++ )
    {
        c = _conn_array[ i ];

        if(  c
         && !c->_io_error
         && !c->_fs_task_ptr->connection_lost()
         &&  c->_server    == server
         &&  c->_user_name == user               )  break;

        LOG("Fs_common::exists_connection " << server << " != " << *c << '\n' );
    }

    return i <= _conn_array.last_index()? c : (Fs_connection*)NULL;
}

//------------------------------------------------------------------------Fs_common::connection

Fs_connection* Fs_common::connection( const Sos_string& server_, const Sos_string& user_,
                                      const Sos_string& password )
{
    Sos_string server;
    Sos_string user;

    server = normalized_server_name( server_ );
    user   = normalized_user_name( user_ );

    Fs_connection* conn = exists_connection( server, user, password );

    if( !conn ) {
        _conn_array.add( SOS_NEW( Fs_connection( server, user, password ) ) );
        conn = _conn_array[ _conn_array.last_index() ];
    }

    return conn;
}

//-------------------------------------------------------------Fs_file::Fs_file

Fs_file::Fs_file()
:
    _zero_              ( this+1),
    _burst_mode         ( no_burst_mode ),
    _read_block_size    ( 0 )
{
}

//------------------------------------------------------------Fs_file::~Fs_file

Fs_file::~Fs_file()
{
    if( _opened ) {
        try {
            close();
        } 
        catch( const Xc& x ) { LOG_ERR( "Fehler in ~Fs_file wird ignoriert: " << x ); }
    }
    SOS_FREE( _burst_buffer_ptr );

    if( _fs_connection_ptr ) {
        if( _holder )  _fs_connection_ptr->close_holder();
        _fs_connection_ptr->_ref_count--;
        _fs_connection_ptr = NULL;
    }
}

//--------------------------------------------------------------Fs_file::init

#if defined SYSTEM_SOLARIS
//extern Bool __is_sos_user_name_enabled();
#endif
//----------------------------------------------------------------Fs_file::open

void Fs_file::open( const char* fn, Open_mode open_mode, const File_spec& file_spec )
{
    Rapid::Fd       fd;
    Sos_ptr<Rapid_openpar> openpar = SOS_NEW( Rapid_openpar );  // Ein bisschen Stack sparen
    int4            blksize;
    Sos_string      filename;
    Sos_string      fs_name;
    Sos_string      user_name = ::sos::user_name();
    Sos_string      password;

    _german_text = read_profile_bool( "", "fs", "german", false );

    for( Sos_option_iterator opt ( fn ); !opt.end(); opt.next() )
    {
        if( opt.flag      ( "noblock" ) )      _dont_block = true;
        else
        if( opt.with_value( "read-block-size" ) ) _read_block_size = opt.as_uintK();
        else
        if( opt.with_value( 's', "server" ) )  fs_name = opt.value();
/* Für die L-Bank darf der Benutzer nicht einstellbar sein! jz 11.5.98
        else
        if( opt.with_value( "user" ) )         user_name = opt.value();
        else
        if( opt.with_value( "password" ) )     password = opt.value();
*/
        else
        if( opt.flag      ( "german" ) )       _german_text = opt.set();
        else
        if( opt.flag      ( "binary" ) )       if( opt.set() )  open_mode = Open_mode( open_mode | binary );
                                                          else  open_mode = Open_mode( open_mode & ~binary );
        else
        if( opt.flag      ( "holder" ) )       _holder = opt.set();
        else
        if( opt.pipe()                 )  { filename = opt.rest(); break; }
        else
        throw_sos_option_error( opt );
    }

    // Marke für SOSSQL überspringen:
    if( length( filename ) > 4  &&  memcmp( c_str( filename ), "-+- ", 4 ) == 0 ) {
        Sos_string fn = filename;
        filename = c_str( fn ) + 4;
    }

    const char* file_name = c_str( filename );

    if( file_name[ 0 ] == '(' ) {
        int pos = position( file_name + 1, ')' );
        if( file_name[ 1 + pos ] != ')' )  throw_xc( "D104" );
        fs_name = as_string( file_name + 1, pos );
        file_name += 1 + pos + 1;
    }
/*
    if( file_spec._field_type_ptr || file_spec._need_type ) {
        insert_record_tabbed = true;
        filename = "tabbed/record " + filename;
    }
*/
    if( !sos_static_ptr()->_fs_common ) {
        Sos_ptr<Fs_common> c = SOS_NEW( Fs_common );
        c->init();
        sos_static_ptr()->_fs_common = +c;
    }

    // Baut ggfs. neue Verbindung auf:
    _fs_connection_ptr = sos_static_ptr()->_fs_common->connection( fs_name, user_name, password );
    _fs_connection_ptr->_ref_count++;

    _fs_task_ptr = _fs_connection_ptr->fs_task_ptr();

    if( !_fs_connection_ptr->_enabled ) {
        _fs_connection_ptr->enable();
    }

    if( _holder  &&  empty( file_name ) )  return;


    if( _read_block_size == 0             )  _read_block_size = sos_static_ptr()->_fs_common->_read_block_size;
    if( _read_block_size > max_block_size )  _read_block_size = max_block_size;
    if( _read_block_size < 1000           )  _read_block_size = 1000;


    ///// Open modus und flags:

    if( (open_mode & (in|out)) == (in|out) ) fd._fdattr.fdinout( true );
    else
    if( open_mode & in  )       fd._fdattr.fdin( true );
    else
    if( open_mode & out )       fd._fdattr.fdout( true );

    if( open_mode & trunc)      openpar->_opflags.opnew( true );

    if( (open_mode & out)  &&  ! (open_mode & (ate|app|in)))
                                openpar->_opflags.opnew( true );

    if( open_mode & seq      )  fd._fdattr.fdseq( true );

    if( open_mode & nocreate )  openpar->_opflags.opexist( true );
                          else  openpar->_opflags.opcreat( true );

    if( open_mode & noreplace ) ;
                          else  openpar->_opflags.opow( true );

    if( open_mode & binary )    _is_text                = false;
                          else  _is_text                = true;
/*
    if ( _is_text )
    {
        char buf[50];
        Area area( buf, sizeof buf - 1 );
        const char* german_str = "ebcdic-german";
        int german_len = strlen( german_str );
        area.length(0);

        read_profile_entry( &area, "", "fox mail", "bs2000-charset" );
#if defined SYSTEM_SOLARIS
        if ( area.length() == 0 ) read_profile_entry( &area, "/etc/sos.ini", "foxmail", "bs2000-charset" );
#endif
        if ( area.length() == german_len && strncmpi( area.char_ptr(), german_str, german_len ) == 0 )
        {
           _german_text = true;
        }
    }
*/
  //LOG( "Fs_file " << ( _is_text? ( _german_text ? "text (german): " : "text (iso): " ) : "binary: " ) << (const char*) file_name << endl )

    //if( strlen( file_name ) + 1 > sizeof openpar->_opname_string0 )  throw_syntax_error( "D104" );
    //strcpy( openpar->_opname_string0, file_name );

    int l  = strlen( file_name );
    int l0 = min( l, (int)sizeof openpar->_opname_string0 - 1 );
    memcpy( openpar->_opname_string0, file_name, l0 );
    openpar->_opname_string0[ l0 ] = '\0';
    int l_rest = 0;
    if( l0 < l ) {
        l0--;  // Platz für '\x01'
        l_rest = l - l0;
        openpar->_opname_string0[ l0     ] = 0x01;    // Rest des Dateinamens folgt
        openpar->_opname_string0[ l0 + 1 ] = '\0';
    }


    if( openpar->_opname_string0[ 0 ] == ' '
     && openpar->_opname_string0[ 1 ] == ' ' )
    {
        openpar->_opname_string0[ 0 ] = '_';
    }

    {
        char* p = openpar->_opname_string0 + position( openpar->_opname_string0, ' ' );
        while( *p == ' ' )  p++;
        if( memcmp( p, "-noblock", 8 ) == 0 ) {
            memset( p, ' ', 8 );
            _dont_block = true;
        }
    }

    fd.fdkp( _key_pos < 0? 0x8000 : _key_pos );
    fd.fdkl( _key_len );
    fd._fdattr.fddupky( file_spec._key_specs._key_spec._duplicate );


    _get_seq_only = fd._fdattr.fdseq() && ( fd._fdattr.fdin() | fd._fdattr.fdinout() );  // wie fs.cxx
    _closed_at_eof = _get_seq_only;
  //LOG( "Fs_file(" << openpar->_opname_string0 << ")\n"
  //     "    _closed_at_eof=" << (int)_closed_at_eof << "\n" );

    blksize = _get_seq_only? _read_block_size //max_block_size
                           : 0;

    _fs_task_ptr->call("OPEN2");
    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    write_bytes  ( os, &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    write_bytes  ( os, &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id );
    write_fd     ( os, fd );
    write_openpar( os, *openpar );
    write_int4   ( os, blksize );
    if( l_rest ) {
        write_int4   ( os, 0       );  // reserve
        write_int2   ( os, l_rest  );  // reserve
        write_bytes  ( os, file_name + l0, l_rest );
    }
    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    Sos_binary_istream* is = _fs_task_ptr->istream_ptr();
    read_fd     ( is, &fd );
    read_openpar( is, openpar );
    read_bytes  ( is, &_fs_file_id, sizeof _fs_file_id );

    if( blksize ) {
        _clear_burst_buffer( getburst_mode );
        _read_burst_buffer();
    }

    is->read_end();

    _key_pos = fd.fdkp();   if( (uint2)_key_pos == 0x8000 )  _key_pos = -1;
    _key_len = fd.fdkl();
    _any_file_ptr->_spec._key_specs._key_spec._key_length   = _key_len;   //? jz 17.5.97
    _any_file_ptr->_spec._key_specs._key_spec._key_position = _key_pos;   //? jz 17.5.97
    _any_file_ptr->_spec._key_specs._key_spec._duplicate    = fd._fdattr.fddupky();

    if( _key_len ) {
        _unique_key = !fd._fdattr.fddupky();
        _position.allocate( _key_len );
        _position.length( _key_len );
        memset( _position.ptr(), 0, _key_len );
        current_key_ptr( &_position );
    }

    _get_burst_with_key = _unique_key;

    _opened = true;
    if( !( open_mode & out ) )  _opened_for_input = true;

/*
    if( insert_record_tabbed ) {
        insert_file( "record/tabbed:" );
    }
*/
}

//------------------------------------------------Fs_file::error_in_burstbuffer

void Fs_file::error_in_burstbuffer()
{
    static const char hex[] = "0123456789ABCDEF";

    if( log_ptr ) 
    {
        *log_ptr << "*** fsfile: Datenfehler im Burst_buffer ***";

        for( const Byte* p = _burst_input_stream._buffer_ptr; p < _burst_input_stream._end_ptr; p++ ) 
        {
            if( ( p-_burst_input_stream._buffer_ptr) % 64 == 0 )  *log_ptr << '\n'; 
            *log_ptr << hex[ *p >> 4 ] << hex[ *p & 0x0F ]; 
        }
        
        *log_ptr << '\n';
    }

    _fs_connection_ptr->_io_error = true;
    
    throw_connection_lost_error( "SOS-1165" );
}

//---------------------------------------------------------------Fs_file::close

void Fs_file::close( Close_mode close_mode )
{
    if( !_fs_connection_ptr )  return;

    if( _opened  &&  _closed_at_eof ) 
    {
        // Burstbuffer zuende lesen, um zu prüfen, ob Datei geschlossen ist
        if( _burst_mode == getburst_mode  &&  _burst_buffer_ptr )
        {
            while( 1 )
            {
                uint2 len; // js 09.11.99

                if( _burst_input_stream.rest_length() == 0 )  { _opened = false; break; } // js 9.11.99
                if( _get_burst_with_key ) 
                {
                   
                    if( _burst_input_stream.rest_length() < _key_len )  goto FEHLER;  //jz 23.6.98
                    _burst_input_stream.skip_bytes( _key_len );
                    
                    if( _burst_input_stream.rest_length() == 0 )  break;
                }
                else
                {
                    if(  _burst_input_stream.rest_length() == 1
                      && _burst_input_stream.read_byte() == 0xFF )  break;  // Folgeblock ohne Schlüssel
                }

                if( _burst_input_stream.rest_length() < 2 )  goto FEHLER;
                len = _burst_input_stream.read_uint2();    // Satzlänge
               
                if( len > _burst_input_stream.rest_length() ) {
FEHLER:
                    _opened = false;
                    error_in_burstbuffer();
                }

                if( _burst_input_stream.rest_length() < len )  goto FEHLER;
                _burst_input_stream.skip_bytes( len );
            }
        }
    }

    if( close_mode == close_normal ) {
        _clear_burst_buffer( no_burst_mode );
        SOS_FREE( _burst_buffer_ptr );
    }

    if( !_fs_connection_ptr->_io_error  && !_fs_connection_ptr->_fs_task_ptr->connection_lost() ) 
    {
        if( _opened ) {
            _opened = false;

            _fs_connection_ptr->fs_task_ptr()->call( "CLOSE" );
            Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
            write_bytes( os, &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
            write_bytes( os, &_fs_connection_ptr->_user_id, sizeof _fs_connection_ptr->_user_id );
            write_bytes( os, &_fs_file_id, sizeof _fs_file_id );
            _fs_task_ptr->write_record();

            if( _opened_for_input ) {       // _opened ist nicht immer richtig gesetzt, RU03 ignorieren
                try {
                    _fs_connection_ptr->read_answer();
                    _fs_task_ptr->istream_ptr()->read_end();
                }
                catch( const Xc& ) {}
            } else {
                _fs_connection_ptr->read_answer();
                _fs_task_ptr->istream_ptr()->read_end();
            }
        }
    }

    Fs_connection* conn_ptr = _fs_connection_ptr;

    _fs_connection_ptr->_ref_count--;
    _fs_connection_ptr = NULL;

    if( _holder )  conn_ptr->close_holder();
}

//--------------------------------------------------------------------------Fs_file::get_record

void Fs_file::get_record( Area& area )
{
    _get_record( &area, false, NULL );
}

//---------------------------------------------------------------------------Fs_file::get_until

void Fs_file::get_until( Area* buffer, const Const_area& until_key )
{
    _get_record( buffer, false, &until_key );
}

//-----------------------------------------------------------------------Fs_file::get_position

void Fs_file::get_position( Area* buffer, const Const_area* until_key )
{
    if( _get_seq_only ) {
        Base_class::get_position( buffer, until_key );
    } else {
        _get_record( buffer, true, until_key );
    }
}

//----------------------------------------------------------Fs_file::get_record

void Fs_file::_get_record( Area* buffer, Bool read_pos, const Const_area* until_key )
{
    if( _eof )  throw_eof_error();   // Nur bei streng sequentiellem Lesen (seq)

    try {
        buffer->length( 0 );

        uint      len;
        uint      l;
        Bool      truncated = false;

        // EOF abfangen bei Null-Dateien
        if( _burst_mode == getburst_mode
         && _burst_buffer_ptr
         && _burst_input_stream.rest_length() == 0 )  throw_eof_error( "D310" );

        if( _burst_mode != getburst_mode  ||  _reading_pos != read_pos ) {
            _reading_pos = read_pos;
            _get_burst( until_key );
        }

        //? _burst_buffer_ptr->begin_transaction(); // next_ptr sichern

        _read_position_from_burst_buffer();

        if( _get_burst_with_key  &&  _reading_pos  &&  _key_len == 1 )  _burst_input_stream.skip_bytes( 1 );  // Füllbyte zur EOF-Erkennung (0xFF)

      //if( ( _key_len == 0 || _reading_pos )  jz 18.5.97
        if( ( !_get_burst_with_key || _reading_pos )
         && _burst_input_stream.rest_length() == 1
         && _burst_input_stream.read_byte() == 0xFF )
        {
            _get_burst( until_key );
            _read_position_from_burst_buffer();
        }
        else
        if( _burst_input_stream.end() )
        {
            //if( _key_len == 0 )  throw_eof_error();  jz 18.5.97
            if( !_get_burst_with_key )  throw_eof_error();
            _get_burst( until_key );
            _read_position_from_burst_buffer();
        }

        if( read_pos ) {
            buffer->assign( _position );
        } else {
            len = _burst_input_stream.read_uint2();    // Satzlänge

            if( len > _burst_input_stream.rest_length() )  error_in_burstbuffer();

            try {
                buffer->allocate_min( len );
            }
            catch( const Xc& ) {}

            l = min( len, buffer->size() );

            if( _is_text ) {
                xlat( buffer->char_ptr(), _burst_input_stream.read_bytes( l ), l, 
                      _german_text ? _fs_connection_ptr->_ebc2iso_german : _fs_connection_ptr->_ebc2iso );
            } else {
                _burst_input_stream.read_fixed( buffer->ptr(), l );
            }

            buffer->length( l );

            if( l < len ) {
                //LOG( "fsfile:: truncated, l=" << l << ", len=" << len << endl )
                truncated = true;
                _burst_input_stream.skip_bytes( len - l );
            }

            if( truncated )   throw_too_long_error( "D320" );
        }
    }
    catch( const Eof_error& )
    {
        if( _closed_at_eof ) {
            _eof = true;
            _opened = false; // damit ein close noch geht!
        }
      //_burst_mode = no_burst_mode;
        throw;
    }
}

//----------------------------------------------------Fs_file::_read_position_from_burst_buffer

void Fs_file::_read_position_from_burst_buffer()
{
    //if( _key_len ) {  jz 18.5.97
    if( _get_burst_with_key ) {
        if( _burst_input_stream.rest_length() < _key_len )   throw_eof_error();
        _burst_input_stream.read_fixed( _position.ptr(), _key_len );
    }
}

//--------------------------------------------------------------------------Fs_file::_get_burst

void Fs_file::_get_burst( const Const_area* until_key )
{
    /// Anfängliche Blockgröße setzen:
    if( _get_block_size == 0 )  {
        _get_block_size = _get_seq_only? _read_block_size//max_block_size
                                       : ( _reading_pos? 60 * _key_len  // MS-ACCESS liest etwa 60? Sätze auf einmal
                                                       : _read_block_size );  // mindesten 1 Satz mit max 32KB muß hineinpassen!
    }

    _clear_burst_buffer( getburst_mode );

    _fs_connection_ptr->fs_task_ptr()->call( "GETBRST2" );

    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();

    write_bytes         ( os, &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    write_bytes         ( os, &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
    write_bytes         ( os, &_fs_file_id    , sizeof _fs_file_id     );
    write_int4          ( os, _get_block_size );

    Bool with_position = _get_burst_with_key || _set_key;
    write_int4          ( os,   ( _reading_pos  * 0x01000000L )
                              + ( with_position * 0x02000000L )
                              + 0 );    // count

    if( with_position ) {
        _set_key = false;
        write_bytes         ( os, _position.ptr() , _key_len );    // Anfangsschlüssel

        if( until_key  &&  !_get_seq_only ) {
            write_bytes( os, until_key->byte_ptr(), _key_len );    // Endeschlüssel
        } else {
            write_byte_repeated ( os, 0xFF            , _key_len );    // Endeschlüssel
        }
    } else {
        // Fileserver kennt die Position des nächsten Satzes
    }

    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    _read_burst_buffer();

    /// Blockgröße für nächsten sequentiellen Zugriff verdoppeln:
    //if( _get_block_size < 19000/2 )
    _get_block_size = min( 2 * _get_block_size, max_block_size );
}

//--------------------------------------------------------------------------Fs_file::put_record

void Fs_file::put_record( const Const_area& area )
{
    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "put" );

    _fs_connection_ptr->fs_task_ptr();  // Connection lost?

    if( _burst_mode != putburst_mode ) {
        _clear_burst_buffer( putburst_mode );  // Burst-Buffer zuruecksetzen
    } else {
        if ( 2 + area.length() > _burst_output_stream.space_left() ) {
            _clear_burst_buffer( putburst_mode );
        }
    }

    if( 2 + area.length() > _burst_output_stream.space_left() )  throw_too_long_error( "D420" );

    _burst_output_stream.write_uint2( area.length() );

    if( _is_text ) {
        xlat( _burst_output_stream.space( area.length() ), area.char_ptr(), area.length(), 
              _german_text ? _fs_connection_ptr->_iso2ebc_german : _fs_connection_ptr->_iso2ebc );
    } else {
        _burst_output_stream.write_fixed_area( area );
    }

    if( _dont_block ) {
        _clear_burst_buffer( no_burst_mode );
    }
}

//----------------------------------------------------------------------Fs_file::get_record_key

void Fs_file::get_record_key( Area& area, const Key& key )
{
    Rapid::Parflget::Flags getflags;
  //Byte            key_buffer [ file_max_key_length ];
  //const void*     key_ptr;

    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "getkey" );

    if( key.length() == 0 )  throw_xc( "SOS-1351" );

    if( _key_len ) {
        if( key.length() != _key_len )  throw_xc( "SOS-1228", key.length(), _key_len );
    } else {
        // get_key, ohne dass die Sätze Schlüssel haben.
        // get_key wird vom Dateitreiber (i.d.R. einem Zugriffsmodul) interpretiert,
        // bei Gößwein als Suchanfrage.
        // Die Schlüssellänge ist in diesem Fall variable (bis 256).
        // set_key ist nicht möglich!
        if( key.length() <= 0  ||  key.length() > max_fs_key_length )  throw_xc( "SOS-1344", key.length(), max_fs_key_length );
    }

    if( _is_text ) {
        xlat( _position.ptr(), key.byte_ptr(), _key_len, 
              _german_text ? _fs_connection_ptr->_iso2ebc_german : _fs_connection_ptr->_iso2ebc );
    } else {
        memcpy( _position.ptr(), key.byte_ptr(), _key_len );
    }

    _clear_burst_buffer( no_burst_mode );

    getflags.getkey( true );

    _fs_connection_ptr->fs_task_ptr()->call( "GETKEY" );
    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    write_bytes         ( os, &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    write_bytes         ( os, &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
    write_bytes         ( os, &_fs_file_id    , sizeof _fs_file_id     );
    write_parflget_flags( os,  getflags );
    write_bytes         ( os,  _position.byte_ptr(), _key_len );
    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    Sos_binary_istream* is = _fs_task_ptr->istream_ptr();
    area.allocate_min( is->rest_length() );
    area.length( is->rest_length() );
    is->read_fixed( area.ptr(), area.length() );
    is->read_end();

    if ( _is_text ) {
        xlat( area.char_ptr(), area.char_ptr(), area.length(), 
              _german_text ? _fs_connection_ptr->_ebc2iso_german : _fs_connection_ptr->_ebc2iso );
    }
}

//--------------------------------------------------------------------------Fs_file::get_record

void Fs_file::get_record_lock( Area& area, Record_lock )
{
    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "getlock" );

    get_record( area );
}

//------------------------------------------------------------------------------Fs_file::insert

void Fs_file::insert( const Const_area& area )
{
    Rapid::Parflput::Flags flags;

    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "insert" );

    flags.putkey( true );
    flags.putins( true );
    _put_key( area, flags );
}

//-------------------------------------------------------------------------------Fs_file::store

void Fs_file::store( const Const_area& area )
{
    Rapid::Parflput::Flags flags;

    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "store" );

    flags.putkey( true );
    _put_key( area, flags );
}

//------------------------------------------------------------------------------Fs_file::update

void Fs_file::update( const Const_area& area )
{
    Rapid::Parflput::Flags flags;

    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "update" );

    flags.putkey( true );
    flags.putupd( true );
    _put_key( area, flags );
}

//---------------------------------------------------------------------------Fs_file::_put_key

void Fs_file::_put_key( const Const_area& area, const Rapid::Parflput::Flags& flags )
{
    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "putkey" );

    _clear_burst_buffer( no_burst_mode );

    _fs_connection_ptr->fs_task_ptr()->call( "PUTKEY" );
    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    write_bytes( os, &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    write_bytes( os, &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
    write_bytes( os, &_fs_file_id    , sizeof _fs_file_id     );
    write_parflput_flags( os, flags );

    if( area.length() > os->space_left() )  throw_too_long_error();

    if( _is_text ) {
        xlat( os->space( area.length() ), area.char_ptr(),
              area.length(), _german_text ? _fs_connection_ptr->_iso2ebc_german : _fs_connection_ptr->_iso2ebc );
    } else {
        os->write_fixed_area( area );
    }

    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    _fs_task_ptr->istream_ptr()->read_end();
}

//-------------------------------------------------------------------------------------Fs_file::set

void Fs_file::set( const Key& key )
{
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "setkey" );

    if( key.length() == 0 )  throw_xc( "SOS-1351" );
    if( key.length() != _key_len )  throw_xc( "SOS-1228", key.length(), _key_len );

    _clear_burst_buffer( no_burst_mode );

    if( key.length() == 0 )  return;  // Vorsichtshalber, falls _key_len == 0

    assert( _position.ptr() ); //?????

    if( _is_text ) {
        xlat( _position.ptr(), key.ptr(), _key_len, 
              _german_text? _fs_connection_ptr->_iso2ebc_german : _fs_connection_ptr->_iso2ebc );
    } else {
        memcpy( _position.ptr(), key.ptr(), _key_len );
    }

    _set_key = true;  // Für _get_burst(), wenn !_get_burst_with_key
}

//-----------------------------------------------------------------Fs_file::del
/*
void Fs_file::del()
{
    _del( Key( _position_ptr ) ); // _position_ptr ist im EBCDIC-Format
}
*/
//-----------------------------------------------------------------Fs_file::del

void Fs_file::del( const Key& key )
{
    Byte key_buffer [ file_max_key_length ];

    _set_key = false;
    if( _get_seq_only )  throw_xc( "SOS-1352", this, "del" );

    if( key.length() == 0 )  throw_xc( "SOS-1351" );

    if( _is_text ) {
        xlat( key_buffer, key.char_ptr(), key.length(), 
              _german_text ? _fs_connection_ptr->_iso2ebc_german : _fs_connection_ptr->_iso2ebc );
        _del( Const_area( key_buffer, key.length() ));
    } else {
        _del( key );
    }
}


//---------------------------------------------------------------Fs_file::_del

void Fs_file::_del( const Key& key )
{
    if( _get_seq_only )  throw_xc( "SOS-1352", this );

    _clear_burst_buffer( no_burst_mode );

    _fs_connection_ptr->fs_task_ptr()->call( "DELKEY" );
    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    os->write_fixed( &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    os->write_fixed( &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
    os->write_fixed( &_fs_file_id    , sizeof _fs_file_id     );
    os->write_fixed_area( key );
    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    _fs_task_ptr->istream_ptr()->read_end();
}

//------------------------------------------------------------------------------Fs_file::invoke

void Fs_file::invoke( const Sos_string& proc_name, uint4 pass,
                      const Const_area& input, Area* output )
{
    char pn [ 32 ];

    if( _get_seq_only )  throw_xc( "SOS-1352", this, "invoke" );

    if( length( proc_name ) > sizeof pn )  throw_xc( "FS-procname", c_str( proc_name ), "zu lang" );
    memset( pn, 0x40, sizeof pn );
    xlat( pn, c_str( proc_name ), length( proc_name ), _fs_connection_ptr->_iso2ebc );

    _clear_burst_buffer( no_burst_mode );

    _fs_connection_ptr->fs_task_ptr()->call( "INVOKE" );
    Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
    os->write_fixed( &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
    os->write_fixed( &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
    os->write_fixed( &_fs_file_id                        , sizeof _fs_file_id     );
    os->write_fixed( pn                                  , sizeof pn );
    os->write_uint4( pass );
    os->write_uint4( 0 );           // Reserve
    os->write_fixed_area( input );
    _fs_task_ptr->write_record();

    _fs_connection_ptr->read_answer();
    Sos_binary_istream* is = _fs_task_ptr->istream_ptr();
    is->skip_bytes( 4 );  // reserve
    output->allocate_min( is->rest_length() );
    output->length( is->rest_length() );
    is->read_fixed( output->ptr(), output->length() );
    is->read_end();
}

//----------------------------------------------------------------Fs_file::info
/*
File_info Fs_file::info()
{
    return File_info( "", "", Key_specs( Key_spec( key_position(), key_length() )));
}
*/
//---------------------------------------------------------------Fs_file::erase

void Fs_file::erase( const char* param )
{
    Sos_string      filename;
    Sos_string      fs_name;
    Sos_string      user_name = ::sos::user_name();
    Sos_string      password;
    Fs_connection*  fs_connection;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if( opt.with_value( 's', "server" ) )  fs_name = opt.value();
        else
        if( opt.with_value( "user" ) )         user_name = opt.value();
        else
        if( opt.with_value( "password" ) )     password = opt.value();
        else
        if( opt.pipe()                 )       filename = opt.rest();
        else
        throw_sos_option_error( opt );
    }

    if( !::sos::sos_static_ptr()->_fs_common ) {
        Sos_ptr<Fs_common> c = SOS_NEW( Fs_common );
        c->init();
        ::sos::sos_static_ptr()->_fs_common = +c;
    }

    // Baut ggfs. neue Verbindung auf:
    fs_connection = ::sos::sos_static_ptr()->_fs_common->connection( fs_name, user_name, password );
    if( !fs_connection->_enabled )  fs_connection->enable();

    fs_connection->erase( filename );
/*
    int ERASE_MIT_FS_NAMEN_IMPLEMENTIEREN;
    throw_xc( "Fs_file::erase", "erase über Fileserver noch nicht implementiert" );

    if( !::sos_static_ptr()->_fs_conn_ptr ) {
        ::sos_static_ptr()->_fs_conn_ptr = SOS_NEW( Fs_connection );
    }

    ((Fs_connection*)+::sos_static_ptr()->_fs_conn_ptr )->erase( filename );
*/
}

//-------------------------------------------------------Fs_file::_clear_burst_buffer

void Fs_file::_clear_burst_buffer( Burst_mode burst_mode )
{
try {
    if( _burst_mode == putburst_mode ) {
        if( _burst_output_stream.length() ) {
            _fs_connection_ptr->fs_task_ptr()->call( "PUTBURST" );
            Sos_binary_ostream* os = _fs_task_ptr->ostream_ptr();
            os->write_fixed( &_fs_connection_ptr->_application_id, sizeof _fs_connection_ptr->_application_id );
            os->write_fixed( &_fs_connection_ptr->_user_id       , sizeof _fs_connection_ptr->_user_id        );
            os->write_fixed( &_fs_file_id    , sizeof _fs_file_id     );
            os->write_fixed( _burst_buffer_ptr, _burst_output_stream.length() );
            _fs_task_ptr->write_record();

            _fs_connection_ptr->read_answer();
            _fs_task_ptr->istream_ptr()->read_end();
         }
    }

    _burst_input_stream.reset( 0 , 0 );
    _burst_output_stream.reset( 0 , 0 );

    if( burst_mode != getburst_mode )  _get_block_size = 0;    // wächst dynamisch, s. get_burst()

    if( burst_mode == no_burst_mode ) {
        SOS_FREE( _burst_buffer_ptr );
    }
    else
    {
        if( !_burst_buffer_ptr ) {
            _burst_buffer_ptr = (Byte*)sos_alloc( max_block_size, "fs burst buffer" );
        }

        if( burst_mode == putburst_mode ) {
            _burst_output_stream.reset( _burst_buffer_ptr, max_block_size );
        }
    }

    _burst_mode = burst_mode;
}
catch( const Xc& )
{
    _burst_mode = no_burst_mode;
    throw;
}
}

//----------------------------------------------------------------------Fs_file::_read_burst_buffer

void Fs_file::_read_burst_buffer()
{
    Sos_binary_istream* is = _fs_task_ptr->istream_ptr();
    uint burst_length = is->rest_length();
    if( burst_length > max_block_size )  {
        _fs_connection_ptr->_io_error = true;
        throw_connection_lost_error( "SOS-1165" );
    }

    is->read_fixed( _burst_buffer_ptr, burst_length );
    is->read_end();

    _burst_input_stream.reset( _burst_buffer_ptr, burst_length );

    _burst_mode = getburst_mode;
}


} //namespace sos
