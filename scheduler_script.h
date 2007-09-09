// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_SCRIPT_H
#define __SCHEDULER_SCRIPT_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------Scheduler_script_interface

struct Scheduler_script_interface : Object, Subsystem
{
    virtual void                set_dom_script              ( const xml::Element_ptr& script_element ) = 0;

    virtual Module*             module                      ()                                      = 0;
    virtual Module_instance*    module_instance             ()                                      = 0;

  protected:                  
                                Scheduler_script_interface  ( Scheduler* scheduler, Type_code t )  : Subsystem( scheduler, this, t ) {}
};

//-------------------------------------------------------------------------------------------------

ptr<Scheduler_script_interface> new_scheduler_script        ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
