// $Id: spooler_task.h 3681 2005-06-02 05:57:42Z jz $

#ifndef __SPOOLER_EVENT_H
#define __SPOOLER_EVENT_H


namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------Scheduler_event

struct Scheduler_event
{
    enum Event_code
    {
        evt_none,
        evt_unknown,
        evt_scheduler_started,
        evt_task_ended,
        evt_disk_full,
        evt_database_error,
        evt_task_start_error,
    };


    static string               name_of_event_code          ( Event_code );


                                Scheduler_event             ( Event_code, Log_level severity, Scheduler_object* );

    void                    set_mail                        ( Com_mail* mail )                      { _mail = mail; }
    void                    set_error                       ( const Xc_copy& x )                    { _error = x; }

    xml::Document_ptr           dom                         ();
    xml::Document_ptr           mail_dom                    ( const xml::Document_ptr& event = xml::Document_ptr() );  // Default: dom()
    int                         send_mail                   ( const xml::Document_ptr& mail  = xml::Document_ptr() );  // Default: mail_dom()

    Spooler*                   _spooler;
    Event_code                 _event_code;
    Log_level                  _severity;
    Scheduler_object*          _object;
    ptr<IUnknown>              _object_iunknown;            // Hält das Objekt (IUnknown ist die gemeinsame eindeutige Oberklasse)
    Xc_copy                    _error;
    ptr<Com_mail>              _mail;                       
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif


