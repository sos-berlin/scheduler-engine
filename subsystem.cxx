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

//----------------------------------------------------------------Subsystem::switch_subsystem_state

bool Subsystem::switch_subsystem_state( Subsystem_state new_state )
{
    bool result = false;
    
    if( _subsystem_state != new_state )
    {
        Z_LOGI2( "scheduler", obj_name() << ": switch_subsystem_state " << string_from_subsystem_state ( new_state ) << "\n" );

        try
        {
            switch( new_state )
            {
                case subsys_initialized:
                {
                    assert_subsystem_state( subsys_not_initialized, __FUNCTION__ );

                    result = subsystem_initialize();
                    break;
                }

                case subsys_loaded:
                {
                    assert_subsystem_state( subsys_initialized, __FUNCTION__ );

                    result = subsystem_load();
                    break;
                }

                case subsys_active:
                {
                    assert_subsystem_state( subsys_loaded, __FUNCTION__ );

                    result = subsystem_activate();
                    break;
                }

                case subsys_stopped:
                {
                    close();
                    break;
                }

                default:
                    throw_subsystem_state_error( new_state, __FUNCTION__ );
            }

            Z_LOG2( "scheduler", obj_name() << ": state=" << string_from_subsystem_state( _subsystem_state ) << "\n" );
        }
        catch( exception& x )
        {
            //_log->error( message_string( "SCHEDULER-332", obj_name(), string_from_subsystem_state( new_state ) ) );
            z::throw_xc( "SCHEDULER-332", obj_name(), string_from_subsystem_state( new_state ), x );
        }
    }

    return result;
}

//------------------------------------------------------------------Subsystem::subsystem_initialize

bool Subsystem::subsystem_initialize()
{
    z::throw_xc( __FUNCTION__, obj_name(), "not implemented" );
}

//------------------------------------------------------------------------Subsystem::subsystem_load

bool Subsystem::subsystem_load()
{
    z::throw_xc( __FUNCTION__, obj_name(), "not implemented" );
}

//--------------------------------------------------------------------Subsystem::subsystem_activate

bool Subsystem::subsystem_activate()
{
    z::throw_xc( __FUNCTION__, obj_name(), "not implemented" );
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
