// $Id: spooler_module_remote.h,v 1.14 2003/08/30 22:40:27 jz Exp $

#ifndef __SPOOLER_MODULE_REMOTE_H
#define __SPOOLER_MODULE_REMOTE_H

#include "../zschimmer/com_remote.h"

namespace sos {
namespace spooler {

//---------------------------------------------------------------------Remote_module_instance_proxy

struct Remote_module_instance_proxy : Com_module_instance_base
{

    struct Operation : Async_operation
    {

        enum Call_state
        {
            c_none,

            c_begin,            // call__begin()
          //c_select_process,   //   Prozess auswählen (evtl
            c_connect,          //   Mit Server verbinden
            c_create_instance,  
            c_construct,        
            c_call_begin,       //   spooler_open() 

            c_end,              // end__begin()
            c_call_end,         //   spooler_close()
            c_release,          //   Release()

            c_finished,
        };


                                Operation                   ( Remote_module_instance_proxy*, Call_state first_state );
                               ~Operation                   ();

        Async_operation*        begin__start                ();
        bool                    begin__end                  ();

        virtual bool            async_finished_             ();
        virtual void            async_continue_             ( bool wait = false );
      //virtual bool            async_has_error_            ()                                      { return _operation? _operation->async_has_error() : false; }
      //virtual void            async_check_error_          ()                                      { if( _operation )  _operation->async_check_error(); }
        virtual string          async_state_text_           ();

        string                  state_name                  ();


        Fill_zero              _zero_;
        Remote_module_instance_proxy* _proxy;
        Call_state             _call_state;
        Multi_qi               _multi_qi;
      //ptr<Async_operation>   _operation;
    };



                                Remote_module_instance_proxy( Module* module, string process_class_name )   : Com_module_instance_base(module), _zero_(_end_), _process_class_name(process_class_name) {}
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

    virtual Async_operation*    call__start                 ( const string& method );
    virtual bool                call__end                   ();


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
