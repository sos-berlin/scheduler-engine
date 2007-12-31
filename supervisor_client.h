// $Id$

namespace sos {
namespace scheduler {
namespace supervisor {

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
