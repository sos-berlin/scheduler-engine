package com.sos.scheduler.engine.jobapi.dotnet;

import java.nio.file.Path;

import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;

import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleInstanceFactory;
import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;

public final class Jni4netModuleInstanceFactory implements
		DotnetModuleInstanceFactory {

	private final static String POWERSHELL_CLASS_NAME = "com.sosberlin.jobscheduler.dotnet.adapter.SosJobSchedulerPowershellAdapter";
	private DotnetBridge dotnetBridge;

	public Jni4netModuleInstanceFactory(Path dllDirectory) throws Exception {
		boolean debug = true; // ??? spooler_log.level() != 0;

		dotnetBridge = new DotnetBridge();
		dotnetBridge.init(dllDirectory, debug);
	}

	public void close() {
		dotnetBridge.close();
	}

	public <T> T newInstance(Class<T> clazz, TaskContext taskContext,
			DotnetModuleReference reference) {
		try {
			if (reference instanceof DotnetModuleReference.DotnetClass) {
				DotnetApiImpl impl = newDotnetObject((DotnetModuleReference.DotnetClass) reference);
				return newSchedulerDotnetAdapter(clazz, impl, taskContext);
			} else if (reference instanceof DotnetModuleReference.Powershell) {
				DotnetApiImpl impl = newPowershellObject((DotnetModuleReference.Powershell) reference);
				return newSchedulerDotnetAdapter(clazz, impl, taskContext);
			} else
				throw new RuntimeException("Unknown class "
						+ reference.getClass().getName());
		} catch (Exception ex) {
			throw new RuntimeException(ex.toString());
		}
	}

	private DotnetApiImpl newDotnetObject(DotnetModuleReference.DotnetClass ref) {
		return new DotnetApiImpl(dotnetBridge.getSchedulerApiTypes(),
				ref.dll(), ref.className());
	}

	private DotnetApiImpl newPowershellObject(
			DotnetModuleReference.Powershell ref) {
		return new DotnetApiImpl(dotnetBridge.getSchedulerApiTypes(),
				dotnetBridge.getDotnetAdapterDll(), POWERSHELL_CLASS_NAME,
				ref.script());
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
