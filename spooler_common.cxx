// $Id$


#include "spooler.h"

namespace sos {
namespace scheduler {

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
