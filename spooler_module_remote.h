// $Id: spooler_module_remote.h,v 1.16 2003/08/31 22:32:42 jz Exp $

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
        c_none,

        c_begin,            // call__begin()
        c_connect,          //   Mit Server verbinden
        c_create_instance,  
        c_construct,        
        c_call_begin,       //   spooler_open() 

      //c_end,              // end__begin()
      //c_call_end,         //   spooler_close()
      //c_release,          //   Release()

        c_finished,
    };


    struct Operation : Async_operation
    {
                                Operation                   ( Remote_module_instance_proxy*, Call_state first_state );
                               ~Operation                   ();

        Async_operation*        begin__start                ();
        bool                    begin__end                  ();

        virtual bool            async_finished_             ();
        virtual bool            async_continue_             ( bool wait )                           { return _proxy->continue_async_operation( this, wait ); }
      //virtual bool            async_has_error_            ();
      //virtual void            async_check_error_          ();
        virtual string          async_state_text_           ();

        string                  state_name                  ();


        Fill_zero              _zero_;
        Remote_module_instance_proxy* _proxy;
        Call_state             _call_state;
        Multi_qi               _multi_qi;
    };



                                Remote_module_instance_proxy( Module* module, string process_class_name )   : Com_module_instance_base(module), _zero_(_end_), _process_class_name(process_class_name) {}
                               ~Remote_module_instance_proxy();

    void                        init                        ();
    void                        load                        ();
    void                        close                       ();
    bool                        kill                        ();
  
    void                        add_obj                     ( const ptr<IDispatch>&, const string& name );
    bool                        name_exists                 ( const string& name );
    Variant                     call                        ( const string& name );

    virtual Async_operation*    begin__start                ();
    virtual bool                begin__end                  ();

    virtual Async_operation*    end__start                  ( bool success );
    virtual void                end__end                    ();

    virtual Async_operation*    step__start                 ();
    virtual bool                step__end                   ();

    virtual Async_operation*    call__start                 ( const string& method );
    virtual bool                call__end                   ();

    virtual Async_operation*    release__start              ();
    virtual void                release__end                ();

    bool                        continue_async_operation    ( Operation*, bool wait );

    Fill_zero                  _zero_;

    string                         _process_class_name;
    ptr<Process>                   _process;
    ptr<object_server::Session>    _session;
    ptr<object_server::Proxy>      _remote_instance;
    ptr<Async_operation>           _operation;
    bool                           _end_success;            // Für end__start()

    Fill_end                   _end_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
