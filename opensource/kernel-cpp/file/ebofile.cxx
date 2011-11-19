// ebofile.cxx
// Joacim Zschimmer 3.1.00

#include "precomp.h"
#include "../kram/sysdep.h"

#if defined SYSTEM_WIN || defined SYSTEM_LINUX

#if defined SYSTEM_UNIX
#   include <dlfcn.h>
#endif
    
#include <stdio.h>
#include <ctype.h>

extern "C"
{
    // Include-Pfade: .../prod/misc/ebo/ebodbs-include und .../prod/misc/ebo/ecbsys-include
#   include "../misc/ebo/ecbsys-include/ecbext.h"
#   include "../misc/ebo/ebodbs-include/ebomax.h"     /* Maximalwerte bei EBO */
#   include "../misc/ebo/ecbsys-include/ecbport.h"    /* Datentypen           */
#   include "../misc/ebo/ecbsys-include/ecberror.h"   /* Fehler               */
#   include "../misc/ebo/ecbsys-include/ecbmemex.h"   /* Fehler               */
#   include "../misc/ebo/ebodbs-include/dberror.h"
#   include "../misc/ebo/ebodbs-include/ecbleaex.h"
}

typedef void (__cdecl *Leasy_function)( pTEXT op, pTEXT re, pTEXT db, pTEXT ar, pTEXT fa, pTEXT si );


#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/soslimtx.h"
#include "../kram/sosprof.h"
#include "../kram/log.h"
#include "../kram/env.h"

#ifdef SYSTEM_MICROSOFT
#   define snprintf _snprintf
#endif

namespace sos {
using namespace std;


const int max_si_segments = 16;
const int max_key_length  = 256;
const int max_buffer_size = 32768;

struct Ebo_static;

//-------------------------------------------------------------------------------------Leasy_re

struct Leasy_re
{
                                Leasy_re                ()                  : _zero_(this+1) {}

    void                        check_rc                ( Sos_object_base* error_object = NULL );

    Fill_zero                  _zero_;
    Sos_limited_text<4>        _rc_lc;                  // Enthält den Rückgabewert der EBO-DBS-Funktion
    char                       _ope_om;                 // Enthält den OPEN/USAGE-Modus bei OPTR
    Sos_limited_text<8>        _redb;                   // Enthält den Namen der zuletzt bearbeiteten Datenbankdatei
    char                       _ope1;                   // Enthält ergänzende Angaben für die Funktionen OPTR, CLTR, RDIR, RHLD, LOCK
    char                       _ope2;                   // Enthält ergänzende Angaben für die Funktionen OPTR, CLTR, RDIR, RHLD, LOCK
    Sos_limited_text<5>        _rc_lce;                 // Enthält den erweiterten Rückgabewert der EBO-DBS-Funktion
};

//-------------------------------------------------------------------------------------Ebo_file

struct Ebo_file : Abs_file
{
    struct Segment
    {
                                Segment                 ()      : _offset(0), _len(0) {}

        uint                   _offset;                 // Beginnend bei 0. Satzlängenfeld wird mitgezählt.
        uint                   _len;
    };


                                Ebo_file                ();
                               ~Ebo_file                ();

  //void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );

    void                        get_record              ( Area& );
    void                        get_record_key          ( Area&, const Key& );
    void                        get_position            ( Area*, const Const_area* until_key );
    void                        set                     ( const Key& );
  //void                        rewind                  ( Key::Number = 0 );

    void                        transfer_read_data      ( Byte*, Area*, Bool key_only );
    void                        write_key               ( Byte* buffer, const Const_area& key );

    static void                 leasy                   ( const char* op, Leasy_re* re, const char* db, Byte* ar = NULL, const char* fa = NULL, const char* si = NULL );
    static void                 leasy2                  ( const char* op, Leasy_re* re, const char* db, Byte* ar = NULL, const char* fa = NULL, const char* si = NULL );

    Fill_zero                  _zero_;
    Sos_string                 _db;
    int                        _fixed_length;
    Bool                       _mainitem_only;          // FA=MAINITEM (nur Schlüssel lesen)
    Sos_string                 _fa;                     // Feldauswahl
    Segment                    _pi;                     // Primärindex
    Sos_simple_array<Segment>  _si;                     // Sekundärindex und Primärindex
    Sos_string                 _si_name;
    Dynamic_area               _position;               // Aktuelle Position in der Datei
    Ebo_static*                _static;
    Ebo_file*                  _tail;
};

//Bool Ebo_file::initialized = false;

//-----------------------------------------------------------------------------------Ebo_static

struct Ebo_static : Sos_self_deleting
{
                                Ebo_static              ();
                               ~Ebo_static              ();

    Fill_zero                  _zero_;
    Bool                       _transaction_open;       // OPTR durchgeführt
    Ebo_file*                  _list;
    bool                       _env_host_set;
    bool                       _env_port_set;
};

DEFINE_SOS_STATIC_PTR( Ebo_static );


//--------------------------------------------------------------------------------Ebo_file_type

struct Ebo_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "ebo"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Ebo_file> f = SOS_NEW( Ebo_file );
        return +f;
    }
};

const Ebo_file_type            _ebo_file_type;
const Abs_file_type&            ebo_file_type = _ebo_file_type;

//---------------------------------------------------------------------------------------static

static Bool                     log_data;
static Leasy_re                 re;
static Sos_string               open_catalog;
static Bool                     single_transaction = false;

//---------------------------------------------------------------------------Leasy_re::check_rc

void Leasy_re::check_rc( Sos_object_base* error_object )
{
    if( _rc_lc != "L000" ) 
    {
        Sos_limited_text<16> text = "EBO-";
        text.append( _rc_lc );

        if( _rc_lc == "L001" ) 
        {
            throw_not_found_error( c_str( text ), error_object );
        }
        else
        if( _rc_lc == "L003" ) 
        {
            throw_eof_error( c_str( text ), error_object );
        }
        else
        {
            Sos_limited_text<50> text2 = "RC-LC=";
            text2.append( _rc_lc );
            text2.append( ",RC-LCE=" );
            text2.append( _rc_lce );
            throw_xc( c_str( text ), c_str( text2 ) );
        }
    }
}

//-----------------------------------------------------------------------------------upper_case

static Sos_string upper_case( const Sos_string& string )
{
    Dynamic_area result;
    
    result.allocate( length( string ) );
    
    for( int i = 0; i < (int)length( string ); i++ ) {
        result.char_ptr()[ i ] = toupper( string[ i ] );
    }

    return as_string( result.char_ptr(), length( string ) );
}

//-----------------------------------------------------------------------Ebo_static::Ebo_static

Ebo_static::Ebo_static()
:
    _zero_(this+1)
{
    string str;

    set_environment_from_sos_ini();

    if( log_ptr )  log_data = read_profile_bool( "", "ebo", "log-data", false );

    if( !getenv( "EBODBS_HOST" ) )
    {
        string host = read_profile_string( "", "ebo", "host" );
        if( !host.empty() ) 
        {
            static char env[101];
            snprintf( env, sizeof env, "EBODBS_HOST=%s", host.c_str() );
            putenv( env );
            LOG( "EBO: putenv " << env << '\n' );
            _env_host_set = true;
        }
    }

    if( !getenv( "EBODBS_PORT" ) )
    {
        string port = read_profile_string( "", "ebo", "port" );
        {
            static char env[101];
            snprintf( env, sizeof env, "EBODBS_PORT=%s", port.c_str() );
            putenv( env );
            LOG( "EBO: putenv " << env << '\n' );
            _env_port_set = true;
        }
    }
}

//----------------------------------------------------------------------Ebo_static::~Ebo_static

Ebo_static::~Ebo_static()
{
    if( !empty( open_catalog ) ) 
    {
        if( _transaction_open ) {
            re._ope1 = 'R';     // Rollback
            re._ope2 = ' ';
            re._ope_om = ' ';
            Ebo_file::leasy( "CLTR", &re, "", NULL, NULL, NULL );    
            // Hier Fehler LS05 ignorieren
            _transaction_open = false;
        }

        Ebo_file::leasy( "CATD", &re, "", NULL, NULL, NULL );
        open_catalog = "";
    }

    if( _env_host_set )  { putenv( "EBODBS_HOST" ); LOG( "EBO: putenv EBODBS_HOST\n" ); }
    if( _env_port_set )  { putenv( "EBODBS_PORT" ); LOG( "EBO: putenv EBODBS_PORT\n" ); }
}

//------------------------------------------------------------------------------Ebo_file::leasy

void Ebo_file::leasy2( const char* op, Leasy_re* re, const char* db, Byte* ar, const char* fa, const char* si )
{
    static Leasy_function leasy_function;
    char    re_buffer   [80];
    char    db_buffer   [4096];
    char    si_buffer   [8];

    memset( re_buffer, 0, sizeof re_buffer );
    memset( re_buffer +  4, ' ', 4 );  strncpy( re_buffer +  4, c_str( re->_rc_lc  ), 4 );
    memset( re_buffer + 52, ' ', 8 );  strncpy( re_buffer + 52, c_str( re->_redb   ), 8 );
    memset( re_buffer + 74, ' ', 5 );  strncpy( re_buffer + 74, c_str( re->_rc_lce ), 5 );
    re_buffer[17] = re->_ope_om;
    re_buffer[69] = re->_ope1;
    re_buffer[70] = re->_ope2;

    memset( db_buffer, ' ', 12 );
    int db_len = length( db );
    if( db_len > (int)sizeof db_buffer - 2 )  throw_too_long_error( "SOS-EBO-101", db_len, sizeof db_buffer - 2 );
    memcpy( db_buffer, db, db_len );

    if( empty( si ) )  si = NULL;
    if( si ) {
        memset( si_buffer, ' ', sizeof si_buffer );
        strncpy( si_buffer, si, sizeof si_buffer );
    }

    if( log_ptr ) {
        *log_ptr << "LEASY " << op << " ope_om=" << re->_ope_om << " ope1=" << re->_ope1 << " ope2=" << re->_ope2;
        if( db )  *log_ptr << " db=" << db;
        if( fa )  *log_ptr << " fa=" << fa;
        if( si )  *log_ptr << " si=" << si;

        if( log_data  &&  ( strcmp(op,"RDIR") == 0  ||  strcmp(op,"SETL") == 0 ) )
        {
            int len = 200;
            while( len > 0  &&  ar[len-1] == 0 )  len--;
            LOG( "\n      ar=" );
            for( int i = 0; i < len; i++ )  *log_ptr << ( (ar[i] & 0x7F) < 0x20? '.' : ar[i] ) << ' ';
            *log_ptr << "\n         " << hex << Const_area( ar, len ) << dec << '\n';
        }

        *log_ptr << flush;
    }

    if( !leasy_function )
    {
#       if defined SYSTEM_UNIX
            static void* ebo_module;

            if( !ebo_module ) 
            {
                string libebo_so = read_profile_string( "", "ebo", "libebo.so", "libebosock.so" );
                LOG( "dlopen(\"" << libebo_so << "\",RTLD_LAZY)\n" );
                ebo_module  = dlopen( libebo_so.c_str(), RTLD_LAZY );
            }

            if( !ebo_module )  throw_xc( "DLOPEN", dlerror() );

            LOG( "dlsym(\"ecb_leasy\")\n" );
            leasy_function = (Leasy_function) dlsym( ebo_module, "ecb_leasy" );
            if( !leasy_function )  throw_xc( "DLSYM", dlerror(), "ecb_leasy" );

            //dlclose( ebo_module ). Nicht schließen, denn sonst wird das Modul entladen.
 
#        else
            leasy_function = ecb_leasy;
#       endif
    }

    leasy_function( (char*)op, re_buffer, db_buffer, (char*)ar, (char*)fa, si? si_buffer : NULL );

    re->_rc_lc .assign( re_buffer +  4, length_without_trailing_spaces( re_buffer +  4, 4 ) );
    re->_redb  .assign( re_buffer + 52, length_without_trailing_spaces( re_buffer + 52, 8 ) );
    re->_rc_lce.assign( re_buffer + 74, length_without_trailing_spaces( re_buffer + 74, 5 ) );
    re->_ope_om = re_buffer[17];
    re->_ope1   = re_buffer[69];
    re->_ope2   = re_buffer[70];

    LOG( "  //  rc_lc=" << re->_rc_lc << " rc_lce=" << re->_rc_lce << '\n' );

    if( log_data  &&  op[0] == 'R'  &&  log_ptr )
    {
        int len = 200;
        while( len > 0  &&  ar[len-1] == 0 )  len--;
        LOG( "      ar=" );
        for( int i = 0; i < len; i++ )  *log_ptr << ( (ar[i] & 0x7F) < 0x20? '.' : ar[i] ) << ' ';
        *log_ptr << "\n         " << hex << Const_area( ar, len ) << dec << '\n';
    }
}

//------------------------------------------------------------------------------Ebo_file::leasy

void Ebo_file::leasy( const char* op, Leasy_re* re, const char* db, Byte* ar, const char* fa, const char* si )
{
    leasy2( op, re, db, ar, fa, si);

    if( strcmp( c_str(re->_rc_lc), "LS05" ) == 0 )   // LS05: EBO-Server ist neu gestartet worden.
    {
        if( strcmp( op, "OPTR" ) == 0 )     // Nur bei OPTR Verbindung neu aufbauen, sonst Fehler LS05 durchlassen
        {
            string catalog = open_catalog;

            // LS05: Verbindung zum neu gestarteten EBO-Server aufbauen

            Leasy_re my_re = *re;
            my_re._ope1 = ' ';
            my_re._ope2 = ' ';
            leasy2( "CATD", &my_re, "", NULL, NULL, NULL );     // Erst die alte Verbindung abbauen
            catalog = "";

            leasy2( "CATD", &my_re, catalog.c_str(), NULL, NULL, NULL );
            my_re.check_rc();
            open_catalog = catalog;

            leasy2( op, re, db, ar, fa, si);
        }
    }
}

//---------------------------------------------------------------------------Ebo_file::Ebo_file

Ebo_file::Ebo_file()
:
    _zero_(this+1)
{
}

//--------------------------------------------------------------------------Ebo_file::~Ebo_file

Ebo_file::~Ebo_file()
{
    Ebo_file** f = &_static->_list; 

    while( *f  &&  *f != this )  f = &(*f)->_tail;
    if( f )  *f = _tail;
}

//-------------------------------------------------------------------------------Ebo_file::open

void Ebo_file::open( const char* parameter, Open_mode open_mode, const File_spec& spec )
{
    LOGI( "Ebo_file::open " << parameter << "\n" );

    string     catalog;
    Sos_string pi;          // Primärindex
    Sos_string si;          // Sekundärindex

    if( open_mode & out )  throw_xc( "D127" );      // Nur Lesen möglich

    if( !sos_static_ptr()->_ebo )  {
        Sos_ptr<Ebo_static> p = SOS_NEW( Ebo_static );
        sos_static_ptr()->_ebo = +p;
    }
    _static = sos_static_ptr()->_ebo;

    for( Sos_option_iterator opt( parameter ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "catalog" ) )  catalog = upper_case( opt.value() );
        else
        if( opt.flag( "singleta" ) )  single_transaction = opt.set();
        else
        if( opt.with_value( "fa" ) )  _fa = upper_case( opt.value() );           // fa=MAINITEM
        else
        if( opt.with_value( "pi" )                                  )  pi = opt.value();                // pi=offset/len
        else
      //if( opt.param()  &&  strnicmp( opt.value(), "pi=", 3 ) == 0 )  pi = c_str( opt.value() ) + 3;   // pi=offset/len
      //else
        if( opt.with_value( "si" ) )  si = opt.value();            // si=name,offset1/len1,offset2/len2,...
        else
        if( opt.with_value( "fixed-length" ) )  _fixed_length = opt.as_uintK();     // Feste Satzlänge der Primärdatei (auch wenn si verwendet wird!)
        else
      //if( opt.param()  &&  empty( _db ) )  _db = upper_case( opt.value() );
        if( opt.param(1) )  _db = upper_case( opt.rest() );
        else throw_sos_option_error( opt );
    }

    single_transaction = true;  // Nicht im Fileserver (sosfs)! (s. RAPIDLEA/FLLEATAA)

    if( !empty( catalog ) ) 
    {
        if( catalog != open_catalog ) 
        {
            if( !empty( open_catalog ) )  throw_xc( "SOS-EBO-102", c_str( catalog ), c_str( open_catalog ) );
    
            leasy( "CATD", &re, c_str( catalog ), NULL, NULL, NULL );
            re.check_rc( this );

            open_catalog = catalog;
        }
    }

    if( _fa == "MAINITEM" )  _mainitem_only = true;

    if( !empty( pi ) ) 
    {
        const char* p0 = c_str( pi );
        const char* p  = strchr( p0, '/' );
        if( !p )  throw_xc( "SOS-EBO-103", p0 );
        _pi._offset = as_uint( as_string( p0, p - p0 ) ) - 1;
        _pi._len    = as_uint( p + 1 );
    }

    if( !empty( si ) ) 
    {
        const char* p0 = c_str( si );
        const char* p  = strchr( p0, ',' );
        if( !p )  throw_xc( "SOS-EBO-104", p0 );

        _si_name = upper_case( as_string( p0, p - p0 ) );
        int len = 0;

        p0 = p + 1;
        while(1) {
            const char* p  = strchr( p0, '/' );
            if( !p )  throw_xc( "SOS-EBO-104", c_str( si ) );
            Segment* s = _si.add_empty();
            s->_offset = as_uint( as_string( p0, p - p0 ) ) - 1;
            p++;
            const char* p1 = strchr( p, ',' );
            if( !p1 )  p1 = p + strlen( p );
            s->_len = as_uint( as_string( p, p1 - p ) );
            len += s->_len;
            if( !*p1 )  break;
            p0 = p1 + 1;
        }

        if( !p )  throw_xc( "SOS-EBO-104", c_str( si ) );

        _si.add( _pi );      // _si[ _si.last_index() ] ist der Primärschlüssel!

        len += _pi._len;

        if( _key_pos > 0 )                        throw_xc( "SOS-EBO-105", _key_pos );
        if( _key_len != 0  &&  _key_len != len )  throw_xc( "SOS-EBO-111", _key_len, len );
        _key_pos = 0;
        _key_len = len;

        if( !empty( _fa ) )  throw_xc( "SOS-EBO-106", c_str( _fa ) );
        _fa = "MAINITEM";
    }
    else
    {
        if( !empty( pi ) )      // _key_pos und _key_len von -pi= übernehmen
        {
            if( _mainitem_only ) {
                if( _key_pos > 0 )  throw_xc( "SOS-EBO-110", _key_pos );
                _key_pos = 0;
            } else {
                int kp = _pi._offset;
                if( _fixed_length == 0 )  kp -= 4;
                if( _key_pos >= 0  &&  _key_pos != kp )  throw_xc( "SOS-EBO-107", _key_pos, kp+1 );
                _key_pos = kp;
            }
            if( _key_len != 0  &&  _key_len != _pi._len )  throw_xc( "SOS-EBO-108", _key_len, _pi._len );
            _key_len = _pi._len;
        }
    }


    if( empty( _fa ) )  _fa = "(ALL)";

    if( _mainitem_only  &&  empty( pi ) )  throw_xc( "SOS-EBO-109" );

    _any_file_ptr->_spec._key_specs._key_spec._key_position = _key_pos;
    _any_file_ptr->_spec._key_specs._key_spec._key_length   = _key_len;

    if( _key_len ) {
        _position.allocate( _key_len );
        _position.length( _key_len );
        memset( _position.ptr(), 0, _key_len );
        current_key_ptr( &_position );
    }

    re._ope_om = '\xFF';    // open modus
    leasy( "OPTR", &re, c_str( _db ) );
    re.check_rc( this );
    _static->_transaction_open = true;

    if( _si.count() )  rewind();

    _tail = _static->_list;
    _static->_list = this;
}

//--------------------------------------------------------------------------Ebo_file::close

void Ebo_file::close( Close_mode )
{
    if( _static->_transaction_open ) {
        re._ope1 = ' ';     // Commit (Autocommit!) 
        re._ope2 = ' ';
        leasy( "CLTR", &re, "", NULL, NULL, NULL );
        // Hier Fehler LS05 ignorieren
        _static->_transaction_open = false;
    }
}

//--------------------------------------------------------------Ebo_file::begin_transaction
/*
void Ebo_file::begin_transaction()
{
    Sos_string db;

    for( Ebo_file* file = sos_static_ptr()->_ebo->_list; file; file = file->_tail )
    {
        if( !file->_transaction_open ) {
            if( length( db ) > 1 )  db += ',';
            db += file->_db;
            file->_transaction_open = true;
        }
    }

    if( length( db ) > 1 ) 
    {
        db += ')';
        re._ope_om = '\xFF';
        leasy( "OPTR", &re, db );
        re->check_rc( this );
    }
}
*/
//---------------------------------------------------------------------Ebo_file::get_record

void Ebo_file::get_record( Area& buffer )
{
    Byte my_buffer [ max_buffer_size ];

    memset( my_buffer, '\x00', sizeof my_buffer );

    leasy( "RNXT", &re, c_str( _db ), my_buffer, c_str( _fa ) );
    re.check_rc( this );

    transfer_read_data( my_buffer, &buffer, _mainitem_only );
}

//-------------------------------------------------------------------Ebo_file::get_position

void Ebo_file::get_position( Area* buffer, const Const_area* )
{
    Byte my_buffer [ max_buffer_size ];

    memset( my_buffer, '\x00', sizeof my_buffer );

    leasy( "RNXT", &re, c_str( _db ), my_buffer, "MAINITEM" );
    re.check_rc( this );

    transfer_read_data( my_buffer, buffer, true );
}

//-----------------------------------------------------------------Ebo_file::get_record_key

void Ebo_file::get_record_key( Area& buffer, const Key& key )
{
  //LOG( "Ebo_file:get_record_key " << key << "\n" );

    Byte        my_buffer [ max_buffer_size ];

  //begin_transaction();

    _position.assign( key );

    memset( my_buffer, '\x00', sizeof my_buffer );
    write_key( my_buffer, key );

    if( _si.count() ) 
    {
        leasy( "SETL", &re, c_str( _db ), my_buffer, c_str( _fa ), c_str( _si_name ) );   // Nur SETL berücksichtigt auch den Primärschlüssel
        re.check_rc( this );

        leasy( "RNXT", &re, c_str( _db ), my_buffer, c_str( _fa ), c_str( _si_name ) );
        re.check_rc( this );

        // Prüfen, ob der gewünschte Satz gelesen worden ist
        const Byte* p     = key.byte_ptr();
        const Byte* p_end = p + key.length();

        for( int i = 0; i <= _si.last_index(); i++ ) 
        {
            Segment* s = &_si[i];
            uint len = min( s->_len, uint( p_end - p ) );
            if( memcmp( my_buffer + s->_offset, p, len ) != 0 )  throw_not_found_error( "D311", this );
            if( len < s->_len )  break;
            p += len;
        }
    } 
    else 
    {
        leasy( "RDIR", &re, c_str(_db), my_buffer, c_str(_fa), c_str(_si_name) );
        re.check_rc( this );
    }

    transfer_read_data( my_buffer, &buffer, _mainitem_only );
}

//-------------------------------------------------------------Ebo_file::transfer_read_data

void Ebo_file::transfer_read_data( Byte* my_buffer, Area* buffer, Bool key_only )
{
    if( _si.count() > 0 )   // Datensatz = Schlüssel, key_only ist also egal
    {
        buffer->allocate_min( _key_len );
        buffer->length( 0 );
        for( int i = 0; i <= _si.last_index(); i++ ) {
            buffer->append( my_buffer + _si[i]._offset, _si[i]._len );
        }
        _position = *buffer;
    }
    else
    {
        const Byte* p;
        int         len;

        if( _fixed_length ) {
            p = my_buffer;
            len = _fixed_length;
        } else {
            p = my_buffer + 4;
            len = ( my_buffer[0] << 8 ) + my_buffer[1] - 4;
        }

        if( key_only )  buffer->assign( p + _key_pos, _key_len );
                  else  buffer->assign( p, len );

        memcpy( _position.ptr(), buffer->byte_ptr() + _pi._offset, _pi._len );
    }
}

//----------------------------------------------------------------------------Ebo_file::set

void Ebo_file::set( const Key& key )
{
    Byte my_buffer [ max_buffer_size ];
 
    _position.assign( key );

    memset( my_buffer, '\x00', sizeof my_buffer );
    write_key( my_buffer, key );

    leasy( "SETL", &re, c_str(_db), my_buffer, c_str(_fa), c_str(_si_name) );
    re.check_rc( this );
}

//----------------------------------------------------------------------Ebo_file::write_key

void Ebo_file::write_key( Byte* my_buffer, const Const_area& key )
{
    if( log_data )  *log_ptr << "Ebo_file key=X'" << hex << key << dec << "\'\n";

    if( _si.count() ) 
    {
        const Byte* p     = key.byte_ptr() + _key_len;
        const Byte* p_end = key.byte_ptr() + key.length();

        for( int i = _si.last_index(); i >= 0; i-- )   // Rückwärts, damit Primärindex nachrangig ist (Eichenauer überlappt gerne seine Schlüssel, jz 10.10.00)
        {
            Segment* s = &_si[i];
            p -= s->_len;
            uint len = s->_len;
            if( p >= p_end )  len = 0; 
            else if( len > p_end - p )  len = p_end - p;
            memcpy( my_buffer + s->_offset, p, len );
            if( len < s->_len )  memset( my_buffer + s->_offset + len, 0, s->_len - len );
        }

        if( log_ptr )
        {
            *log_ptr << "Ebo_file SI=X'" << hex;

            for( int i = 0; i <= _si.last_index(); i++ )
            {
                Segment* s = &_si[i];
                *log_ptr << Const_area( my_buffer + s->_offset, s->_len ) << ' ';
            }

            *log_ptr << dec << "'\n";
        }

        // RDIR nimmt für den Primärschlüssel immer X'00' (SETL verhält sich richtig)
    } 
    else 
    {
        memset( my_buffer + _pi._offset, 0, _pi._len );
        memcpy( my_buffer + _pi._offset, key.ptr(), _pi._len );
        LOG( "Ebo_file PI=X'" << hex << Const_area( my_buffer + _pi._offset, _pi._len ) << dec << "'\n" );
    }
}

} //namespace sos

#endif
