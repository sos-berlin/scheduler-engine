package com.sos.scheduler.engine.tests.excluded.js721;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.TimerTask;

import org.apache.log4j.Logger;

public class TaskObserver extends TimerTask {

	private static final Logger logger = Logger.getLogger(TaskObserver.class);
	
	private final Long start;
	private final String filename;
	private final PrintWriter out;
	private final TaskInfo testClass;

	public TaskObserver(String filename, TaskInfo classInTest) throws IOException {
		this.filename = filename;
		this.testClass = classInTest;
		FileWriter outFile = new FileWriter(this.filename);
		out = new PrintWriter(outFile);
		out.println("duration;running;highwater;ended;estimated");
		start = System.currentTimeMillis();
	}

	@Override
	public void run() {
		long runningSince = (this.scheduledExecutionTime() - start) / 1000;
		TaskInfo c = testClass;
		logger.debug(runningSince + ": " + c.currentlyRunningTasks() + "(high " + c.highwaterTasks() + ") are currently running (" + c.endedTasks() + " of " + c.estimatedTasks() + " are finished)");
		out.println(runningSince + ";" + c.currentlyRunningTasks() + ";" + c.highwaterTasks() + ";" + c.endedTasks() + ";" + c.estimatedTasks());
	}

	public void close() {
		out.close();
	}
	
}
