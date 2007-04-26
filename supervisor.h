// $Id: spooler_remote.h 4682 2006-12-22 12:02:12Z jz $

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------Supervisor_interface
    
struct Supervisor_interface: Object, Subsystem
{
                                Supervisor_interface        ( Scheduler* scheduler, Type_code tc )  : Subsystem( scheduler, this, tc ) {}

    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
    virtual void                execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* ) = 0;
};


ptr<Supervisor_interface>       new_supervisor              ( Scheduler* );

//-----------------------------------------------------------------------Remote_scheduler_interface

struct Remote_scheduler_interface : zschimmer::Object
{
    virtual void                connection_lost_event       ( const exception* )                    = 0;
};

//----------------------------------------------------------------------Supervisor_client_interface

struct Supervisor_client_interface: Object, Subsystem
{
                                Supervisor_client_interface ( Scheduler* scheduler, Type_code tc )  : Subsystem( scheduler, this, tc ) {}
};


ptr<Supervisor_client_interface> new_supervisor_client      ( Scheduler*, const Host_and_port& );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
