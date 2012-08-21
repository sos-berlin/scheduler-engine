package com.sos.scheduler.engine.tests.jira.js628;

import sos.spooler.Job_impl;

public class SpoolerProcessFalse extends Job_impl {
    @Override
    public boolean spooler_process() {
        return false;
    }
}
