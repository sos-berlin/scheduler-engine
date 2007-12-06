// $Id$


#include "spooler.h"

namespace sos {
namespace scheduler {

//--------------------------------------------------------------------variable_set_from_environment

ptr<Com_variable_set> variable_set_from_environment()
{
    Z_LOG2( "scheduler", Z_FUNCTION << "\n" );   // Um zu sehen, ob wir im richtigen Prozess sind (Haupt- oder Task-Prozess?)

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
    // Diese Operationen werden in spooler_history.cxx fortgesetzt, wenn auf die DB gewartet wird.
    // Ebenso bei anderen Warte-Operationen

    return is_communication_operation( op ); //||
           //Das wird rekursiv und sowieso ist vielleicht die Datenbank geschlossen:  cluster::is_heartbeat_operation( op );
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
