// $Id: spooler_task.cxx 3714 2005-06-16 08:30:05Z jz $


#include "spooler.h"

namespace sos {
namespace spooler {

//--------------------------------------------------------------Scheduler_object::name_of_type_code
    
string Scheduler_object::name_of_type_code( Scheduler_object::Type_code type_code )
{
    switch( type_code )
    {
        case type_none:             return "none";
        case type_scheduler:        return "Scheduler";
        case type_job:              return "Job";
        case type_task:             return "Task";
        case type_order:            return "Order";
        case type_job_chain:        return "Job_chain";
        default:                    return S() << "Type_code(" << type_code << ")";
    }
}

//-----------------------------------------------------------Scheduler_object::mail_xslt_stylesheet
    
Xslt_stylesheet Scheduler_object::mail_xslt_stylesheet()
{ 
    return Xslt_stylesheet(); 
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
