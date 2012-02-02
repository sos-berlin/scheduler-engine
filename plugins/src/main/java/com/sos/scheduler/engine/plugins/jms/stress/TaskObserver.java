package com.sos.scheduler.engine.plugins.jms.stress;

import java.io.IOException;
import java.util.TimerTask;

import org.apache.log4j.Logger;

public class TaskObserver extends TimerTask {

	private static final Logger logger = Logger.getLogger(TaskObserver.class);
	
	private final Long start;
	private final TaskInfo testClass;
	private long runningSince;

	public TaskObserver(TaskInfo classInTest) throws IOException {
		this.testClass = classInTest;
		this.start = System.currentTimeMillis();
		this.runningSince = 0;
	}
	
	@Override
	public void run() {
		TaskInfo c = testClass;
		runningSince = (this.scheduledExecutionTime() - start) / 1000;
		logger.debug(runningSince + ": " + c.currentlyRunningTasks() + " (high " + c.highwaterTasks() + ") are currently running (" + c.endedTasks() + " of " + c.estimatedTasks() + " are finished)");
		c.onInterval(c);
	}
	
	public long runningSince() {
		return runningSince; 
	}
	
	public void close() {;}
	
}
