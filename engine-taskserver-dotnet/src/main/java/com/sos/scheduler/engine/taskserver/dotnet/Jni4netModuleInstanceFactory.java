package com.sos.scheduler.engine.taskserver.dotnet;

import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleInstanceFactory;
import com.sos.scheduler.engine.taskserver.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.taskserver.dotnet.api.TaskContext;
import java.nio.file.Path;
import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;

public final class Jni4netModuleInstanceFactory implements
        DotnetModuleInstanceFactory {

    private DotnetBridge dotnetBridge = null;
    private final Path dllDirectory;

    public Jni4netModuleInstanceFactory(Path dllDirectory) throws Exception {
        this.dllDirectory = dllDirectory;
    }

    private synchronized void initialize() throws Exception {
        if (dotnetBridge == null) {
            dotnetBridge = new DotnetBridge();
            boolean debug = System.getProperty("jni4net.debug") != null;
            dotnetBridge.init(dllDirectory, debug);
        }
    }

    public void close() {
    }

    public <T> T newInstance(Class<T> clazz, TaskContext taskContext, DotnetModuleReference reference) throws Exception {
        initialize();
        return newSchedulerDotnetAdapter(clazz, reference, taskContext);
    }

    @SuppressWarnings("unchecked")
    private <T> T newSchedulerDotnetAdapter(Class<T> clazz,
            DotnetModuleReference reference, TaskContext taskContext)
            throws Exception {
        DotnetApiImpl dotnetObject = new DotnetApiImpl(dotnetBridge, reference, taskContext);

        if (Job_impl.class.isAssignableFrom(clazz)) {
            return (T)new DotnetJob(taskContext, dotnetObject);
        } else if (Monitor_impl.class.isAssignableFrom(clazz)) {
            return (T)new DotnetMonitor(dotnetObject);
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
