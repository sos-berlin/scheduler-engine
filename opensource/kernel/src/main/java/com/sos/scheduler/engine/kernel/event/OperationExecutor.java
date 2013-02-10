package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.inject.Inject;
import javax.inject.Singleton;
import java.util.LinkedList;
import java.util.Queue;

@Singleton
public final class OperationExecutor implements OperationQueue {
    private static final Logger logger = LoggerFactory.getLogger(OperationExecutor.class);

    private final Queue<Runnable> operations = new LinkedList<Runnable>();
    private final PrefixLog log;
    private boolean isEmpty = true;

    @Inject private OperationExecutor(PrefixLog log) {
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
            assert operations.isEmpty();
        }
    }

    private void executeRunnable(Runnable c) {
        try {
            logger.debug("{}", c);
            c.run();
        } catch (Exception x) {
            log.error(x.toString());
            logger.error("{}", x);
        }
    }
}
