package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.util.Time;

public class Main {
    private final SchedulerController schedulerController = new SchedulerThreadController();

    public final int apply(String[] args) {
        schedulerController.startScheduler(args);
        schedulerController.waitForTermination(Time.eternal);
        return schedulerController.exitCode();
    }

    public static void main(String[] args) throws Exception {
        int exitCode = new Main().apply(args);
        if (exitCode != 0)  throw new ExitCodeException(exitCode);
    }
}
