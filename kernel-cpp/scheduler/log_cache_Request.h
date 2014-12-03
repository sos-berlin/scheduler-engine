#ifndef __SOS_SCHEDULER_LOG_CACHE_FILE_REQUEST_H
#define __SOS_SCHEDULER_LOG_CACHE_FILE_REQUEST_H

namespace sos {
namespace scheduler {
namespace log {
namespace cache {

struct Request_cache;

struct Request : Object { 
    public: static ptr<Request> of(Request_cache*, Prefix_log*, int64 number);
    public: virtual Prefix_log* log() const = 0;
    public: virtual int64 number() const = 0;
};
    
}}}}

#endif
