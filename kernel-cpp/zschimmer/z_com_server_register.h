// $Id: z_com_server_register.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __Z_COM_SERVER_REGISTER_H
#define __Z_COM_SERVER_REGISTER_H

namespace zschimmer {
namespace com {

//-------------------------------------------------------------------------------------------------

//HRESULT                         com_register_typelib        ( const string& typelib_filename, const GUID& typelib_id, const string& name );
//HRESULT                         com_unregister_typelib      ( const GUID& typelib_id, const string& name, const string& version );

HRESULT                         Com_register_class          ( const string& filename, const GUID& typelib_id, const CLSID&, const string& class_name, const string& version, const string& title );
HRESULT                         Com_unregister_class        ( const IID& clsid, const string& class_name, const string& version );

//-------------------------------------------------------------------------------------------------

} //namespace com
} //namespace zschimmer

#endif
