// $Id: spooler_process.h,v 1.3 2003/08/29 08:14:04 jz Exp $

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace spooler {

//------------------------------------------------------------------------------------------Process

struct Process : zschimmer::Object
{
    //typedef object_server::Session Session;

                                Process                     ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}

    void                        start                       ();
    object_server::Session*     session                     ()                                      { return _session; }
    void                        async_continue              ();
    void                        add_module_instance         ( Module_instance* )                    { InterlockedIncrement( &_module_instance_count ); }
    void                        remove_module_instance      ( Module_instance* );
    int                         module_instance_count       ()                                      { return _module_instance_count; }
    void                    set_temporary                   ( bool t )                              { _temporary = t; }

    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );

    
  private:
    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long                       _module_instance_count;
};



/*
struct Spooler_process : zschimmer::Object
{
                                Spooler_process             ( Spooler_thread* t )                   : _thread(t), _spooler(t->_spooler), _zero_(this+1) {}


  private:
    bool                        step                        ();
    bool                        do_something                ( Task* );



    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Spooler_thread*            _thread;

    Task_list                  _task_list;
    bool                       _task_closed;

    object_server::Session*    _session;                    // NULL: Kein Prozess (Tasks laufen nicht in einem separaten Prozess)
};
*/

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
