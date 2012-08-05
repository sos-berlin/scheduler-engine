package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.data.job.TaskId;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.cppproxy.TaskC;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

import javax.annotation.Nullable;

public final class Task implements UnmodifiableTask, Sister, EventSource {
    private final TaskC cppProxy;

    protected Task(TaskC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {}

    public static class Type implements SisterType<Task, TaskC> {
        @Override public final Task sister(TaskC proxy, Sister context) {
            return new Task(proxy);
        }
    }

    @Override public Job getJob() {
        return cppProxy.job().getSister();
    }

    @Override public String toString() {
        return Task.class.getSimpleName();
    }

	@Override @Nullable public UnmodifiableOrder getOrderOrNull() {
		return cppProxy.order().getSister();
	}

	@Override public TaskId getId() {
		return new TaskId( cppProxy.id() );
	}
}
