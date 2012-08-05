package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.kernel.CppScheduler;
import com.sos.scheduler.engine.kernel.util.Time;

class Main {
    private final SchedulerController schedulerController = new SchedulerThreadController(Main.class.getName());

    private int apply(String[] args) {
        CppScheduler.loadModuleFromPath();  // TODO Methode nur provisorisch. Besser den genauen Pfad übergeben, als Kommandozeilenparameter.
        schedulerController.startScheduler(args);
        schedulerController.tryWaitForTermination(Time.eternal);
        return schedulerController.exitCode();
    }

    public static void main(String[] args) throws ExitCodeException {
        int exitCode = new Main().apply(args);
        if (exitCode != 0)  throw new ExitCodeException(exitCode);
    }
}
