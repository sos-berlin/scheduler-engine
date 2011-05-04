#include "precomp.h"
//#define MODULE_NAME "soswin"
// soswin.cpp                                           (c) SOS GmbH Berlin
//                                                          Joacim Zschimmer

#include "../kram/sysdep.h"

#if defined SYSTEM_STARVIEW         // Dieses Modul benötigt StarView

#include "../kram/sos.h"
#include "../kram/log.h"

#include "soswin.h"

using namespace std;
namespace sos {

//-------------------------------------------------------Key_code::normed_value

Key_code::Value Key_code::normed_value() const
{
    Value key_value = value();

    // Egal, ob linke oder rechte Shift/Cntrl-Taste gedrückt:
    if( key_value & Key_code::shift )  key_value |= Key_code::shift;
    if( key_value & Key_code::cntrl )  key_value |= Key_code::cntrl;

    return key_value;
}

//----------------------------------------------Abs_sos_window::~Abs_sos_window

Abs_sos_window::~Abs_sos_window()
{
}

//------------------------------------------------------Sos_window::~Sos_window

Sos_window::~Sos_window()
{
}

//------------------------------------Abs_sos_application::~Abs_sos_application

Abs_sos_application::~Abs_sos_application()
{
}

//------------------------------------Abs_sos_application::~Abs_sos_application

Abs_sos_application::Reschedule_status Abs_sos_application::reschedule()
{
    Reschedule_status s = reschedule_normal;
    _rescheduled++;
    s = _reschedule();
    if( s == reschedule_terminate )  _terminate = true;
    _rescheduled--;
    return s;
}

//---------------------------------------------Sos_application::Sos_application

Sos_application::Sos_application()
{
    _impl_ptr = window_system_ptr();
}

//--------------------------------------------Sos_application::~Sos_application

Sos_application::~Sos_application()
{
}

//----------------------------------------------------Sos_application::_execute

void Sos_application::_execute()
{
    _impl_ptr->execute();
}

//-------------------------------------------------Sos_application::_reschedule

Sos_application::Reschedule_status Sos_application::_reschedule()
{
    Reschedule_status s = _impl_ptr->reschedule();
    return s;
}

//-------------------------------------------------------Sos_application::_busy

void Sos_application::_busy( Bool b )
{
    _impl_ptr->busy( b );
}

//-------------------------------------------------------Sos_application::user_event_function

void Sos_application::user_event_function( User_event_function f )
{
    _impl_ptr->user_event_function( f );
}

} //namespace sos


#endif      // SYSTEM_STARVIEW
