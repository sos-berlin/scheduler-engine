// $Id: winapifl.cxx 11394 2005-04-03 08:30:29Z jz $                                     ©1998 SOS Software Gmbh
// Jörg Schwiemann




#include "precomp.h"


#if defined SYSTEM_WIN32 // Registry sollte unter WIN16 über die Nicht-Ex-Methoden gehen, Version nur unter WIN32

#include <windows.h>
#if defined SYSTEM_WIN16
#   include <shellapi.h>
#endif

#include <limits.h>

#include <ctype.h>
#include "../kram/sysdep.h"
#include "../kram/sos.h"
#include "../file/absfile.h"
#include "../kram/sosopt.h"
#include "../kram/soslimtx.h"

namespace sos {


enum Winapi_style { was_none=0, was_registry=1, was_version=2 };

//------------------------------------------------------------------------------------winapi_file

struct Winapi_file : Abs_file
{
                                    Winapi_file             ();
                                   ~Winapi_file             ();

    void                            open                ( const char*, Open_mode, const File_spec& );
    void                            close               ( Close_mode );

  protected:
    void                            get_record          ( Area& );

  private:
    Fill_zero                      _zero_;
    Bool                           _eof;
    // Registry
    HKEY                           _key;
    Sos_string                     _key_name;
    // Version
    Sos_string                     _version; // Windows-Version
    Winapi_style                   _style;
};

//----------------------------------------------------------------------winapi_file_type

struct Winapi_file_type : Abs_file_type
{
    virtual const char*         name                    () const { return "winapi"; }
    virtual Sos_ptr<Abs_file>   create_base_file        () const
    {
        Sos_ptr<Winapi_file> f = SOS_NEW( Winapi_file() );
        return +f;
    }
};

const Winapi_file_type      _winapi_file_type;
const Abs_file_type&        winapi_file_type = _winapi_file_type;

//----------------------------------------------------------------------------------------const

// --------------------------------------------------------------------winapi_file::winapi_file

Winapi_file::Winapi_file()
:
    _zero_(this+1)
{
}

// -------------------------------------------------------------------winapi_file::~winapi_file

Winapi_file::~Winapi_file()
{
}

// -------------------------------------------------------------------------winapi_file::open

void Winapi_file::open( const char* param, Open_mode open_mode, const File_spec& file_spec )
{
    Sos_string  reg_key;
    Sos_string  key;

    for( Sos_option_iterator opt ( param ); !opt.end(); opt.next() )
    {
        if(      opt.with_value( "registry") ) { _style = was_registry; reg_key = opt.value(); }
        else if( opt.flag( "version") )        { _style = was_version; }
        else throw_sos_option_error( opt );
    }
    
    switch ( _style ) {
    case was_version:
        OSVERSIONINFO ver;
        ver.dwOSVersionInfoSize = sizeof ver;
        if( !GetVersionEx( &ver ) )  throw_mswin_error( "GetVersionEx" );

        switch ( ver.dwPlatformId ) {
        case VER_PLATFORM_WIN32s:           _version = "WIN32s";        break;
        case VER_PLATFORM_WIN32_WINDOWS:    _version = "WIN32_WINDOWS"; break;
        case VER_PLATFORM_WIN32_NT:         _version = "WIN32_NT";      break;
        }

        break;
    case was_registry:
    {
        const char* slash = strchr( c_str(reg_key), '\\' );
        if ( !slash ) throw_syntax_error( "SOS-1390" );
        _key_name = slash + 1;

        key = sub_string( c_str(reg_key), 0, slash-c_str(reg_key) );

        if      ( key == "HKEY_CLASSES_ROOT"  || key == "HKCR" ) _key = HKEY_CLASSES_ROOT;
        else if ( key == "HKEY_CURRENT_USER"  || key == "HKCU" ) _key = HKEY_CURRENT_USER;
        else if ( key == "HKEY_LOCAL_MACHINE" || key == "HKLM" ) _key = HKEY_LOCAL_MACHINE;
        else if ( key == "HKEY_USERS"                          ) _key = HKEY_USERS;
        else {
            throw_syntax_error( "SOS-1391", c_str(key) );
        }
        LOG( "Winapi_file::open: hkey=" << key << " name=" << _key_name << '\n' );
        break;
    }
    default: throw_xc( "Winapi: missing style" );
    }
}

// --------------------------------------------------------------------------Winapi_file::close

void Winapi_file::close( Close_mode close_mode )
{
}


// ---------------------------------------------------------------------winapi_file::get_record

void Winapi_file::get_record( Area& buffer )
{
    switch ( _style ) {
    case was_version:
        buffer.assign( c_str(_version) );
        break;
    case was_registry:
    {
        Dynamic_area buff;

        if ( _eof ) throw_eof_error();
        _eof = true;

        HKEY ini_key = 0;
        int ret;
        unsigned long len;

        Sos_string key;
        Sos_string subkey = "";
        const char* key_name = c_str(_key_name);
        const char* slash = strrchr( key_name, '\\' );

        if ( slash ) {
            subkey = slash + 1;
            key = sub_string( key_name, 0, slash-key_name );
        } else key = key_name;


        ret = RegOpenKeyEx( _key, c_str(key), 0, KEY_READ | KEY_EXECUTE, &ini_key );
        if( ret != ERROR_SUCCESS ) {
            long err_code = GetLastError();
            LPVOID lpMsgBuf;

            if ( err_code != 0 ) {
                FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                               FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    
                               NULL,    
                               err_code,    
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               (LPTSTR) &lpMsgBuf,    0,    NULL );// Process any inserts in lpMsgBuf.
                LOG( "Winapi_file::get_record: RegOpenKeyEx(key=" << _key << ",subkey='" << key << "') error=" << err_code << " => " << (LPCTSTR)lpMsgBuf << '\n' );
                // Free the buffer.
                LocalFree( lpMsgBuf );
                throw_eof_error();
            }
        }

        buff.allocate_min( 1024 );
        len = buff.size();
        DWORD reg_type;
        ret = RegQueryValueEx( ini_key, c_str(subkey), NULL, &reg_type, (unsigned char*)buff.char_ptr(), &len );
        if ( ret != ERROR_SUCCESS ) {
            long  err_code = GetLastError();
            LPVOID lpMsgBuf;

            if ( err_code != 0 ) {
                FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                               FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    
                               NULL,    
                               err_code,    
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               (LPTSTR) &lpMsgBuf,    0,    NULL );// Process any inserts in lpMsgBuf.
                LOG( "Winapi_file::get_record: RegQueryValue('" << subkey << "' error=" << err_code << " => " << (LPCTSTR)lpMsgBuf << '\n' );
                // Free the buffer.
                LocalFree( lpMsgBuf );
            
                RegCloseKey( ini_key );
                throw_eof_error();
            }
        }

        if ( reg_type != REG_SZ && reg_type != REG_NONE ) {
            LOG( "winapi_file: unsupported reg_type: " << reg_type << '\n' );
            throw_eof_error();
        }

        RegCloseKey( ini_key );
    
        buffer.assign( buff.char_ptr(), len - 1 );
        break;
    }
    }
}


} //namespace sos

#endif // SYSTEM_WIN32


