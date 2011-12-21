package com.sos.scheduler.engine.test.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.*;

import org.apache.log4j.Logger;

public class LockFile extends  Observable implements Runnable  {

	private final static Logger logger = Logger.getLogger(LockFile.class);
	
	private final File file;
	private final int duration;
	
	public LockFile(File fileToLock, int durationInSeconds) {
		file = fileToLock;
		duration = durationInSeconds;
	}
	
	@Override
	public void run() {
		FileInputStream in = null;
		try {
			in = new FileInputStream(file);
		    java.nio.channels.FileLock lock = in.getChannel().tryLock(0L, Long.MAX_VALUE, true);
		    try {
		    	Thread.sleep(duration*1000);
		    } catch (InterruptedException e) {
		    	logger.error("sleep interrupted - " + e.getMessage());
		    	logger.trace("error in sleep", e);
		    	tellObserver(e);
			} finally {
		        lock.release();
		    }
		} catch (FileNotFoundException e) {
			String message = "file '" + file.getAbsolutePath() + "' does not exist";
	    	logger.error(message);
	    	logger.trace(message, e);
	    	tellObserver(e);
		} catch (IOException e) {
	    	logger.error(e.getMessage());
	    	logger.trace(e.getMessage(), e);
	    	tellObserver(e);
		} finally {
		    if (in != null)
				try {
					in.close();
				} catch (IOException e) {
					String message = "file '" + file.getAbsolutePath() + "' could not be closed";
			    	logger.error(message);
			    	logger.trace(message, e);
			    	tellObserver(e);
				}
		}
	}
	
	private void tellObserver(Exception e) {
		setChanged();
		notifyObservers(e);
	}

}
