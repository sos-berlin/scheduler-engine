package com.sos.scheduler.engine.kernel.scheduler;

import com.google.inject.ImplementedBy;
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel;
import com.sos.scheduler.engine.kernel.Scheduler;

@ImplementedBy(Scheduler.class)
public interface SchedulerXmlCommandExecutor {
    String uncheckedExecuteXml(String xml);
    String uncheckedExecuteXml(String xml, SchedulerSecurityLevel securityLevel);
    String executeXml(String xml);
}
