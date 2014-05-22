// olereg.cxx                                (C)1997 SOS GmbH Berlin
//                                              Joacim Zschimmer
//#define MODULE_NAME "hostole2"

/*
    Funktionen zur Registrierung der .dll bzw .exe

    HRESULT hostole_register_server();
    HRESULT hostole_unregister_server();
*/

#include "precomp.h"

#if defined __BORLANDC__
#   include <windows.h>
#   include <ole2.h>
#   include <variant.h>
#   include <shellapi.h>
#   include <cstring.h>
#endif

#ifdef _WIN32
#   include "olectl.h"         // DllRegisterServer
#endif

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosappl.h"
#include "../kram/licence.h"

//#include "hostole.hrc"
#include "../kram/olestd.h"
#include "olereg.h"

#undef LOG  //OLE
#define LOG(X) do{ char _buffer_[300];ostrstream _s_(_buffer_,sizeof _buffer_); _s_ << X << '\0';_buffer_[sizeof _buffer_-1]='\0';OutputDebugString(_buffer_);}while(0)


#ifdef _WIN32
#   define _32    "32"
#   define _16_32 "32"
#else
#   define _32    ""
#   define _16_32 "16"
#endif


namespace sos {
using namespace std;

//Count number of objects and number of locks.
extern ULONG       hostole_object_count;
extern ULONG       hostole_lock;
extern const Bool _dll;

Sos_string module_filename();
Sos_string directory_of_path( const Sos_string& );      // sysdep.cxx
Sos_string basename_of_path( const Sos_string& path );  // sysdep.cxx

//--------------------------------------------------------------------------------reg_write_key

void reg_write_key( HKEY hkey, const Sos_string& key_name, const Sos_string& name, const Sos_string& value )
{
    LONG  err;

    if( length( key_name ) == '\0' ) 
    {
#       ifdef _WIN32
            err = RegSetValueExA( hkey, c_str( name ), 0, REG_SZ, (const Byte*)c_str( value ), length( value ) + 1 );
            if( err )  { SetLastError( err ); throw_mswin_error( "RegSetValueEx" ); }
#       else
            err = RegSetValue( hkey, c_str( name ), REG_SZ, c_str( value ), length( value ) + 1 );
            if( err )  throw_xc( Msg_code( "MSWIN-RegSetValue-", err ) );
#       endif
    }
    else
    {
        HKEY  key = 0;

#       ifdef _WIN32
            DWORD disposition;
            err = RegCreateKeyEx( hkey, c_str( key_name ), 0, "", 0, KEY_WRITE, NULL, &key, &disposition );
            if( err )  { SetLastError( err ); throw_mswin_error( "RegCreateKeyEx" ); }
#       else
            err = RegCreateKey( hkey, c_str( key_name ), &key );
            if( err )  throw_xc( Msg_code( "MSWIN-RegCreateKey-", err ) );
#       endif
    
        reg_write_key( key, "", name, value );
    
        RegCloseKey( key );
    }
}

//-------------------------------------------------------------------------------reg_key_exists

Bool reg_key_exists( HKEY key, const char* key_name )
{
    HKEY k;
    long err;

#   ifdef _WIN32
        err = RegOpenKeyEx( key, key_name, 0, KEY_QUERY_VALUE, &k );
#   else
        err = RegOpenKey( key, key_name, &k );
#   endif
    if( !err )  RegCloseKey( k );

    return err == ERROR_SUCCESS;  //err != ERROR_BADKEY;  Bei Fehler wird 2 geliefert!?
}

//--------------------------------------------------------------------------------major_version

static Sos_string major_version( const Sos_string& version )
{
    return as_string( c_str( version ), position( c_str( version ), '.' ) );
}

//----------------------------------------------------------------------hostole_register_typelib

HRESULT hostole_register_typelib( const IID& typelib_id, const Sos_string& name, const Sos_string& version,
                                  const Sos_string& typelib_filename )
{
    try {
      //int         len;
        Sos_string  typelib_filename;
      //Sos_string  major_vers = major_version( version );

#       ifdef SYSTEM_WIN32
            typelib_filename = module_filename();
#       else
            typelib_filename = directory_of_path( module_filename() ) + '\\' + basename_of_path( module_filename() ) + ".tlb";
#       endif

        HRESULT   hr;
        ITypeLib* type_lib            = NULL;
        BSTR      type_lib_filename_w = SysAllocStringLen_char( c_str( typelib_filename ), length( typelib_filename ) );

        hr = LoadTypeLib( type_lib_filename_w, &type_lib );
        if( FAILED( hr ) )  return (HRESULT)SELFREG_E_TYPELIB;

        hr = RegisterTypeLib( type_lib, type_lib_filename_w, NULL );
        if( FAILED( hr ) )  return (HRESULT)SELFREG_E_TYPELIB;

        SysFreeString( type_lib_filename_w );

        return NOERROR;
    }
    catch( const Xc& x ) {
        SHOW_MSG( "Fehler beim Registrieren: " << x );
        return (HRESULT)E_FAIL;
    }
}


//--------------------------------------------------------------------hostole_unregister_typelib

HRESULT hostole_unregister_typelib( const IID& typelib_id, const Sos_string& name, const Sos_string& version )
{
    UnRegisterTypeLib( typelib_id,
                       1,1, //major_version( version ), minor_version( version ),
                       LANG_NEUTRAL,
#                      if defined SYSTEM_WIN32
                           SYS_WIN32
#                      else
#                          error SYSKIND
#                      endif
                     );

    return NOERROR;
}

//----------------------------------------------------------------------hostole_register_class

HRESULT hostole_register_class( const IID& typelib_id, const IID& clsid, const Sos_string& name, 
                                const Sos_string& version, const Sos_string& title  )
{
    try {
        long        err;
        int         len;
        HKEY        key = 0;
        Sos_string  k;
        Sos_string  major_vers    = major_version( version );
        Sos_string  name_major    = name + "." + major_vers;


        OLECHAR typelib_id_text [ 38+1 ];
        len = StringFromGUID2( typelib_id, typelib_id_text, sizeof typelib_id_text );
        if( len == 0 )  throw_mswin_error( "StringFromGUID2" );

        OLECHAR clsid_text [ 38+1 ];
        len = StringFromGUID2( clsid, clsid_text, sizeof clsid_text );
        if( len == 0 )  throw_mswin_error( "StringFromGUID2" );

        // HKEY_CLASSES_ROOT\hostWare.File

#       if defined _WIN32
            DWORD disposition;
            err = RegCreateKeyEx( HKEY_CLASSES_ROOT, c_str( name ),
                                  0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disposition );
            if( err )  { SetLastError( err ); throw_mswin_error( "RegCreateKeyEx" ); }
#        else
            err = RegCreateKey( HKEY_CLASSES_ROOT, c_str( name ), &key );
            if( err )  throw_xc( Msg_code( "MSWIN-RegCreateKey-", err ) );
#       endif

        reg_write_key( key, ""              , "", title );
        reg_write_key( key, "CLSID"         , "", w_as_string( clsid_text ) );
        reg_write_key( key, "NotInsertable" , "", "" );
        reg_write_key( key, "CurVer"        , "", name + "." + major_vers );

        RegCloseKey( key );


        // HKEY_CLASSES_ROOT\hostWare.File.1

#       if defined _WIN32
            err = RegCreateKeyEx( HKEY_CLASSES_ROOT, c_str(name_major),
                                  0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disposition );
            if( err )  { SetLastError( err ); throw_mswin_error( "RegCreateKeyEx" ); }
#        else
            err = RegCreateKey( HKEY_CLASSES_ROOT, c_str( name_major ), &key );
            if( err )  throw_xc( Msg_code( "MSWIN-RegCreateKey-", err ) );
#       endif

        reg_write_key( key, ""                            , "", title );       //  + " (Version " + version + ')'
        reg_write_key( key, "CLSID"                       , "", w_as_string(clsid_text) );
        reg_write_key( key, "NotInsertable"               , "", ""  );

        RegCloseKey( key );


    
        k = "CLSID\\";
        k += w_as_string(clsid_text);
#       ifdef _WIN32
            err = RegCreateKeyEx( HKEY_CLASSES_ROOT, c_str( k ),
                                  0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &disposition );
            if( err )  { SetLastError( err ); throw_mswin_error( "RegCreateKeyEx" ); }
#        else
            err = RegCreateKey( HKEY_CLASSES_ROOT, c_str( k ), &key );
            if( err )  throw_xc( Msg_code( "MSWIN-RegCreateKey-", err ) );
#       endif

        reg_write_key( key, ""                        , "", title );  //  + " (Version " + version + ")"

        string server_type = _dll? "InprocServer" _32 : "LocalServer" _32;
        reg_write_key( key, server_type, "", module_filename() );
        reg_write_key( key, server_type, "ThreadingModel", "Both" );

        reg_write_key( key, "NotInsertable"           , "", "" );   // damit erscheint Objekt nicht in "Insert Object Dialog boxes"
        reg_write_key( key, "ProgID"                  , "", name_major );
        reg_write_key( key, "Programmable"            , "", "" );   // nach K. Brockschmidt
        reg_write_key( key, "VersionIndependentProgID", "", name );
        reg_write_key( key, "TypeLib"                 , "", w_as_string(typelib_id_text) );

        RegCloseKey( key );

        return NOERROR;
    }
    catch( const Xc& x ) {
        SHOW_MSG( "Fehler beim Registrieren: " << x );
        return (HRESULT)E_FAIL;
    }
}

//----------------------------------------------------------------------hostole_unregister_class

HRESULT hostole_unregister_class( const IID& typelib_id, const IID& clsid, const Sos_string& name, 
                                  const Sos_string& version )
{
    Sos_string  k;
    Sos_string  str;
    OLECHAR     clsid_text [ 38+1 ];
    int         len;
    HKEY        key;
    Sos_string  major_vers    = major_version( version );
    Sos_string  name_major    = name + "." + major_vers;
    //Bool        delete_all = false;

    len = StringFromGUID2( clsid, clsid_text, sizeof clsid_text );
    if( len == 0 )  throw_mswin_error( "StringFromGUID2" );

    k = "CLSID\\";
    k += w_as_string(clsid_text);

    if( RegOpenKey( HKEY_CLASSES_ROOT, c_str( k ), &key ) == ERROR_SUCCESS )
    {
        RegDeleteKey( key, "InprocServer" );
        RegDeleteKey( key, "InprocServer32" );
        RegDeleteKey( key, "LocalServer" );
        RegDeleteKey( key, "LocalServer32" );
        RegDeleteKey( key, "NotInsertable" );
        RegDeleteKey( key, "ProgID" );
        RegDeleteKey( key, "Programmable" );
        RegDeleteKey( key, "TypeLib" );
        RegDeleteKey( key, "VersionIndependentProgID" );
        RegCloseKey( key );
        RegDeleteKey( HKEY_CLASSES_ROOT, c_str( k ) );
    }
    else
    {
        //delete_all = true;
    }

    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name + "\\CLSID") );
    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name + "\\CurVer") );
    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name + "\\NotInsertable") );
    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name) );

    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name_major + "\\CLSID") );
    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name_major + "\\NotInsertable") );
    RegDeleteKey( HKEY_CLASSES_ROOT, c_str(name_major) );

    return S_OK;
}


} //namespace sos
