// $Id: spooler_wait.h,v 1.10 2001/02/08 11:21:16 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>

namespace sos {
namespace spooler {

bool wait_for_event( HANDLE handle, double wait_time );

struct Wait_handles;

//--------------------------------------------------------------------------------------------Event

struct Event : Handle
{
                                Event                       ( const string& name = "" );
                                Event                       ( const string& name, HANDLE h ) : Handle(h), _zero_(this+1), _name(name) {}
                               ~Event                       ();

    void                        close                       ();
    void                        set_name                    ( const string& name )              { _name = name; }
    void                        add_to                      ( Wait_handles* );
    void                        remove_from                 ( Wait_handles* );

    bool                        wait                        ( double wait_time );
    void                        set_signal                  ();
    void                        signal                      ();
  //virtual                     signal_event                ()                                  {}
    bool                        signaled                    () const                            { return _signaled; }
    bool                        signaled_then_reset         ();
    void                        reset                       ()                                  { _signaled = false; }

    string                      name                        () const                            { return _name; }
    string                      as_string                   () const                            { return "Ereignis " + _name; }
    friend ostream&             operator <<                 ( ostream& s, const Event& w )      { return s << w.as_string(); }


  protected:
                                Event                       ( const Event& );             // Nicht implementiert
    void                        operator =                  ( const Event& );             // Nicht implementiert


    Fill_zero                  _zero_;
    string                     _name;
    bool                       _signaled;
    Thread_semaphore           _lock;
    bool                       _waiting;
    vector<Wait_handles*>      _wait_handles;
};

//-------------------------------------------------------------------------------------Wait_handles

struct Wait_handles
{
                                Wait_handles                ( Prefix_log* log )             : _log(log) {}
                               ~Wait_handles                ();


    void                        close                       ();
  //void                        clear                       ()                              { _handles.clear(); _events.clear(); }
    void                        add                         ( Event* );
    void                        remove                      ( Event* );
    void                        wait_until                  ( Time );
    void                        wait                        ( double time );


  protected:
                                Wait_handles                ( const Wait_handles& );        // Nicht implementiert
    void                        operator =                  ( const Wait_handles& );        // Nicht implementiert


    Prefix_log*                _log;
    vector<HANDLE>             _handles;
    vector<Event*>             _events;

  public:
    Thread_semaphore           _lock;
    bool                       _waiting;
};

//--------------------------------------------------------------------------------Directory_watcher

struct Directory_watcher : Event
{
#   ifdef SYSTEM_WIN

                                Directory_watcher           ()                              : Event("",NULL) {}
                               ~Directory_watcher           ()                              { close(); }

                                operator bool               ()                              { return _handle != NULL; }
                                operator !                  ()                              { return _handle == NULL; }

        void                    watch_directory             ( const string& );
        void                    watch_again                 ();
        
#    else

                                operator bool               ()                              { return false; }
                                operator !                  ()                              { return true; };
        
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif