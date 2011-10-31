package com.sos.scheduler.engine.kernel.event;

import java.util.LinkedList;
import java.util.Queue;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.event.SchedulerEntersSleepStateEvent;

//TODO SchedulerEntersSleepStateEvent ausbauen und durch JavaSubsystem-Methode ersezten
public final class DeferredOperationExecutor {
    private static final Logger logger = Logger.getLogger(DeferredOperationExecutor.class);

    private final Queue<SchedulerOperation> operations = new LinkedList<SchedulerOperation>();
    private final PrefixLog log;
    private boolean isEmpty = true;

    public DeferredOperationExecutor(PrefixLog log) {
        this.log = log;     // TODO Eigenes PrefixLog
    }

    public void addOperation(SchedulerOperation c) {
        operations.add(c);
        isEmpty = false;
    }

    private void executeOperations() {
        while (true) {
            SchedulerOperation o = operations.poll();
            if (o == null) break;
            executeOperation(o);
        }
        isEmpty = true;
        assert(operations.isEmpty());
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

    @EventHandler public void handleEvent(SchedulerIsCallableEvent e) throws Exception {
        if (!isEmpty) {
            if (e instanceof SchedulerEntersSleepStateEvent) {
                executeOperations();
            }
        }
    }
}
