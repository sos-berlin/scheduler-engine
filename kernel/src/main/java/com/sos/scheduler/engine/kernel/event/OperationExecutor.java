package com.sos.scheduler.engine.kernel.event;

import java.util.LinkedList;
import java.util.Queue;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.log.PrefixLog;

public final class OperationExecutor implements OperationCollector {
    private static final Logger logger = Logger.getLogger(OperationExecutor.class);

    private final Queue<SimpleSchedulerOperation> operations = new LinkedList<SimpleSchedulerOperation>();
    private final PrefixLog log;
    private boolean isEmpty = true;

    public OperationExecutor(PrefixLog log) {
        this.log = log;     // TODO Eigenes PrefixLog
    }

    @Override public void addOperation(SimpleSchedulerOperation c) {
        operations.add(c);
        isEmpty = false;
    }

    public void execute() {
        if (!isEmpty) {
            while (true) {
                SimpleSchedulerOperation o = operations.poll();
                if (o == null) break;
                executeOperation(o);
            }
            isEmpty = true;
            assert(operations.isEmpty());
        }
    }

    private void executeOperation(SimpleSchedulerOperation c) {
        try {
            logger.debug(c);
            c.execute();
        } catch (Exception x) {
            log.error(x.toString());
            logger.error(x, x);
        }
    }
}
