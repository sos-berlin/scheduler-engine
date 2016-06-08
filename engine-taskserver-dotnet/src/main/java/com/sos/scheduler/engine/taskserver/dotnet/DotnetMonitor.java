package com.sos.scheduler.engine.taskserver.dotnet;

import com.sos.scheduler.engine.taskserver.dotnet.api.TaskContext;

public final class DotnetMonitor extends sos.spooler.Monitor_impl {
	private final static String LOG_PREFIX = "[.NET module]";
	private final DotnetApiImpl apiImpl;

	DotnetMonitor(DotnetApiImpl dotnetApiImpl)
			throws Exception {
		this.apiImpl = dotnetApiImpl;
	}

	@Override
	public boolean spooler_task_before() throws Exception {
		try {
			return apiImpl.spooler_task_before();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public void spooler_task_after() throws Exception {
		try {
			apiImpl.spooler_task_after();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public boolean spooler_process_before() throws Exception {
		try {
			return apiImpl.spooler_process_before();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public boolean spooler_process_after(boolean spooler_process_result)
			throws Exception {
		try {
			return apiImpl.spooler_process_after(spooler_process_result);
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	private String getLogMessage(String msg) {
		return String.format("%s %s", LOG_PREFIX, msg);
	}
}
