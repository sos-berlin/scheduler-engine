// $Id: spooler_job.h 4888 2007-03-18 15:47:33Z jz $

#ifndef __SCHEDULER_LOCK_H
#define __SCHEDULER_LOCK_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------------Scheduler_lock

struct Scheduler_lock : Object, Scheduler_object
{
    enum Lock_mode
    { 
        lk_non_exclusive = 0, 
        lk_exclusive     = 1
    };


                                Scheduler_lock              ( Lock_subsystem* );
                               ~Scheduler_lock              ();

    void                        close                       ();
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    string                      name                        () const                                { return _name; }
    bool                        request_lock_for            ( Lock_holder* );
    void                        release_lock_for            ( Lock_holder* );
    void                        remove_waiting_holder       ( Lock_holder* );
    bool                        is_exclusive                () const;
    string                      obj_name                    () const;


  private:
    void                        clear_waiting_queue         ();
    void                        remove_holder               ( Lock_holder* );


    Fill_zero                  _zero_;
    string                     _name;
    typedef list<Lock_holder*>  Holder_list;
    Holder_list                _holder_list;
    vector<Holder_list>        _waiting_queue;              // Index: Lock_mode, eine Liste für lk_non_exclusive und eine für lk_exclusive
    Lock_subsystem*            _lock_subsystem;
};

//-------------------------------------------------------------------------------------Lock_holder
    
struct Lock_holder : Object, Scheduler_object
{
    enum State
    {
        s_none,
        s_is_waiting_for_lock,
        s_is_holding_lock
    };


                                Lock_holder                 ( Scheduler_object* );
    virtual                    ~Lock_holder                 ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        close                       ();
    void                        load                        ();
    string                      lock_name                   ()                                      { return _lock_name; }
    Scheduler_lock*             lock                        () const                                { return _lock; }
    State                       state                       () const                                { return _state; }
    Scheduler_lock::Lock_mode   lock_mode                   () const                                { return _lock_mode; }
    bool                        request_lock                ();
    void                        release_lock                ();
    void                        remove_from_waiting_queue   ();

    virtual void                on_lock_is_available        ( Scheduler_lock* )                     = 0;
    virtual Prefix_log*         log                         () const                                = 0;

    string                      obj_name                    () const;

  private:
    friend struct               Scheduler_lock;
    void                        clear_lock_reference        ();
    void                    set_state                       ( State );

    Fill_zero                  _zero_;
    string                     _lock_name;
    Scheduler_lock*            _lock;
    Scheduler_lock::Lock_mode  _lock_mode;
    State                      _state;
    Time                       _state_time;

  protected:
    Scheduler_object*          _object;
};

//-----------------------------------------------------------------------------------Lock_subsystem

struct Lock_subsystem : Subsystem
{
                                Lock_subsystem              ( Scheduler* );

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    Scheduler_lock*             lock                        ( const string& name );
    Scheduler_lock*             lock_or_null                ( const string& name );

  private:
    typedef map< string, ptr<Scheduler_lock> > Lock_map;
    Lock_map                   _lock_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
