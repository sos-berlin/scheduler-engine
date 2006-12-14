// $Id$

#ifndef __SCHEDULER_OBJECT_H
#define __SCHEDULER_OBJECT_H

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------------Scheduler_object

struct Scheduler_object
{
    enum Type_code
    {
        type_none,
        type_scheduler,
        type_job,
        type_task,
        type_order,
        type_job_chain,
        type_database,
        type_web_service,
        type_web_service_operation,
        type_web_service_request,
        type_web_service_response,
        type_process,
        type_directory_file_order_source,
        type_scheduler_event_manager,
        type_scheduler_member,
        type_heart_beat,
        type_exclusive_scheduler_watchdog
      //type_subprocess_register
    };


    static string               name_of_type_code           ( Type_code );


                                Scheduler_object            ( Spooler* sp, IUnknown* me, Type_code code )         : _spooler(sp), _my_iunknown(me), _scheduler_object_type_code(code) {}
    virtual                    ~Scheduler_object            ()                                      {}    // Für gcc


    Type_code                   scheduler_type_code         () const                                { return _scheduler_object_type_code; }
    void                    set_mail_xslt_stylesheet_path   ( const string& path )                  { _mail_xslt_stylesheet.release();  _mail_xslt_stylesheet_path = path; }
    virtual ptr<Xslt_stylesheet> mail_xslt_stylesheet       ();
    virtual Prefix_log*         log                         ()                                      = 0;
    virtual void                print_xml_child_elements_for_event( String_stream*, Scheduler_event* )  {}

    Spooler*                   _spooler;
    IUnknown*                  _my_iunknown;
    Type_code                  _scheduler_object_type_code;
    ptr<Xslt_stylesheet>       _mail_xslt_stylesheet;
    string                     _mail_xslt_stylesheet_path;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
