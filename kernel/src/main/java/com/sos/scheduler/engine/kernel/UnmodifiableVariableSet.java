package com.sos.scheduler.engine.kernel;

public interface UnmodifiableVariableSet {

	public abstract String get(String name);

	public abstract String getStrictly(String name);

}