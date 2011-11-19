package com.sos.scheduler.engine.kernel.event;

import java.util.LinkedList;
import java.util.Queue;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.kernel.log.PrefixLog;

public final class OperationExecutor implements OperationQueue {
    private static final Logger logger = Logger.getLogger(OperationExecutor.class);

    private final Queue<Runnable> operations = new LinkedList<Runnable>();
    private final PrefixLog log;
    private boolean isEmpty = true;

    public OperationExecutor(PrefixLog log) {
        this.log = log;     // TODO Eigenes PrefixLog
    }

    @Override public void add(Runnable o) {
        operations.add(o);
        isEmpty = false;
    }

    public void execute() {
        if (!isEmpty) {
            while (true) {
                Runnable o = operations.poll();
                if (o == null) break;
                executeRunnable(o);
            }
            isEmpty = true;
            assert(operations.isEmpty());
        }
    }

    private void executeRunnable(Runnable c) {
        try {
            logger.debug(c);
            c.run();
        } catch (Exception x) {
            log.error(x.toString());
            logger.error(x, x);
        }
    }
}
