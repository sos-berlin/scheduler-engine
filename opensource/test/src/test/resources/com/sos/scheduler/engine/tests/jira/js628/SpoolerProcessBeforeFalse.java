package com.sos.scheduler.engine.tests.jira.js628;

import sos.spooler.Monitor_impl;

public class SpoolerProcessBeforeFalse extends Monitor_impl {
    @Override
    public boolean spooler_process_before() {
        return false;
    }
}
