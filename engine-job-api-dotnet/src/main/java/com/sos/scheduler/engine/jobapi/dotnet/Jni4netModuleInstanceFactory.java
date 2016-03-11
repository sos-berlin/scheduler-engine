package com.sos.scheduler.engine.jobapi.dotnet;

import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleInstanceFactory;
import com.sos.scheduler.engine.jobapi.dotnet.api.DotnetModuleReference;
import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;
import java.nio.file.Path;
import sos.spooler.Job_impl;
import sos.spooler.Monitor_impl;

/**
 * @author Joacim Zschimmer
 */
public final class Jni4netModuleInstanceFactory implements DotnetModuleInstanceFactory {
    private final Path jni4netDirectory;

    public Jni4netModuleInstanceFactory(Path dllDirectory) {
        this.jni4netDirectory = dllDirectory;
        // TODO Hier jni4net usw. initialisieren
    }

    public void close() {
        // TODO Wenn jni4net oder .Net entladen werden kann, dann wollen wir das vielleicht tun.
    }

    public <T> T newInstance(Class<T> clazz, TaskContext taskContext, DotnetModuleReference reference) {
        if (reference instanceof DotnetModuleReference.DotnetClass) {
            DotnetModuleReference.DotnetClass ref = (DotnetModuleReference.DotnetClass)reference;
            Object myDotnetObject = newDotnetObject(clazz, ref);
            return newSchedulerDotnetAdapter(clazz, myDotnetObject, taskContext);
        } else
        if (reference instanceof DotnetModuleReference.Powershell) {
            DotnetModuleReference.Powershell ref = (DotnetModuleReference.Powershell)reference;
            Object myDotnetObject = newPowershellObject(clazz, ref);
            return newSchedulerDotnetAdapter(clazz, myDotnetObject, taskContext);
        } else
            throw new RuntimeException("Unknown class " + reference.getClass().getName());
    }

    private <T> Object newDotnetObject(Class<T> clazz, DotnetModuleReference.DotnetClass ref) {
        Path dll = ref.dll();
        String s = ref.className();
        return null; //Bridge.newDotnetObject(ref.dll(), ref.className());
    }

    private <T> Object newPowershellObject(Class<T> clazz, DotnetModuleReference.Powershell ref) {
        String script = ref.script();
        //loadPowershellScript(script);
        return null; //Bridge.newDotnetObject(...)
    }

    private <T> T newSchedulerDotnetAdapter(Class<T> clazz, Object dotnetObject, TaskContext taskContext) {
        if (Job_impl.class.isAssignableFrom(clazz)) {
            @SuppressWarnings("unchecked")
            T result = (T)new DotnetJob(taskContext, dotnetObject);
            return result;
        }
        else
        if (Monitor_impl.class.isAssignableFrom(clazz)) {
            @SuppressWarnings("unchecked")
            T result = (T)new DotnetMonitor(taskContext, dotnetObject);
            return result;
        } else
            throw new IllegalArgumentException("Unsupported " + clazz);
    }

    private <T> void closeInstance(T instance) {
        // TODO Close DotnetJob or DotnetMonitor
    }
}
