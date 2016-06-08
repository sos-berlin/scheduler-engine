package com.sos.scheduler.engine.taskserver.dotnet;

import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.taskserver.dotnet.api.TaskContext;
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
    private static final String POWERSHELL_CLASS_NAME = "sos.spooler.PowershellAdapter";

    private system.Type apiImplType;
    private system.Object apiImplInstance;
    private DotnetBridge bridge;
    private DotnetModuleReference reference;
    private Path path;
    private String className;

    public DotnetApiImpl(DotnetBridge dotnetBridge, DotnetModuleReference ref) {
        bridge = dotnetBridge;
        reference = ref;
    }

    public void init(Spooler spooler, Job spoolerJob, Task spoolerTask, Log spoolerLog) {
        setPropertiesFromReference();
        setApiImplType();
        initApiImplInstance(spooler, spoolerJob, spoolerTask, spoolerLog);
        bridge.addDotnetApiImpl(this);
    }

    private void setPropertiesFromReference() {
        if (reference instanceof DotnetModuleReference.Powershell) {
            path = bridge.getDotnetAdapterDll();
            className = POWERSHELL_CLASS_NAME;
        } else if (reference instanceof DotnetModuleReference.DotnetClass) {
            path = ((DotnetModuleReference.DotnetClass)reference).dll();
            className = ((DotnetModuleReference.DotnetClass)reference).className();
        } else
            throw new IllegalArgumentException();

        if (!Files.exists(path, LinkOption.NOFOLLOW_LINKS)) {
            throw new RuntimeException(String.format("File not found: %s",
                    path.toString()));
        }
    }

    private void setApiImplType() {

        Assembly assembly;
        try {
            assembly = Assembly.LoadFrom(path.toString());
        } catch (Exception ex) {
            throw new RuntimeException(String.format("[%s] Can't load assembly: %s",
                    path.toString(), ex.toString()));
        }

        apiImplType = Optional.ofNullable(assembly.GetType(className))
                .orElseThrow(
                        () -> new RuntimeException(String.format(
                                "[%s] Class not found: %s", path.toString(),
                                className)));
    }

    private void initApiImplInstance(Spooler spooler, Job spoolerJob,
            Task spoolerTask, Log spoolerLog) {

        if (reference instanceof DotnetModuleReference.Powershell) {
            system.Type[] types = {
                    bridge.getSchedulerApiTypes()[0],
                    bridge.getSchedulerApiTypes()[1],
                    bridge.getSchedulerApiTypes()[2],
                    bridge.getSchedulerApiTypes()[3],
                    system.Type.GetType("System.String")
                    };

            system.Object[] params = {
                    Bridge.wrapJVM(spooler),
                    Bridge.wrapJVM(spoolerJob),
                    Bridge.wrapJVM(spoolerTask),
                    Bridge.wrapJVM(spoolerLog),
                    new system.String(((DotnetModuleReference.Powershell)reference).script())
                    };

            apiImplInstance = Optional
                    .ofNullable(DotnetInvoker.createInstance(apiImplType, types,params))
                    .orElseThrow(
                            () -> new RuntimeException(
                                    String.format("[%s] Could not create a new instance of the class %s",
                                            path.toString(), className)));

        } else if (reference instanceof DotnetModuleReference.DotnetClass) {
            system.Type[] types = new system.Type[]{};
            system.Object[] params = new system.Object[] {};

            apiImplInstance = Optional
                    .ofNullable(DotnetInvoker.createInstance(apiImplType,types,params))
                    .orElseThrow(
                            () -> new RuntimeException(
                                    String.format(
                                            "[%s] Could not create a new instance of the class %s",
                                            path.toString(), className)));

            setApiImplInstanceProperty("spooler",spooler);
            setApiImplInstanceProperty("spooler_job",spoolerJob);
            setApiImplInstanceProperty("spooler_task",spoolerTask);
            setApiImplInstanceProperty("spooler_log",spoolerLog);
        }
    }

    private void setApiImplInstanceProperty(String name,java.lang.Object value){
        apiImplType.GetProperty(name).SetValue(apiImplInstance,	Bridge.wrapJVM(value),null);
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
        system.Type[] paramTypes = new system.Type[] { system.Type.GetType("System.Boolean") };
        system.Object[] params = new system.Object[] { toDotnetBoolean(spooler_process_result) };
        return DotnetInvoker.invokeMethod(apiImplType, apiImplInstance,
                "spooler_process_after", paramTypes, params,spooler_process_result);
    }

    public void close() throws Exception {
        if (reference instanceof DotnetModuleReference.Powershell) {
            if (apiImplType != null && apiImplInstance != null) {
                DotnetInvoker.invokeMethod(apiImplType, apiImplInstance, "Close");
            }
        }
    }
}
