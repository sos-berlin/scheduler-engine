// $Id: spooler_module_remote.h,v 1.2 2003/05/29 20:17:21 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_H
#define __SPOOLER_MODULE_REMOTE_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------Remote_module_instance_proxy

struct Remote_module_instance_proxy : Com_module_instance_base
{
                                Remote_module_instance_proxy( Module* module )                      : Com_module_instance_base(module), _zero_(this+1) {}
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
