// olereg.h                                        ©1997 SOS GmbH Berlin
//                                                 Joacim Zschimmer
// Funktionen zum Registrieren eines OLE-Servers


#ifndef __OLEREG_H
#define __OLEREG_H

namespace sos
{

HRESULT hostole_register_server();
HRESULT hostole_unregister_server();

HRESULT hostole_register_typelib( const IID& typelib_id, const Sos_string& name, const Sos_string& version, const Sos_string& typelib_filename );
HRESULT hostole_unregister_typelib( const IID& typelib_id, const Sos_string& name, const Sos_string& version );

HRESULT hostole_register_class( const IID& typelib_id, const IID& clsid, const Sos_string& name, 
                                const Sos_string& version, const Sos_string& title );

HRESULT hostole_unregister_class( const IID& typelib_id, const IID& clsid, const Sos_string& name, const Sos_string& version );

} //namespace sos


#endif
