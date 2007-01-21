// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

//----------------------------------------------------------------------string_from_subsystem_state
    
string string_from_subsystem_state( Subsystem_state state )
{
    switch( state )
    {
        case subsys_not_initialized:    return "not_initialized";
        case subsys_initialized:        return "initialized";
        case subsys_loaded:             return "loaded";
        case subsys_active:             return "active";
        case subsys_stopped:            return "stopped";

        default: 
            Z_DEBUG_ONLY( assert("subsystem_state"==NULL) );
            return "Subsystem_state:" + as_string( state );
    }
}

//-----------------------------------------------------------Subsystem::throw_subsystem_state_error

void Subsystem::throw_subsystem_state_error( Subsystem_state state, const string& message_text )
{
    z::throw_xc( "SUBSYSTEM-STATE-ERROR", string_from_subsystem_state( state ), string_from_subsystem_state( _subsystem_state ), message_text );
}

//----------------------------------------------------------------Subsystem::assert_subsystem_state

void Subsystem::assert_subsystem_state( Subsystem_state expected_state, const string& message_text )
{
    if( _subsystem_state != expected_state )  throw_subsystem_state_error( expected_state, message_text );
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
