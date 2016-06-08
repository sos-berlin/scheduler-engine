package com.sos.scheduler.engine.taskserver.dotnet;

import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleInstanceFactory;
import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.taskserver.dotnet.api.TaskContext;
import java.nio.file.Path;
import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;

public final class Jni4netModuleInstanceFactory implements
		DotnetModuleInstanceFactory {

	private DotnetBridge dotnetBridge;
	private final Path dllDirectory;

	public Jni4netModuleInstanceFactory(Path dllDirectory) throws Exception {
		this.dllDirectory = dllDirectory;
		boolean debug = System.getProperty("jni4net.debug") != null;
		dotnetBridge = new DotnetBridge();
		dotnetBridge.init(dllDirectory, debug);
	}

	public void close() {
		dotnetBridge.close();
	}

	public <T> T newInstance(Class<T> clazz, TaskContext taskContext, DotnetModuleReference reference) throws Exception {
		if (reference instanceof DotnetModuleReference.DotnetClass || reference instanceof DotnetModuleReference.Powershell) {
			DotnetApiImpl impl = newDotnetObject(reference);
			return newSchedulerDotnetAdapter(clazz, impl, taskContext);
		} else
			throw new RuntimeException("Unsupported " + reference.getClass());
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

	@Override
	public String toString() {
		return "Jni4netModuleInstanceFactory(" + dllDirectory + ")";
	}
}
