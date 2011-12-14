package com.sos.scheduler.engine.tests.jira.js801;

import java.util.Date;

public class AtMonitor extends sos.spooler.Monitor_impl {
    @Override public boolean spooler_process_after(boolean result) {
        spooler_task.order().set_at(new Date(new Date().getTime() + 2000));
        return result;
    }
}
