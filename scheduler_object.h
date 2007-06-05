// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_OBJECT_H
#define __SCHEDULER_OBJECT_H

namespace sos {
namespace scheduler {


struct Database;
struct Order_subsystem_interface;

//---------------------------------------------------------------------------------Scheduler_object

struct Scheduler_object
{
    enum Type_code
    {
        type_none,

        type_active_schedulers_watchdog,
        type_cluster_member,
        type_database,
        type_database_order_detector,
        type_directory_file_order_source,
        type_exclusive_scheduler_watchdog,
        type_heart_beat,
        type_heart_beat_watchdog_thread,
        type_http_file_directory,
        type_http_server,
        type_java_subsystem,
        type_job,
        type_job_chain,
        type_job_chain_group,
        type_job_subsystem,
        type_lock,
        type_lock_holder,
        type_lock_requestor,
        type_lock_subsystem,
        type_lock_use,
        type_web_service,
        type_web_service_operation,
        type_web_service_request,
        type_web_service_response,
        type_web_services,
        type_order,
        type_order_subsystem,
        type_process,
        type_process_class,
        type_process_class_subsystem,
        type_scheduler,
        type_scheduler_event_manager,
        type_scheduler_script,
        type_supervisor,
        type_supervisor_client,
        type_supervisor_client_connection,
        type_task,
        type_task_subsystem,
        type_xml_client_connection,
      //type_subprocess_register
    };


    static string               name_of_type_code           ( Type_code );


                                Scheduler_object            ( Spooler*, IUnknown* me, Type_code );
    virtual                    ~Scheduler_object            ()                                      {}


    Type_code                   scheduler_type_code         () const                                { return _scheduler_object_type_code; }
    void                    set_mail_xslt_stylesheet_path   ( const string& path )                  { _mail_xslt_stylesheet.release();  _mail_xslt_stylesheet_path = path; }
    virtual void                close                       ()                                      {}
    virtual ptr<Xslt_stylesheet> mail_xslt_stylesheet       ();
    virtual void                print_xml_child_elements_for_event( String_stream*, Scheduler_event* )  {}
    virtual string              obj_name                    () const                                { return name_of_type_code( _scheduler_object_type_code ); }
    virtual IDispatch*          idispatch                   ();
    IUnknown*                   iunknown                    () const                                { return _my_iunknown; }
    virtual void                write_element_attributes    ( const xml::Element_ptr& ) const;

    Prefix_log*                 log                         ()                                      { return _log; }
    Database*                   db                          () const;
    Job_subsystem_interface*    job_subsystem               () const;
    Task_subsystem*             task_subsystem              () const;
    Order_subsystem_interface*  order_subsystem             () const;

    Spooler*                   _spooler;
    IUnknown*                  _my_iunknown;
    Type_code                  _scheduler_object_type_code;
    ptr<Xslt_stylesheet>       _mail_xslt_stylesheet;
    string                     _mail_xslt_stylesheet_path;
    ptr<Prefix_log>            _log;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
