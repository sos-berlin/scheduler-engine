// $Id: spooler_wait.h,v 1.7 2001/01/30 12:22:57 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>

namespace sos {
namespace spooler {

bool wait_for_event( const Handle& handle, double wait_time );

//--------------------------------------------------------------------------------------Wait_handle
/*
struct Wait_handle : Handle
{
                                Wait_handle                 ( HANDLE h, const string& name )    : Handle(h), _name(name) {}

    void                        add                         ( Wait_handles* w )                 { _wait_handles_list.push_back(w); }
    void                        remove                      ( Wait_handles* );

    virtual                     signal                      ()                                  { _signaled = true; }

    string                      as_string                   () const                            { return "Ereignis " + _name; }

    const                      _name;
    bool                       _signaled;
    vector<Wait_handles*>      _wait_handles_list;
};
*/
//-------------------------------------------------------------------------------------Wait_handles

struct Wait_handles
{
    struct Entry
    {
                                Entry                       ( const string& name, Task* task = NULL ) : _event_name(name), _task(task) {}

        string                 _event_name;
        Task*                  _task;
    };


                                Wait_handles                ( Spooler* spooler )            : _spooler(spooler) {}


#   ifdef SYSTEM_WIN

        void                    clear                       ()                              { _handles.clear(); _entries.clear(); }
        void                    add                         ( HANDLE, const string& name, Task* = NULL );
        void                    remove                      ( HANDLE );
        void                    wait                        ( double time = latter_day );

        vector<HANDLE>         _handles;
        vector<Entry>          _entries;

#   endif        

    Spooler*                   _spooler;
    Thread_semaphore           _semaphore;
};

//--------------------------------------------------------------------------------Directory_watcher

struct Directory_watcher
{
#   ifdef SYSTEM_WIN

                                Directory_watcher           ( Task* task )                  : _handle(0), _signaled(false), _task(task) {}
                               ~Directory_watcher           ()                              { close(); }

                                operator bool               ()                              { return _handle != NULL; }
                                operator !                  ()                              { return _handle == NULL; }

        void                    watch_directory             ( const string& );
        void                    watch_again                 ();
        void                    close                       ();
        
        HANDLE                 _handle;

#    else

                                operator bool               ()                              { return false; }
                                operator !                  ()                              { return true; };
        
#   endif


    Task*                      _task;
    bool                       _signaled;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif