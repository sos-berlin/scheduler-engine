// $Id: spooler_order.h,v 1.3 2002/09/14 16:23:08 jz Exp $

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

    void                    set_title                   ( const string& title )                     { THREAD_LOCK(_lock)  _title = title; }
    string&                     title                   ()                                          { THREAD_LOCK_RETURN( _lock, string, _title ); }
    string                      obj_name                ()                                          { THREAD_LOCK_RETURN( _lock, string, string_from_variant(_id) + rtrim( "  " + _title ) ); }
                                                            
    void                    set_priority                ( Priority );
    Priority                    priority                ()                                          { return _priority; }

    Job_chain*                  job_chain               ()                                          { return _job_chain; }

    void                    set_job                     ( Job* );
    void                    set_job                     ( spooler_com::Ijob* );
    void                    set_job_by_name             ( const string& );
    Job*                        job                     ();

    void                    set_state                   ( const State& );
    State                       state                   ();

    void                    set_state_text              ( const wstring& state_text )               { THREAD_LOCK( _lock )  _state_text = state_text.c_str(); }
    wstring                     state_text              ()                                          { THREAD_LOCK_RETURN( _lock, wstring, _state_text ); }

    void                    set_payload                 ( const VARIANT& payload )                  { THREAD_LOCK( _lock )  _payload = payload; }
    Payload                     payload                 ()                                          { THREAD_LOCK_RETURN( _lock, Variant, _payload ); }

  //Com_order*                  com_order               ()                                          { THREAD_LOCK_RETURN( _lock, Com_order*, _com_order ); }
    Com_job*                    com_job                 ();
  //CComPtr<spooler_com::Ijob_chain> com_job_chain      ();


    void                        add_to_order_queue      ( Order_queue* );

    // Auftrag in einer Jobkette:
    void                        add_to_job_chain        ( Job_chain* );
    void                        remove_from_job_chain   ();
    void                        move_to_node            ( Job_chain_node* );
    void                        postprocessing          ( bool success, Prefix_log* );              // Verarbeitung nach spooler_process()



  private:
    friend struct               Order_queue;

    Fill_zero                  _zero_;    
    Thread_semaphore           _lock;
    Spooler*                   _spooler;

    Id                         _id;
    Priority                   _priority;
    State                      _state;
    wstring                    _state_text;
    string                     _title;
    Payload                    _payload;
    bool                       _error;

    Job_chain*                 _job_chain;              
    Job_chain_node*            _job_chain_node;         // Nächster Stelle, falls in einer Jobkette
    bool                       _in_job_queue;           // Auftrag ist in _job_chain_node->_job->order_queue() eingehängt
    bool                       _moved;                  // true, wenn Job state oder job geändert hat. Dann nicht automatisch in Jobkette weitersetzen
    Order_queue*               _order_queue;            // Auftrag ist in einer Auftragsliste, aber nicht in einer Jobkette. _job_chain == NULL, _job_chain_node == NULL!
};

//-----------------------------------------------------------------------------------Job_chain_node

struct Job_chain_node : Object
{
                                Job_chain_node          ()                                          : _zero_(this+1) {}


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
    Job_chain_node*             node_from_job           ( Job* );

    Order*                      add_order               ( VARIANT* order_or_payload, VARIANT* job_or_state );

  //Com_job_chain*              com_job_chain           ()                                          { THREAD_LOCK_RETURN( _lock, Com_job_chain*, _com_job_chain ); }


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    friend struct               Order;
    Thread_semaphore           _lock;
    string                     _name;
    bool                       _finished;               // add_job() gesperrt, add_order() frei
  //CComPtr<Com_job_chain>     _com_job_chain;

    typedef list< ptr<Job_chain_node> >  Chain;
    Chain                      _chain;

    typedef map< State, Job_chain_node* >  Map;
    Map                        _map;
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
    ptr<Order>                  pop                     ();
    Job*                        job                     () const                                    { return _job; }


    Fill_zero                  _zero_;
    Spooler*                   _spooler;

  private:
    Thread_semaphore           _lock;
    Job*                       _job;
    Prefix_log*                _log;
    CComPtr<Com_order_queue>   _com_order_queue;
    typedef list< ptr<Order> >  Queue;
    Queue                      _queue;
  //map< Id, ptr<Order> >      _id_order_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
