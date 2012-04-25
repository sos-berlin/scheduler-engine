package com.sos.scheduler.engine.tests.jira.js644.stop;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.sos.scheduler.engine.data.folder.FileBasedType;
import com.sos.scheduler.engine.data.folder.TypedPath;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

import static com.sos.scheduler.engine.tests.jira.js644.stop.JS644StoppedJobTest.M.*;

/** Testet ob ein gestoppter und veränderter Job wieder korrekt in die Jobkette eingehängt wird und erneut anläuft. */
public final class JS644StoppedJobTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS644StoppedJobTest.class);
    private static final TypedPath jobPath = FileBasedType.job.typedPath("/a");
    enum M { taskEnded, orderStateChanged, terminated }
    private final Gate<M> threadGate = new Gate<M>();

    @Test public void test() throws Exception {
        controller().setTerminateOnError(false);   // Wegen SCHEDULER-280  Process terminated with exit code 1 (0x1)
        controller().startScheduler();
        threadGate.expect(taskEnded, shortTimeout);
        modifyFile(controller().environment().fileFromPath(jobPath));
        expectEvents();
        controller().terminateScheduler();
    }

    private void expectEvents() throws Exception {
        try {
            threadGate.expect(taskEnded, shortTimeout);
        } catch(Exception x) {
            logger.warn(scheduler().executeXml("<job.why job='"+ jobPath.getPath() +"'/>"));
            throw x;
        }
    }

    private void modifyFile(File f) {
        try {
            assert f.exists() : "Datei fehlt: " + f;
            Files.append(" ", f, Charsets.UTF_8);
        } catch (IOException x) { throw new RuntimeException(x); }
    }

    @EventHandler public void handleEvent(TaskEndedEvent e) throws InterruptedException {
        threadGate.put(taskEnded);
    }

    @EventHandler public void handleEvent(OrderStateChangedEvent e) throws InterruptedException {
        threadGate.put(orderStateChanged);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        threadGate.put(terminated);
    }
}
