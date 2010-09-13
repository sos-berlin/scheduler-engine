// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_JAVA_SUBSYSTEM_H
#define __SCHEDULER_JAVA_SUBSYSTEM_H

namespace sos {
namespace scheduler {

//-----------------------------------------------------------------------Java_subsystem_interface

struct Java_subsystem_interface : Object, Subsystem
{
                                Java_subsystem_interface    ( Scheduler* scheduler, Type_code t )   : Subsystem( scheduler, this, t ) {}

    virtual javabridge::Vm*     java_vm                     ()                                      = 0;
    virtual void            set_java_options                ( const string& )                       = 0;
    virtual void                prepend_class_path          ( const string& )                       = 0;

    static string               classname_of_scheduler_object(const string&);
    static ptr<javabridge::Java_idispatch>  instance_of_scheduler_object( IDispatch*, const string&);
};

//-------------------------------------------------------------------------------------------------

ptr<Java_subsystem_interface>   new_java_subsystem          ( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
