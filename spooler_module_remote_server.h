// $Id: spooler_module_remote_server.h,v 1.2 2003/05/29 20:17:21 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_SERVER_H
#define __SPOOLER_MODULE_REMOTE_SERVER_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

Z_DEFINE_GUID( CLSID_Remote_module_instance_server, 0x0628c299, 0x0aa2, 0x4546, 0xbb, 0xe7, 0x38, 0xeb, 0xc2, 0x95, 0x28, 0x34 );   // {0628C299-0AA2-4546-BBE7-38EBC2952834}
Z_DEFINE_GUID(   IID_Remote_module_instance_server, 0x3e6bf40f, 0xe23a, 0x457b, 0xbf, 0x28, 0x79, 0x7b, 0x34, 0x51, 0xae, 0xd3 );   // {3E6BF40F-E23A-457b-BF28-797B3451AED3}

//--------------------------------------------------------------------Remote_module_instance_server

struct Remote_module_instance_server : Com_module_instance_base
{
                                Remote_module_instance_server()                                 : Com_module_instance_base(NULL), _zero_(this+1) {}
                               ~Remote_module_instance_server();

    static HRESULT              create_instance             ( const CLSID&, IUnknown** );

    STDMETHODIMP               _spooler_construct           ( SAFEARRAY* );
    STDMETHODIMP               _spooler_add_obj             ( IDispatch*, BSTR name );


    Fill_zero                  _zero_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
