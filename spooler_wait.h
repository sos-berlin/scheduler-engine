// $Id: spooler_wait.h,v 1.20 2002/03/18 10:11:40 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>
#include "../zschimmer/regex_class.h"

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
    virtual void                set_signal                  ();
    void                        signal                      ( const string& signal_name = "" );
  //virtual                     signal_event                ()                                  {}
    bool                        signaled                    () const                            { return _signaled; }
    bool                        signaled_then_reset         ();
    void                        reset                       ();

    string                      name                        () const                            { return _name; }
    string                      as_string                   () const;
    friend ostream&             operator <<                 ( ostream& s, const Event& w )      { return s << w.as_string(); }


  protected:
                                Event                       ( const Event& );                   // Nicht implementiert
    void                        operator =                  ( const Event& );                   // Nicht implementiert

    virtual void                close_handle                ();


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

                                Directory_watcher           ( Prefix_log* log )             : Event("",NULL), _log(log) {}
                               ~Directory_watcher           ()                              { close(); }

                                operator bool               ()                              { return _handle != NULL; }
                                operator !                  ()                              { return _handle == NULL; }

        void                    watch_directory             ( const string& directory, const string& filename_pattern = "" );
        bool                    match                       ();
        
#    else

                                operator bool               ()                              { return false; }
                                operator !                  ()                              { return true; };
        
#   endif

    virtual void                set_signal                  ();
    string                      directory                   () const                        { return _directory; }
    string                      filename_pattern            () const                        { return _filename_pattern; }

  protected: 
    virtual void                close_handle                ();

  private:
    Prefix_log*                _log;
    string                     _directory;
    string                     _filename_pattern;
    zschimmer::Regex           _filename_regex;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif