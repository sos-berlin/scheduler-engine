package com.sos.scheduler.engine.jobapi.dotnet;

import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.util.Optional;

import net.sf.jni4net.Bridge;
import sos.spooler.Job;
import sos.spooler.Log;
import sos.spooler.Spooler;
import sos.spooler.Task;
import system.reflection.Assembly;

public class DotnetApiImpl {
	private system.Type apiImplType;
	private system.Object apiImplInstance = null;
	private system.Type[] schedulerApiTypes = null;
	private Path path;
	private String className;
	private String script;

	public DotnetApiImpl(system.Type[] types, Path dll, String dotnetClassName) {
		this(types, dll, dotnetClassName, null);
	}

	public DotnetApiImpl(system.Type[] types, Path dll, String dotnetClassName,
			String script) {
		this.schedulerApiTypes = types;
		this.path = dll;
		this.className = dotnetClassName;
		this.script = script;
	}

	public void init(Spooler spooler, Job spoolerJob, Task spoolerTask,
			Log spoolerLog) throws Exception {

		if (!Files.exists(path, LinkOption.NOFOLLOW_LINKS)) {
			throw new Exception(String.format("File not found: %s",
					path.toString()));
		}

		Assembly assembly = null;
		try {
			assembly = Assembly.LoadFrom(path.toString());
		} catch (Exception ex) {
			throw new Exception(String.format("[%s] Can't load assembly: %s",
					path.toString(), ex.toString()));
		}

		apiImplType = Optional.ofNullable(assembly.GetType(this.className))
				.orElseThrow(
						() -> new Exception(String.format(
								"[%s] Class not found: %s", path.toString(),
								this.className)));

		system.Object[] params = new system.Object[4];
		params[0] = Bridge.wrapJVM(spooler);
		params[1] = Bridge.wrapJVM(spoolerJob);
		params[2] = Bridge.wrapJVM(spoolerTask);
		params[3] = Bridge.wrapJVM(spoolerLog);

		apiImplInstance = Optional.ofNullable(
				DotnetInvoker.createInstance(apiImplType, schedulerApiTypes,
						params)).orElseThrow(
				() -> new Exception(String.format(
						"[%s] Could not create a new instance of the class %s",
						path.toString(), this.className)));

		if (this.script != null) {
			invokeSetScriptMethod(this.script);
		}
	}

	private void invokeSetScriptMethod(String script) throws Exception {
		DotnetInvoker.invokeMethod(apiImplType, apiImplInstance, "SetScript",
				script);
	}

	public boolean spooler_init() throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_init", true);
	}

	public void spooler_exit() throws Exception {
		if (apiImplType != null && apiImplInstance != null) {
			DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
					"spooler_exit");
		}
	}

	public boolean spooler_open() throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_open", true);
	}

	public void spooler_close() throws Exception {
		if (apiImplType != null && apiImplInstance != null) {
			DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
					"spooler_close");
		}
	}

	public boolean spooler_process() throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_process", false);
	}

	public void spooler_on_error() throws Exception {
		if (apiImplType != null && apiImplInstance != null) {
			DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
					"spooler_on_error");
		}
	}

	public void spooler_on_success() throws Exception {
		if (apiImplType != null && apiImplInstance != null) {
			DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
					"spooler_on_success");
		}
	}

	public boolean spooler_task_before() throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_task_before", true);
	}

	public void spooler_task_after() throws Exception {
		DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_task_after");

	}

	public boolean spooler_process_before() throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_process_before", true);
	}

	private system.Object toDotnetBoolean(boolean value) throws Exception {
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"ToBoolean", value ? "true" : "false");
	}

	public boolean spooler_process_after(boolean spooler_process_result)
			throws Exception {
		system.Type[] paramTypes = new system.Type[] { system.Type
				.GetType("System.Boolean") };
		system.Object[] params = new system.Object[] { toDotnetBoolean(spooler_process_result) };
		return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
				"spooler_process_after", paramTypes, params,
				spooler_process_result);
	}

}
