package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.schedule.Schedule;

@CppClass(clas="sos::scheduler::schedule::Schedule_subsystem_interface", directory="scheduler", include="spooler.h")
public interface Schedule_subsystemC extends SubsystemC<Schedule, ScheduleC>, CppProxy {
    //ScheduleSubsystem.Type sisterType = new ScheduleSubsystem.Type();
}
