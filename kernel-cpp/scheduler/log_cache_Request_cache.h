#ifndef __SOS_SCHEDULER_LOG_CACHE_FILE_REQUEST_CACHE_H
#define __SOS_SCHEDULER_LOG_CACHE_FILE_REQUEST_CACHE_H

namespace sos {
namespace scheduler {
namespace log {
namespace cache {

struct Request;

struct Request_cache : Object {
    public: static ptr<Request_cache> new_instance();
    public: virtual void remove(Prefix_log* log)  = 0;
    public: virtual void cache(Prefix_log*) = 0;
    public: virtual ptr<Request> request(Prefix_log*) = 0;
};

}}}}

#endif