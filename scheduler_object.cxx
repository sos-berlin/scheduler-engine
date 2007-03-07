// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com


#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------Scheduler_object::name_of_type_code
    
string Scheduler_object::name_of_type_code( Scheduler_object::Type_code type_code )
{
    switch( type_code )
    {
        case type_none:                         return "none";

        case type_active_schedulers_watchdog:   return "Active_schedulers_watchdog";
        case type_cluster_member:               return "Cluster_member";
        case type_database:                     return "Database";
        case type_database_order_detector:      return "Database_order_detector";
        case type_exclusive_scheduler_watchdog: return "Exclusive_scheduler_watchdog";
        case type_directory_file_order_source:  return "Directory_file_order_source";
        case type_heart_beat:                   return "Heart_beat";
        case type_heart_beat_watchdog_thread:   return "Heart_beat_watchdog_thread";
        case type_http_server:                  return "Http_server";
        case type_http_file_directory:          return "Http_file_directory";
        case type_java_subsystem:               return "Java_subsystem";
        case type_job:                          return "Job";
        case type_job_chain:                    return "Job_chain";
        case type_job_subsystem:                return "Job_subsystem";
        case type_web_service:                  return "Web_service";
        case type_web_service_operation:        return "Web_service_operation";
        case type_web_service_request:          return "Web_service_request";
        case type_web_service_response:         return "Web_service_response";
        case type_web_services:                 return "Web_services";
        case type_order:                        return "Order";
        case type_order_subsystem:              return "Order_subsystem";
        case type_process:                      return "Process";
        case type_scheduler_event_manager:      return "Scheduler_event_manager";
        case type_scheduler_script:             return "Scheduler_script";
        case type_scheduler:                    return "Scheduler";
        case type_task:                         return "Task";
        case type_task_subsystem:               return "Task_subsystem";
        case type_xml_client_connection:        return "Xml_client_connection";
        default:                                return S() << "Type_code(" << type_code << ")";
    }
}

//---------------------------------------------------------------S-cheduler_object::Scheduler_object

Scheduler_object::Scheduler_object( Spooler* spooler, IUnknown* me, Type_code code )
: 
    _spooler(spooler), 
    _my_iunknown(me), 
    _scheduler_object_type_code(code)
{
    if( this == spooler )
    {
        _log = Z_NEW( Prefix_log( 1 ) );       // Der Scheduler initialisiert beim Start sein Hauptprotokoll selbst
    }
    else
    {
        _log = Z_NEW( Prefix_log( this ) );
        _log->set_prefix( obj_name() );
    }
}

//----------------------------------------------------------------------Scheduler_object::idispatch
    
IDispatch* Scheduler_object::idispatch()
{
    z::throw_xc( __FUNCTION__, obj_name() );
}

//-----------------------------------------------------------Scheduler_object::mail_xslt_stylesheet
    
ptr<Xslt_stylesheet> Scheduler_object::mail_xslt_stylesheet()
{ 
    if( !_mail_xslt_stylesheet )
    {
        if( _mail_xslt_stylesheet_path != "" )
        {
            ptr<Xslt_stylesheet> stylesheet = Z_NEW( Xslt_stylesheet );
            stylesheet->load_file( _mail_xslt_stylesheet_path );
            
            _mail_xslt_stylesheet = stylesheet;
        }
        else
        if( this != _spooler )
        {
            _mail_xslt_stylesheet = _spooler->mail_xslt_stylesheet();
        }
    }

    return _mail_xslt_stylesheet; 
}

//-----------------------------------------------------------------------------Scheduler_object::db

Database* Scheduler_object::db() const 
{
    return _spooler->_db;
}

//------------------------------------------------------------------Scheduler_object::job_subsystem

Job_subsystem_interface* Scheduler_object::job_subsystem() const
{
    return _spooler->job_subsystem();
}

//-----------------------------------------------------------------Scheduler_object::task_subsystem

Task_subsystem* Scheduler_object::task_subsystem() const
{
    return _spooler->task_subsystem();
}

//----------------------------------------------------------------Scheduler_object::order_subsystem

Order_subsystem_interface* Scheduler_object::order_subsystem() const
{
    return _spooler->order_subsystem();
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
