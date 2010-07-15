// $Id$

#include "spooler.h"

using namespace std;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------static
//--------------------------------------------------------------------------------------------const

//-------------------------------------------------------------------------------------------------

struct Event_subsystem_impl : Event_subsystem
{
                                Event_subsystem_impl        ( Scheduler* scheduler )                : Event_subsystem( scheduler, type_event_subsystem ), _zero_(this+1) {}

    // Subsystem
    void                        close                       ()                                      {}
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
  //bool                        subsystem_activate          ();
    string                      name                        () const                                { return "http_server"; }

    // Event_subsystem
    void                        report                      ( const Scheduler_event2& );


    Fill_zero                  _zero_;
    //javaproxy::..::EventSubsystem _java_event_subsystem;
};

//------------------------------------------------------------------------------new_event_subsystem

ptr<Event_subsystem> new_event_subsystem( Scheduler* scheduler )
{
    ptr<Event_subsystem_impl> result = Z_NEW( Event_subsystem_impl( scheduler ) );
    return +result;
}

//-------------------------------------------------------Event_subsystem_impl::subsystem_initialize

bool Event_subsystem_impl::subsystem_initialize()
{
    // In Java eine Klasse Event_subsystem deklarieren mit der Methode report()
    //_java_event_subsystem = javaproxy::...::EventSubsystem::new_instance();
	_subsystem_state = Subsystem_state::subsys_initialized;
	return true;
}

//------------------------------------------------------------Event_subsystem_impl::subsystem_load
bool Event_subsystem_impl::subsystem_load()
{
	_subsystem_state = Subsystem_state::subsys_loaded;
	return true;
}

//---------------------------------------------------------------------Event_subsystem_impl::report

void Event_subsystem_impl::report( const Scheduler_event2& event )
{
    try {
        _log->info( event.obj_name() );

        //ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(event->idispatch(), "name?");
        //_java_event_subsystem.report( (javaproxy::...::Scheduler_event)java_idispatch->get_jobject() );
        //Für Java-Methode EventSubsystem.report( Scheduler_event e );

        // Wenn C++-Cast nicht klappt:
        //_java_event_subsystem.report( java_idispatch->get_jobject() );
        //Für Java-Method  report( Object o ) { ... (Scheduler_event)o }

        if( Scheduler_script* s = spooler()->scheduler_script_subsystem()->scheduler_script_or_null( Absolute_path( "/scheduler-event" ) ) ) {
            s->module_instance()->call_if_exists( "spooler_event(Lsos/spooler/Order)V", event.iunknown() );
        }
    }
    catch( exception& x ) { _log->warn( x.what() ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
