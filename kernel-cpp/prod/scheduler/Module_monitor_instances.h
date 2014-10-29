// $Id$

#ifndef __MODULE_MONITTOR_INSTANCES_H
#define __MODULE_MONITTOR_INSTANCES_H

namespace sos {
namespace scheduler {

struct Module_monitors;
struct Module_monitor_instance;

//-------------------------------------------------------------------------Module_monitor_instances 

struct Module_monitor_instances : Object
{
                                Module_monitor_instances    ( Has_log* log, Module_monitors* );

    void                        close_instances             ();
    void                        clear_instances             ();
    void                        create_instances            ();
    void                        init                        ();
    void                    set_job_name                    ( const string& );
    void                    set_task_id                     ( int id );
    void                    set_log                         ( Has_log* );
    void                        attach_task                 ( Task*, Prefix_log* log );
    void                        detach_task                 ();
    void                        add_obj                     ( IDispatch*, const string& name );
    bool                        load                        ();
    Variant                     spooler_process_before      ();
    Variant                     spooler_process_after       ( Variant );
//
    bool                        is_empty                    ()                                      { return _instance_list.empty(); }
    Fill_zero                  _zero_;

  private:
    Module_monitors* const     _monitors;
    Delegated_log              _log;

    typedef std::list< ptr<Module_monitor_instance> >  Instance_list;
    Instance_list              _instance_list;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
