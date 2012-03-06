package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.model.events.InfoTask;

public class JMSTaskAdapter extends InfoTask {
	
	private JMSTaskAdapter(UnmodifiableTask kernelEventTask) {
		super();
		setId( String.valueOf(kernelEventTask.getId()) );
	}

	public static JMSTaskAdapter createInstance(UnmodifiableTask kernelEventTask) {
		return new JMSTaskAdapter(kernelEventTask);
	}

}
