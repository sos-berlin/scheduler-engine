// $Id: spooler_module_remote.h,v 1.7 2003/08/27 10:22:58 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_H
#define __SPOOLER_MODULE_REMOTE_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------Remote_module_instance_proxy

struct Remote_module_instance_proxy : Com_module_instance_base, 
                                      Async_operation
{
    enum Call_state
    {
        c_null,
        c_create_instance,
        c_begin,
      //c_end,
      //c_step,        
        c_finished
    };


                                Remote_module_instance_proxy( Module* module )                      : Com_module_instance_base(module), _zero_(_end_) {}
                               ~Remote_module_instance_proxy();

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
  
    void                        add_obj                     ( const ptr<IDispatch>&, const string& name );
    bool                        name_exists                 ( const string& name );
    Variant                     call                        ( const string& name );

    virtual Async_operation*    begin__start                ();
    virtual bool                begin__end                  ();

    virtual Async_operation*    end__start                  ( bool success );
    virtual void                end__end                    ();

    virtual Async_operation*    step__start                 ();
    virtual bool                step__end                   ();

    virtual bool                async_finished              ();
    virtual void                async_continue              ( bool wait = false );
    virtual bool                async_has_error             ()                                      { return _error || ( _operation? _operation->async_has_error() : false ); }
    virtual void                async_check_error           ()                                      { if( _error )  throw *_error; else if( _operation )  _operation->async_check_error(); }

    Fill_zero                  _zero_;

    ptr<Process>                   _process;
    ptr<object_server::Session>    _session;
    ptr<object_server::Proxy>      _remote_instance;
    ptr<Async_operation>           _operation;
    Call_state                     _call_state;
    Multi_qi                       _multi_qi;
    Xc_copy                        _error;

    Fill_end                   _end_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
