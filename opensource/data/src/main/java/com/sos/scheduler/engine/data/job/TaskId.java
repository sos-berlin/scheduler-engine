package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.base.StringValue;
import org.codehaus.jackson.map.annotate.JsonDeserialize;

@JsonDeserialize(using=TaskIdDeserializer.class)
public class TaskId extends StringValue {
	public TaskId(int x) { super( String.valueOf(x) ); }
}
