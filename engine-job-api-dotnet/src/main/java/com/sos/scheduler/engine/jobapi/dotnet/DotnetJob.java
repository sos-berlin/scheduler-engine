package com.sos.scheduler.engine.jobapi.dotnet;

import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;

public final class DotnetJob extends sos.spooler.Job_impl {
	private final static String LOG_PREFIX = "[.NET module]";
	private final DotnetApiImpl apiImpl;

	DotnetJob(TaskContext taskContext, DotnetApiImpl dotnetApiImpl)
			throws Exception {
		spooler_log = taskContext.spoolerLog();
		spooler_task = taskContext.spoolerTask();
		spooler_job = taskContext.spoolerJob();
		spooler = taskContext.spooler();
		this.apiImpl = dotnetApiImpl;
		this.apiImpl.init(spooler, spooler_job, spooler_task, spooler_log);
	}

	@Override
	public boolean spooler_init() throws Exception {
		try {
			return apiImpl.spooler_init();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public void spooler_exit() throws Exception {
		try {
			apiImpl.spooler_exit();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public boolean spooler_open() throws Exception {
		try {
			return apiImpl.spooler_open();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public void spooler_close() throws Exception {
		try {
			apiImpl.spooler_close();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public boolean spooler_process() throws Exception {
		try {
			return apiImpl.spooler_process();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public void spooler_on_error() throws Exception {
		try {
			apiImpl.spooler_on_error();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	@Override
	public void spooler_on_success() throws Exception {
		try {
			apiImpl.spooler_on_success();
		} catch (Exception ex) {
			throw new Exception(getLogMessage(ex.toString()));
		}
	}

	private String getLogMessage(String msg) {
		return String.format("%s %s", LOG_PREFIX, msg);
	}

}
