package com.sos.scheduler.engine.tests.jira.js644.continuous;

import static com.sos.scheduler.engine.kernel.util.Util.sleepUntilInterrupted;
import static java.lang.Math.random;

import java.io.File;

final class FilesModifierRunnable implements Runnable {
    private final FilesModifier filesModifier;

    FilesModifierRunnable(Iterable<File> files) {
        filesModifier = new FilesModifier(files);
    }

    @Override public void run() {
        while(true) {
            filesModifier.modifyRandom();
            int pause = 1 + (int)(4000*random()) / filesModifier.fileCount();   // Nach 2s Pause sieht der Scheduler eine Dateiänderung als stabil an
            boolean interrupted = sleepUntilInterrupted(pause);
            if (interrupted) break;
        }
    }
}
