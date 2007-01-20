// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_SCRIPT_H
#define __SCHEDULER_SCRIPT_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------Scheduler_script_interface

struct Scheduler_script_interface : Subsystem
{
    virtual void                set_dom_script              ( const xml::Element_ptr& script_element, const Time& xml_mod_time, const string& include_path ) = 0;

    virtual Module_instance*    module_instance             ()                                      = 0;

  protected:                  
                                Scheduler_script_interface  ( Scheduler* scheduler, Type_code t )   : Subsystem( scheduler, t ) {}
};

//-------------------------------------------------------------------------------------------------

ptr<Scheduler_script_interface> new_scheduler_script        ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
