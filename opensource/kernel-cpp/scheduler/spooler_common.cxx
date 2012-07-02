// $Id: spooler_common.cxx 14879 2011-07-21 14:40:33Z ss $


#include "spooler.h"

namespace sos {
namespace scheduler {

const string reason_start_element_name = "start_reason.why";
const string obstacle_element_name = "obstacle";

//--------------------------------------------------------------------variable_set_from_environment

ptr<Com_variable_set> variable_set_from_environment()
{
    //Z_LOG2( "scheduler", Z_FUNCTION << "\n" );   // Um zu sehen, ob wir im richtigen Prozess sind (Haupt- oder Task-Prozess?)

    ptr<Com_variable_set> result = new Com_variable_set();
    for( char** e = environ; *e; e++ )
    {
        const char* equal = strchr( *e, '=' );
        if( equal )  result->set_var( string( *e, equal - *e ), equal + 1 );
    }

    return result;
}

//---------------------------------------------------------------is_allowed_operation_while_waiting

bool is_allowed_operation_while_waiting( Async_operation* op )
{
    // Diese Operationen werden in database.cxx fortgesetzt, wenn auf die DB gewartet wird.
    // Ebenso bei anderen Warte-Operationen

    return is_communication_operation( op ); //||
           //Das wird rekursiv und sowieso ist vielleicht die Datenbank geschlossen:  cluster::is_heartbeat_operation( op );
}

//-------------------------------------------------------------------------------------------------

xml::Element_ptr append_obstacle_element(const xml::Element_ptr& element, const string& attribute_name, const string& value) {
    xml::Element_ptr result = element.append_new_element(obstacle_element_name);
    result.setAttribute(attribute_name, value);
    return result;
}

//-------------------------------------------------------------------------------------------------

xml::Element_ptr append_obstacle_element(const xml::Element_ptr& element, const xml::Element_ptr& obstacle_child) {
    xml::Element_ptr result = element.append_new_element(obstacle_element_name);
    result.appendChild(obstacle_child);
    return result;
}

//----------------------------------------------------------------------------require_not_attribute

void require_not_attribute(const xml::Element_ptr& e, const string& name) 
{
    if (e.hasAttribute(name))  z::throw_xc("SCHEDULER-232", e.nodeName(), name, e.getAttribute(name));
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
