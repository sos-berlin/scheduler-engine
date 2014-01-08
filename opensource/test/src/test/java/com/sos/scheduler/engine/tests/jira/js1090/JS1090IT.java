package com.sos.scheduler.engine.tests.jira.js1090;

import com.google.common.io.Files;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Ignore;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.Arrays;

import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertNotNull;

public final class JS1090IT extends SchedulerTest {

    private static final String jobChain = "myJobChain";
    private static final String job = "a";
    private static final String includeContent = "echo *** hello world ***";

    private CommandBuilder cmd = new CommandBuilder();
    private String logFileContent = null;

    // läuft derzeit auf Fehler, da JS-1090 noch nicht korrigiert
    @Ignore
    public void test() throws Exception {
        controller().prepare();
        File include = new File(controller().environment().configDirectory(),"include.txt");
        Files.append(includeContent, include, Charset.forName("UTF-8"));
        controller().activateScheduler(Arrays.asList("-e") );
        String command = cmd.addOrder(jobChain,"myId").getCommand();
        scheduler().executeXml(command);
        controller().waitForTermination(shortTimeout);
        assertNotNull(logFileContent);
        assertTrue(logFileContent.contains(includeContent));
    }

    @EventHandler
    public void handleEvent(OrderFinishedEvent e) throws IOException {
        scheduler().terminate();
        File logFile = new File(controller().environment().logDirectory(),"task." + job + ".log");
        logFileContent = Files.toString(logFile,Charset.forName("UTF-8"));
    }

}
