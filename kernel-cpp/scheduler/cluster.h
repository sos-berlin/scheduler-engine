// $Id: cluster.h 13198 2007-12-06 14:13:38Z jz $

#ifndef __SCHEDULER_CLUSTER_H
#define __SCHEDULER_CLUSTER_H


namespace sos {
namespace scheduler {
namespace cluster {

//--------------------------------------------------------------------------------------------const

const extern int                default_backup_precedence;
const extern int                default_non_backup_precedence;
const extern int                default_heart_beat_timeout;
const extern int                default_heart_beat_own_timeout;
const extern int                default_heart_beat_warn_timeout;

// Für set_continue_exclusive_operation
const extern string             continue_exclusive_non_backup;      // Nur Non-backup-Scheduler darf exklusiv werden
const extern string             continue_exclusive_this;            // Nur Scheduler mit unserem Url darf exklusiv werden
const extern string             continue_exclusive_any;             // Jeder darf exklusiv werden

//------------------------------------------------------------------------------------Configuration

struct Configuration
{
                                Configuration               ();

    void                        set_dom                     ( const xml::Element_ptr& cluster_element );
    void                        finish                      ();


    Fill_zero                  _zero_;
    bool                       _is_backup_member;
    int                        _backup_precedence;
    bool                       _demand_exclusiveness;
    bool                       _orders_are_distributed;
    bool                       _suppress_watchdog_thread;
    int                        _heart_beat_timeout;         // Großzügigere Frist für den Herzschlag, nach der der Scheduler für tot erklärt wird
    int                        _heart_beat_own_timeout;     // < _heart_beat_timeout      Zur eigenen Prüfung, etwas kürzer als _heart_beat_timeout
    int                        _heart_beat_warn_timeout;    // < _heart_beat_own_timeout  Nach dieser Zeit ohne Herzschlag gibt's eine Warnung
};

//----------------------------------------------------------------------Cluster_subsystem_interface

struct Cluster_subsystem_interface : Object, Subsystem
{
                                Cluster_subsystem_interface ( Scheduler* scheduler, Type_code t )   :  Subsystem( scheduler, this, t ) {}


    virtual void            set_configuration               ( const Configuration& )                = 0;
    virtual void            set_continue_exclusive_operation( const string& http_url )              = 0;  // Oder continue_exclusive_non_backup etc.
    virtual string              my_member_id                ()                                      = 0;
    virtual int                 backup_precedence           ()                                      = 0;
    virtual bool                check_is_active             ( Transaction* = NULL )                 = 0;
    virtual bool                do_a_heart_beat_when_needed ( const string& debug_text )            = 0;
    virtual bool                has_exclusiveness           ()                                      = 0;
    virtual bool             is_active                      ()                                      = 0;
    virtual bool             is_member_allowed_to_start     ()                                      = 0;
    virtual bool             is_backup                      ()                                      = 0;
    virtual bool             is_exclusiveness_lost          ()                                      = 0;
    virtual bool                set_command_for_all_schedulers_but_me( Transaction*, const string& command ) = 0;
    virtual bool                set_command_for_scheduler            ( Transaction*, const string& command, const string& member_id ) = 0;
    virtual bool                delete_dead_scheduler_record( const string& cluster_member_id )     = 0;
    virtual void                show_active_schedulers      ( Transaction*, bool exclusive_only = false ) = 0;

    virtual string              http_url_of_member_id       ( const string& cluster_member_id )     = 0;
    virtual void                check                       ()                                      = 0;

    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
};


ptr<Cluster_subsystem_interface> new_cluster_subsystem      ( Scheduler* );

//-------------------------------------------------------------------------------------------------

bool                            is_heartbeat_operation      ( Async_operation* );

//-------------------------------------------------------------------------------------------------

} //namespace cluster
} //namespace scheduler
} //namespace sos

#endif
