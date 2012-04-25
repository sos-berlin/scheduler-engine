package com.sos.scheduler.engine.tests.jira.js644.v3;

import com.google.common.io.Files;
import com.sos.scheduler.engine.data.folder.TypedPath;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;

import static com.google.common.base.Charsets.UTF_8;
import static com.sos.scheduler.engine.data.folder.FileBasedType.job;
import static com.sos.scheduler.engine.data.folder.FileBasedType.jobChain;
import static java.lang.Thread.sleep;
import static org.junit.Assert.fail;

public final class JS644v3Test extends SchedulerTest {
    private static final Logger logger = LoggerFactory.getLogger(JS644v3Test.class);
    private static final TypedPath lowerCaseJobChainPath = jobChain.typedPath("/lowerCase");
    private static final TypedPath upperCaseJobChainPath = jobChain.typedPath("/upperCase");
    private static final TypedPath jobPath = job.typedPath("/a");
    private static final Time orderTimeout = Time.of(10);

    private final Gate<Boolean> lowerCaseGate = new Gate<Boolean>(lowerCaseJobChainPath.toString());
    private final Gate<Boolean> upperCaseGate = new Gate<Boolean>(upperCaseJobChainPath.toString());

    @Test public void test() throws Exception {
        try {
            controller().activateScheduler();
            prepare();
        } catch (Throwable t) {
            throw new Error("VOM TEST UNBEABSICHTIGTER FEHLER: "+t, t);
        }
        runOrders();
    }

    private void prepare() throws Exception {
        runOrders();     // zwei Mal nur zum Beleg, dass runOrders() funktioniert
        runOrders();
        modify(lowerCaseJobChainPath);
        modify(upperCaseJobChainPath);
        sleep(100);
        modify(jobPath);
        sleep(4000);
    }

    private void modify(TypedPath p) throws IOException {
        File file = controller().environment().fileFromPath(p);
        String text = Files.toString(file, UTF_8);
        Files.write(text +" ", file, UTF_8);
    }

    private void runOrders() throws InterruptedException {
        scheduler().executeXml("<modify_order job_chain='"+ lowerCaseJobChainPath.getPath() +"' order='1' at='now'/>");
        scheduler().executeXml("<modify_order job_chain='"+ upperCaseJobChainPath.getPath() +"' order='1' at='now'/>");
        waitForFinishedOrder(lowerCaseGate);
        waitForFinishedOrder(upperCaseGate);
    }

    private static boolean waitForFinishedOrder(Gate<Boolean> gate) throws InterruptedException {
        Boolean ok = gate.poll(orderTimeout);
        if (ok == null) fail("An order has not been finished in time");
        if (ok) logger.debug("An order has been finished");
        return ok;
    }

    @EventHandler public void handleEvent(OrderFinishedEvent e) throws InterruptedException {
        (e.getKey().getJobChainPath().equals(lowerCaseJobChainPath.getPath())? lowerCaseGate : upperCaseGate).put(true);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        lowerCaseGate.put(false);
    }
}
