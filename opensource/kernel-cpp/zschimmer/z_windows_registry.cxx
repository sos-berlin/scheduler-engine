// $Id: z_windows_registry.cxx 11394 2005-04-03 08:30:29Z jz $

#include "zschimmer.h"
#ifdef Z_WINDOWS

#include "z_windows_registry.h"
#include "log.h"

namespace zschimmer {
namespace windows {

//-----------------------------------------------------------------------------registry_query_value

string registry_query_value( HKEY hkey, const string &key, const string& deflt )
{
    char    value        [ 1000+1 ];
    long    value_len    = sizeof value;

    long err = RegQueryValue( hkey, key.c_str(), value, &value_len );

    return !err? value : deflt;
}

//-----------------------------------------------------------------------Registry_key::Registry_key

Registry_key::Registry_key( HKEY base_hkey, const string& key_name, REGSAM access_rights, Create_key create_key )
{
    open( base_hkey, key_name, access_rights, create_key );
}

//-----------------------------------------------------------------------Registry_key::Registry_key

Registry_key::Registry_key( const Registry_key& key, const string& key_name, REGSAM access_rights, Create_key create_key )
{
    _base_hkey_name = key._name;
    open( (HKEY)key, key_name, access_rights, create_key );
}

//----------------------------------------------------------------------Registry_key::~Registry_key

Registry_key::~Registry_key()
{ 
    close();
}

//----------------------------------------------------------------------Registry_key::~Registry_key

void Registry_key::close()
{ 
    _error = ERROR_SUCCESS;

    if( _hkey && _owner )  _error = RegCloseKey( _hkey ); 

    _hkey       = 0;
    _base_hkey  = 0;
    _name       = "";
    _owner      = false;
}

//-------------------------------------------------------------------------------Registry_key::open

void Registry_key::open( HKEY hkey )
{
    _hkey  = hkey;
    _owner = false;
    _error = 0;
}

//-------------------------------------------------------------------------------Registry_key::open

void Registry_key::open( const Registry_key& key, const string& key_name, REGSAM access_rights, Create_key create_key )
{ 
    _base_hkey_name = key._name;  
    open( (HKEY)key, key_name, access_rights, create_key );
}

//-----------------------------------------------------------------------Registry_key::Registry_key

void Registry_key::open( HKEY base_hkey, const string& key_name, REGSAM access_rights, Create_key create_key )
{
    _error = 0;
    _hkey = 0;
    _base_hkey = base_hkey;

    if( _base_hkey_name.empty() )
    {
        if( base_hkey == HKEY_CLASSES_ROOT  ) _base_hkey_name = "HKEY_CLASSES_ROOT"; 
        else
        if( base_hkey == HKEY_CURRENT_USER  ) _base_hkey_name = "HKEY_CURRENT_USER";
        else
        if( base_hkey == HKEY_LOCAL_MACHINE ) _base_hkey_name = "HKEY_LOCAL_MACHINE"; 
        else
                                              _base_hkey_name = hex_from_int( (int)base_hkey );
    }

    if( create_key == create )
    {
        Z_LOG( "RegCreateKeyEx(" << _base_hkey_name << "," << key_name << ")\n" );

        long error = RegCreateKeyEx( base_hkey, key_name.c_str(), 0, "", 0, access_rights, NULL, &_hkey, NULL );
        if( error )  throw_mswin( error, "RegCreateKeyEx" );
    }
    else
    {
        long error = RegOpenKeyEx( base_hkey, key_name.c_str(), 0, access_rights, &_hkey );
        if( error )  throw_mswin( error, "RegOpenKeyEx", key_name.c_str() );
    }

    _name      = _base_hkey_name + "\\" + key_name;
    _owner     = true;
}

//-----------------------------------------------------------------------Registry_key::Registry_key

bool Registry_key::try_open( HKEY base_hkey, const string& key_name, REGSAM access_rights )
{
    _error = RegOpenKeyEx( base_hkey, key_name.c_str(), 0, access_rights, &_hkey );

    if( _error != ERROR_SUCCESS )  
    {
        Z_LOG( "RegOpenKeyEx  (" << _base_hkey_name << "," << key_name << ") ==> " << _error << "\n" );
        return false;
        //if( _error == ERROR_FILE_NOT_FOUND )  return false;
        //throw_mswin( _error, "RegOpenKeyEx", key_name.c_str() ); 
    }

    _base_hkey = base_hkey;
    _name      = _base_hkey_name + "\\" + key_name;
    _owner     = true;

    return true;
}

//-------------------------------------------------------------------------Registry_key::get_string

string Registry_key::get_string( const string& entry_name, const string& deflt )
{
    DWORD       type        = 0;
    const int   buffer_size = 4000;
    DWORD       length      = buffer_size;
    Byte*       buffer      = new Byte[ buffer_size+1 ];


    _error = RegQueryValueExA( _hkey, entry_name.c_str(), 0, &type, buffer, &length );
    
    if( _error )  
    { 
        delete buffer; 
        if( _error == ERROR_FILE_NOT_FOUND )  return deflt;
        throw_mswin( _error, "RegGetValueEx", (_name + ":" + entry_name).c_str() ); 
    }

    if( type != REG_SZ )  
    { 
        delete buffer;  
        throw_xc( "RegGetValueEx-NotString", _name + ":" + entry_name ); 
    }

    if( buffer[ length-1 ] == '\0' )  length--;
    string result ( (const char*)buffer, length );

    delete [] buffer;
    return result;
}

//--------------------------------------------------------------------------Registry_key::set_value

void Registry_key::set_value( const string& entry_name, const string& value )
{
    Z_LOG( "RegSetValueEx (" << _name << "," << entry_name << "," << value << ")\n" );

    _error = RegSetValueEx( _hkey, entry_name.c_str(), 0, REG_SZ, (const Byte*)value.c_str(), value.length() + 1 );
    if( _error )  throw_mswin( _error, "RegSetValueEx" );
}

//-------------------------------------------------------------------------Registry_key::set_subkey

void Registry_key::set_subkey( const string& subkey_name, const string& value )
{
    Registry_key( *this, subkey_name, KEY_WRITE, create ).set_value( "", value );;
}

//----------------------------------------------------------------------Registry_key::delete_subkey

bool Registry_key::delete_subkey( const string& subkey_name )
{ 
    _error = RegDeleteKey( _hkey, subkey_name.c_str() ); 
    Z_LOG( "RegDeleteKey  (" << _name << "," << subkey_name << ") ==> " << _error << "\n" );

    return _error == 0;
}

//----------------------------------------------------------Registry_key::delete_subkey_if_existing

bool Registry_key::delete_subkey_if_existing( const string& subkey_name )
{ 
    _error = RegDeleteKey( _hkey, subkey_name.c_str() ); 
    Z_LOG( "RegDeleteKey  (" << _name << "," << subkey_name << ") ==> " << _error << "\n" );

    return _error == 0  &&  _error != ERROR_FILE_NOT_FOUND;
}

//-------------------------------------------------------------------------Registry_key::delete_key

long Registry_key::delete_key()
{
    _error = RegDeleteKey( _base_hkey, _name.c_str() );

    return _error == ERROR_SUCCESS;
}

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif

