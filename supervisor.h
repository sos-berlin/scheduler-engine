// $Id$

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------------Supervisor_interface
    
struct Supervisor_interface : Subsystem, Object
{
                                Supervisor_interface        ( Scheduler* scheduler, Type_code tc )  : Subsystem( scheduler, this, tc ) {}

    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
    virtual void                execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* ) = 0;
    virtual void                execute_configuration_fetch_updated_files( xml::Xml_writer*, const xml::Element_ptr&, Communication::Operation* ) = 0;
};


ptr<Supervisor_interface>       new_supervisor              ( Scheduler* );

//-----------------------------------------------------------------------Remote_scheduler_interface

struct Remote_scheduler_interface : zschimmer::Object
{
    virtual void                connection_lost_event       ( const exception* )                    = 0;
};

//----------------------------------------------------------------------Supervisor_client_interface

struct Supervisor_client_interface: idispatch_implementation< Supervisor_client_interface, spooler_com::Isupervisor_client >,
                                    Subsystem
{
                                Supervisor_client_interface ( Scheduler*, Type_code, Class_descriptor* );
};


ptr<Supervisor_client_interface> new_supervisor_client      ( Scheduler*, const Host_and_port& );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
