// $Id: spooler_wait.h,v 1.1 2001/01/13 18:41:19 jz Exp $

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
                               ~Directory_watcher           ()                              { if( _handle )  CloseHandle( _handle ); }

                                operator HANDLE             ()                              { return _handle; }
                                operator !                  ()                              { return _handle == NULL; };

        void                    watch_directory             ( const string& );
        void                    watch_again                 ();
        
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
#   ifdef SYSTEM_WIN

        void                    add                         ( HANDLE, Task* = NULL );
        void                    remove                      ( HANDLE );
        void                    wait                        ( double time );

        vector<HANDLE>         _handles;
        vector<Task*>          _tasks;

#   endif        
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif