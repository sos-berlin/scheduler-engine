package com.sos.scheduler.engine.plugins.jms.stress;

public interface TaskInfo extends TaskInfoListener {
	int currentlyRunningTasks();
	int endedTasks();
	int highwaterTasks();
}
