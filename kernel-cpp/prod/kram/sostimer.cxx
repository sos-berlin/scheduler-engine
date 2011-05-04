// sostimer.cxx

#include "precomp.h"

#include <limits.h>

#include "../kram/sos.h"
#include "../kram/sosstat.h"
#include "../kram/sostimer.h"

#include <time.h>

// Sos_timer_manager wird von sosstat.cxx initialisiert und ist immer über
// sos_static_ptr()->_timer_manager erreichbar.

using namespace std;
namespace sos {

DEFINE_SOS_STATIC_PTR( Sos_timer_manager )

//------------------------------------------------------------------------------clock_as_double
// clock_as_double() gives the current time, in seconds, elapsed since 00:00:00 GMT, January 1, 1970.

double clock_as_double()
{
    time_t t;

    return (double)time( &t );
}

//-------------------------------------------------------------------------Sos_timer::Sos_timer

Sos_timer::Sos_timer()
:
    _zero_(this+1)
{
    //obj_const_name( "Sos_timer" );
}

//-------------------------------------------------------------------------Sos_timer::Sos_timer

Sos_timer::Sos_timer( Sos_timer_callback* f, void* callback_data, double time )
:
    _zero_(this+1),
    _callback ( f ),
    _callback_data ( callback_data ),
    _alarm_time ( time )
{
}

//-----------------------------------------------------------------------Sos_timer_node::remove

void Sos_timer_node::remove()
{
    sos_static_ptr()->_timer_manager->remove_timer( this );
}

//-----------------------------------------------------------------Sos_timer_manager::add_timer

Sos_timer_handle Sos_timer_manager::add_timer( Sos_timer_callback* f, void* callback_data, double time )
{
    Sos_timer timer ( f, callback_data, time );

    Sos_ptr<Sos_timer_node>* n = &_list;

    while( *n  &&  (*n)->_timer._alarm_time < time )  n = &(*n)->_tail;

    Sos_ptr<Sos_timer_node> timer_node = SOS_NEW( Sos_timer_node( timer, *n ) );

    *n = timer_node;

    return *n;
}
    
//-----------------------------------------------------------------Sos_timer_manager::add_timer

void Sos_timer_manager::remove_timer( Sos_timer_node* timer_node )
{
    Sos_ptr<Sos_timer_node>* n = &_list;

    while( *n  &&  *n != timer_node )  n = &(*n)->_tail;

    if( +*n != timer_node )  throw_xc( "Sos_timer_manager::remove_timer", "Unbekannter Sos_timer" );

    Sos_ptr<Sos_timer_node> halter = *n;  //? vorsichtshalber

    *n = (*n)->_tail;
}

//-----------------------------------------------------------------Sos_timer_manager::wait_time

double Sos_timer_manager::wait_time()
{
    return _list? max( _list->_timer._alarm_time - clock_as_double(), (double)0.0 )
                : INT32_MAX - 1;
}

//---------------------------------------------------------------------Sos_timer_manager::check

void Sos_timer_manager::check()
{
    while( _list )
    {
        double clock = clock_as_double();
        if( _list->_timer._alarm_time > clock )  break;

        // Timer aus der Liste entfernen
        Sos_ptr<Sos_timer_node> timer_node = _list;
        _list = timer_node->_tail;

        // Callback aufrufen:
        (*timer_node->_timer._callback)( timer_node->_timer._callback_data );
    }
}

//------------------------------------------------------------------------Sos_timer::~Sos_timer

Sos_timer::~Sos_timer()
{
}


} //namespace sos

