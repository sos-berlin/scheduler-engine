package com.sos.scheduler.engine.plugins.jms.stress;

import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class TaskObserver extends TimerTask {

	private static final Logger logger = Logger.getLogger(TaskObserver.class);
	
	private final Long start;
	private final TaskInfo testClass;
    private final Timer timer;
    private final int estimatedTasks;
    private long runningSince;


	public TaskObserver(TaskInfo classInTest, int estimatedTasks) throws IOException {
		this.testClass = classInTest;
		this.start = System.currentTimeMillis();
		this.runningSince = 0;
        this.estimatedTasks = estimatedTasks;
        timer = new Timer();
	}
	
	@Override
	public void run() {
		TaskInfo c = testClass;
		runningSince = (this.scheduledExecutionTime() - start) / 1000;
		logger.debug(runningSince + ": " + c.currentlyRunningTasks() + " (high " + c.highwaterTasks() + ") are currently running (" + c.endedTasks() + " of " + estimatedTasks + " are finished)");
		c.onInterval(c);
	}
	
	public long runningSince() {
		return runningSince; 
	}

    public int getEstimatedTasks() {
        return estimatedTasks;
    }

    public void start(long delay, long period) {
        timer.schedule(this, delay, period); // schedule the timer task
    }

    public void stop() {
        close();
    }

	public void close() {
        timer.cancel();
        timer.purge();
    }
}
