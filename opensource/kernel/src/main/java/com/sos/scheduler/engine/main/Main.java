package com.sos.scheduler.engine.main;

import com.sos.scheduler.engine.kernel.CppScheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import org.joda.time.Duration;

import static java.util.Arrays.asList;

class Main {
    private final SchedulerController schedulerController = new SchedulerThreadController(Main.class.getName(), CppSettings.Empty());

    private int apply(String[] args) {
        CppScheduler.loadModuleFromPath();  // TODO Methode nur provisorisch. Besser den genauen Pfad übergeben, als Kommandozeilenparameter.
        schedulerController.startScheduler(asList(args));
        schedulerController.tryWaitForTermination(new Duration(Long.MAX_VALUE));
        return schedulerController.exitCode();
    }

    public static void main(String[] args) {
        int exitCode = new Main().apply(args);
        if (exitCode != 0) throw new ExitCodeException(exitCode);
    }
}
