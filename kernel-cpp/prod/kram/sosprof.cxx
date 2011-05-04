// $Id$

#include "precomp.h"
#include "sysdep.h"

#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "sos.h"
#include "log.h"
#include "stdfield.h"
#include "../file/absfile.h"
#include "soslimtx.h"
#include "sosopt.h"
#include "sosprof.h"
#include "licence.h"
#include "sosuser.h"
#include "log.h"

// wg. _licence-"check2
#include "sosclien.h"
#include "sostimer.h"

#include "../zschimmer/log.h"

#if defined SYSTEM_WIN
#   include <windows.h>
#endif

#define SOSPROF_DEFAULT_MAGIC_TOKEN "+++ KEIN EINTRAG +++"


namespace sos {

/* s. sosstat.h
enum Profile_source         // Gelesene Quellen des Dateinamens für sos.ini. Gibt auch die Priorität an. Niedrige Priorität kann höhere nicht überschreiben.
{
    source_default      = 0x01,     // "sos.ini"
    source_environment  = 0x02,     // SOS_INI
    source_program      = 0x04,     // -sos.ini=
    source_registry     = 0x08      // Windows-Registrierung
};
*/

//-------------------------------------------------------------------------------------------------

static bool static_sos_ini_filename_logged = false;

//-------------------------------------------------------------------------------------------------

inline Profile_source operator |= ( Profile_source& a, Profile_source b ) 
{ 
    return a = (Profile_source)( a | b ); 
}


void truncate_spaces( Area* area );     // truncsp.cxx
const int max_group_size = 32000; //32767;
const int max_entry_length = 100;

//#if defined SYSTEM_WIN
    //static Sos_string sos_ini_msg_;
    //Sos_string sos_ini_msg() { return sos_ini_msg_; }
//#endif

//-------------------------------------------------------------------------set_sos_ini_filename

void set_sos_ini_filename( const Sos_string& filename )
{
    Z_MUTEX( hostware_mutex )
    {
        if( sos_static_ptr()->_profile_source <= source_program )
        {
            sos_static_ptr()->_profile        = filename;
            sos_static_ptr()->_profile_source = source_program;
            Z_LOG2( "ini", "set_sos_ini_filename(\"" << sos_static_ptr()->_profile << "\")  (Option -sos.ini=?)\n" );  // Normalerweise ist die .log-Datei noch geschlossen

            sos_static_ptr()->_licence->check2();
        }
    }

    zschimmer::set_environment_variable( "SOS_INI", filename );     // 2006-04-14
}

//--------------------------------------------------------------------------read_profile_string

Sos_string read_profile_string( const char* filename, const char* group_name, const char* entry_name, const char* default_result )
{
    ZERO_RETURN_VALUE( Sos_string ) // js 28/8/97
    Dynamic_area buffer ( 4096+1 );
    buffer.char_ptr()[ 0 ] = '\0';
    buffer.length(0);
    read_profile_entry( &buffer, filename, group_name, entry_name, default_result );
    return as_string( buffer );
}

//-----------------------------------------------------------------read_existing_profile_string

Sos_string read_existing_profile_string( const char* filename, const char* group_name, const char* entry_name )
{
    ZERO_RETURN_VALUE( Sos_string ) // js 28/8/97
    Dynamic_area buffer ( 4096+1 );
    buffer.char_ptr()[ 0 ] = '\0';
    buffer.length(0);
    Bool exists = read_profile_entry( &buffer, filename, group_name, entry_name );

    if( !exists )  { Xc x ( "SOS-1197" ); x.insert( entry_name ); x.insert( group_name ); x.insert( filename ); throw x; }
    //jz 30.11.97 if( buffer.length() == 0 ) { Xc x ( "SOS-1197" ); x.insert( entry_name ); x.insert( group_name ); x.insert( filename ); throw x; }

    return as_string( buffer );
}

//-------------------------------------------------------------------------write_profile_string

void write_profile_string( const Sos_string& str, const char* filename, const char* group_name, const char* entry_name )
{
    const char* p = ::sos::c_str( str );

    write_profile_entry( Const_area( p, ::sos::length( str ) ), filename, group_name, entry_name );
}

//----------------------------------------------------------------------------read_profile_bool

Bool read_profile_bool( const char* filename, const char* group_name, const char* entry_name, Bool def )
{
    //char buf[20];     // Führt in Solaris zum Absturz (Exception und assertion)
    //Area area( buf, sizeof buf );
    //area.length(0);
    Sos_limited_text<50> buffer;
    //Dynamic_area buffer;

    try {
        read_profile_entry( &buffer, filename, group_name, entry_name );
    }
    catch( const Not_found_error& )
    {
        return def;
    }

    if ( buffer.length() == 0 ) return def;

    truncate_spaces( &buffer );
    buffer += '\0';
    if ( strcmpi( buffer.char_ptr(), "on" )    == 0 ||
         strcmpi( buffer.char_ptr(), "ja" )    == 0 ||
         strcmpi( buffer.char_ptr(), "j"  )    == 0 ||
         strcmpi( buffer.char_ptr(), "yes" )   == 0 ||
         strcmpi( buffer.char_ptr(), "y"   )   == 0 ||
         strcmpi( buffer.char_ptr(), "true" )  == 0 ||
         strcmp(  buffer.char_ptr(), "1" )     == 0 ) return true;
    if ( strcmpi( buffer.char_ptr(), "off" )   == 0 ||
         strcmpi( buffer.char_ptr(), "nein" )  == 0 ||
         strcmpi( buffer.char_ptr(), "n"   )   == 0 ||
         strcmpi( buffer.char_ptr(), "no" )    == 0 ||
         strcmpi( buffer.char_ptr(), "false" ) == 0 ||
         strcmp(  buffer.char_ptr(), "0" )     == 0 ) return false;

    return def;
}

//----------------------------------------------------------------------------file_exists
/*
Bool file_exists( const char* filename )
{
    Any_file f;

    try {
        f.open( filename, File_base::in );
        f.close();
        return true;
    } catch ( const Xc& ) {
        return false;
    }
}
*/
//---------------------------------------------------------------------------read_registry
#if defined SYSTEM_WIN

static string read_registry( const string& filename, bool is_sos_ini )
{
    WORD version = LOWORD( GetVersion() );
    if( ( ( (uint)LOBYTE(version) << 8 ) | (uint)HIBYTE(version) ) >= 0x351 )
    {
        const char* key_name = "SOFTWARE\\SOS Software";
        const char* key_name_old = "SOFTWARE\\SOS Software\\sos.ini";   // Zur Kompatibilität
        Bool        found = false;

        char        sos_ini_old [ _MAX_PATH ];
        long        sos_ini_len_old = sizeof sos_ini_old;

        char        sos_ini     [ _MAX_PATH ];
        unsigned long sos_ini_len = sizeof sos_ini;
        long        ret;
        
        HKEY        sos_ini_key         = 0;
        HKEY        sos_ini_key_old     = 0;

        ret = RegOpenKeyEx( HKEY_LOCAL_MACHINE, key_name, 0, KEY_READ | KEY_EXECUTE, &sos_ini_key );

        if ( ret != ERROR_SUCCESS ) goto OLD_REG;

        DWORD reg_type;
        ret = RegQueryValueEx( sos_ini_key, c_str(filename), NULL, &reg_type, (unsigned char*)sos_ini, &sos_ini_len );
        
        // TODO: reg_type überprüfen
        RegCloseKey( sos_ini_key );

        if ( ret != ERROR_SUCCESS ) goto OLD_REG;

        return sos_ini;


        OLD_REG: // alter Schlüsselstil mit Standardwert wird aus Kompatibilität noch unterstützt
        if( !found  &&  is_sos_ini )
        {
            ret = RegOpenKey( HKEY_LOCAL_MACHINE, key_name_old, &sos_ini_key_old );
            if ( ret != ERROR_SUCCESS ) { if ( found ) goto RETURN_SOS_INI; else goto NO_REG; }

            ret = RegQueryValue( sos_ini_key_old, "", sos_ini_old, &sos_ini_len_old );
        
            RegCloseKey( sos_ini_key_old );

            if ( ret != ERROR_SUCCESS ) { if ( found ) goto RETURN_SOS_INI; else goto NO_REG; }

            if ( found ) {
                // TODO: Konflikt melden!
                //sos_ini_msg_  = "sos.ini Warnung: Doppelte Einträge in der Registrierung gefunden! Die Werteversion wird bevorzugt: ";
                //sos_ini_msg_ += sos_ini;
            } else {
                // TODO: Benutzung des alten Stils melden!
                //sos_ini_msg_  = "sos.ini Warnung: Die veraltete Schlüsselversion wird verwendet: ";
                //sos_ini_msg_ += sos_ini;
                //sos_static_ptr()->_profile = sos_ini_old;
            }

            RETURN_SOS_INI:
            // Überprüfen, ob Datei existiert bzw. lesbar ist
            // if ( !file_exists(found?sos_ini:sos_ini_old) ) throw_xc( "SOS-1009", found?sos_ini:sos_ini_old );
            
            return found? sos_ini : sos_ini_old;

            NO_REG: ;
        }
    }

    return "";
}

#endif
//----------------------------------------------------------------------------ini_filename

static Sos_string ini_filename( const Sos_string& filename_ )
{
    Sos_string  filename   = empty(filename_)? "sos.ini" : filename_;
    bool        is_sos_ini = stricmp( c_str(filename), "sos.ini" ) == 0;

    if( is_sos_ini )
    {
#       ifdef SYSTEM_WIN
            Z_MUTEX( hostware_mutex )
            {
                if( sos_static_ptr()->_profile_source < source_registry 
                && ( sos_static_ptr()->_profile_source_checked & source_registry ) == 0 )
                {
                    string registry_filename = read_registry( filename, is_sos_ini );
                    if( !registry_filename.empty() ) 
                    {
                        sos_static_ptr()->_profile        = registry_filename;
                        sos_static_ptr()->_profile_source = source_registry;
                        Z_LOG2( "ini", "Registry sos.ini=" << sos_static_ptr()->_profile << "\n" );  // Normalerweise ist die .log-Datei noch geschlossen
                    }

                    sos_static_ptr()->_profile_source_checked |= source_registry;
                }
            }
#       endif


      //if( sos_static_ptr()->_profile_source < source_program )
      //{ 
      //    S. set_sos_ini_filename()
      //}


        if( sos_static_ptr()->_profile_source < source_environment 
         && ( sos_static_ptr()->_profile_source_checked & source_environment ) == 0 )
        { 
            const char* p = getenv( "SOS_INI" );
            if( p )
            {
                sos_static_ptr()->_profile        = p;
                sos_static_ptr()->_profile_source = source_environment;
                Z_LOG2( "ini", "Umgebungsvariable SOS_INI sos.ini=" << sos_static_ptr()->_profile << "\n" );  // Normalerweise ist die .log-Datei noch geschlossen
            }

            sos_static_ptr()->_profile_source_checked |= source_environment;
        }


        if( sos_static_ptr()->_profile_source < source_default )
        { 
            sos_static_ptr()->_profile = "sos.ini";
            sos_static_ptr()->_profile_source = source_default;
            Z_LOG2( "ini", "Default sos.ini=" << sos_static_ptr()->_profile << "\n" );  // Normalerweise ist die .log-Datei noch geschlossen
        }

        if( !static_sos_ini_filename_logged && sos_static_ptr()->_log_ptr )  
        { 
            static_sos_ini_filename_logged = true;  
            Z_LOG( "sos.ini=" << sos_static_ptr()->_profile << "\n" ); 
        }

        return sos_static_ptr()->_profile;
    }
    else
    {
        if( strchr( c_str(filename), '/' ) || strchr( c_str(filename), '\\' ) )  return filename;   // Mit Verzeichnis?

#       ifdef SYSTEM_WIN
            string registry_filename = read_registry( filename, is_sos_ini );
            if( !registry_filename.empty() )  filename = registry_filename;
#       endif

        return filename;
    }
}

//---------------------------------------------------------------------------------read_profile_int
#if defined SYSTEM_WIN

int read_profile_int( const char* fn, const char* group_name, const char* entry_name, int deflt )
{
    Sos_string filename = ini_filename( fn );
    Z_LOG2( "ini", "GetPrivateProfileInt(\"" << group_name << "\",\"" << entry_name << "\",\"" << deflt << "\",\"" << filename << "\")\n" );
    return GetPrivateProfileInt( group_name, entry_name, deflt, c_str( filename ) );
}

#else

int read_profile_int( const char* filename, const char* group_name, const char* entry_name,
                        int deflt )
{
    Sos_string erg = read_profile_string( filename, group_name, entry_name );
    if ( erg == "" ) return deflt;
    return as_int( erg );
}
#endif
//--------------------------------------------------------------------------------read_profile_uint
#if defined SYSTEM_WIN

uint read_profile_uint( const char* fn, const char* group_name, const char* entry_name,
                        uint deflt )
{
    Sos_string filename = ini_filename( fn );
    Z_LOG2( "ini", "GetPrivateProfileInt(\"" << group_name << "\",\"" << entry_name << "\",\"" << deflt << "\",\"" << filename << "\")\n" );
    return GetPrivateProfileInt( group_name, entry_name, deflt, c_str( filename ) );
}

#else

uint read_profile_uint( const char* filename, const char* group_name, const char* entry_name,
                        uint deflt )
{
    Sos_string erg = read_profile_string( filename, group_name, entry_name );
    if ( erg == "" ) return deflt;
    return as_uint( erg );
}
#endif

#if 0
void write_profile_uint( uint value, const char* filename, const char* group_name, const char* entry_name )
{
    write_profile_string( as_string( (int)value ), filename, group_name, entry_name );
}
#endif

//---------------------------------------------------------------------------write_profile_bool

void write_profile_bool( Bool b, const char* filename, const char* group_name, const char* entry_name )
{
    char buf[2];
    strcpy( buf, b ? "1" : "0" );

    write_profile_entry( Const_area( buf, 2 ), filename, group_name, entry_name );
}

//---------------------------------------------------------------------------read_profile_double

double read_profile_double( const char* filename, const char* group_name, const char* entry_name, double deflt )
{
    Sos_limited_text<50> buffer;

    try {
        read_profile_entry( &buffer, filename, group_name, entry_name );
    }
    catch( const Not_found_error& )
    {
        return deflt;
    }

    truncate_spaces( &buffer );
    if( buffer.length() == 0 )  return deflt;

    try {
        return as_double( c_str(buffer) );
    }
    catch( const Xc& )
    {
        throw_xc( "SOS-1421", group_name, entry_name );
        return 0.0;
    }
}

//---------------------------------------------------------------------------Sos_profile_record

struct Sos_profile_record
{
    //Sos_limited_text<max_entry_length>      _entry;
    char                       _entry [ max_entry_length + 1 ];
    char                       _value [ 4096 + 1 ];
    Bool                       _value_null;
    //Sos_limited_text<1024>     _value;
};

//-----------------------------------------------------------------------------Sos_profile_file

struct Sos_profile_file : Abs_file
{
    BASE_CLASS( Abs_file )

                                Sos_profile_file        ();
                               ~Sos_profile_file        ();

  protected:
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode );
    void                        get_record              ( Area& );
    void                        get_record_key          ( Area&, const Key& );
    void                        rewind                  ( Key::Number );
    void                        store                   ( const Const_area& );
    void                        insert                  ( const Const_area& );
    void                        update                  ( const Const_area& );
    void                        del                     ();
    void                        del                     ( const Key& );

    void                       _obj_print               ( ostream* ) const;

  private:
    void                        make_type();

    Fill_zero                  _zero_;
    Sos_string                 _filename;
    Sos_string                 _group_name;
    Sos_string                 _entry_name;
    Sos_limited_text<max_entry_length> _current_key;
    Dynamic_area               _buffer;
    const char*                _ptr;
    Bool                       _eof;
    Bool                       _changed;
    Bool                       _update_allowed;

    Sos_ptr<Record_type>       _record_type;
};

//------------------------------------------------------------------------Sos_profile_file_type

struct Sos_profile_file_type : Abs_file_type
{
    BASE_CLASS( Abs_file_type )

                                Sos_profile_file_type   ();

    const char*                 name                    () const  { return "profile"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Sos_profile_file> file = SOS_NEW_PTR( Sos_profile_file );
        return +file;
    }

};

      Sos_profile_file_type  _sos_profile_file_type;
const Abs_file_type&          sos_profile_file_type = _sos_profile_file_type;

//-------------------------------------------------Sos_profile_file_type::Sos_profile_file_type

Sos_profile_file_type::Sos_profile_file_type()
{
    // make_type(); js 11.9.96 Any_file ist evtl. noch nicht definiert
}

//-----------------------------------------------------------Sos_profile_file::Sos_profile_file

Sos_profile_file::Sos_profile_file()
:
    _zero_ ( this+1 )
{
}

//----------------------------------------------------------Sos_profile_file::~Sos_profile_file

Sos_profile_file::~Sos_profile_file()
{
}

//------------------------------------------------------------------Sos_profile_file::make_type

void Sos_profile_file::make_type()
{
    _record_type = Record_type::create();
    _record_type->name( "Sos_profile" );

    Sos_ptr<String0_type>   string0_type;
    Record_type*            t = _record_type;     t->allocate_fields(2);
    Sos_profile_record*     o = NULL;
    long                    offset;

    //RECORD_TYPE_ADD_LIMTEXT     ( entry, 0 );
    //RECORD_TYPE_ADD_LIMTEXT_NULL( value, 0 );


    string0_type = SOS_NEW( String0_type( sizeof o->_entry - 1 ) );
    offset = (long)&o->_entry;
    t->add_field( +string0_type, "entry", offset, -1, 0 );

    //_key_pos und _key_len nicht setzen, denn die Sätze sind nicht geordnet!
    //_key_pos = offset;
    //_key_len = string0_type->field_size();

    string0_type = SOS_NEW( String0_type( sizeof o->_value - 1 ) );
    offset = (long)&o->_value;
    t->add_field( +string0_type, "value", offset, (long)&o->_value_null, 0 );
}

//-----------------------------------------------------------------------Sos_profile_file::open

void Sos_profile_file::open( const char* filename, Open_mode, const File_spec& )
{
    for( Sos_option_iterator opt( filename ); !opt.end(); opt.next() )
    {
        if( opt.with_value( "group" )
         || opt.with_value( "section" ) )   _group_name = opt.value();
        else
        if( opt.with_value( "entry" ) )     _entry_name = opt.value();
        else
        if( opt.param() || opt.pipe() )     _filename   = opt.rest();
        else throw_sos_option_error( opt );
    }

    if( empty(_group_name ) )  throw_xc( "SOS-1412" );

    if( empty(_entry_name) )
    {
        make_type();
        _any_file_ptr->_spec._field_type_ptr = +_record_type;
        //_key_len = spec.key_length();
    }
}

//-----------------------------------------------------------------------Sos_profile_file::open

void Sos_profile_file::close( Close_mode )
{
#   if defined SYSTEM_WINDOWS
        if( _changed ) {
            // flush buffer:
            WritePrivateProfileString( NULL, NULL, NULL, c_str( _filename ) );
        }
#   endif
}

//-----------------------------------------------------------------Sos_profile_file::get_record

void Sos_profile_file::get_record( Area& buffer )
{
    if( _eof )  throw_eof_error();

    Bool ok;

    _update_allowed = false;

    if( !_ptr ) {
        Bool ok = read_profile_entry( &_buffer, c_str( _filename ), c_str( _group_name ), c_str(_entry_name), 
                                      SOSPROF_DEFAULT_MAGIC_TOKEN );
        if( !ok )  throw_eof_error();

        if( !empty(_entry_name) )
        {
            buffer.assign( _buffer );
            _eof = true;
            return;
        }

        _ptr = _buffer.char_ptr();
    }

    const char* e = _ptr + strlen( _ptr );
    if( e == _ptr )  throw_eof_error();

    if( (int)buffer.ptr() % sizeof (int) )  throw_xc( "SOS-1261", "Sos_profile_file" );
    buffer.allocate_length( _record_type->field_size() ); //sizeof (Sos_profile_record) );
    buffer.fill( '\0' );
    Sos_profile_record* r = (Sos_profile_record*) buffer.ptr();
    new(r) Sos_profile_record;

    _current_key = _ptr;

    Area( r->_entry, sizeof r->_entry - 1 ).assign( _ptr );
    r->_entry[ strlen( _ptr ) ] = '\0';

    Area value ( r->_value, sizeof r->_value - 1 );
    ok = read_profile_entry( &value, c_str( _filename ), c_str( _group_name ), _ptr );
    r->_value[ value.length() ] = '\0';
    r->_value_null = !ok;

    _ptr = e + 1;

    _update_allowed = true;
}

//---------------------------------------------------------------------Sos_profile_file::rewind

void Sos_profile_file::rewind( Key::Number )
{
    //LOG( "Sos_profile_file::rewind\n" );
    _update_allowed = false;

    if( true/*_changed*/ )  _ptr = NULL;    // Abschnitt wieder einlesen
              else  _ptr = _buffer.char_ptr();
}

//-------------------------------------------------------------Sos_profile_file::get_record_key

void Sos_profile_file::get_record_key( Area& buffer, const Key& key )
{
    Sos_limited_text<max_entry_length> entry_name;

    if( (int)buffer.ptr() % sizeof (int) )  throw_xc( "SOS-1261", "Sos_profile_file" );

	 buffer.allocate_length( sizeof (Sos_profile_record) );
	 buffer.fill( '\0' );
	 Sos_profile_record* r = (Sos_profile_record*)buffer.ptr();
	 new(r) Sos_profile_record;

	 _update_allowed = false;
	 entry_name.assign( key.char_ptr(), length_without_trailing_spaces( key.char_ptr(), key.length() ) );
    //entry_name.length( length_without_trailing_spaces( entry_name.char_ptr(), entry_name.length() ) );

    _current_key = key.char_ptr();

    //? jz 2.7.01 Area( r->_entry, sizeof r->_entry ).assign( key.char_ptr(), strlen( key.char_ptr() + 1 ) );
    Area( r->_entry, sizeof r->_entry ).assign( key );

    Area value ( r->_value, sizeof r->_value - 1 );
    Bool ok = read_profile_entry( &value, c_str( _filename ), c_str( _group_name ), c_str( entry_name ) );
    if( !ok )  throw_not_found_error( "D311" );
    r->_value[ value.length() ] = '\0';
    r->_value_null = false;

    _update_allowed = true;
}

//---------------------------------------------------------------------Sos_profile_file::store

void Sos_profile_file::store( const Const_area& record )
{
    Dynamic_area        buffer;
    Sos_profile_record* r  = (Sos_profile_record*) record.ptr();

    _update_allowed = false;

    if( record.length() != _record_type->field_size() )  throw_xc( "LENGTH-ERROR", "Sos_profile_file" );

    write_profile( c_str( _filename ), c_str( _group_name ), r->_entry, r->_value_null? NULL : r->_value );

    _changed = true;
}

//---------------------------------------------------------------------Sos_profile_file::insert

void Sos_profile_file::insert( const Const_area& record )
{
    Dynamic_area        buffer;
    Sos_profile_record* r  = (Sos_profile_record*) record.ptr();

    _update_allowed = false;

    if( record.length() != _record_type->field_size() )  throw_xc( "LENGTH-ERROR", "Sos_profile_file" );

    Bool exists = read_profile_entry( &buffer, c_str( _filename ), c_str( _group_name ), r->_entry );
    if( exists )  throw_duplicate_error( "D411" );

    write_profile( c_str( _filename ), c_str( _group_name ), r->_entry, r->_value_null? NULL : r->_value );

    _changed = true;
}

//---------------------------------------------------------------------Sos_profile_file::update

void Sos_profile_file::update( const Const_area& record )
{
    if( !_update_allowed )  throw_xc( "SOS-1233" );
    _update_allowed = false;

    Sos_profile_record* r = (Sos_profile_record*) record.ptr();
    if( (int)r % sizeof (int) )  throw_xc( "SOS-1261", "Sos_profile_file" );

    //if( r->_entry != _current_key )  throw_xc( "SOS-1229", "profile" );
    if( stricmp( r->_entry, c_str( _current_key ) ) != 0 )
    {
        del();
    }

    write_profile( c_str( _filename ), c_str( _group_name ), r->_entry, r->_value_null? NULL : r->_value );

    _changed = true;
}

//------------------------------------------------------------------------Sos_profile_file::del

void Sos_profile_file::del()
{
    _update_allowed = false;
    write_profile( c_str( _filename ), c_str( _group_name ), c_str( _current_key ), NULL );
    _changed = true;
}

//------------------------------------------------------------------------Sos_profile_file::del

void Sos_profile_file::del( const Key& key )
{
    _current_key.assign( key.char_ptr(), length_without_trailing_spaces( key.char_ptr(), key.length() ) );
    //_current_key = key.char_ptr();
    del();
}

//-----------------------------------------------------------------Sos_profile_file::_obj_print

void Sos_profile_file::_obj_print( ostream* s ) const
{
    *s << "Sos_profile_file( \"" << _filename << "\", \"" << _group_name << "\" )";
}

//----------------------------------------------------------------------------assert_no_include

static void assert_no_include( const Const_area& buffer )
{
    const char* p = buffer.char_ptr();

    while( *p ) {
        while( *p == ' ' )  p++;
        if( strnicmp( p, "include=", 8 ) == 0 )  throw_xc( "SOS-INCLUDE-NOT-ALLOWED", "sos.ini" );
        p += strlen( p ) + 1;
    }
}

#if defined SYSTEM_WIN


int static my_GetPrivateProfileString( const char* section, const char* key, const char* deflt,
                                       char* result, int size, const char* ini_file )
{
    if( !section || !section[0] )  throw_xc( "SOS-1443", key );

    result[0] = '\0';
    string default_string = ltrim(deflt);  // Für Windows 95
    int len = GetPrivateProfileString( !section || section[0] == '\0'? NULL : section, 
                                       !key     || key    [0] == '\0'? NULL : key, 
                                       default_string.c_str(), result, size, ini_file );

    if( zschimmer::Log_ptr log = "ini" )
    {
        *log << "GetPrivateProfileString(\"" << section << "\",";
        if( key )  *log << '"' << key << '"';
              else *log << "NULL";
        *log << ",\"" << deflt << "\",," << size << ",\"";
        *log << ini_file << "\") returns \"" << result << '"' << endl;
    }

    return len;
}



Bool read_profile_entry( Area* value_ptr, const char* fn, const char* group_name, const char* entry_name, const char* default_result )
{
    Sos_string filename = ini_filename( fn? fn : "" );

    if( empty( entry_name ) ) 
    {
        // Liefert alle Entry-Namen ohne Werte durch '\0' getrennt. Das ganze wird durch ein extra '\0' abgeschlossen.

        Dynamic_area buffer ( max_group_size );
        int len = my_GetPrivateProfileString( group_name, NULL, "",
                                              buffer.char_ptr(), buffer.size(),
                                              c_str( filename ) );
        buffer.length( len + 1 );
        value_ptr->assign( buffer );
        assert_no_include( buffer );
        return true;  // immer?
    }
    else
    {
        const char* deflt = SOSPROF_DEFAULT_MAGIC_TOKEN;
        //value_ptr->length(0);
        //value_ptr->char_ptr()[0] = 0;
        Dynamic_area buffer ( 4096+1+1 );

        if( !memchr( value_ptr->char_ptr(), '\0', value_ptr->length() ) ) {
            value_ptr->allocate_min( value_ptr->length() + 1 );
            value_ptr->char_ptr()[ value_ptr->length() ] = '\0';
        }
        // entweder auf  sos_init_parameters zurückführen oder selba nach sos.ini suchen ...

        int len = my_GetPrivateProfileString( group_name, entry_name, deflt,
                                              buffer.char_ptr(), buffer.size(),
                                              c_str( filename ) );

        if( strcmp( deflt, buffer.char_ptr() ) == 0 )
        {
            Dynamic_area include_buffer ( 4096+1 );
          /*int include_len =*/my_GetPrivateProfileString( group_name, "include", deflt,
                                                           include_buffer.char_ptr(), include_buffer.size(),
                                                           c_str( filename ) );
            include_buffer.char_ptr()[ include_buffer.size() - 1 ] ='\0';
            if( strcmp( deflt, include_buffer.char_ptr() ) != 0 )
            {
                len = my_GetPrivateProfileString( group_name, entry_name, deflt,
                                                  buffer.char_ptr(), buffer.size(),
                                                  include_buffer.char_ptr() );
            }
        }

        if( len == buffer.size() - 1 )  { Too_long_error x ( "SOS-1196" ); x.insert( entry_name ); x.insert( group_name ); x.insert( buffer.size() - 2 ); throw x; }
        if( strcmp( deflt, buffer.char_ptr() ) == 0 ) {
            value_ptr->assign( default_result );  //jz 30.11.97
            return false;
        }

        value_ptr->assign( buffer.char_ptr(), len + 1 );
        value_ptr->length( len );
        return true;
    }
}

//--------------------------------------------------------------------------------write_profile

void write_profile( const char* fn, const char* section,
                    const char* entry, const char* value )
{
    // value == NULL: delete
    // Wenn die Datei keinen Pfad hat, wird sie auf jeden Fall im Windows-Verzeichnis
    // angelegt.
    Sos_string filename = ini_filename( fn );

    BOOL ok = WritePrivateProfileString( section, entry, value, c_str( filename ) );
    if( !ok )  throw_mswin_error( "WritePrivateProfileString" );
}


void write_profile_entry( const Const_area& value, const char* fn, const char* group_name, const char* entry_name )
{
    Sos_string filename = ini_filename( fn );

    char* buffer = new char[value.length()+1];
        if ( !buffer ) throw_xc( "R101" );
    memcpy( buffer, value.char_ptr(), value.length() );
    buffer[value.length()] = 0;

    BOOL erg = WritePrivateProfileString( group_name, entry_name, buffer, c_str( filename ) );
    delete [] buffer;

    if ( !erg ) throw( "WriteEntryFailed" );
}

#else

// UNIX: include-Mechanismus, /etc/sos.conf, ~/sos.conf


static int __ini_lookup( Any_file*, Area*, const char*, const char*, int=0 );

extern Bool __is_sos_user_name_enabled();

//-----------------------------------------------------------------------------------find_group

static void find_group( Any_file* f, const char* group_name )
{
    Dynamic_area line ( 4096 + 2 );
    const int group_name_length = strlen( group_name ); 

    while(1) {
        while(1) {
            f->get( &line );
            if( line.length() >= 3  &&  line.char_ptr()[ 0 ] == '[' )  break;
        }

        if( line.length() >= group_name_length + 2
         && strnicmp( line.char_ptr() + 1, group_name, group_name_length ) == 0
         && line.char_ptr()[ group_name_length + 1 ] == ']' )  break;
    }
}


//----------------------------------------------------------------------------read_profile_entry

Bool read_profile_entry( Area* value_ptr, const char* filename_,
                         const char* group_name, const char* entry_name, const char* default_result )
{
Bool ok;

//LOGI( "read_profile_entry( ," << filename_ << "," << group_name << "," << entry_name << ")\n" )
try {
    Any_file      f;
    Dynamic_area  area;
  //int           group_name_length = strlen( group_name );
  //int           entry_name_length = strlen( entry_name );
  //int           len;
    Bool          opened = false;
    Sos_string    filename;

    filename = ini_filename( filename_? filename_ : "" );

    // unter Unix evtl. auch ~/.sosrc o.ä. (Environment-Variable SOSINI=xxxx ?)
    //if( !empty( filename ) )
    if( strchr( filename.c_str(), '/' ) )
    {
       f.open( "file:" + filename, Any_file::in );
       opened = true;
    }
    else
/*
    if( getenv( "SOS_INI" ) != NULL )
    {
       f.open( getenv( "SOS_INI" ), Any_file::in );
       opened = true;
    }
    else
*/
    {  // SOS.INI Suchstrategie ( ~/sos.ini, ~/.sos.ini, /etc/sos.ini )
        char home_sos_ini[_MAX_PATH];
        char _filename[_MAX_PATH];


        if ( __is_sos_user_name_enabled() ) // sendmail - Prob
        {
            //LOG( "read_profile_entry: home_dir=" << Sos_user::home_dir().c_str() ) << '\n' );
            Sos_string str = Sos_user::home_dir();
            strcpy( home_sos_ini, c_str( str ) );
            strcat( home_sos_ini, "/" );

            strcpy( _filename, home_sos_ini );
            strcat( _filename, filename.c_str() );  //"sos.ini" );

            try {
                f.open( string("file:") + _filename, Any_file::in );
                opened = true;
            }
            catch( const Not_exist_error& )    {}
          //10.1.97 catch( ... ) { throw; }

            if( !opened ) {
                strcpy( _filename, home_sos_ini );
                strcat( _filename, filename.c_str() ); //".sos.ini" );

                try {
                    f.open( string("file:") + _filename, Any_file::in ); 
                    opened = true;
                }
                catch( const Not_exist_error& ) {}
                //10.1.97 catch( ... ) { throw; }
            }
        }

#       if defined SYSTEM_UNIX
            if( !opened ) {
                f.open( "file:/etc/" + filename, Any_file::in );
                opened = true;
            }
#       endif
    }

    if( !opened )  throw_not_exist_error( "D140", filename.c_str() );

    area.allocate_min( 4096+1 );   // max 4096 Zeichen pro Zeile

    if( empty( entry_name ) ) 
    {
        // Liefert alle Entry-Namen ohne Werte durch '\0' getrennt. Das ganze wird durch ein extra '\0' abgeschlossen.

        find_group( &f, group_name );
        Dynamic_area buffer ( max_group_size );
        Dynamic_area line   ( 4096 + 1 );

        try {
            while(1) {
                f.get( &line );  line += '\0';
                char* p = line.char_ptr();

                while( *p == ' ' )  p++;     // Blanks überspringen
                
                if( *p == '[' )  break;      // Nächster Abschnitt beginnt?

                if( isalnum( *p ) ) {
                    char* q = strchr( p, '=' );
                    if( q ) {
                        while( q > p  &&  q[-1] == ' ' )  q--;
                        *q = '\0';
                        buffer.append( p, q+1 - p );  //buffer += '\0';
                    }
                }
            }
        }
        catch( const Eof_error& ) {}

        buffer += '\0';
        value_ptr->assign( buffer );
        assert_no_include( buffer );
    }
    else
    {
        int pos = __ini_lookup( &f, &area, group_name, entry_name ); 
        value_ptr->assign( area.char_ptr() + pos, area.length() - pos );

        while( area.char_ptr()[ area.length() - 1 ] == '\\' ) {
            value_ptr->length( value_ptr->length() - 1 );       // '\\' wegnehmen
            f.get( &area );
            value_ptr->append( area );
        }
    }


    ok = true;
}
catch( const Not_found_error& ) { ok = false; }
catch( const Not_exist_error& ) { ok = false; }
catch( const Eof_error& )       { ok = false; }

if( !ok )  value_ptr->assign( default_result );         // jz 30.11.97
else
{
    truncate_spaces( value_ptr );
}

return ok;
}


static int __ini_lookup( Any_file* f, Area* area_ptr, const char* group_name, const char* entry_name, int include_depth )
{
  //int           group_name_length = strlen( group_name );
    int           entry_name_length = strlen( entry_name );
    const char*   include_str = "include=";
    int           include_str_length = strlen( include_str );

    if( include_depth > 5 )  throw_xc( "SOS-1141" );

    area_ptr->length( 0 );
/*
    if ( area_ptr->size() > 0 )
    {
        area_ptr->char_ptr()[0] = 0;
    } else return; // leeres Area ?
*/
    find_group( f, group_name );

    while(1) {
        f->get( area_ptr );  

        if( area_ptr->length() > entry_name_length
         && strnicmp( area_ptr->char_ptr(), entry_name, entry_name_length ) == 0 )
        {
            const char* p = area_ptr->char_ptr() + entry_name_length;
            while( p < area_ptr->char_ptr() + area_ptr->length()  &&  isspace( *p ) )  p++;
            if( p < area_ptr->char_ptr() + area_ptr->length()  &&  *p == '=' ) {
                p++;
                while( p < area_ptr->char_ptr() + area_ptr->length()  &&  isspace( *p ) )  p++;
                return p  - area_ptr->char_ptr();
            }
        }

        if( area_ptr->length() > 1 && area_ptr->char_ptr()[ 0 ] == '[' )  throw_not_found_error();

        if ( area_ptr->length() > include_str_length
          && strnicmp( area_ptr->char_ptr(), include_str, include_str_length ) == 0
          /*&& include_depth == 0*/ )
        {
            char include_filename[_MAX_PATH];
            memcpy( include_filename, area_ptr->char_ptr()+include_str_length, area_ptr->length() - include_str_length );
            include_filename[area_ptr->length() - include_str_length] = 0;

            area_ptr->length(0); // dieser record interessiert nicht mehr

            try {
                Any_file include_file( include_filename, Any_file::in );
                __ini_lookup( &include_file, area_ptr, group_name, entry_name, include_depth+1 );
                if ( area_ptr->length() > 0 ) break;
            }
            catch(...) {}
        }
    }

    return 0;
}



void write_profile_entry( const Const_area& value, const char* filename, const char* group_name, const char* entry_name )
{
    throw_xc( "write_profile_entry" );
}

void write_profile( const char* filename, const char* section,
                    const char* entry, const char* value )
{
    throw_xc( "write_profile" );
}


#endif // SYSTEM_WIN

} //namespace sos
