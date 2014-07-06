package com.sos.scheduler.engine.tests.jira.js644.continuous;

import sos.spooler.Job_impl;

import static java.lang.Thread.sleep;

public class TestJob extends Job_impl {
    @Override public final boolean spooler_open() throws InterruptedException {
        sleep(5000);
        return true;
    }

    @Override public final boolean spooler_process() throws InterruptedException {
        sleep(1000);
        return true;
    }
}
