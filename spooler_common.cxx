// $Id$


#include "spooler.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------Scheduler_object::name_of_type_code
    
string Scheduler_object::name_of_type_code( Scheduler_object::Type_code type_code )
{
    switch( type_code )
    {
        case type_none:                     return "none";
        case type_scheduler:                return "Scheduler";
        case type_job:                      return "Job";
        case type_task:                     return "Task";
        case type_order:                    return "Order";
        case type_job_chain:                return "Job_chain";
        case type_database:                 return "Database";
        case type_web_service:              return "Web_service";
        case type_web_service_operation:    return "Web_service_operation";
        case type_web_service_request:      return "Web_service_request";
        case type_web_service_response:     return "Web_service_response";
        case type_scheduler_event_manager:  return "Scheduler_event_manager";
        case type_scheduler_member:         return "Scheduler_member";
        case type_active_scheduler_heart_beat: return "Active_scheduler_heart_beat";
        case type_inactive_scheduler_watchdog: return "Inactive_scheduler_watchdog";
        default:                            return S() << "Type_code(" << type_code << ")";
    }
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

//--------------------------------------------------------------------variable_set_from_environment

ptr<Com_variable_set> variable_set_from_environment()
{
    Z_LOG2( "scheduler", __FUNCTION__ << "\n" );   // Um zu sehen, ob wir im richtigen Prozess sind (Haupt- oder Task-Prozess?)

    ptr<Com_variable_set> result = new Com_variable_set();
    for( char** e = environ; *e; e++ )
    {
        const char* equal = strchr( *e, '=' );
        if( equal )  result->set_var( string( *e, equal - *e ), equal + 1 );
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
