// $Id: spooler_wait.h,v 1.17 2001/11/09 17:08:39 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>

namespace sos {
namespace spooler {

void windows_message_step();
bool wait_for_event( HANDLE handle, double wait_time );

struct Wait_handles;

//--------------------------------------------------------------------------------------------Event

struct Event : Handle
{
                                Event                       ( const string& name = "" );
                                Event                       ( const string& name, HANDLE h ) : Handle(h), _zero_(this+1), _name(name) {}
                               ~Event                       ();

    void                        operator =                  ( const HANDLE& h )                 { set_handle(h); }

    void                        close                       ();
    void                        set_name                    ( const string& name )              { _name = name; }
    void                        add_to                      ( Wait_handles* );
    void                        remove_from                 ( Wait_handles* );

    bool                        wait                        ( double wait_time );
    void                        set_signal                  ();
    void                        signal                      ( const string& signal_name = "" );
  //virtual                     signal_event                ()                                  {}
    bool                        signaled                    () const                            { return _signaled; }
    bool                        signaled_then_reset         ();
    void                        reset                       ()                                  { _signaled = false; _signal_name = ""; }

    string                      name                        () const                            { return _name; }
    string                      as_string                   () const;
    friend ostream&             operator <<                 ( ostream& s, const Event& w )      { return s << w.as_string(); }


  protected:
                                Event                       ( const Event& );             // Nicht implementiert
    void                        operator =                  ( const Event& );             // Nicht implementiert


    Fill_zero                  _zero_;
    string                     _name;                       
    string                     _signal_name;
    bool                       _signaled;
    Thread_semaphore           _lock;
  //bool                       _waiting;
    vector<Wait_handles*>      _wait_handles;
};

//-------------------------------------------------------------------------------------Wait_handles

struct Wait_handles
{
                                Wait_handles                ( Spooler* spooler, Prefix_log* log )     : _spooler(spooler),_log(log) {}
                               ~Wait_handles                ();


    void                        close                       ();
  //void                        clear                       ()                              { _handles.clear(); _events.clear(); }
    void                        add                         ( Event* );
    void                        add_handle                  ( HANDLE );
    void                        remove                      ( Event* );
    void                        remove_handle               ( HANDLE, Event* for_internal_use_only = NULL );
    int                         wait_until                  ( Time );
    int                         wait                        ( double time );
    bool                        empty                       () const                        { return _events.empty(); }

    string                      as_string                   () const;
    friend ostream&             operator <<                 ( ostream& s, const Wait_handles& w ) { return s << w.as_string(); }


  protected:
                                Wait_handles                ( const Wait_handles& );        // Nicht implementiert
    void                        operator =                  ( const Wait_handles& );        // Nicht implementiert


    Spooler*                   _spooler;
    Prefix_log*                _log;
    vector<HANDLE>             _handles;
    typedef vector<Event*>      Event_vector;
    Event_vector               _events;

  public:
    Thread_semaphore           _lock;
  //bool                       _waiting;
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

    string                      directory                   () const                        { return _directory; }

  private:
    string                     _directory;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif