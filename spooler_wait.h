// $Id: spooler_wait.h,v 1.34 2002/12/03 12:54:46 jz Exp $

#ifndef __SPOOLER_WAIT_H
#define __SPOOLER_WAIT_H

#include <vector>
#include "../zschimmer/regex_class.h"
#include "../zschimmer/threads.h"

namespace sos {
namespace spooler {


#ifdef Z_WINDOWS
    void windows_message_step();
    bool wait_for_event( HANDLE handle, double wait_time );
#endif


struct Wait_handles;

//--------------------------------------------------------------------------------------------Event

struct Event : z::Event
{
    typedef z::Event            Base_class;


                                Event                       ( const string& name = "" )             : Base_class( dont_create, name ), _zero_(this+1) {}

    virtual void                close                       ();
    void                        add_to                      ( Wait_handles* );
    void                        remove_from                 ( Wait_handles* );

  private:
    Fill_zero                  _zero_;
    vector<Wait_handles*>      _wait_handles;
};

//-------------------------------------------------------------------------------------Wait_handles

struct Wait_handles : Non_cloneable
{
                                Wait_handles                ( Spooler* spooler, Prefix_log* log )     : _spooler(spooler),_log(log) {}
                                Wait_handles                ( const Wait_handles& );
                               ~Wait_handles                ();


    Wait_handles&               operator +=                 ( Wait_handles& );

    void                        clear                       ();
    void                        close                       ();
  //void                        clear                       ()                                      { _handles.clear(); _events.clear(); }

    void                        add                         ( zschimmer::Event* );
    void                        remove                      ( zschimmer::Event* );

#ifdef Z_WINDOWS
    void                        add_handle                  ( HANDLE );
  //void                        remove_handle               ( HANDLE, zschimmer::Event* for_internal_use_only = NULL );
    HANDLE                      operator []                 ( int index )                           { return _handles[index]; }
#endif

    int                         wait_until                  ( Time );                               // Berücksichtigt Sommerzeitumstellung
    int                         wait_until_2                ( Time );
    int                         wait                        ( double time );

    bool                        signaled                    ();
    int                         length                      ()                                      { return _events.size(); }
    bool                        empty                       () const                                { return _events.empty(); }

    string                      as_string                   ();
    friend ostream&             operator <<                 ( ostream& s, Wait_handles& w )         { return s << w.as_string(); }


  protected:
    Spooler*                   _spooler;
    Prefix_log*                _log;

#ifdef Z_WINDOWS
    vector<HANDLE>             _handles;
#endif

    typedef vector<z::Event*>   Event_vector;
    Event_vector               _events;

  public:
    Thread_semaphore           _lock;
};

//--------------------------------------------------------------------------------Directory_watcher

struct Directory_watcher : Event
{
#   ifdef SYSTEM_WIN

                                Directory_watcher           ( Prefix_log* log )             : _log(log) {}
                               ~Directory_watcher           ()                              { close(); }

                                operator bool               ()                              { return _handle != NULL; }
                                operator !                  ()                              { return _handle == NULL; }

        void                    watch_directory             ( const string& directory, const string& filename_pattern = "" );
        bool                    match                       ();
        
#    else

                                operator bool               ()                              { return false; }
        bool                    operator !                  ()                              { return true; };
        
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
    z::Regex                   _filename_regex;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
