package com.sos.scheduler.engine.kernelcpptest.excluded.js644;

import java.io.File;
import java.util.Collection;


public class FilesModifierThread extends Thread {
    private final FilesModifier filesModifier;


    public FilesModifierThread(Collection<File> files) {
        filesModifier = new FilesModifier(files);
    }


    @Override public void run() {
        try {
            int pause = 4000 / filesModifier.getFileCount() + 1;   // Mindestens 2s Pause, damit Scheduler Datei als stabil ansieht.
            while(true) {
                filesModifier.modifyNext();
                Thread.sleep(pause);
            }
        } catch (InterruptedException x) {
            // Ende
        }
    }
}
