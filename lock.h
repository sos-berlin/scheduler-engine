// $Id: spooler_job.h 4888 2007-03-18 15:47:33Z jz $

#ifndef __SCHEDULER_LOCK_H
#define __SCHEDULER_LOCK_H

namespace sos {
namespace scheduler {
namespace lock {

struct Requestor;
struct Holder;
struct Use;

//---------------------------------------------------------------------------------------------Lock

struct Lock : Object, Scheduler_object, Non_cloneable
{
    enum Lock_mode
    { 
        lk_exclusive     = 0,   // Index für _waiting_queues
        lk_non_exclusive = 1 
    };


                                Lock                        ( Lock_subsystem*, const string& name );
                               ~Lock                        ();

    void                        close                       ();
    void                        prepare_remove              ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    string                      name                        () const                                { return _name; }
    string                      path                        () const                                { return _name; }
    bool                        its_my_turn                 ( const Use* );
    void                        register_lock_use           ( Use* lock_use )                       { _use_set.insert( lock_use ); }
    void                        unregister_lock_use         ( Use* lock_use )                       { _use_set.erase( lock_use ); }
    void                        require_lock_for            ( Holder*, Use* );
    void                        release_lock_for            ( Holder* );
    int                         enqueue_lock_use            ( Use* );
    void                        dequeue_lock_use            ( Use* );
    int                         count_exclusive_holders     () const                                { return _lock_mode == lk_non_exclusive? _holder_set.size() : 0; }
    bool                        is_free_for                 ( Lock_mode ) const;
    bool                        is_free                     () const                                { return _holder_set.empty(); }             
    string                      obj_name                    () const;
    string                      string_from_holders         () const;
    string                      string_from_uses            () const;


  private:
    Fill_zero                  _zero_;
    string                     _name;
    int                        _max_non_exclusive;
    Lock_mode                  _lock_mode;                  // Nur gültig, wenn !_holder_set.empty()

    typedef stdext::hash_set<Holder*>  Holder_set;
    Holder_set                 _holder_set;

    typedef list<Use*>          Use_list;
    vector<Use_list>           _waiting_queues;             // Index: Lock_mode, eine Liste für lk_non_exclusive und eine für lk_exclusive

    typedef stdext::hash_set<Use*>  Use_set;
    Use_set                        _use_set;

    Lock_subsystem*            _lock_subsystem;
};

//----------------------------------------------------------------------------------------------Use

struct Use : Object, Scheduler_object, Non_cloneable
{
                                Use                         ( Requestor* );
                               ~Use                         ();

    void                        close                       ();
    void                        load                        ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    Lock*                       lock                        () const;
    Lock*                       lock_or_null                () const;
    Requestor*                  requestor                   () const                                { return _requestor; }
    string                      lock_path                   () const                                { return _lock_path; }
    Lock::Lock_mode             lock_mode                   () const                                { return _lock_mode; }

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    string                     _lock_path;
    Lock::Lock_mode            _lock_mode;
    Requestor* const           _requestor;
};

//----------------------------------------------------------------------------------------Requestor

struct Requestor : Object, Scheduler_object, Non_cloneable
{
                                Requestor                   ( Scheduler_object* );
                               ~Requestor                   ();

    void                        close                       ();

    void                    set_dom                         ( const xml::Element_ptr& );            // Für <lock.use>, Use
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        load                        ();
    bool                        is_enqueued                 () const                                { return _is_enqueued; }
    bool                        locks_are_available         () const;
    void                        enqueue_lock_requests       ();
    void                        dequeue_lock_requests       ( Log_level = log_debug3 );
    Scheduler_object*           object                      () const                                { return _object; }

    virtual void                on_locks_are_available      ()                                      = 0;

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    Scheduler_object*          _object;
    bool                       _is_enqueued;

  public:
    typedef list< ptr<Use> >    Use_list;
    Use_list                   _use_list;
}; 

//-------------------------------------------------------------------------------------------Holder
    
struct Holder : Object, Scheduler_object, Non_cloneable
{
                                Holder                      ( Scheduler_object*, const Requestor* );
    virtual                    ~Holder                      ();

    void                        close                       ();

    const Requestor*            requestor                   ()                                      { return _requestor; }
    void                        hold_locks                  ();
    void                        release_locks               ();
    Scheduler_object*           object                      () const                                { return _object; }

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    bool                       _is_holding;
    const Requestor* const     _requestor;

  protected:
    Scheduler_object*          _object;                     // Task
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
    xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );
    void                        execute_xml_lock            ( const xml::Element_ptr& );
    void                        execute_xml_lock_remove     ( const xml::Element_ptr& );

    bool                        is_empty                    () const                                { return _lock_map.empty(); }
    Lock*                       lock                        ( const string& name );
    Lock*                       lock_or_null                ( const string& name );

  private:
    typedef map< string, ptr<Lock> > Lock_map;
    Lock_map                   _lock_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos

#endif
