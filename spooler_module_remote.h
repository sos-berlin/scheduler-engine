// $Id$

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

        c_release_begin,    // Wenn Monitor.spooler_task_before() false geliefert hat
        c_release,

        c_finished,
    };


    struct Operation : Async_operation
    {
                                Operation                   ( Remote_module_instance_proxy*, Call_state first_state );
                               ~Operation                   ();

        Async_operation*        begin__start                ();
        bool                    begin__end                  ();

        virtual bool            async_finished_             () const;
        virtual bool            async_continue_             ( Continue_flags flags )                    { return _proxy->continue_async_operation( this, flags ); }
      //virtual bool            async_has_error_            ();
      //virtual void            async_check_error_          ();
        virtual string          async_state_text_           () const;

        string                  state_name                  () const;


        Fill_zero              _zero_;
        Remote_module_instance_proxy* _proxy;
        Call_state             _call_state;
        Multi_qi               _multi_qi;
        bool                   _bool_result;
    };



                                Remote_module_instance_proxy( Module* module )                      : Com_module_instance_base(module), _zero_(_end_) {}
                               ~Remote_module_instance_proxy();

    void                        init                        ();
    bool                        load                        ();
    void                        close                       ();
    bool                        kill                        ();
    void                        detach_process              ();
  
    void                        add_obj                     ( IDispatch*, const string& name );
  //void                        add_log_obj                 ( Com_log*, const string& name );
    bool                        name_exists                 ( const string& name );
    Variant                     call                        ( const string& name );

    bool                        try_to_get_process          ();

            Async_operation*    close__start                ();
            void                close__end                  ();

    virtual Async_operation*    begin__start                ();
    virtual bool                begin__end                  ();

    virtual Async_operation*    end__start                  ( bool success );
    virtual void                end__end                    ();

    virtual Async_operation*    step__start                 ();
    virtual Variant             step__end                   ();

    virtual Async_operation*    call__start                 ( const string& method );
    virtual Variant             call__end                   ();

    virtual Async_operation*    release__start              ();
    virtual void                release__end                ();

    bool                        continue_async_operation    ( Operation*, Async_operation::Continue_flags );
    void                        check_connection_error      ();
    int                         exit_code                   ();
    int                         termination_signal          ();
    string                      stdout_path                 ();
    string                      stderr_path                 ();

    Fill_zero                  _zero_;

  //string                         _process_class_name;
    ptr<Process>                   _process;
    ptr<object_server::Session>    _session;
    ptr<object_server::Proxy>      _remote_instance;
    ptr<Async_operation>           _operation;
    bool                           _end_success;            // Für end__start()

    string                      _server_hostname;
    int                         _server_port;
    int                         _exit_code;
    int                         _termination_signal;

    Fill_end                   _end_;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
