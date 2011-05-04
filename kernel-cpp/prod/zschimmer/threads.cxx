// $Id$

#include "zschimmer.h"
#include "threads.h"

namespace zschimmer {

//-----------------------------------------------------------------------------Simple_event::signal

void Simple_event::signal( const string& name )
{
    Z_MUTEX( _mutex )
    {
        _signal_name = name;
        _signaled = true;
    }
}

//-----------------------------------------------------------------------Simple_event::async_signal

void Simple_event::async_signal( const char* )
{
    _signaled = true;
}

//----------------------------------------------------------------Simple_event::signaled_then_reset

bool Simple_event::signaled_then_reset()
{
    bool signaled = false;

    Z_MUTEX( _mutex )
    {
        signaled = _signaled;
        _signal_name = "";
        _signaled = false;
    }

    return signaled;
}

//-------------------------------------------------------------------------------Simple_event::reset

void Simple_event::reset()
{
    Z_MUTEX( _mutex )
    {
        _signal_name = "";
        _signaled = false;
    }
}

//--------------------------------------------------------------------------------Simple_event::wait

void Simple_event::wait()
{
    throw_xc( "Simple_event::wait" );
}

//--------------------------------------------------------------------------------Simple_event::wait

bool Simple_event::wait( double )
{
    throw_xc( "Simple_event::wait" );
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
