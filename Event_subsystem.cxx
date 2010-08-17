// $Id$

#include "spooler.h"
#include "../javaproxy/com__sos__scheduler__eventing__SchedulerEventListener.h"
#include "../javaproxy/java__lang__String.h"

using namespace std;

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------static
//--------------------------------------------------------------------------------------------const

//-------------------------------------------------------------------------------------------------

struct Event_subsystem_impl : Event_subsystem
{
//                                Event_subsystem_impl        ( Scheduler* scheduler )                : Event_subsystem( scheduler, type_event_subsystem ), _zero_(this+1) {}
                                  Event_subsystem_impl        ( Scheduler* scheduler );

    // Subsystem
    void                        close                       ()                                      {}
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
  //bool                        subsystem_activate          ();
    string                      name                        () const                                { return "event_subsystem"; }

    // Event_subsystem
    void                        report                      ( Scheduler_event2& );

    Fill_zero                  _zero_;

    //javaproxy::com::sos::scheduler::eventing::SchedulerEventListener _event_listener;

};


// ----------------------------------------------------------------------------
Event_subsystem_impl::Event_subsystem_impl (Scheduler* scheduler)
:  
	Event_subsystem( scheduler, type_event_subsystem ),	
	_zero_(this+1)
{
}

//------------------------------------------------------------------------------new_event_subsystem
ptr<Event_subsystem> new_event_subsystem( Scheduler* scheduler )
{
    ptr<Event_subsystem_impl> result = Z_NEW( Event_subsystem_impl( scheduler ) );
    return +result;
}

//-------------------------------------------------------Event_subsystem_impl::subsystem_initialize

bool Event_subsystem_impl::subsystem_initialize()
{
	//_event_listener = javaproxy::com::sos::scheduler::eventing::SchedulerEventListener::new_instance();
	_subsystem_state = subsys_initialized;
	return true;
}

//------------------------------------------------------------Event_subsystem_impl::subsystem_load
bool Event_subsystem_impl::subsystem_load()
{
	_subsystem_state = subsys_loaded;
	return true;
}

//---------------------------------------------------------------------Event_subsystem_impl::report

void Event_subsystem_impl::report( Scheduler_event2& event )
{
	return;

    try {
        _log->info( event.obj_name() );

	    //_event_listener.newEvent(event.code()); 
		
		// ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(event.object()->idispatch, "spooler_order");
		ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(event.object()->idispatch(), "spooler_order");
//	    _event_listener.newEvent((::javaproxy::java::lang::Object)java_idispatch->get_jobject() );
	 //   _event_listener.newEvent( java_idispatch->get_jobject() );

		// ptr<javabridge::Java_idispatch> java_idispatch = Java_subsystem_interface::instance_of_scheduler_object(event->idispatch(), "name?");
        //_java_event_subsystem.report( (javaproxy::...::Scheduler_event)java_idispatch->get_jobject() );
        //Für Java-Methode EventSubsystem.report( Scheduler_event e );

        // Wenn C++-Cast nicht klappt:
        //_java_event_subsystem.report( java_idispatch->get_jobject() );
        //Für Java-Method  report( Object o ) { ... (Scheduler_event)o }

        //if( Scheduler_script* s = spooler()->scheduler_script_subsystem()->scheduler_script_or_null( Absolute_path( "/scheduler-event" ) ) ) {
        //    s->module_instance()->call_if_exists( "spooler_event(Lsos/spooler/Order)V", event.iunknown() );
        //}
    }
    catch( exception& x ) { _log->warn( x.what() ); }
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
