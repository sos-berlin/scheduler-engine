// $Id: z_com_server_register.cxx 11394 2005-04-03 08:30:29Z jz $

#include "zschimmer.h"
#include "z_com_server.h"
#include "z_com_server_register.h"
#include "z_windows.h"
#include "z_windows_registry.h"

using namespace std;
using namespace zschimmer::windows;

//-------------------------------------------------------------------------------------------------

namespace zschimmer { 
namespace com { 

const bool dll = true;

//------------------------------------------------------------------------------------major_version

static string major_version( const string& version )
{
    uint point = version.find( '.' );
    if( point == string::npos )  point = version.length();

    return string( version.data(), point );
}

//------------------------------------------------------------------------------------minor_version
/*
static string minor_version( const string& version )
{
    int point = version.find( '.' );
    if( point == string::npos )  point = -1;

    return version.c_str() + point + 1;
}
*/
//-----------------------------------------------------------------------------com_register_typelib
/*
HRESULT com_register_typelib( const string& typelib_filename, const GUID& typelib_id, const string& name )
{
    HRESULT         hr;
    ptr<ITypeLib>   typelib;
    Bstr            typelib_filename_bstr = typelib_filename;

    hr = LoadTypeLib( typelib_filename_bstr, typelib.pp() );
    if( FAILED(hr) )  return SELFREG_E_TYPELIB;

    hr = RegisterTypeLib( typelib, typelib_filename_bstr, NULL );
    if( FAILED(hr) )  return SELFREG_E_TYPELIB;

    return S_OK;
}

//---------------------------------------------------------------------------com_unregister_typelib

HRESULT com_unregister_typelib( const GUID& typelib_id, const string& name, const string& version )
{
    HRESULT hr;

    hr = UnRegisterTypeLib( typelib_id, as_int( major_version(version) ), as_int( minor_version(version) ), LANG_NEUTRAL, SYS_WIN32 );

    return hr;
}
*/
//-------------------------------------------------------------------------------Com_register_class

HRESULT Com_register_class( const string& module_filename, const GUID& typelib_id, const CLSID& clsid, const string& class_name, const string& version, const string& title )
{
    try 
    {
        string  class_name_with_version = class_name + "." + major_version( version );
        string  typelib_id_string       = string_from_guid( typelib_id );
        string  clsid_string            = string_from_guid( clsid );


        // HKEY_CLASSES_ROOT\classname
        {
            Registry_key classname_key ( HKEY_CLASSES_ROOT, class_name, KEY_WRITE, Registry_key::create );

            if( !title.empty() )  classname_key.set_value( "", title );
            classname_key.set_subkey( "CLSID"         , string_from_guid( clsid ) );        // Für Windows 2000 SP4 nötig
            classname_key.set_subkey( "NotInsertable" , "" );
            classname_key.set_subkey( "CurVer"        , class_name_with_version );
        }


        // HKEY_CLASSES_ROOT\classname.1
        {
            Registry_key classnamever_key ( HKEY_CLASSES_ROOT, class_name_with_version, KEY_WRITE, Registry_key::create );

            if( !title.empty() )  classnamever_key.set_value( "", title );
            classnamever_key.set_subkey( "CLSID"         , string_from_guid( clsid ) );
            classnamever_key.set_subkey( "NotInsertable" , "" );
        }


        // HKEY_CLASSES_ROOT\CLSID\{...}
        {
            Registry_key clsid_key ( HKEY_CLASSES_ROOT, "CLSID\\" + clsid_string, KEY_WRITE, Registry_key::create );
            clsid_key.set_value( "", rtrim( class_name_with_version + " " + title ) );

            {
                Registry_key inprocserver_key ( clsid_key, "InprocServer32", KEY_ALL_ACCESS, Registry_key::create );
                inprocserver_key.set_value( ""              , module_filename );
                inprocserver_key.set_value( "ThreadingModel", "Both" );
            }

            clsid_key.set_subkey( "NotInsertable"           , "" );               // damit erscheint Objekt nicht in "Insert Object Dialog boxes"
            clsid_key.set_subkey( "ProgID"                  , class_name_with_version );
            clsid_key.set_subkey( "Programmable"            , "" );
            clsid_key.set_subkey( "VersionIndependentProgID", class_name );
            clsid_key.set_subkey( "TypeLib"                 , typelib_id_string );
        }

        return NOERROR;
    }
    catch( const Xc& x ) 
    {
        string text =  x.what();
        MessageBox( 0, text.c_str(), "Fehler beim Registrieren", MB_TASKMODAL );

        return E_FAIL;
    }
}

//-----------------------------------------------------------------------------Com_unregister_class

HRESULT Com_unregister_class( const IID& clsid, const string& class_name, const string& version )
{
    string          clsid_string            = string_from_guid( clsid );
    Registry_key    classes_root_key        ( HKEY_CLASSES_ROOT );
    Registry_key    clsid_key               ( HKEY_CLASSES_ROOT, "CLSID", KEY_SET_VALUE );
    HRESULT         hr = S_OK;

  //string key_name = ;

    Registry_key key;

    if( key.try_open( clsid_key, clsid_string, KEY_SET_VALUE ) )
    {
        if( !key.delete_subkey_if_existing( "InprocServer32"            ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "NotInsertable"             ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "ProgID"                    ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "Programmable"              ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "TypeLib"                   ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "VersionIndependentProgID"  ) )  hr = key.error();
        if( !key.delete_key()                                             )  hr = key.error();

        key.close();
    }
    else
    if( key.error() != ERROR_FILE_NOT_FOUND )  hr = key.error();

    if( key.try_open( HKEY_CLASSES_ROOT, class_name, KEY_SET_VALUE ) )
    {
        if( !key.delete_subkey_if_existing( "CurVer"         ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "CLSID"          ) )  hr = key.error();
        if( !key.delete_subkey_if_existing( "NotInsertable"  ) )  hr = key.error();
        if( !key.delete_key()                      )  hr = key.error();
        key.close();
    }
    else
    if( key.error() != ERROR_FILE_NOT_FOUND )  hr = key.error();


    string major = major_version( version );
    if( !major.empty() )
    {
        string class_name_with_version = class_name + "." + major;

        if( key.try_open( HKEY_CLASSES_ROOT, class_name_with_version, KEY_SET_VALUE ) )
        {
            if( !key.delete_subkey_if_existing( "CLSID"          ) )  hr = key.error();
            if( !key.delete_subkey_if_existing( "NotInsertable"  ) )  hr = key.error();
            if( !key.delete_key()                                  )  hr = key.error();
            key.close();
        }
        else
        if( key.error() != ERROR_FILE_NOT_FOUND )  hr = key.error();
    }

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer
