// $Id: spooler_order.h,v 1.28 2003/11/25 12:07:51 jz Exp $

#ifndef __SPOOLER_ORDER_H
#define __SPOOLER_ORDER_H


namespace sos {
namespace spooler {

struct Job_chain;
struct Job_chain_node;
struct Order_queue;
struct Order;

//--------------------------------------------------------------------------------------------Order

struct Order : Com_order
{
    Fill_zero                  _zero_;    

    typedef Variant             Payload;
    typedef int                 Priority;               // Höherer Wert bedeutet höhere Priorität
    typedef Variant             State;
    typedef Variant             Id;


    Z_GNU_ONLY(                 Order                   (); )                                       // Für gcc 3.2. Nicht implementiert
                                Order                   ( Spooler* spooler )                        : _zero_(this+1), _spooler(spooler), _log(spooler), Com_order(this) { init(); }
                                Order                   ( Spooler* spooler, const VARIANT& );
                                Order                   ( Spooler* spooler, const Record& );
                               ~Order                   ();

    void                        init                    ();
    void                        attach_task             ( Task* );
    void                        open_log                ();
    void                        close                   ();

    Prefix_log&                 log                     ()                                          { return _log; }
    
    void                    set_id                      ( const Variant& );
    Id                          id                      ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _id ); }
    void                    set_default_id              ();
    bool                        id_is_equal             ( const Id& id )                            { if( _id_locked ) return _id == id; else THREAD_LOCK_RETURN( _lock, bool, _id == id ); }

    void                    set_title                   ( const string& title )                     { THREAD_LOCK(_lock)  _title = title,  _title_modified = true; }
    string&                     title                   ()                                          { THREAD_LOCK_RETURN( _lock, string, _title ); }
    string                      obj_name                ();
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                () const                                    { return _priority; }

    Job_chain*                  job_chain               () const                                    { return _job_chain; }
    Job_chain_node*             job_chain_node          () const                                    { return _job_chain_node; }
    Order_queue*                order_queue             ();

    bool                        finished                ();

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const string& );
    Job*                        job                     ();

  //void                    set_task                    ( Task* task )                              { _task = task; }

    void                    set_state                   ( const State& );
    void                    set_state2                  ( const State&, bool is_error_state = false );
    State                       state                   ()                                          { THREAD_LOCK_RETURN( _lock, State, _state ); }
    bool                        state_is_equal          ( const State& state )                      { THREAD_LOCK_RETURN( _lock, bool, _state == state ); }

    void                    set_state_text              ( const string& state_text )                { THREAD_LOCK( _lock )  _state_text = state_text,  _state_text_modified = true; }
    string                      state_text              ()                                          { THREAD_LOCK_RETURN( _lock, string, _state_text ); }

    Time                        start_time              () const                                    { return _start_time; }
    Time                        end_time                () const                                    { return _end_time; }

    void                    set_payload                 ( const VARIANT& );
    Payload                     payload                 ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _payload ); }

    Com_job*                    com_job                 ();


    void                        add_to_order_queue      ( Order_queue* );
    void                        add_to_job              ( const string& job_name );

    void                        setback_                ();

    // Auftrag in einer Jobkette:
    void                        add_to_job_chain        ( Job_chain* );
    void                        remove_from_job_chain   ();
    void                        move_to_node            ( Job_chain_node* );
    void                        postprocessing          ( bool success );                           // Verarbeitung nach spooler_process()
    void                        processing_error        ();

    xml::Element_ptr            dom                     ( const xml::Document_ptr&, Show_what );

    Prefix_log                 _log;


  private:
    void                        postprocessing2         ();

    friend struct               Order_queue;
    friend struct               Job_chain;
    friend void                 Spooler_db::insert_order( Order* );
    friend void                 Spooler_db::update_order( Order* );


    Thread_semaphore           _lock;
    Spooler*                   _spooler;

    Id                         _id;
    bool                       _id_locked;              // Einmal gesperrt, immer gesperrt
    bool                       _is_users_id;            // Id ist nicht vom Spooler generiert, also nicht sicher eindeutig.
    Priority                   _priority;
    bool                       _priority_modified;
    State                      _state;
    string                     _state_text;
    bool                       _state_text_modified;
    string                     _title;
    bool                       _title_modified;
    Job_chain*                 _job_chain;              
    Job_chain_node*            _job_chain_node;         // Nächster Stelle, falls in einer Jobkette
    Order_queue*               _order_queue;            // Auftrag ist in einer Auftragsliste, aber nicht in einer Jobkette. _job_chain == NULL, _job_chain_node == NULL!
    Payload                    _payload;
    bool                       _payload_modified;       // (Bei einem Objekt wird nur bemerkt, dass die Referenz geändert wurde, nicht das Objekt selbst)

    Time                       _created;
    Time                       _start_time;             // Erster Jobschritt
    Time                       _end_time;

    bool                       _in_job_queue;           // Auftrag ist in _job_chain_node->_job->order_queue() eingehängt
  //bool                       _in_process;             // Auftrag wird gerade von spooler_process() verarbeitet 
    Task*                      _task;                   // Auftrag wird gerade von dieser Task in spooler_process() verarbeitet 
    bool                       _moved;                  // true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    Time                       _setback;                // Bis wann der Auftrag zurückgestellt ist
    int                        _setback_count;
    bool                       _is_in_database;
};

//-----------------------------------------------------------------------------------Job_chain_node

struct Job_chain_node : Com_job_chain_node 
{
                                Job_chain_node          ()                                          : _zero_(this+1) {}

    xml::Element_ptr            dom                     ( const xml::Document_ptr&, Show_what, Job_chain* );



    Fill_zero                  _zero_;

    Job*                       _job;                    // NULL: Kein Job, Auftrag endet

    Order::State               _state;                  // Bezeichnung des Zustands

    Order::State               _next_state;             // Bezeichnung des Folgezustands
    Job_chain_node*            _next_node;              // Folgeknoten
    
    Order::State               _error_state;            // Bezeichnung des Fehlerzustands
    Job_chain_node*            _error_node;             // Fehlerknoten

    int                        _priority;               // Das ist die Entfernung zum letzten Knoten + 1, negativ (also -1, -2, -3, ...)
};

//----------------------------------------------------------------------------------------Job_chain

struct Job_chain : Com_job_chain
{
    typedef Variant             State;


    Z_GNU_ONLY(                 Job_chain               ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Job_chain               ( Spooler* );
                               ~Job_chain               ();

    void                    set_name                    ( const string& name )                      { THREAD_LOCK( _lock )  _name = name,  _log.set_prefix( "Jobchain " + _name ); }
    string                      name                    ()                                          { THREAD_LOCK_RETURN( _lock, string, _name ); }

    void                    set_finished                ( bool b )                                  { _finished = b; }
    bool                        finished                () const                                    { return _finished; }
    void                        load_orders_from_database();

    void                        add_job                 ( Job*, const State& input_state, const State& output_state = error_variant, const State& error_state = error_variant );
    void                        finish                  ();

    Job_chain_node*             node_from_state         ( const State& );
    Job_chain_node*             node_from_state_or_null ( const State& );
    Job_chain_node*             node_from_job           ( Job* );

    Order*                      add_order               ( VARIANT* order_or_payload, VARIANT* job_or_state );
    ptr<Order>                  order                   ( const Order::Id& id );

    void                        register_order          ( Order* );                                 // Um doppelte Auftragskennungen zu entdecken: Fehler SCHEDULER-186
    void                        unregister_order        ( Order* );

    int                         order_count             ();

    xml::Element_ptr            dom                     ( const xml::Document_ptr&, Show_what );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    friend struct               Order;
    Thread_semaphore           _lock;
    Prefix_log                 _log;
    string                     _name;
    bool                       _finished;               // add_job() gesperrt, add_order() frei

    typedef list< ptr<Job_chain_node> >  Chain;
    Chain                      _chain;

    typedef map< string, Order* >   Order_map;
    Order_map                  _order_map;
};

//--------------------------------------------------------------------------------Internal_priority
/*
struct Internal_priority
{
                                Internal_priority       ( Priority p = 0 )                          : _ext(p), _sub(0) {}

    bool                        operator <              ( const Internal_priority& p ) const        { return _ext < p._ext? -1
                                                                                                             _ext > p._ext? +1
                                                                                                             _sub < p._sub? -1
                                                                                                             _sub > p._sub? +1
                                                                                                                          :  0; }
    
    Priority                   _ext;
    int                        _sub;
};
*/
//--------------------------------------------------------------------------------------Order_queue

struct Order_queue : Com_order_queue
{
    Z_GNU_ONLY(                 Order_queue             ();  )                                      // Für gcc 3.2. Nicht implementiert
                                Order_queue             ( Job*, Prefix_log* );
                               ~Order_queue             ();

    void                        add_order               ( Order* );
  //Order*                      add_order               ( const Order::Payload& );
    void                        remove_order            ( Order* );
    int                         length                  ( Job_chain* = NULL );
    bool                        empty                   ()                                          { return _queue.empty(); }
    Order*                      first_order             ( const Time& now );
    bool                        has_order               ( const Time& now )                         { return first_order(now) != NULL; }
    ptr<Order>                  get_order_for_processing( const Time& now );
    Time                        next_time               ();
    void                        update_priorities       ();
    ptr<Order>                  order_or_null           ( const Order::Id& );
    Job*                        job                     () const                                    { return _job; }
    xml::Element_ptr            dom                     ( const xml::Document_ptr&, Show_what, Job_chain* );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Thread_semaphore           _lock;
    Job*                       _job;
    Prefix_log*                _log;
    ptr<Com_order_queue>       _com_order_queue;
    
    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;                  // Order._setback == 0
    Queue                      _setback_queue ;         // Order._setback > 0

    int                        _lowest_priority;        // Zur Optimierung
    int                        _highest_priority;       // Zur Optimierung
    bool                       _has_users_id;           // D.h. id auf Eindeutigkeit prüfen. Bei selbst generierten Ids überflüssig. Zur Optimierung.


  //typedef map<Order::Id, Queue::iterator >   Id_map;
  //Id_map                                    _id_map;

};

//-------------------------------------------------------------------------------------------------

//string                          string_from_state       ( Order::State );

} //namespace spooler
} //namespace sos

#endif
