package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.util.Observable;
import java.util.Observer;

import org.apache.log4j.Logger;
import org.junit.Test;
import org.junit.Ignore;

public class LockFileTest implements Observer {
	
	@SuppressWarnings("unused")
	private static final Logger logger = Logger.getLogger(LockFileTest.class);
	
	private final static String lockfile = "fileToLock.txt";
	private RuntimeException result = null;

	/**
	 * Locks the same file twice. The second try to lock the file will cause a FileAlreadyLockedException.
	 * It will be notified in method update and thrown in method postprocessing. 
	 * @throws Exception
	 */
	@Test (expected=FileAlreadyLockedException.class)
	public void testLockFileTwice() throws Exception {
		preprocessing();
		File lock = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, lockfile);
		LockFile l = new LockFile(lock, 2);
		l.addObserver(this);
		l.lock();
		l.lock();
		while(l.isAlive()) {}
		postprocessing();
	}

	// Probleme auf w2k3
	@Ignore
	public void testLockFileSingle() throws Exception {
		preprocessing();
		File lock = JSFileUtils.getLocalResourceFile(JSCommandUtilsTest.class, lockfile);
		LockFile l = new LockFile(lock, 2);		// lock the file for 2 seconds
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
