// $Id: z_windows_registry.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __ZSCHIMMER_WINDOWS_REGISTRY_H
#define __ZSCHIMMER_WINDOWS_REGISTRY_H

#ifdef Z_WINDOWS

namespace zschimmer {
namespace windows {

/*
    Werte für REGSAM:

    DELETE                  The right to delete the object. 
    READ_CONTROL            The right to read the information in the object's security descriptor, not including the information in the SACL. 
    WRITE_DAC               The right to modify the DACL in the object's security descriptor. 
    WRITE_OWNER             The right to change the owner in the object's security descriptor. 

    STANDARD_RIGHTS_ALL     Combines DELETE, READ_CONTROL, WRITE_DAC, WRITE_OWNER, and SYNCHRONIZE access. 
    STANDARD_RIGHTS_EXECUTE Currently defined to equal READ_CONTROL. 
    STANDARD_RIGHTS_READ    Currently defined to equal READ_CONTROL. 
    STANDARD_RIGHTS_REQUIRED Combines DELETE, READ_CONTROL, WRITE_DAC, and WRITE_OWNER access. 
    STANDARD_RIGHTS_WRITE   Currently defined to equal READ_CONTROL

    KEY_ALL_ACCESS          Combines the STANDARD_RIGHTS_REQUIRED, KEY_QUERY_VALUE, KEY_SET_VALUE, KEY_CREATE_SUB_KEY, KEY_ENUMERATE_SUB_KEYS, KEY_NOTIFY, and KEY_CREATE_LINK access rights. 
    KEY_CREATE_LINK         Reserved for system use. 
    KEY_CREATE_SUB_KEY      Required to create a subkey of a registry key. 
    KEY_ENUMERATE_SUB_KEYS  Required to enumerate the subkeys of a registry key. 
    KEY_EXECUTE             Equivalent to KEY_READ. 
    KEY_NOTIFY              Required to request change notifications for a registry key or for subkeys of a registry key. 
  ->KEY_QUERY_VALUE         Required to query the values of a registry key. 
    KEY_READ                Combines the STANDARD_RIGHTS_READ, KEY_QUERY_VALUE, KEY_ENUMERATE_SUB_KEYS, and KEY_NOTIFY values. 
    KEY_SET_VALUE           Required to create, delete, or set a registry value. 
    KEY_WOW64_64KEY         Windows XP:  Enables a 64- or 32-bit application to open a 64-bit key.
    KEY_WOW64_32KEY         Windows XP:  Enables a 64- or 32-bit application to open a 32-bit key.
    KEY_WRITE               Combines the STANDARD_RIGHTS_WRITE, KEY_SET_VALUE, and KEY_CREATE_SUB_KEY access rights. 
*/
//-------------------------------------------------------------------------------------------------

string                          registry_query_value       ( HKEY, const string& key, const string& deflt );

//-------------------------------------------------------------------------------------Registry_key

struct Registry_key
{
    enum Create_key { dont_create, create };


                                Registry_key                ( HKEY hkey = 0 )                       { open( hkey ); }
                                Registry_key                ( const Registry_key& key )             { open( key ); }
                                Registry_key                ( HKEY base_hkey     , const string& key_name, REGSAM access_rights, Create_key create_key = dont_create );
                                Registry_key                ( const Registry_key&, const string& key_name, REGSAM access_rights, Create_key create_key = dont_create );
                               ~Registry_key                ();

    bool                        try_open                    ( HKEY base_hkey, const string& key_name, REGSAM access_rights ); //= KEY_ALL_ACCESS );
    void                        close                       ();

                                operator HKEY               () const                                { return _hkey; }
    
    string                      get_string                  ( const string& entry_name, const string& deflt );

    void                        set_value                   ( const string& entry_name, const string& value );
    void                        set_subkey                  ( const string& subkey_name, const string& value );
    bool                        delete_subkey               ( const string& subkey_name );
    bool                        delete_subkey_if_existing   ( const string& subkey_name );
    HRESULT                     delete_key                  ();
    HRESULT                     error                       ()                                      { return _error; }

  private:
    void                        open                        ( HKEY hkey );
    void                        open                        ( const Registry_key& key )             { _base_hkey_name = key._name;  open( (HKEY)key ); }

    void                        open                        ( HKEY base_hkey         , const string& key_name, REGSAM access_rights, Create_key create_key = dont_create );
    void                        open                        ( const Registry_key& key, const string& key_name, REGSAM, Create_key = dont_create );

    HKEY                       _hkey;
    bool                       _owner;
    HRESULT                    _error;

    HKEY                       _base_hkey;
    string                     _base_hkey_name;
    string                     _name;
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
#endif
