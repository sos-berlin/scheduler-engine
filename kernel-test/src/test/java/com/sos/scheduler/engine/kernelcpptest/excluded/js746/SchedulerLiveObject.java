package com.sos.scheduler.engine.kernelcpptest.excluded.js746;

public class SchedulerLiveObject {
	
	private final String name;
	private final ObjectType type;
	
	public SchedulerLiveObject(String name, ObjectType type) {
		this.name = name;
		this.type = type;
	}
	
	public String getName() {
		return this.name;
	}
	
	public ObjectType getType() {
		return this.type;
	}
	
	public boolean isOrder() {
		return (this.type == ObjectType.ORDER) ? true : false;
	}
	
	public enum ObjectType {
		STANDALONE_JOB,
		ORDER;
		
		public String getText() {
			String result = "standalone job";
			if (this == ObjectType.ORDER) result = "order";
			return result;
		}
	}

}
