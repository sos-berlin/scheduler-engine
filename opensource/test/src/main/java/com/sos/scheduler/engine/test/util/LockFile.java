package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.nio.channels.OverlappingFileLockException;
import java.util.Observable;
import java.util.Observer;

import org.apache.log4j.Logger;

public class LockFile extends Observable implements Observer {
	
	private final static Logger logger = Logger.getLogger(LockFile.class);
	private final LockFileThread lock;
	private Thread t;

	public LockFile(String filename, int durationInSeconds) {
		lock = prepare(new File(filename), durationInSeconds);
	}
	
	public LockFile(File file, int durationInSeconds) {
		lock = prepare(file, durationInSeconds);
	}

	private LockFileThread prepare(File file, int durationInSeconds) {
		LockFileThread result = new LockFileThread(file, durationInSeconds);
		result.addObserver(this);
		return result;
	}
	
	public void lock() {
		t = new Thread(lock);
		t.start();
	}

	@Override
	public void update(Observable o, Object arg) {
		setChanged();
		notifyObservers( arg );
	}

	public boolean isAlreadyLocked() {
		return lock.isAlreadyLocked();
	}

	public boolean isAlive() {
		return t.isAlive();
	}
	
}
