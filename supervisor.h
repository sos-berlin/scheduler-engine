// $Id$

namespace sos {
namespace scheduler {
namespace supervisor {

//-----------------------------------------------------------------------------Supervisor_interface
    
struct Supervisor_interface : Subsystem, Object
{
                                Supervisor_interface        ( Scheduler* scheduler, Type_code tc )  : Subsystem( scheduler, this, tc ) {}

    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& ) = 0;
    virtual void                execute_register_remote_scheduler( const xml::Element_ptr&, Communication::Operation* ) = 0;
    virtual ptr<Command_response> execute_xml               ( const xml::Element_ptr&, Command_processor* ) = 0;
};


ptr<Supervisor_interface>       new_supervisor              ( Scheduler* );

//-----------------------------------------------------------------------Remote_scheduler_interface

struct Remote_scheduler_interface : Async_operation
{
    virtual void                connection_lost_event       ( const exception* )                    = 0;
    virtual ptr<Command_response> execute_xml               ( const xml::Element_ptr&, Command_processor* ) = 0;
};

//----------------------------------------------------------------------Supervisor_client_interface

struct Supervisor_client_interface: idispatch_implementation< Supervisor_client_interface, spooler_com::Isupervisor_client >,
                                    Subsystem
{
                                Supervisor_client_interface ( Scheduler*, Type_code, Class_descriptor* );

    virtual bool                is_ready                    () const                                = 0;
    virtual bool                connection_failed           () const                                = 0;
    virtual void                start_update_configuration  ()                                      = 0;
};


ptr<Supervisor_client_interface> new_supervisor_client      ( Scheduler*, const Host_and_port& );

//-------------------------------------------------------------------------------------------------

} //namespace supervisor
} //namespace scheduler
} //namespace sos
