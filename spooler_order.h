// $Id: spooler_order.h,v 1.10 2002/10/02 12:54:38 jz Exp $

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
    typedef Variant             Payload;
    typedef int                 Priority;               // Höherer Wert bedeutet höhere Priorität
    typedef Variant             State;
    typedef Variant             Id;

    
                                Order                   ( Spooler* spooler )                        : _zero_(this+1), _spooler(spooler), Com_order(this) {}
                                Order                   ( Spooler* spooler, const VARIANT& );
                               ~Order                   ();


    void                    set_id                      ( const Variant& );
    Id                          id                      ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _id ); }
    void                    set_default_id              ();
    bool                        id_is_equal             ( const Id& id )                            { if( _id_locked ) return _id == id; else THREAD_LOCK_RETURN( _lock, bool, _id == id ); }

    void                    set_title                   ( const string& title )                     { THREAD_LOCK(_lock)  _title = title; }
    string&                     title                   ()                                          { THREAD_LOCK_RETURN( _lock, string, _title ); }
    string                      obj_name                ()                                          { THREAD_LOCK_RETURN( _lock, string, string_from_variant(_id) + rtrim( "  " + _title ) ); }
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                ()                                          { return _priority; }

    Job_chain*                  job_chain               ()                                          { return _job_chain; }
    Order_queue*                order_queue             ();

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const string& );
    Job*                        job                     ();

    void                    set_state                   ( const State& );
    State                       state                   ()                                          { THREAD_LOCK_RETURN( _lock, State, _state ); }
    bool                        state_is_equal          ( const State& state )                      { THREAD_LOCK_RETURN( _lock, bool, _state == state ); }

    void                    set_state_text              ( const wstring& state_text )               { THREAD_LOCK( _lock )  _state_text = state_text.c_str(); }
    wstring                     state_text              ()                                          { THREAD_LOCK_RETURN( _lock, wstring, _state_text ); }

    void                    set_payload                 ( const VARIANT& payload )                  { THREAD_LOCK( _lock )  _payload = payload; }
    Payload                     payload                 ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _payload ); }

    Com_job*                    com_job                 ();


    void                        add_to_order_queue      ( Order_queue* );
    void                        add_to_job              ( const string& job_name );

    // Auftrag in einer Jobkette:
    void                        add_to_job_chain        ( Job_chain* );
    void                        remove_from_job_chain   ();
    void                        move_to_node            ( Job_chain_node* );
    void                        postprocessing          ( bool success, Prefix_log* );              // Verarbeitung nach spooler_process()
    void                        processing_error        ();

    xml::Element_ptr            xml                     ( xml::Document_ptr, Show_what );


  private:
    friend struct               Order_queue;



    Fill_zero                  _zero_;    
    Thread_semaphore           _lock;
    Spooler*                   _spooler;

    Id                         _id;
    bool                       _id_locked;              // Einmal gesperrt, immer gesperrt
    bool                       _is_users_id;            // Id ist nicht vom Spooler generiert, also nicht sicher eindeutig.
    Priority                   _priority;
    State                      _state;
    wstring                    _state_text;
    string                     _title;
    Payload                    _payload;
    Job_chain*                 _job_chain;              
    Job_chain_node*            _job_chain_node;         // Nächster Stelle, falls in einer Jobkette
    bool                       _in_job_queue;           // Auftrag ist in _job_chain_node->_job->order_queue() eingehängt
    bool                       _in_process;             // Auftrag wird gerade von spooler_process() verarbeitet 
    bool                       _moved;                  // true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    Order_queue*               _order_queue;            // Auftrag ist in einer Auftragsliste, aber nicht in einer Jobkette. _job_chain == NULL, _job_chain_node == NULL!
};

//-----------------------------------------------------------------------------------Job_chain_node

struct Job_chain_node : Object
{
                                Job_chain_node          ()                                          : _zero_(this+1) {}

    xml::Element_ptr            xml                     ( xml::Document_ptr, Show_what );



    Fill_zero                  _zero_;

    Job*                       _job;                    // NULL: Kein Job, Auftrag endet

    Order::State               _state;                  // Bezeichnung des Zustands

    Order::State               _next_state;             // Bezeichnung des Folgezustands
    Job_chain_node*            _next_node;              // Folgeknoten
    
    Order::State               _error_state;            // Bezeichnung des Fehlerzustands
    Job_chain_node*            _error_node;             // Fehlerknoten
};

//----------------------------------------------------------------------------------------Job_chain

struct Job_chain : Com_job_chain
{
    typedef Variant             State;


                                Job_chain               ( Spooler* );
                               ~Job_chain               ();

    void                    set_name                    ( const string& name )                      { THREAD_LOCK( _lock )  _name = name; }
    string                      name                    ()                                          { THREAD_LOCK_RETURN( _lock, string, _name ); }

    bool                        finished                () const                                    { return _finished; }

    void                        add_job                 ( Job*, const State& input_state, const State& output_state, const State& error_state );
    void                        finish                  ();

    Job_chain_node*             node_from_state         ( const State& );
    Job_chain_node*             node_from_state_or_null ( const State& );
    Job_chain_node*             node_from_job           ( Job* );

    Order*                      add_order               ( VARIANT* order_or_payload, VARIANT* job_or_state );
    ptr<Order>                  order                   ( const Order::Id& id );

    int                         order_count             ();

    xml::Element_ptr            xml                     ( xml::Document_ptr, Show_what );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    friend struct               Order;
    Thread_semaphore           _lock;
    string                     _name;
    bool                       _finished;               // add_job() gesperrt, add_order() frei

    typedef list< ptr<Job_chain_node> >  Chain;
    Chain                      _chain;
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
                                Order_queue             ( Job*, Prefix_log* );
                               ~Order_queue             ();

    void                        add_order               ( Order* );
  //Order*                      add_order               ( const Order::Payload& );
    void                        remove_order            ( Order* );
    int                         length                  () const                                    { return _queue.size(); }
    bool                        empty                   () const                                    { return _queue.empty(); }
    ptr<Order>                  get_order_for_processing();
    ptr<Order>                  order_or_null           ( const Order::Id& );
    Job*                        job                     () const                                    { return _job; }
    xml::Element_ptr            xml                     ( xml::Document_ptr, Show_what );


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Thread_semaphore           _lock;
    Job*                       _job;
    Prefix_log*                _log;
    ptr<Com_order_queue>       _com_order_queue;
    
    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;

    int                        _highest_priority;       // Zur Optimierung
    bool                       _has_users_id;           // D.h. id auf Eindeutigkeit prüfen. Bei selbst generierten Ids überflüssig. Zur Optimierung.


  //typedef map<Order::Id, Queue::iterator >   Id_map;
  //Id_map                                    _id_map;

};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
