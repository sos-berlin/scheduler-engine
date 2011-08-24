#include "spooler.h"
#include "log_cache_Request.h"
#include "log_cache_Request_cache.h"

namespace sos {
namespace scheduler {
namespace log {
namespace cache {

struct Request_impl : Request {
    private: Fill_zero              _zero_;
    private: Request_cache* const   _cache;
    private: ptr<Prefix_log> const  _log;
    public: int64                   _number;

    public: Request_impl(Request_cache* cache, Prefix_log* log, int64 number) :
        _zero_(this+1),
        _cache(cache),
        _log(log),
        _number(number)
    {}

    public: ~Request_impl() {
        _log->close_file();
        _log->set_append(true);   // Damit nach erneutem Öffnen die Datei fortgesetzt wird.
    }

    public: Prefix_log* log() const {  return _log; }
    public: int64 number() const { return _number; }
};

ptr<Request> Request::of(Request_cache* cache, Prefix_log* log, int64 number) {
    log->try_reopen_file();
    ptr<Request_impl> result = log->file_is_opened()? Z_NEW(Request_impl(cache, log, number)) : NULL;
    return +result;
}

}}}}
