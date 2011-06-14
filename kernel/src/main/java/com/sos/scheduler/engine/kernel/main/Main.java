package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.util.Time;


public class Main {
    private final SchedulerController schedulerController = new SchedulerThread();


    public final int apply(String[] args) {
        schedulerController.loadModule();
        schedulerController.startScheduler(args);
        schedulerController.waitForTermination(Time.eternal);
        return schedulerController.getExitCode();
    }

    public static void main(String[] args) throws Exception {
        int exitCode = new Main().apply(args);
        System.exit(exitCode);  //NOSONAR
    }
}
