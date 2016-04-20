package com.sos.scheduler.engine.jobapi.dotnet;

import java.nio.file.Path;

import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;

import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleInstanceFactory;
import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;

public final class Jni4netModuleInstanceFactory implements
		DotnetModuleInstanceFactory {

	private DotnetBridge dotnetBridge;

	public Jni4netModuleInstanceFactory(Path dllDirectory) throws Exception {
		boolean debug = System.getProperty("jni4net.debug") != null;
		dotnetBridge = new DotnetBridge();
		dotnetBridge.init(dllDirectory, debug);
	}

	public void close() {
		dotnetBridge.close();
	}

	public <T> T newInstance(Class<T> clazz, TaskContext taskContext, DotnetModuleReference reference) throws Exception {
		if (reference instanceof DotnetModuleReference.DotnetClass) {
			DotnetApiImpl impl = newDotnetObject(reference);
			return newSchedulerDotnetAdapter(clazz, impl, taskContext);
		} else if (reference instanceof DotnetModuleReference.Powershell) {
			DotnetApiImpl impl = newDotnetObject(reference);
			return newSchedulerDotnetAdapter(clazz, impl, taskContext);
		} else
			throw new RuntimeException("Unknown class " + reference.getClass().getName());
	}

	private DotnetApiImpl newDotnetObject(DotnetModuleReference ref) {
		return new DotnetApiImpl(dotnetBridge,ref);
	}

	@SuppressWarnings("unchecked")
	private <T> T newSchedulerDotnetAdapter(Class<T> clazz,
			DotnetApiImpl dotnetObject, TaskContext taskContext)
			throws Exception {
		if (Job_impl.class.isAssignableFrom(clazz)) {
			return (T) new DotnetJob(taskContext, dotnetObject);
		} else if (Monitor_impl.class.isAssignableFrom(clazz)) {
			return (T) new DotnetMonitor(taskContext, dotnetObject);
		} else {
			throw new IllegalArgumentException("Unsupported " + clazz);
		}
	}

	private <T> void closeInstance(T instance) {
		// TODO Close DotnetJob or DotnetMonitor
	}
}
