package com.sos.scheduler.engine.kernel.scheduler;

public interface SchedulerXmlCommandExecutor {
    String uncheckedExecuteXml(String xml);
    String executeXml(String xml);
}
