// $Id: spooler_process.h,v 1.6 2003/08/31 19:51:29 jz Exp $

#ifndef __SPOOLER_PROCESS_H
#define __SPOOLER_PROCESS_H

#include "../zschimmer/com_remote.h"


namespace sos {
namespace spooler {


struct Process_class;


//------------------------------------------------------------------------------------------Process

struct Process : zschimmer::Object
{
    //typedef object_server::Session Session;

                                Process                     ( Spooler* sp )                         : _spooler(sp), _zero_(this+1) {}

    void                        start                       ();
    object_server::Session*     session                     ()                                      { return _session; }
  //void                    set_event                       ( Event* e )                            { if( _connection )  _connection->set_event( e ); }
    bool                        async_continue              ();
    void                        add_module_instance         ( Module_instance* )                    { InterlockedIncrement( &_module_instance_count ); }
    void                        remove_module_instance      ( Module_instance* );
    int                         module_instance_count       ()                                      { return _module_instance_count; }
    void                    set_temporary                   ( bool t )                              { _temporary = t; }

    void                    set_dom                         ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );

    
//private:
    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    Spooler*                   _spooler;
    ptr<object_server::Connection> _connection;             // Verbindung zum Prozess
    ptr<object_server::Session>    _session;                // Wir haben immer nur eine Session pro Verbindung
    bool                       _temporary;                  // Löschen, wenn kein Module_instance mehr läuft
    long                       _module_instance_count;
    int                        _timeout;                    // Max. Dauer einer Operation
    Process_class*             _process_class;
};

//-------------------------------------------------------------------------------------Process_list

typedef list< ptr<Process> >    Process_list;

//------------------------------------------------------------------------------------Process_class

struct Process_class : zschimmer::Object
{
                                Process_class               ( Spooler* sp, const string& name )        : _zero_(this+1), _spooler(sp), _name(name) {}
    explicit                    Process_class               ( Spooler* sp, const xml::Element_ptr& e ) : _zero_(this+1), _spooler(sp) { set_dom( e ); }

    
    void                        add_process                 ( Process* );
    void                        remove_process              ( Process* );

    Process*                    start_process               ();
    Process*                    select_process_if_available ();                                   // Startet bei Bedarf. Bei _max_processes: return NULL

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom                         ( const xml::Document_ptr&, Show_what );


    Fill_zero                  _zero_;
    Thread_semaphore           _lock;
    string                     _name;
    int                        _max_processes;
    Spooler*                   _spooler;
    Process_list               _process_list;
};

//-------------------------------------------------------------------------------Process_class_list

typedef list< ptr<Process_class> >   Process_class_list;

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
