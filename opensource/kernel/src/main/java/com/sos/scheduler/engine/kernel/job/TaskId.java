package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.util.StringValue;

public class TaskId extends StringValue {
	public TaskId(int x) { super( String.valueOf(x) ); }
}
