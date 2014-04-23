package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.schedule.Schedule;

@CppClass(clas="sos::scheduler::schedule::Schedule", directory="scheduler", include="spooler.h")
public interface ScheduleC extends CppProxyWithSister<Schedule>, File_basedC<Schedule> {
    Schedule.Type sisterType = new Schedule.Type();
}
