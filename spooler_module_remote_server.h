// $Id: spooler_module_remote_server.h,v 1.1 2003/05/23 06:40:28 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_SERVER_H
#define __SPOOLER_MODULE_REMOTE_SERVER_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------Remote_module_instance_server

struct Remote_module_instance_server : Com_module_instance_base
{
                                Remote_module_instance_server()                                 : Com_module_instance_base(script), _zero_(this+1) {}
                               ~Remote_module_instance_server();


    STDMETHODIMP               _spooler_construct           ( SAFEARRAY* );
    STDMETHODIMP               _spooler_add_obj             ( IDispatch*, BSTR name );


    Fill_zero                  _zero_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
