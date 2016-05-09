package com.sos.scheduler.engine.taskserver.dotnet;

import com.sos.scheduler.engine.taskserver.dotnet.dlls.DotnetDlls$;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import net.sf.jni4net.Bridge;
import system.reflection.Assembly;

public class DotnetBridge {
	private system.Type[] schedulerApiTypes = null;
	private Path dotnetAdapterDll = null;
	private List<DotnetApiImpl> dotnetApiImpls = new ArrayList<>();

	public void init(Path path, boolean debug) throws Exception {
		if (!Files.exists(path, LinkOption.NOFOLLOW_LINKS)) {
			throw new Exception(String.format(
					"Can't initialize jni4net Bridge. Directory not found: %s",
					path.toString()));
		}
 		initJni4NetBridge(path, debug);
		initJni4JobSchedulerApi(path);
	}

	private static void initJni4NetBridge(Path path, boolean debug)
			throws Exception {
		Bridge.setDebug(debug);
		Bridge.setVerbose(debug);
		Bridge.init(path.toFile());
	}

	private void initJni4JobSchedulerApi(Path path) throws Exception {
		dotnetAdapterDll = path.resolve(DotnetDlls$.MODULE$.DllName());

		Assembly apiProxyAssembly = Assembly.LoadFrom(dotnetAdapterDll.toString());
		Bridge.RegisterAssembly(apiProxyAssembly);
		Bridge.LoadAndRegisterAssemblyFrom(this.dotnetAdapterDll.toFile());

		schedulerApiTypes = new system.Type[4];
		schedulerApiTypes[0] = apiProxyAssembly.GetType("sos.spooler.Spooler");
		schedulerApiTypes[1] = apiProxyAssembly.GetType("sos.spooler.Job");
		schedulerApiTypes[2] = apiProxyAssembly.GetType("sos.spooler.Task");
		schedulerApiTypes[3] = apiProxyAssembly.GetType("sos.spooler.Log");

		if (Arrays.asList(schedulerApiTypes).contains(null)) {
			throw new Exception(
					String.format(
							"[%s] Not found one or more JobScheduler job api types in the assembly [%s]",
							apiProxyAssembly.getLocation(), apiProxyAssembly));
		}
	}

	public void close(){
		for(DotnetApiImpl apiImpl : dotnetApiImpls){
            try {
                apiImpl.close();
            }
            catch(Exception ex){
            }
 		}
	}

	public system.Type[] getSchedulerApiTypes() { return schedulerApiTypes; }

	public Path getDotnetAdapterDll() {
		return dotnetAdapterDll;
	}

	public void addDotnetApiImpl(DotnetApiImpl apiImpl) {
		dotnetApiImpls.add(apiImpl);
	}

}
