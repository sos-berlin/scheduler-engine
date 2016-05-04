package com.sos.scheduler.engine.taskserver.dotnet;

import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Optional;

import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleReference;
import net.sf.jni4net.Bridge;
import sos.spooler.Job;
import sos.spooler.Log;
import sos.spooler.Spooler;
import sos.spooler.Task;
import system.reflection.Assembly;

public class DotnetApiImpl {
	private final static String POWERSHELL_CLASS_NAME = "sos.spooler.PowershellAdapter";

	private system.Type apiImplType;
	private system.Object apiImplInstance = null;
	private DotnetBridge bridge;
	private DotnetModuleReference reference;

	public DotnetApiImpl(DotnetBridge dotnetBridge,	 DotnetModuleReference ref) {
		this.bridge = dotnetBridge;
		this.reference = ref;
	}

	public void init(Spooler spooler, Job spoolerJob, Task spoolerTask,
			Log spoolerLog) throws Exception {

		system.Type[] types = null;
		system.Object[] params = null;
		Path path = null;
		String className = null;
		String script = null;

		if (reference instanceof DotnetModuleReference.Powershell) {
			types = Arrays.copyOf(bridge.getSchedulerApiTypes(),bridge.getSchedulerApiTypes().length+1);
        	types[bridge.getSchedulerApiTypes().length] = system.Type.GetType("System.String");

			path = bridge.getDotnetAdapterDll();
			className = POWERSHELL_CLASS_NAME;
		    script = Optional.ofNullable(((DotnetModuleReference.Powershell)reference).script())
                    .orElseThrow(
                            () -> new Exception(String.format("Missing script")));


		}
		else if (reference instanceof DotnetModuleReference.DotnetClass) {
			types = bridge.getSchedulerApiTypes();

			path = ((DotnetModuleReference.DotnetClass)reference).dll();
		    className = Optional.ofNullable(((DotnetModuleReference.DotnetClass)reference).className())
                    .orElseThrow(
                            () -> new Exception(String.format("Missing className")));
		}

        if (!Files.exists(path, LinkOption.NOFOLLOW_LINKS)) {
			throw new Exception(String.format("File not found: %s",
					path.toString()));
		}

		Assembly assembly;
		try {
			assembly = Assembly.LoadFrom(path.toString());
		} catch (Exception ex) {
			throw new Exception(String.format("[%s] Can't load assembly: %s",
					path.toString(), ex.toString()));
		}

        Path tmpPath = path;
        String tmpClassName = className;
        apiImplType = Optional.ofNullable(assembly.GetType(className))
				.orElseThrow(
						() -> new Exception(String.format(
								"[%s] Class not found: %s", tmpPath.toString(),
                                tmpClassName)));

      	params = new system.Object[types.length];
		params[0] = Bridge.wrapJVM(spooler);
		params[1] = Bridge.wrapJVM(spoolerJob);
		params[2] = Bridge.wrapJVM(spoolerTask);
		params[3] = Bridge.wrapJVM(spoolerLog);
		if (reference instanceof DotnetModuleReference.Powershell){
			params[4] = new system.String(script);
		}

		apiImplInstance = Optional.ofNullable(
				DotnetInvoker.createInstance(apiImplType, types,
						params)).orElseThrow(
				() -> new Exception(String.format(
						"[%s] Could not create a new instance of the class %s",
						tmpPath.toString(), tmpClassName)));

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
