package com.sos.scheduler.engine.test.util;

import com.google.common.io.Resources;
import org.junit.Ignore;
import org.junit.Test;

import java.io.File;
import java.net.URL;
import java.util.Observable;
import java.util.Observer;

public class LockFileTest implements Observer {

    private final static String lockfile = "com/sos/scheduler/engine/test/util/fileToLock.txt";
    private RuntimeException result = null;

    /**
     * Locks the same file twice. The second try to lock the file will cause a FileAlreadyLockedException.
     * It will be notified in method update and thrown in method postprocessing.
     * @throws FileAlreadyLockedException
     */
    @Test (expected=FileAlreadyLockedException.class)
    public void testLockFileTwice() throws Exception {
        preprocessing();
        URL url = Resources.getResource(lockfile);
        LockFile l = new LockFile( new File(url.toURI()), 2);
        l.addObserver(this);
        l.lock();
        l.lock();
        while(l.isAlive()) {}
        postprocessing();
    }

    //TODO Probleme auf w2k3
    @Ignore
    public void testLockFileSingle() throws Exception {
        preprocessing();
        URL url = Resources.getResource(lockfile);
        // File lock = FileUtils.getTempFile(CommandBuilderTest.class, lockfile);
        LockFile l = new LockFile( new File(url.toURI()), 2);		// lock the file for 2 seconds
        l.addObserver(this);
        l.lock();
        Thread.sleep(3000);						// wait until the file is released
        l.lock();
        while(l.isAlive()) {}
        postprocessing();
    }

    /* If the file is already locked, an exception will notified to all observers (other
      * Exception also will be notified).
      *
      * (non-Javadoc)
      * @see java.util.Observer#update(java.util.Observable, java.lang.Object)
      */
    @Override
    public void update(Observable o, Object arg) {
        if (arg instanceof RuntimeException)
            result = (RuntimeException)arg;
    }

    private void preprocessing() {
        result = null;
    }

    /**
     * Throw an exception notified from the thread.
     */
    private void postprocessing() {
        if (result != null) throw (result);
    }

}
