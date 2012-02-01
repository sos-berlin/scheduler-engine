package com.sos.scheduler.engine.kernel.job;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.TaskC;
import com.sos.scheduler.engine.kernel.folder.FileBased;

public final class Task extends FileBased implements UnmodifiableTask {
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

    public Job getJob() {
        return cppProxy.job().getSister();
    }

    @Override public String toString() {
        return Task.class.getSimpleName();
    }

	@Override
	@Nullable
	public UnmodifiableOrder getOrderOrNull() {
		return cppProxy.order().getSister();
	}

	@Override
	public TaskId getId() {
		return new TaskId( cppProxy.id() );
	}
}
