package com.sos.scheduler.engine.tests.jira.js1026;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;

import java.io.IOException;

import static org.junit.Assert.assertTrue;

/**
 * Test for SCHEDULER_RETURN_VALUES in standalone shell jobs.
 * The standalone jobs test1 and test2 will be started.
 * job test1 starts job test2 with command start_job.
 *
 * @author Florian Schreiber
 */
public final class JS1026ShellJobIT extends SchedulerTest {

    private static final ImmutableList<String> jobNames = ImmutableList.of("test1");

    private final CommandBuilder util = new CommandBuilder();
    private int taskCount = 0;

    @Test
    public void test() {
        controller().activateScheduler();
        for (String jobName : jobNames) {
            controller().scheduler().executeXml(util.startJobImmediately(jobName).getCommand());
        }
        controller().waitForTermination();
    }

    @HotEventHandler
    public void handleTaskEnded(TaskEnded e, UnmodifiableTask t) throws IOException {
        taskCount++;
        // Job test2 is started by command start_job of job test1
        if (taskCount == jobNames.size() + 1) {
            assertObject(t, "testvar1", "value1");
            assertObject(t, "testvar2", "newvalue2");
            assertObject(t, "testvar3", "value3");
            assertObject(t, "testvar4", "newvalue4");
            assertObject(t, "testvar5", "value5");
            controller().terminateScheduler();
        }
    }

    /**
     * checks if an estimated object was given
     */
    private static void assertObject(UnmodifiableTask t, String name, String expected) {
        String value = t.parameterValue(name);
        assertTrue(name + " is not set in scheduler variables", !value.equals(""));
        assertTrue(name +"="+ value + " is not valid - " + expected + " expected", value.equals(expected));
    }
}
