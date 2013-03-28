#ifndef __SCHEDULER_TIMED_CALL_H
#define __SCHEDULER_TIMED_CALL_H

#include "../zschimmer/Call.h"

namespace sos {
namespace scheduler {

//---------------------------------------------------------------------------------------Timed_call
// Ein Aufruf zu einer bestimmten Zeit

struct Timed_call : z::Call, javabridge::has_proxy<Timed_call> 
{
  private:
    Time _at;

  protected:
    Timed_call(const Time& at) : _at(at) {}

  public:
    int64 at_millis() const { return at().millis(); }
    
    Time at() const { return _at; }

    virtual void call() const = 0;
};

//--------------------------------------------------------------------------------------Object_call
// Ein Aufruf eines Scheduler_object zu einer bestimmten Zeit

struct Object_call : Timed_call {
  protected: 
    Object_call(const Time& at = Time(0)) : Timed_call(at) {}

    string obj_name() const;

  protected:
    virtual Scheduler_object* object() const = 0;

    virtual string call_name() const { return name_of_type_info(typeid(*this)); }
};

//-------------------------------------------------------------------------------------Type_int_map
// Bildet Typnamen auf natürliche Zahlen ab

struct Type_int_map {
    static Type_int_map static_singleton;

  private:
    Mutex _mutex;
    stdext::hash_map<string, int> _map;

  public:
    int type_to_int(const type_info&);
};

//------------------------------------------------------------------------------------object_call<>
// Ein Aufruf einer Methode OBJECT::on_call(const CALL&) zu einer bestimmten Zeit

template<typename OBJECT, typename CALL>
struct object_call : Object_call {
  private: 
    OBJECT* const _object;

  protected:
    object_call(OBJECT* o) : _object(o) {}
    object_call(const Time& at, OBJECT* o) : Object_call(at), _object(o) {}

    Scheduler_object* object() const { return _object; }

  public:
    void call() const {
        _object->on_call(*(const CALL*)this);
    }

    static int type_id() { return Type_int_map::static_singleton.type_to_int(typeid(CALL)); }
};

//-------------------------------------------------------------------------------DEFINE_SIMPLE_CALL
// Definiert einen Klasse für den parameterlosen Aufruf

#define DEFINE_SIMPLE_CALL(OBJECT_TYPE, CALL) \
    struct CALL : object_call<OBJECT_TYPE, CALL> { \
        CALL(OBJECT_TYPE* o) : object_call<OBJECT_TYPE, CALL>(Time(0), o) {} \
        CALL(const Time& at, OBJECT_TYPE* o) : object_call<OBJECT_TYPE, CALL>(at, o) {} \
    };


//struct Operation_holder {
//  private:
//    Spooler* _spooler;
//    ptr<Timed_call> _operation;
//
//  public:
//    Operation_holder(Spooler*);
//
//    void enqueue(Timed_call*);
//    void cancel();
//    Time at() const;
//};
//
//
//
//template<typename OBJECT, typename OPERATION>
//struct simple_operation_holder {
//private:
//    OBJECT* _object;
//    Operation_holder _holder;
//
//  public:
//    simple_operation_holder(OBJECT* o) : _object(o), _holder(o->spooler()) {}
//
//    ~simple_operation_holder() {
//        cancel();
//    }
//
//    void call_at(const Time& at) {
//        if (at.is_never())
//            _holder.cancel();
//        else
//            _holder.enqueue(Z_NEW(OPERATION(at, _object)));
//    }
//
//    void cancel() {
//        _holder.cancel();
//    }
//
//    //Time at() const {
//    //    return _holder.at();
//    //}
//};

//---------------------------------------------------------------------------------------Async_call

struct Typed_call_register;

struct Async_call : Call {
    template<typename CALL>
    Async_call(Typed_call_register* r, ptr<CALL> c) : _typed_call_register(r), _timed_call(c), _type_id(CALL::type_id()) {}
        
    void call() const;

  private:
    Typed_call_register* const _typed_call_register;
    ptr<Timed_call> const _timed_call;
    int const _type_id;
};

//------------------------------------------------------------------------------Typed_call_register

struct Typed_call_register {
    friend struct Async_call;

  private:
    mutable z::Mutex _mutex;
    Spooler* _spooler;
    typedef stdext::hash_map<int, ptr<Timed_call> > Map;
    Map _map;

  protected:
    Typed_call_register(Spooler*);

  public:
    ~Typed_call_register();
    Time next_time() const;

  protected:
    void enqueue_id(int id, Timed_call*);
    void cancel_id(int id);
    void cancel_entry(ptr<Timed_call>*);
    Time at(int id) const;
    const Timed_call* get(int id) const;
};

//----------------------------------------------------------------------------typed_call_register<>

template<typename OBJECT>
struct typed_call_register : Typed_call_register {
  private:
    OBJECT* _object;
    
  public:
    typed_call_register(OBJECT* o) : Typed_call_register(o->spooler()), _object(o) {}

    template<typename CALL>
    void call_at(const Time& t) {
        if (t != at<CALL>()) {
            if (t.is_never()) cancel<CALL>();
            else call<CALL>(Z_NEW(CALL(t, _object)));
        }
    }
    
    template<typename CALL>
    void call() {
        call<CALL>(Z_NEW(CALL(Time(0), _object)));
    }
    
    template<typename CALL>
    void call(CALL* call) {
        enqueue_id(CALL::type_id(), call);
    }

    template<typename CALL>
    void call(const ptr<CALL>& call) {
        enqueue_id(CALL::type_id(), call);
    }

    template<typename CALL>
    void cancel() {
        cancel_id(CALL::type_id());
    }
    
    template<typename CALL>
    Time at() const {
        return Typed_call_register::at(CALL::type_id());
    }

    //template<typename CALL>
    //const CALL* get() const {
    //    return (CALL*)get(CALL::type_id());
    //}

    // Zur Koppelung mit Async_operation
    template<typename CALL>
    ptr<Async_call> new_async_call() {
        return Z_NEW(Async_call(this, Z_NEW(CALL(_object))));
    }
};

}}

#endif