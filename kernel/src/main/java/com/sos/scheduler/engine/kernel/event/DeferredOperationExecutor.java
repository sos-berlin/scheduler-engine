package com.sos.scheduler.engine.kernel.event;

import java.util.LinkedList;
import java.util.Queue;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.event.SchedulerEntersSleepStateEvent;

public class DeferredOperationExecutor {
    private static final Logger logger = Logger.getLogger(DeferredOperationExecutor.class);

    private final Queue<SchedulerOperation> operations = new LinkedList<SchedulerOperation>();
    private final PrefixLog log;

    public DeferredOperationExecutor(EventSubsystem eventSubsystem) {
        log = eventSubsystem.log();
        eventSubsystem.subscribe(new MyEventSubscriber());
    }

    public void addOperation(SchedulerOperation c) {
        operations.add(c);
    }

    private void executeOperations() {
        while (true) {
            SchedulerOperation o = operations.poll();
            if (o == null) break;
            executeOperation(o);
        }
    }

    private void executeOperation(SchedulerOperation c) {
        try {
            logger.debug(c);
            c.execute();
        } catch (Exception x) {
            log.error(x.toString());
            logger.error(x, x);
        }
    }

    private class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event e) throws Exception {
            if (e instanceof SchedulerIsCallableEvent && e instanceof SchedulerEntersSleepStateEvent) {
                executeOperations();
            }
        }
    }
}
