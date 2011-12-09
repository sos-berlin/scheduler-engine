package com.sos.scheduler.engine.tests.excluded.js721;

public interface TaskInfo {
	public int currentlyRunningTasks();
	public int endedTasks();
	public int highwaterTasks();
	public int estimatedTasks();
}
