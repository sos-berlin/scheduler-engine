// $Id: supervisor.h 13239 2007-12-31 11:54:19Z jz $

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

//-------------------------------------------------------------------------------------------------

} //namespace supervisor
} //namespace scheduler
} //namespace sos
