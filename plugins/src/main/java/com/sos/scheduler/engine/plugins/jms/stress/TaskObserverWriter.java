package com.sos.scheduler.engine.plugins.jms.stress;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

public class TaskObserverWriter extends TaskObserver implements TaskInfoListener {

	private final PrintWriter out;

	public TaskObserverWriter(String filename, TaskInfo classInTest, int estimatedTasks) throws IOException {
		super(classInTest, estimatedTasks);
		FileWriter outFile = new FileWriter(filename);
		out = new PrintWriter(outFile);
		out.println("duration;running;highwater;ended;estimated");
	}
	
	public void close() {
		out.close();
	}

	@Override
	public void onInterval(TaskInfo info) {
		out.println(runningSince() + ";" + info.currentlyRunningTasks() + ";" + info.highwaterTasks() + ";" + info.endedTasks() + ";" + getEstimatedTasks());
	}
	
}
