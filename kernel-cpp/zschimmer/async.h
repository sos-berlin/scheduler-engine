// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __Z_ASYNC_H
#define __Z_ASYNC_H

#include "threads.h"

//-------------------------------------------------------------------------------------------------

namespace zschimmer { 

struct Async_manager;
struct Async_manager_printer;

//----------------------------------------------------------------------------------Async_operation

struct Async_operation : Object
{
    enum Continue_flags
    {
        cont_default                = 0x00,
        cont_wait                   = 0x01,
        cont_next_gmtime_reached    = 0x02,
        cont_signaled               = 0x04
    };

                                Async_operation         ( Async_manager* = NULL );
    virtual                    ~Async_operation         ();

    virtual string              obj_name                () const                                    { return async_state_text(); }  //"Async_operation"; }


    void                    set_async_manager           ( Async_manager* );
    Async_manager*              async_manager           () const                                    { return _manager; }

    void                    set_async_child             ( Async_operation* child );                 // Setzt child->_parent == this
    void                    set_async_parent            ( Async_operation* parent )                 { _parent = parent; } 
    Async_operation*            async_parent            () const                                    { return _parent; }
    Async_operation*            async_child             () const                                    { return _child; }

    Async_operation*            async_super_operation   ()                                          { return _parent? _parent : this; }
    bool                    set_async_warning_issued    ();                                         // Setzt oberste Operation (_parent->_parent->...)

    void                        async_finish            ()                                          { async_finish_(); }
    bool                        async_continue          ( Continue_flags = cont_default );          // Operation fortsetzen
    bool                        async_has_error         () const;
    bool                        async_finished          () const                                    { return async_has_error() || async_finished_(); }
    void                        async_check_error       ( const string& text = "" )                 { async_check_error( text, true ); }
    void                        async_check_exception   ( const string& text = "" )                 { async_check_error( text, false ); }
    void                        async_reset_error       ()                                          { _error = false; _error_name = _error_code = _error_what = ""; }
    string                      async_state_text        () const;
    bool                        async_kill              ()                                          { return async_kill_(); }
    void                        async_abort             ()                                          { async_abort_(); }
    void                    set_async_delay             ( int t )                                   { set_async_delay( (double)t ); }
    void                    set_async_delay             ( double );
    void                        async_wake              ()                                          { set_async_next_gmtime( (time_t)0 ); }
    void                    set_async_next_gmtime       ( time_t t )                                { set_async_next_gmtime( (double)t ); }
    void                    set_async_next_gmtime       ( double );
    double                      async_next_gmtime       () const                                    { return _next_gmtime; }   // Wann wieder async_continue() rufen?
    bool                        async_next_gmtime_reached() const;
  //void                        async_signal            ()                                          { set_async_next_gmtime( 0 ); } //_signaled = signal; }
    bool                        async_signaled          ()                                          { return async_signaled_(); }
    void                        async_clear_signaled    ()                                          { async_clear_signaled_(); }
  //void                        async_on_signal_from_child( Async_operation* op )                   { async_on_signal_from_child_( op ); }
  //virtual Socket_event*       async_event             ()                                          { throw_xc( "NO ASYNC_EVENT" ); }

  protected:
    void                        async_check_error       ( const string& text, bool check_finished );
    virtual bool                async_continue_         ( Continue_flags )                          = 0;    // Operation fortsetzen
    void                        async_finish_           ();                                         // Operation beenden, ggfs. blockieren!
    virtual bool                async_finished_         () const                                    = 0;
    virtual bool                async_has_error_        () const                                    { return false; }       // Operation ist fehlerhaft (Fehler vom Server)
    virtual void                async_check_error_      ()                                          {}                      // Löst Exception aus, wenn Operation fehlerhaft war
    virtual string              async_state_text_       () const                                    = 0;
    virtual bool                async_kill_             ();
    virtual void                async_abort_            ();
    virtual int                 async_pid               ()                                          { return 0; }
    virtual bool                async_signaled_         ()                                          { return false; } //_next_gmtime == 0; } //_signaled; }
    virtual void                async_clear_signaled_   ()                                          {}
  //virtual void                async_on_signal_from_child_( Async_operation* )                     {}
  //virtual void                async_on_child_finished ( Async_operation* child )                  {}



  private:
    friend struct               Async_manager;


    Fill_zero                  _zero_;

    ptr<Async_manager>         _manager;
    ptr<Async_operation>       _child;
    Async_operation*           _parent;                 // Diese Operation (this) kann Bestandteil einer komplexeren (_parent) sein

    double                     _next_gmtime;
    bool                       _async_warning_issued;   // Um mehrfache Warnungen zu vermeiden, wenn Operation wartet, obwohl Verbindung asynchron ist

    bool                       _error;
    string                     _error_name;
    string                     _error_code;
    string                     _error_what;
};


Z_DEFINE_BITWISE_ENUM_OPERATIONS( Async_operation::Continue_flags );

//-----------------------------------------------------------------------------------Sync_operation

struct Sync_operation : Async_operation
{
    virtual bool                async_continue_         ( Continue_flags )                          { return false; }
    virtual bool                async_finished_         () const                                    { return true; }
    virtual string              async_state_text_       () const                                    { return "Sync_operation"; }
};

extern Sync_operation           dummy_sync_operation;

//----------------------------------------------------------------------------------Operation_is_ok

typedef bool Operation_is_ok( Async_operation* );

//------------------------------------------------------------------------------------Async_manager

struct Async_manager : Object
{
    void                        add_operation           ( Async_operation* );
    void                        remove_operation        ( Async_operation* );

    bool                        async_continue          ()                                          { return async_continue_selected( NULL ); }
    bool                        async_continue_selected ( Operation_is_ok* );
    double                      async_next_gmtime       ();
    Async_operation*            async_next_operation    ();
    virtual string              string_from_operations  ( const string& separator = ", " );
    void                        string_from_operations2 ( Async_manager_printer*, int indent, Async_operation* parent );

    bool                        is_empty                () const                                    { return _timed_operations_queue.empty(); }

    

  private:
    typedef std::list< Async_operation* >    Operations_queue;
  //Operations_queue           _signaled_operations_queue;
    Operations_queue           _timed_operations_queue;
    bool                       _timed_operations_queue_modified;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
