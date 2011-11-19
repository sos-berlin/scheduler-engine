package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.TaskC;
import com.sos.scheduler.engine.kernel.folder.FileBased;

public class Task extends FileBased implements UnmodifiableTask {
    private final TaskC cppProxy;

    protected Task(Platform p, TaskC cppProxy) {
        super(p);
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {}

    public static class Type implements SisterType<Task, TaskC> {
        @Override public final Task sister(TaskC proxy, Sister context) {
            return new Task(Platform.of(context), proxy);
        }
    }

    @Override public String toString() {
        return Task.class.getSimpleName();
    }
}
