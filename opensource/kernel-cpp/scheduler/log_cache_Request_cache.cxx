#include "spooler.h"
#include "log_cache_Request_cache.h"
#include "log_cache_Request.h"

namespace sos {
namespace scheduler {
namespace log {
namespace cache {

struct Request_cache_impl : Request_cache {
    private: typedef stdext::hash_map<Prefix_log*,ptr<Request> > Map;

    private: Fill_zero  _zero_;
    private: Map        _map;
    private: int64      _next_number;

    public: Request_cache_impl() : _zero_(this+1) {}

    public: ~Request_cache_impl() {
        Z_FOR_EACH_CONST(Map, _map, it)  Z_LOG2("scheduler", Z_FUNCTION << "() " << it->second->obj_name());
        //Z_DEBUG_ONLY(assert(_map.empty()));
    }

    public: void remove(Prefix_log* log) {
        Map::iterator it = _map.find(log);
        if (it != _map.end())
            _map.erase(it);
    }

    public: void cache(Prefix_log* log) {
        cache(request(log));
    }

    public: ptr<Request> request(Prefix_log* log) {
        ptr<Request> result = cached_request(log);
        if (!result) {
            if (becomes_full())
                remove_oldest();
            result = Request::of(this, log, _next_number++);
            if (result)
                cache(result);
        }
        return result;
    }

    public: void cache(Request* request) {
        Map::iterator it = _map.find(request->log());
        if (it == _map.end()) {
            if (is_full())
                remove_oldest();
            _map[request->log()] = request;
            assert(_map.size() <= max_open_log_files);
        }
    }

    private: void remove_oldest() {
        _map.erase(oldest());
    }

    private: ptr<Request> cached_request(Prefix_log* log) {
        Map::iterator it = _map.find(log);
        return it == _map.end()? NULL : it->second;
    }

    private: Map::iterator oldest() {
        Map::iterator result = _map.end();
        Z_FOR_EACH(Map, _map, it)
            if (result == _map.end()  ||  it->second->number() < result->second->number())
                result = it;
        return result;
    }

    private: bool is_full() { return _map.size() >= max_open_log_files; }
    private: bool becomes_full() { return _map.size() + 1 >= max_open_log_files; }
};


ptr<Request_cache> Request_cache::new_instance() {
    ptr<Request_cache_impl> result = Z_NEW(Request_cache_impl); 
    return +result;
}

}}}}
