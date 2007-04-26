// $Id: scheduler_script.h 4773 2007-01-20 23:15:39Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_JAVA_SUBSYSTEM_H
#define __SCHEDULER_JAVA_SUBSYSTEM_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------Java_subsystem_interface

struct Java_subsystem_interface : Object, Subsystem
{
                                Java_subsystem_interface    ( Scheduler* scheduler, Type_code t )   : Subsystem( scheduler, this, t ) {}

    virtual java::Vm*           java_vm                     ()                                      = 0;
};

//-------------------------------------------------------------------------------------------------

ptr<Java_subsystem_interface>   new_java_subsystem          ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
