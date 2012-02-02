package com.sos.scheduler.engine.plugins.jms.stress;

public interface TaskInfo extends TaskInfoListener {
	public int currentlyRunningTasks();
	public int endedTasks();
	public int highwaterTasks();
	public int estimatedTasks();
}
