package com.sos.scheduler.engine.tests.jira.js1090;

import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderFinished;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.File;
import org.junit.Test;
import static com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

public final class JS1090IT extends SchedulerTest {

    private static final JobChainPath jobChainPath = new JobChainPath("/myJobChain");
    private static final JobPath jobPath = new JobPath("/a");
    private static final String echoText = "+++ hello world +++";
    private static final String includeContent = "echo " + echoText;

    private final CommandBuilder cmd = new CommandBuilder();
    private String logFileContent = null;

    @Test
    public void test() throws Exception {
        controller().prepare();
        File includeFile = new File(controller().environment().liveDirectory(), "include.txt");
        Files.write(includeContent, includeFile, schedulerEncoding);
        controller().activateScheduler();
        String command = cmd.addOrder(jobChainPath.string(), "myId").getCommand();
        scheduler().executeXml(command);
        controller().waitForTermination();
        assertNotNull(logFileContent);
        assertTrue(logFileContent.contains(echoText));
    }

    @EventHandler
    public void handleEvent(KeyedEvent<OrderFinished> g) {
        controller().terminateScheduler();
        logFileContent = controller().environment().taskLogFileString(jobPath);
    }
}
