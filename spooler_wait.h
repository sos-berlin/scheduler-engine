// $Id: spooler_wait.h,v 1.3 2001/01/22 13:42:08 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>

namespace sos {
namespace spooler {

using namespace std;

//--------------------------------------------------------------------------------Directory_watcher

struct Directory_watcher
{
#   ifdef SYSTEM_WIN

                                Directory_watcher           ()                              : _handle(0), _signaled(false) {}
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


    bool                       _signaled;
};

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

        void                    add                         ( HANDLE, const string& name, Task* = NULL );
        void                    remove                      ( HANDLE );
        void                    wait                        ( double time );

        vector<HANDLE>         _handles;
        vector<Entry>          _entries;

#   endif        

    Spooler*                   _spooler;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif