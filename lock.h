// $Id: spooler_job.h 4888 2007-03-18 15:47:33Z jz $

#ifndef __SCHEDULER_LOCK_H
#define __SCHEDULER_LOCK_H

namespace sos {
namespace scheduler {

struct Lock_requestor;
struct Lock_holder;

//-----------------------------------------------------------------------------------Scheduler_lock

struct Scheduler_lock : Object, Scheduler_object, Non_cloneable
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
    bool                        is_available_for            ( const Lock_requestor* );
    bool                        request_lock_for            ( Lock_holder* );
    void                        release_lock_for            ( Lock_holder* );
    void                        enqueue_lock_requestor      ( Lock_requestor* );
    void                        dequeue_lock_requestor      ( Lock_requestor* );
    bool                        is_exclusive                () const;
    string                      obj_name                    () const;


  private:
  //void                        clear_waiting_queue         ();


    Fill_zero                  _zero_;
    string                     _name;
    int                        _max_non_exclusive;

    typedef stdext::hash_set<Lock_holder*>  Holder_set;
    Holder_set                 _holder_set;

    typedef list<Lock_requestor*> Requestor_list;
    vector<Requestor_list>     _waiting_queues;             // Index: Lock_mode, eine Liste für lk_non_exclusive und eine für lk_exclusive

    Lock_subsystem*            _lock_subsystem;
};

//-----------------------------------------------------------------------------------Lock_requestor

struct Lock_requestor : Object, Scheduler_object, Non_cloneable
{
                                Lock_requestor              ( Scheduler_object* );
                               ~Lock_requestor              ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        load                        ();
    string                      lock_name                   () const                                { return _lock_name; }
    Scheduler_lock::Lock_mode   lock_mode                   () const                                { return _lock_mode; }
    Scheduler_lock*             lock                        () const;
    bool                        is_enqueued                 () const                                { return _is_enqueued; }
    bool                        is_lock_available           () const                                { return lock()->is_available_for( this ); }
    void                        enqueue_lock_request        ();
    void                        dequeue_lock_request        ();

    virtual void                on_lock_is_available        ( Scheduler_lock* )                     = 0;

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    string                     _lock_name;
    Scheduler_lock::Lock_mode  _lock_mode;
    Scheduler_object*          _object;
    bool                       _is_enqueued;
};

//-------------------------------------------------------------------------------------Lock_holder
    
struct Lock_holder : Object, Scheduler_object, Non_cloneable
{
                                Lock_holder                 ( Scheduler_object*, const Lock_requestor* );
    virtual                    ~Lock_holder                 ();

    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        close                       ();

    const Lock_requestor*       lock_requestor              ()                                      { return _lock_requestor; }
    string                      lock_name                   ()                                      { return _lock_requestor->lock_name(); }
    Scheduler_lock::Lock_mode   lock_mode                   () const                                { return _lock_requestor->lock_mode(); }
    Scheduler_lock*             lock                        () const                                { return _lock_requestor->lock(); }
    bool                        request_lock                ();
    void                        release_lock                ();

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    bool                       _is_holding;
    const Lock_requestor* const _lock_requestor;
    //string                     _lock_name;
    //Scheduler_lock::Lock_mode  _lock_mode;

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
