// $Id$

#ifndef __SCHEDULER__JAVA_THREAD_UNLOCKER__
#define __SCHEDULER__JAVA_THREAD_UNLOCKER__

namespace sos {
namespace scheduler {


struct Java_thread_unlocker {
    Java_thread_unlocker(Spooler* spooler) :
        _spooler(NULL) 
    { 
        spooler->schedulerJ().threadUnlock(); 
        _spooler = spooler;
    }

    ~Java_thread_unlocker() 
    { 
        if (_spooler)  _spooler->schedulerJ().threadLock(); 
    }

  private:
    Spooler* _spooler;
};


}} //namespaces

#endif
