// $Id: spooler_wait.h,v 1.47 2003/12/31 11:05:47 jz Exp $

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

#ifdef Z_WINDOWS
    typedef z::Event            System_event;
#else
    typedef z::Simple_event     System_event;
#endif

//--------------------------------------------------------------------------------------------Event

struct Event : System_event
{
    typedef System_event        Base_class;


                                Event                       ( const string& name = "" )             : Base_class( dont_create, name ), _zero_(this+1) {}
                               ~Event                       ()                                      { close(); }

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

    void                        add                         ( System_event* );
    void                        remove                      ( System_event* );

#ifdef Z_WINDOWS
    void                        add_handle                  ( HANDLE );
  //void                        remove_handle               ( HANDLE, zschimmer::Event* for_internal_use_only = NULL );
    HANDLE                      operator []                 ( int index )                           { return _handles[index]; }
#endif

    bool                        wait_until                  ( Time );                               // Berücksichtigt Sommerzeitumstellung
    bool                        wait_until_2                ( Time );
    bool                        wait                        ( double time );

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

    typedef vector<z::Event_base*>   Event_vector;
    Event_vector               _events;

  public:
    Thread_semaphore           _lock;
};

//--------------------------------------------------------------------------------Directory_watcher

struct Directory_watcher : Event  //, Async_operation
{
    Fill_zero                  _zero_;

    Z_GNU_ONLY(                 Directory_watcher           (); )
                                Directory_watcher           ( Prefix_log* log )                     : _zero_(this+1), _log(log) {}
                               ~Directory_watcher           ()                                      { close(); }

    void                        watch_directory             ( const string& directory, const string& filename_pattern = "" );
    void                        renew                       ();
    bool                        has_changed                 ();
    bool                        has_changed_2               ( bool throw_error = false );
    bool                        match                       ();

#ifndef Z_WINDOWS    
    bool                        valid                       () const                                { return !_directory.empty(); }
#endif

    void                        set_signaled                ();
    void                        reset                       ();
    virtual bool                signaled                    ()                                      { return has_changed(); }

    string                      directory                   () const                                { return _directory; }
    string                      filename_pattern            () const                                { return _filename_pattern; }

  protected: 
    virtual void                close_handle                ();

  //virtual bool                async_continue_             ( bool wait )                           { return has_changed(); }
  //virtual bool                async_finished_             ()                                      { return false; }   // Nie fertig
  //virtual string              async_state_text_           ()                                      { return "Directory_watcher(\"" + _filename_pattern + "\")" };

  private:
    Prefix_log*                _log;
    string                     _directory;
    string                     _filename_pattern;
    z::Regex                   _filename_regex;

#   ifndef Z_WINDOWS
        typedef list<string>    Filenames;
        Filenames              _filenames[2];               // Ein alter und ein neuer Zustand
        int                    _filenames_idx;              // Index des gerade alten Zustands
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
