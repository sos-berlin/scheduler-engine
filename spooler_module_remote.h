// $Id: spooler_module_remote.h,v 1.6 2003/08/25 20:41:26 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_H
#define __SPOOLER_MODULE_REMOTE_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------Remote_module_instance_proxy

struct Remote_module_instance_proxy : Com_module_instance_base
{
    enum Call_state
    {
        c_null,
        c_create_instance,
        c_call_begin,
        c_finished
    };


                                Remote_module_instance_proxy( Module* module )                      : Com_module_instance_base(module), _zero_(this+1) {}
                               ~Remote_module_instance_proxy();

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
  
    void                        add_obj                     ( const ptr<IDispatch>&, const string& name );
    bool                        name_exists                 ( const string& name );
    Variant                     call                        ( const string& name );

    virtual void                begin__start                ();
  //virtual void                begin__start                ( const Object_list& );
    virtual bool                begin__end                  ();

    virtual void                end__start                  ( bool success );
    virtual void                end__end                    ();

    virtual void                step__start                 ();
    virtual bool                step__end                   ();

    virtual bool                operation_finished          ();
    virtual void                process                     ( bool wait = false );

    Fill_zero                  _zero_;

    ptr<object_server::Session>    _session;
    ptr<object_server::Proxy>      _remote_instance;
  //ptr<object_server::Operation>  _operation;
    Call_state                     _call_state;
    Multi_qi                       _multi_qi;
    Xc_copy                        _error;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
