// $Id: spooler_module_remote.h,v 1.1 2003/05/23 06:40:28 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_H
#define __SPOOLER_MODULE_REMOTE_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

Z_DEFINE_GUID( CLSID_Remote_module_instance, 0x0628c299, 0x0aa2, 0x4546, 0xbb, 0xe7, 0x38, 0xeb, 0xc2, 0x95, 0x28, 0x34 );   // {0628C299-0AA2-4546-BBE7-38EBC2952834}
Z_DEFINE_GUID(   IID_Remote_module_instance, 0x3e6bf40f, 0xe23a, 0x457b, 0xbf, 0x28, 0x79, 0x7b, 0x34, 0x51, 0xae, 0xd3 );   // {3E6BF40F-E23A-457b-BF28-797B3451AED3}

//---------------------------------------------------------------------Remote_module_instance_proxy

struct Remote_module_instance_proxy : Com_module_instance_base
{
                                Remote_module_instance_proxy( Module* script )                      : Com_module_instance_base(script), _zero_(this+1) {}
                               ~Remote_module_instance_proxy();

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
  //IDispatch*                  dispatch                    () const                                { return _idispatch; }
    void                        add_obj                     ( const ptr<IDispatch>&, const string& name );
    bool                        name_exists                 ( const string& name );
    Variant                     call                        ( const string& name );
  //Variant                     call                        ( const string& name, int param );


    Fill_zero                  _zero_;
    ptr<object_server::Proxy>  _remote_instance;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
