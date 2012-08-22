#include "precomp.h"
//#define MODULE_NAME "licence"
//#define COPYRIGHT   "(c)1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#undef __USELOCALES__    // strftime() ANSI!

#include <ctype.h>
#include <time.h>
#include <stdio.h>

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosarray.h"
#include "../kram/soslimtx.h"
#include "../kram/sosprof.h"
#include "../kram/licence.h"
#include "../kram/log.h"
#include "../kram/sysdep.h"

#ifndef SYSTEM_WIN32
#   include "../kram/sossock.h"    // hostname
#else
#   include <windows.h>
#   include <winreg.h>
#endif

#include "../zschimmer/zschimmer.h"

using namespace std;

namespace sos {

time_t sos_time( time_t* );  // in sysdep.cxx


DEFINE_SOS_DELETE_ARRAY( const char* );

/*
    Inhalt des Lizenzschlüssels:

    Aussteller "SOS"
    Kundenkürzel
    Seriennummer
    Einstellungen: Schlüsselwörter mit jeweils einem Parameter (Text oder int)
    Sicherungscode

    - Ein PC kann mehrere Lizenzschlüssel haben.
    - Die Einstellungen mehrerer Lizenzschlüssel mit der selben Seriennummer werden
      zusammengefügt. Was bei widersprechenden Einstellungen?
    - Bei einem Update bleibt die Seriennummer fest - D.h. die alte Lizenz ist überflüssig.
    - Die selbe Seriennummer darf nicht an zwei PCs verwendet werden.
    - Lizenzschlüssel mit verschiedenen Seriennummern? werden auch zusammengefügt.
    - Nur eine Seriennummer am PC oder mehrere, die dann aber alle nur an diesen einem
      PC erlaubt (Maximum muss es geben, alle SOS-KUNDE-NR müssen in einen Broadcast passen).

    AUFBAU DES LIZENZSCHLÜSSELS:
    aussteller-kunde-seriennr-einstellungen-sicherungscode
    SOS-XBANK-123-1-2-5-B1999-I119961231-9832YHZDMQ
    SOS-XBANK-124-1-2-5-B1999-I119961231-8734YHZDMR

    Der Lizenzschlüssel besteht also aus durch Bindestrich getrennten Teilen, die aus
    Ziffern und großen Buchstaben bestehen.

    Einstellungen und Sicherungscode sind codiert:
    An jeder Stelle ist eines der Zeichen 0-9A-Z.

    Der Sicherungscode nimmt 6 Stellen ein.

    EINSTELLUNGEN:
    Jede Einstellung besteht aus einem Parameternamen und einem Parameterwert, der ein Text
    oder eine Zahl sein kann.

    Ein Parametername ist zweistellig 01 bis ZZ.

    SICHERUNGSCODE:
    Der Sicherungscode nimmt 6 Stellen ein und errechnet sich aus allen anderen Teilen
    (2.176.782.336 Zustände).
*/

//---------------------------------------------------------------------------------------------

//static const Sos_string empty_value_string  = "\x01";         // Nur das erste Zeichen wird geprüft. Gesetzt, aber ohne Wert. "" == nicht gesetzt
static const int        zz                  = base*35 + 35;
static const Sos_date   min_date            ( 1996, 9, 2 );

//const char* Sos_licence::_component_names[ licence_upper_limit + 1 ];

struct Comp_text
{
    int         _code;
    const char* _text;
    const char* _zz_licence_value;  // NULL, wenn nicht im Sammel-Code ZZ enthalten
}
comp_text[] =
{
    { licence_hostapi               , "hostAPI"             , ""    },      // 2
    { licence_hostdde               , "hostDDE"             , ""    },      // 3
    { licence_hostole               , "hostOLE"             , ""    },      // 4
    { licence_hostodbc              , "hostODBC"            , ""    },      // 5
    { licence_hostphp               , "hostPHP"             , ""    },      // 6
    { licence_scheduler             , "Scheduler"           , ""    },      // 7
    { licence_soscopy               , "hostcopy"            , ""    },      // 8
    { licence_e9750                 , "e9750"               , ""    },      // 9
    { licence_sossql                , "sossql"              , ""    },      // A
    { licence_letter                , "Letter"              , ""    },      // B
    { licence_licence_key           , "Lizenzgenerator"     , NULL  },      // 10
    { licence_fs                    , "fs"                  , ""    },      // 11
    { licence_e370                  , "e370"                , ""    },      // 12
    { licence_hostapi_hostdde       , "hostAPI & hostDDE"   , NULL  },      // 13
    { licence_fra2rtf               , "frame2rtf"           , ""    },      // 14
    { licence_saacke                , "Saacke"              , NULL  },      // 15   Wird für scheduler.database.password verwendet, siehe scheduler/spooler.cxx
  //{ licence_saacke                , "Saacke"              , ""    },      // 15
    { licence_foxgate               , "FOXgate"             , ""    },      // 17
    { licence_scheduler_database_password, "scheduler.database.password", NULL }, // 18
    { licence_os                    , "Betriebssystem"      , NULL  },      // 1A  "W" Windows, "S" Solaris
  //{ licence_odbc_blob             , "ODBC Blob"           , NULL  },      // 1B
    { licence_fs_demo               , "FS demo"             , ""    },      // 16
    { licence_verfallsdatum_2000    , "Verfallsdatum"       , NULL  },      // 20
    { licence_scheduler_console     , "Job Scheduler Console", ""   },      // 21
    { licence_scheduler_agent       , "Job Scheduler Agent"  , ""   },      // 22

  //{ licence_wbrz_logos_to_loga    , "LOGOS to LOGA2001"   , ""    },      // 30
    { licence_kis                   , "KIS"                 , ""    },      // 31
    { licence_sisy                  , "SISY"                , ""    },      // 32
    { licence_saacke_winword        , "Saacke hostAPI"      , ""    },      // 33
};
/*
SOS_INIT( licence )
{
    for( int i = 0; i < NO_OF( comp_text ); i++ ) {
        Comp_text* c = &comp_text[ i ];
        Sos_licence::_component_names[ c->_code ] = c->_text;
    }
}
*/
/*
struct Sos_licence_entry
{
    int                        _param;
    const char*                _value;
};

static Sos_licence_entry super_licence [] =         // Parameter ZZ für SOS GmbH
{
  //{ licence_os                    , "WS" },       // 1  "W" Windows, "S" Solaris,    "N" NT?
    { licence_hostapi               , "" },         // 2
    { licence_hostdde               , "" },         // 3
    { licence_hostole               , "" },         // 4
    { licence_hostodbc              , "" },         // 5
    { licence_hostphp               , "" },         // 6
    { licence_spooler               , "" },         // 7
    { licence_soscopy               , "" },         // 8
    { licence_e9750                 , "" },         // 9
    { licence_sossql                , "" },         // A
    { licence_letter                , "" },         // B
  //{ licence_licence_key           , "" },         // 10
    { licence_fs                    , "" },         // 11
    { licence_e370                  , "" },         // 12
  //{ licence_hostapi_hostdde       , "" },         // 13  Nur einer von beiden
    { licence_fra2rtf               , "" },         // 14
    { licence_saacke                , "" },         // 15  SAB, SAN Firmenlizenzen
  //{ licence_fs_demo               , "" }          // 16  Beschränkte Anzahl Clients
  //{ licence_verfallsdatum_1900    , "961231" }    // 19
  //{ licence_verfallsdatum_2000    , "000101" }    // 20
    { licence_wbrz_logos_to_loga    , "" },         // 30
    { licence_kis                   , "" },         // 31
  //{ licence_sisy                  , "" },         // 32
    { licence_saacke_winword        , "" }          // 33
};
*/
//-------------------------------------------------------------------Sos_seriennr::Sos_seriennr

Sos_seriennr::Sos_seriennr()
:
    _zero_ ( this+1 )
{
}

//--------------------------------------------------------------------Sos_seriennr::operator ==

Bool Sos_seriennr::operator== ( const Sos_seriennr& o ) const
{
    return    strcmp( _ausstellerkuerzel, o._ausstellerkuerzel ) == 0
           && strcmp( _kundenkuerzel, o._kundenkuerzel ) == 0
           && _lfdnr == o._lfdnr;
}

//-------------------------------------------------------------------------Sos_licence::Sos_licence

Sos_licence::Sos_licence()
:
    _zero_ ( this + 1 )
{
    init();
}

//-------------------------------------------------------------------------Sos_licence::Sos_licence

Sos_licence::Sos_licence( const string& key )
:
    _zero_ ( this + 1 )
{
    init();
    read_key( key );
}

//--------------------------------------------------------------------------------Sos_licence::init

void Sos_licence::init()
{
    _empty_value_string = "\x01";         // Nur das erste Zeichen wird geprüft. Gesetzt, aber ohne Wert. "" == nicht gesetzt
    _seriennr.obj_const_name( "Sos_licence::_seriennr" );
    _seriennr.first_index( 1 );

    _par.obj_const_name( "Sos_licence::_par" );
}

//--------------------------------------------------------------------Sos_licence::~Sos_licence

Sos_licence::~Sos_licence()
{
/*
    for( int i = _par.last_index(); i >= _par.first_index(); i-- ) {
        const char*& p = _par[ i ];
        if( p  &&  p[0] != '\0' )  sos_free( (char*)p );
        p = NULL;
    }
*/
}

//-------------------------------------------------------------------------------------add_byte

static uint4 add_byte( uint4 code, Byte b )
{
    const uint4 prim = 0x7efefefdL;
    return   ( ( ( code % prim ) << 1 ) | ( code >> (32-1) ) )
           ^ (Byte)(                b   + ( code & 0x200? 0x6A : 0xA6 ) ) ;
}

//-----------------------------------------------------------------------------------add_string

static void add_string( uint4* code, const char* p )
{
    while( *p )  *code = add_byte( *code, *p++ );
}

//------------------------------------------------------Sos_licence::berechneter_sicherungscode

uint4 Sos_licence::berechneter_sicherungscode() const
{
    uint4           code = 0;
    Sos_seriennr*   s    = &_seriennr[ 1 ];

    for( int h = 1; h <= 7; h++ )
    {
        add_string( &code, s->_ausstellerkuerzel );
        add_string( &code, s->_kundenkuerzel );

        const char* p0 = (char*)&s->_lfdnr;
        const char* p  = p0;
        while( p < p0 + sizeof s->_lfdnr )  {
            if( *p )  code = add_byte( code, *p );
            p++;
        }

        code = add_byte( code, s->_date.year() );    
        code = add_byte( code, s->_date.month() );   // Byte 0x01 .. 0x0C
        code = add_byte( code, s->_date.day() );     // Byte 0x01 .. 0x1F

        for( int i = _par.first_index(); i <= _par.last_index(); i++ ) {
            const char* p = (*this)[ i ];
            if( p ) {  
                if( i >= 0x100 )  code = add_byte( code, (Byte)( i >> 8 ) );
                code = add_byte( code, (Byte)i );
                add_string( &code, p );
            }
        }

        if( _zz )  code = add_byte( add_byte( code, 35 ), 35 );

        code = add_byte( code, _salz );
    }

    return code % ( (uint4)base*base*base*base*base*base - 1 );  // 36**6 - 1
}

//-------------------------------------------------------------------------licence_int_as_alnum

char licence_int_as_alnum( int i )
{
    return i < 10? '0' + i
                 : 'A' + i - 10;
}

//----------------------------------------------------------Sos_licence::string_from_licence_int

string Sos_licence::string_from_licence_int( int code )
{
    string result;
    result += licence_int_as_alnum( code / base );
    result += licence_int_as_alnum( code % base );
    return result;
}

//--------------------------------------------------------------------Sos_licence::alnum_as_int

int Sos_licence::alnum_as_int( char c )
{
    int result = alnum_as_int_or_minus( c );
    if( result < 0 )  throw_xc( "SOS-1003" );
    return result;
}

//---------------------------------------------------------------Sos_licence::alnum_as_int_or_minus

int Sos_licence::alnum_as_int_or_minus( char c )
{
    if( isdigit( c ) )  return c - '0';
    if( islower( c ) )  return c - 'a' + 10;  //jz 31.7.97, damit 'o' von '0' leichter unterschieden wird
    if( isupper( c ) )  return c - 'A' + 10;

    return -1;
}

//------------------------------------------------------------------Sos_licence::alnum_as_uint4

uint4 Sos_licence::alnum_as_uint4( const char* p, int n )
{
    uint4 value = 0;
    uint4 b     = 1;
    while( n ) {
        value += b * alnum_as_int( *p++ );
        b *= base;
        n--;
    }
    return value;
}

//-------------------------------------------------------------------------Sos_licence::set_par

void Sos_licence::set_par( int n, const char* str )
{
    set_par( n, str, str? strlen( str ) : 0 );
}

//-------------------------------------------------------------------------Sos_licence::set_par

void Sos_licence::set_par( int n, const char* value, int len )
{
    if( !value )  return;

    if( n > _par.last_index() )  _par.last_index( n );

    if( len == 0 )  _par[ n ] = _empty_value_string;
              else  _par[ n ] = as_string( value, len );
/*
    char* str;

    if( len == 0 )  str = "";
    else {
        str = (char*)sos_alloc( len + 1 );
        memcpy( str, value, len );
        str[ len ] = '\0';
    }

    _par[ n ] = str;
*/
}

//------------------------------------------------------------------------Sos_licence::read_key

void Sos_licence::read_key( const char* key )
{
    _installed_keys.push_back( key );

    try {
        const char* p = key;
        const char* p2;
        char*       q;

        _seriennr.last_index( 1 );
        Sos_seriennr& s = _seriennr[ 1 ];

        // Ausstellerkürzel

        q = s._ausstellerkuerzel;
        while( *p != '-'  &&  q < s._ausstellerkuerzel + sizeof s._ausstellerkuerzel - 1 ) {
            *q++ = toupper( *p++ );
        }

        if( *p != '-' )  throw_xc( "SOS-1003" );
        p++;

        // Kundenkürzel

        q = s._kundenkuerzel;
        while( *p != '-'  &&  q < s._kundenkuerzel + sizeof s._kundenkuerzel - 1 ) {
            *q++ = toupper( *p++ );
        }

        if( *p != '-' )  throw_xc( "SOS-1003" );
        p++;

        // Laufende Nummer
        while( isdigit( *p ) ) {
            s._lfdnr = 10 * s._lfdnr + *p++ - '0';
        }

        if( *p != '-' )  throw_xc( "SOS-1003" );
        p++;

        {   // Datum
            int year  = 1990 + alnum_as_int( *p++ );
            int month = alnum_as_int( *p++ );
            int day   = alnum_as_int( *p++ );
            s._date.assign( year, month, day );
            if( s._date < min_date )  throw_xc( "SOS-1003" );
            if( *p != '-' )  throw_xc( "SOS-1003" );
            p++;
        }

        // Einstellungen
        p2 = p + strlen( p );
        while( p2[-1] != '-' )  p2--;

        while( p < p2 ) {
            int n = alnum_as_int( *p++ );
            if( *p != '-' )  n = base * n + alnum_as_int( *p++ );

            const char* q = strchr( p, '-' );
            if( !q )  throw_xc( "SOS-1003" );;

            if( n == zz ) {     // Superschlüssel? set_par() verzögern
                if( q != p )  throw_xc( "SOS-1003" );
                _zz = true;
            } else {
                set_par( n, p, q - p );
            }

            p = q + 1;
        }

        // Sicherungscode

        if( strlen( p ) != 7 )  throw_xc( "SOS-1003" );

        _salz = *p++;
        _sicherungscode = alnum_as_uint4( p, 6 );  p += 6;

        while( isspace( *p ) )  p++;
        if( *p )  throw_xc( "SOS-1003" );

        check_key();

        if( _zz ) {
            for( int i = 0; i < NO_OF( comp_text ); i++ ) 
                if( comp_text[ i ]._zz_licence_value )
                    set_par( comp_text[ i ]._code, comp_text[ i ]._zz_licence_value );
        }
    }
    catch( Xc& x ) {
        RENEW( *this, Sos_licence );
        x.insert( key );
        throw;
    }
}

//-------------------------------------------------------------------Sos_licence::merge_licence

void Sos_licence::merge_licence( const Sos_licence& lic )
{
    Z_FOR_EACH_CONST( list<string>, lic._installed_keys, it )  _installed_keys.push_back( *it );

    if( _par.last_index() < lic._par.last_index() )  _par.last_index( lic._par.last_index() );

	int i;
    for( i = lic._par.first_index(); i <= lic._par.last_index(); i++ ) {
        const char* p = lic[ i ];
        if( p )  set_par( i, p );
    }

    for( i = 1; i <= _seriennr.last_index(); i++ )  if( _seriennr[ i ] == lic._seriennr[ 1 ] )  break;
    if( i > _seriennr.last_index() ) {
        if( i > max_seriennr_anzahl )  throw_xc( "SOS-1004", max_seriennr_anzahl );
        _seriennr.add( lic._seriennr[ 1 ] );
    }
}

//---------------------------------------------------------------------------Sos_licence::check

void Sos_licence::check()
{
#   if defined SYSTEM_WIN32
        const char* key_name    = "SOFTWARE\\SOS Software\\licences";
        ulong       type        = 0;
        long        ret;
        HKEY        hkey  = 0;

        try {
            ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ, &hkey );
            if( ret == ERROR_SUCCESS ) {
                for( int i = 0;; i++ )
                {
                    char  text           [ max_licence_key_length + 1 ];
                    ulong text_len       = sizeof text;
                    char  value_name     [ 100 ];
                    ulong value_name_len = sizeof value_name;

                    ret = RegEnumValue( hkey, i, value_name, &value_name_len, NULL, &type, (Byte*)text, &text_len );
                    if( ret == ERROR_NO_MORE_ITEMS )  break;
                    if( ret != ERROR_SUCCESS )  break;//throw_xc( Error_code( "MSWIN-RegEnumValue-", ret ) );

                    if( type == REG_SZ  &&  !sos::empty( text ) ) {
                        Sos_licence lic;
                        lic.read_key( text );
                        merge_licence( lic );
                    }
                }

                RegCloseKey( hkey );
            }
        }
        catch( const Xc& ) {
            if( hkey )  RegCloseKey( hkey );
            throw;
        }
#   endif
        
    check2();
}

//--------------------------------------------------------------------------Sos_licence::check2

void Sos_licence::check2()
{
    Sos_string  key;

    key = read_profile_string( "", "licence", "key1" );

    if( sos::empty( key ) ) {
        Sos_string filename;
        filename = read_profile_string( "", "licence", "file" );
        if( !sos::empty( filename ) )  check_from_file( filename );
                          //else  throw_xc( "SOS-1000" );
        return;
    }

    {
        Sos_licence lic;
        lic.read_key( c_str( key ) );
        merge_licence( lic );
    }

    for( int i = 2;; i++ ) {
        char k [ 10 ];
        sprintf( k, "key%d", i );
        Sos_string key2;
        key2 = read_profile_string( "", "licence", k );
        if( sos::empty( key2 ) )  break;
        Sos_licence lic;
        lic.read_key( c_str( key2 ) );
        merge_licence( lic );
    }
}

//-----------------------------------------------------------------Sos_licence::check_from_file

void Sos_licence::check_from_file( const Sos_string& filename )
{
/*
    FILE* file = 0;
    try {
        Byte l [ 3 ];
        file = open( c_str( filename ), O_RDONLY | O_BINARY );

        read( file, l, 3 );
        uint4 len = ( (uint4)l[0] << 16 ) | ( (uint2)l[1] << 8 ) | l[2];
        Dynamic_area buffer ( len );
        int rc = read( file, buffer.ptr(), buffer.size() );
        if( rc != len )  throw_xc( "SOS-1007", c_str( filename ) );
        buffer.length( rc );

        const char* p     = buffer.char_ptr();
        const char* p_end = buffer.char_ptr() + buffer.length();
        while( p < p_end ) {
            const char* q = (char*)memchr( p, '\0', p_end - p );
            if( !q )  goto ERROR;
            if( strcmp( p, c_str( hostname ) ) == 0 )  break;
            p = q + 1+2;
        }
        if( p >= p_end )  throw_xc( "SOS-1008", c_str( hostname ), c_str( filename ) );

        uint4 pos =


    }
    catch(...) {
        close( file );
        throw;
    }
*/
/*
    hostname.upper_case();

    Any_file file ( filename, Any_file::in );

    while(1) {
        file.get( &line );
        if( line.length() == 0 )  continue;
        const char* q = memchr( line.ptr(), '=', line.length() );
        if( q == line.char_ptr()  ||  !q )  goto ERROR;
        Area( line.ptr(), q - line.char_ptr() ).upper_case();
        if( q - p == length( hostname )
         && memcmp( line.ptr(), c_str( hostname ), q - p ) == 0 )
        {
*/

#   ifdef SYSTEM_WIN32
        char hostname [ MAX_COMPUTERNAME_LENGTH + 1 ];
        ulong hostname_len = NO_OF( hostname );
        if( !GetComputerName( hostname, &hostname_len ) )  throw_mswin_error( "GetComputerName" );
#    else
        char hostname [ 100 ];
        memset( hostname, 0, sizeof hostname );
        gethostname( hostname, sizeof hostname - 1 );
#   endif

    Sos_string licence_keys = read_profile_string( c_str( filename ), "licence", c_str( hostname ) );
    if( sos::empty( licence_keys ) )  throw_xc( "SOS-1008", c_str( hostname ), c_str( filename ) );

    Sos_string key;
    const char* p = c_str( licence_keys );

    while( 1 ) {
        while( isspace( *p ) )  p++;
        if( !*p )  break;

        key = "";
        while( *p  &&  !isspace( *p ) )  key += *p++;

        Sos_licence lic;
        lic.read_key( c_str( key ) );
        merge_licence( lic );
    }
}

//-----------------------------------------------------------------------Sos_licence::check_key

void Sos_licence::check_key()
{
    Sos_seriennr* s = &_seriennr[ 1 ];

    const char* p;
    char  buffer[8];

    if( _sicherungscode != berechneter_sicherungscode() )  throw_xc( "SOS-1003" );

    p = (*this)[ licence_verfallsdatum_1900 ];
    if( p ) {
        memcpy( buffer, "19", 2 );
    }
    else
    {
        p = (*this)[ licence_verfallsdatum_2000 ];
        if( p ) {
            memcpy( buffer, "20", 2 );
        }
    }

    if( p ) {
        time_t    t1;
        char      date [ 8+1 ];

        memcpy( buffer+2, p, 6 );
        p = buffer;

        tzset(); sos_time( &t1 );
        strftime( date, sizeof date, "%Y%m%d", localtime( &t1 ) );
        // strcmp() kann mit Disassembler entdeckt werden:
        for( int i = 0; i < 8; i++ ) {
            if( date[i] < p[i] )  break;
            if( date[i] > p[i] )  {
                //throw_xc( "SOS-1005", p );
                s->_old = true;
                s->_invalid = true;
            }
        }

        // Rechneruhr zurückgestellt?
        if( Sos_date::today() < s->_date )  s->_old = s->_invalid = true;
    }

    p = (*this)[ licence_os ];
    if( p ) {
#       if defined SYSTEM_WIN
            if( !strchr( p, 'W' ) )  s->_invalid = true;
#       elif defined SYSTEM_SOLARIS
            if( !strchr( p, 'S' ) )  s->_invalid = true;
#       endif
    }

    if( s->_invalid ) {         // Alle Parameter löschen:
        _par.last_index( 0 );
        _zz = false;
    }
}

//---------------------------------------------------------------------Sos_licence::operator []

const char* Sos_licence::operator[] ( int i ) const
{
    if( i > _par.last_index() )  return NULL;

    const Sos_string& str = _par[ i ];

    if( length( str ) == 0              )  return NULL;
    if( str[0] == _empty_value_string[0] )  return "";
    return c_str( str );
}

//--------------------------------------------------------------------------Sos_licence::_throw
/*
void Sos_licence::_throw() const
{

}
*/

//------------------------------------------------------------------Sos_licence::component_name

string Sos_licence::component_name( int code )
{
    for( int i = 0; i < NO_OF( comp_text ); i++ ) {
        Comp_text* c = &comp_text[ i ];
        if( c->_code == code )   return c->_text;
    }

    char text [ 15 ];
    sprintf( text, "%c%c", licence_int_as_alnum( code / base ), licence_int_as_alnum( code % base ) );
    return text;
}

//-----------------------------------------------------Sos_licence::component_code_of_name_or_minus

int Sos_licence::component_code_of_name_or_minus( const string& name )
{
    int result = 0;

    for( int i = 0; i < name.length(); i++ )
    {
        int d = alnum_as_int_or_minus( name[ i ] );
        if( d < 0 )  return -1;
        result = result * base + d;
    }

    return result;
}

//--------------------------------------------------------------------Sos_licence::is_demo_version

boolean Sos_licence::is_demo_version()
{
    boolean result = true;
    Z_FOR_EACH_CONST( list<string>, _installed_keys, it )
    {
       Sos_licence lic;
       lic.read_key( *it );
       Sos_seriennr* s = &lic._seriennr[ 1 ];
       Z_LOG2("scheduler","Key: " << lic.key() << ", Kunde: " << s->_kundenkuerzel << "\n");
       if ( strcmp(s->_kundenkuerzel,"DEMO")!=0 && strcmp(s->_kundenkuerzel,"SOS")!=0 ) 
       {
          result = false;
          break;
       }
    }
    return result;
}

//------------------------------------------------------------------------------------check_licence

void check_licence( const char* component_name )
{
    int code = Sos_licence::component_code_of_name_or_minus( component_name );

    for( int i = 0; i < NO_OF( comp_text ); i++ ) 
    {
        Comp_text* c = &comp_text[ i ];

        if( stricmp( c->_text, component_name ) == 0  ||
            code >= 0  &&  c->_code == code )   
        {
            if( !SOS_LICENCE( c->_code ) )  throw_xc( "SOS-1000", component_name );
            return;
        }
    }

    throw_xc( "SOS-1000", component_name );
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
