// $Id: spooler_job.h 4888 2007-03-18 15:47:33Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SCHEDULER_LOCK_H
#define __SCHEDULER_LOCK_H

namespace sos {
namespace scheduler {
namespace lock {

struct Holder;
struct Lock_folder;
struct Requestor;
struct Use;

//---------------------------------------------------------------------------------------------Lock

struct Lock : idispatch_implementation< Lock, spooler_com::Ilock>, 
              file_based< Lock, Lock_folder, Lock_subsystem >, 
              Non_cloneable
{
    enum Lock_mode
    { 
        lk_exclusive     = 0,   // Index für _waiting_queues
        lk_non_exclusive = 1 
    };


                                Lock                        ( Lock_subsystem*, const string& name = "" );
                               ~Lock                        ();


    // Scheduler_object

    void                        close                       ();
    string                      obj_name                    () const;


    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }


    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    bool                        prepare_to_remove           ();
    bool                        can_be_removed_now          ();

    bool                        prepare_to_replace          ();
    Lock*                       replace_now                 ();


    // Ilock

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Lock"; }
    STDMETHODIMP            put_Name                        ( BSTR );     
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( name(), result ); }
    STDMETHODIMP            put_Max_non_exclusive           ( int );
    STDMETHODIMP            get_Max_non_exclusive           ( int* result )                         { *result = _config._max_non_exclusive;  return S_OK; }
    STDMETHODIMP                Remove                      ();


    //

    Lock_folder*                lock_folder                 () const                                { return typed_folder(); }


    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    void                        execute_xml                 ( const xml::Element_ptr&, const Show_what& );

    void                    set_max_non_exclusive           ( int );
    bool                        its_my_turn                 ( const Use* );
    void                        register_lock_use           ( Use* lock_use )                       { _use_set.insert( lock_use ); }
    void                        unregister_lock_use         ( Use* lock_use )                       { _use_set.erase( lock_use ); }
    void                        require_lock_for            ( Holder*, Use* );
    void                        release_lock_for            ( Holder* );
    int                         enqueue_lock_use            ( Use* );
    void                        dequeue_lock_use            ( Use* );
    int                         count_non_exclusive_holders () const                                { return _lock_mode == lk_non_exclusive? _holder_set.size() : 0; }
    bool                        is_free_for                 ( Lock_mode ) const;
    bool                        is_free                     () const;
    string                      string_from_holders         () const;
    string                      string_from_uses            () const;


  private:
    Fill_zero                  _zero_;

    struct Configuration
    {
        int                    _max_non_exclusive;
    };

    Configuration              _config;


    Lock_mode                  _lock_mode;                  // Nur gültig, wenn !_holder_set.empty()
    State                      _state;
    bool                       _remove;

    typedef stdext::hash_set<Holder*>  Holder_set;
    Holder_set                 _holder_set;

    typedef list<Use*>          Use_list;
    vector<Use_list>           _waiting_queues;             // Index: Lock_mode, eine Liste für lk_non_exclusive und eine für lk_exclusive

    typedef stdext::hash_set<Use*>  Use_set;
    Use_set                        _use_set;


    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//--------------------------------------------------------------------------------------Lock_folder

struct Lock_folder : typed_folder< Lock >
{
                                Lock_folder                 ( Folder* );
                               ~Lock_folder                 ();


    void                        set_dom                     ( const xml::Element_ptr& );
    void                        execute_xml_lock            ( const xml::Element_ptr& );
    void                        add_lock                    ( Lock* lock )                          { add_file_based( lock ); }
    void                        remove_lock                 ( Lock* lock )                          { remove_file_based( lock ); }
    Lock*                       lock                        ( const string& name )                  { return file_based( name ); }
    Lock*                       lock_or_null                ( const string& name )                  { return file_based_or_null( name ); }
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
};

//----------------------------------------------------------------------------------------------Use
// Verbindet Lock mit Lock_requestor

struct Use : Object, 
             Pendant,
             Scheduler_object, 
             Non_cloneable
{
                                Use                         ( Requestor* );
                               ~Use                         ();

    void                        close                       ();
    void                        initialize                  ();
    void                        load                        ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );


    // Pendant:
    bool                        on_dependant_incarnated     ( File_based* );


    Lock*                       lock                        () const;
    Lock*                       lock_or_null                () const;
    Requestor*                  requestor                   () const                                { return _requestor; }
    folder::Path                lock_path                   () const                                { return _lock_path; }
    Lock::Lock_mode             lock_mode                   () const                                { return _lock_mode; }

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    folder::Path               _lock_path;
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

    void                        initialize                  ();
    void                        load                        ();

    bool                        is_enqueued                 () const                                { return _is_enqueued; }
    bool                        locks_are_available         () const;
    bool                        enqueue_lock_requests       ();
    void                        dequeue_lock_requests       ( Log_level = log_debug3 );
    Scheduler_object*           object                      () const                                { return _object; }

  //void                        on_new_lock                 ( Lock* );
    virtual void                on_locks_are_available      ()                                      = 0;
  //virtual void                on_removing_lock            ( lock::Lock* )                         = 0;

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

struct Lock_subsystem : idispatch_implementation< Lock_subsystem, spooler_com::Ilocks>, 
                        file_based_subsystem< Lock >
{
                                Lock_subsystem              ( Scheduler* );

    // Subsystem

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();



    // File_based_subsystem

    string                      object_type_name            () const                                { return "Lock"; }
    string                      filename_extension          () const                                { return ".lock.xml"; }
  //string                      normalized_name             ( const string& name ) const            { return name; }
    ptr<Lock>                   new_file_based              ();


    ptr<Lock_folder>            new_lock_folder             ( Folder* folder )                      { return Z_NEW( Lock_folder( folder ) ); }

    // Ilocks

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Locks"; }
    STDMETHODIMP            get_Lock                        ( BSTR, spooler_com::Ilock** );
    STDMETHODIMP            get_Lock_or_null                ( BSTR, spooler_com::Ilock** );
    STDMETHODIMP                Create_lock                 ( spooler_com::Ilock** );
    STDMETHODIMP                Add_lock                    ( spooler_com::Ilock* );



    Lock*                       lock                        ( const string& path ) const            { return file_based( path ); }
    Lock*                       lock_or_null                ( const string& path ) const            { return file_based_or_null( path ); }

    xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );

  private:
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos

#endif
