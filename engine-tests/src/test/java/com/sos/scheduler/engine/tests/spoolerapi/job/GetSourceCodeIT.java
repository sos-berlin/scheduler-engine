package com.sos.scheduler.engine.tests.spoolerapi.job;

import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.data.job.TaskKey;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;
import static com.google.common.base.Charsets.UTF_8;
import static org.junit.Assert.assertEquals;

/**
 * see JS-898, JS-1199
 */
public final class GetSourceCodeIT extends SchedulerTest {

    private static final ImmutableList<String> jobs = ImmutableList.of("javascript_intern", "javascript_include");
    private static final String expectedFilename = "expected-content.xml";

    private int taskCount = 0;
    private final Map<String,String> resultMap = new HashMap<>();

    @Test
    public void test() throws IOException {
        CommandBuilder cmd = new CommandBuilder();
        controller().prepare();
        String expectedCode = getExpectedSourceCode();
        controller().activateScheduler();
        for (String jobName : jobs) {
            controller().scheduler().executeXml(cmd.startJobImmediately(jobName).getCommand());
        }
        controller().waitForTermination();
        for (String jobName : jobs) {
            String scriptCode = resultMap.get(jobName);
            assertEquals("<include> in job "+ jobName + " is not as expected:", expectedCode, scriptCode);
        }
    }

    private String getExpectedSourceCode() throws IOException {
        File f = new File(controller().environment().liveDirectory(), expectedFilename);
        return Files.toString(f, UTF_8).trim();
    }

    @EventHandler
    public void handleTaskEnded(KeyedEvent<TaskEnded> e) {
        TaskKey taskKey = (TaskKey)e.key();
        String jobName = taskKey.jobPath().name();
        String scriptCode = instance(SchedulerVariableSet.class).apply(jobName).trim().replace("\r\n", "\n");
        resultMap.put(jobName,scriptCode);
        taskCount++;
        if (taskCount == jobs.size())
            controller().terminateScheduler();
    }
}
