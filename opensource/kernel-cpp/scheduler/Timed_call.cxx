#include "spooler.h"

namespace sos {
namespace scheduler {

using namespace std;


Type_int_map Type_int_map::static_singleton;



string Object_call::obj_name() const 
{ 
    return S() << "(" << object()->obj_name() << ") <- " << call_name(); 
}


int Type_int_map::type_to_int(const type_info& t) {
    // Registriert den Typ, wenn nicht schon bekannt, und liefert eine ID
    Z_MUTEX(_mutex) {
        string id_string = id_of_type_info(t);
        stdext::hash_map<string,int>::iterator i = _map.find(id_string);
        if (i != _map.end()) {
            return i->second;
        } else {
            int id_int = _map.size();
            _map[id_string] = id_int;
            return id_int;
        }
    }
    z::throw_xc(Z_FUNCTION);
};

//Operation_holder::Operation_holder(Spooler* spooler) 
//    : _spooler(spooler) {}
//
//
//void Operation_holder::enqueue(Operation* o) {
//    cancel();
//    if (!o->at().is_never()) {
//        _spooler->enqueue_operation(o);
//        _operation = o;
//    }
//}
//
//
//void Operation_holder::cancel() {
//    if (_operation) {
//        _spooler->cancel_operation(_operation);
//        _operation = NULL;
//    }
//}
//
//
//Time Operation_holder::at() const {
//    return _operation? _operation->at() : Time::never;
//}
//


Typed_call_register::Typed_call_register(Spooler* spooler) 
    : _spooler(spooler) {}


Typed_call_register::~Typed_call_register() {
    Z_FOR_EACH(Map, _map, i) 
        cancel_entry(&i->second);
}


void Typed_call_register::enqueue_id(int id, Timed_call* o) {
    cancel_id(id);
    _map[id] = o;
    _spooler->enqueue_call(o);
}


void Typed_call_register::cancel_id(int id) {
    Map::iterator i = _map.find(id);
    if (i != _map.end())
        cancel_entry(&i->second);
}


void Typed_call_register::cancel_entry(ptr<Timed_call>* entry) {
    if (Timed_call* o = *entry) {
        _spooler->cancel_call(o);
        *entry = (Timed_call*)NULL;
    }
}


Time Typed_call_register::next_time() const {
    Time result = Time::never;
    Z_FOR_EACH_CONST(Map, _map, i) {
        if (const Timed_call* t = i->second)
            if (result < t->at()) result = t->at();
    }
    return result;
}

}}
