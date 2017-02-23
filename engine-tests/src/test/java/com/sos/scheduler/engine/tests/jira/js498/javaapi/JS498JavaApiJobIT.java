package com.sos.scheduler.engine.tests.jira.js498.javaapi;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.List;
import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with a JavaScript engine. The test starts different standalone jobs.
 */
public final class JS498JavaApiJobIT extends SchedulerTest {

    private static final ImmutableList<String> jobNames = ImmutableList.of(
            "script_only",
            "objects_noorder",
            "functions_noorder"
    );

    private final CommandBuilder util = new CommandBuilder();
    private ImmutableMap<String,String> resultMap;
    private int taskCount = 0;

    @Test
    public void test() throws IOException {
        //controller().activateScheduler("-e","-ignore-process-classes","-log-level=info","-log=" + logFile);
        controller().activateScheduler();
        File resultFile = prepareResultFile();
        for (String jobName : jobNames) {
            controller().scheduler().executeXml(util.startJobImmediately(jobName).getCommand());
        }
        assertThat(instance(SchedulerVariableSet.class).apply("scheduler_script"), equalTo("*(spooler_init)"));
        controller().waitForTermination();
        resultMap = getResultMap(resultFile);
        checkScriptOnlyJob();
        checkObjectsJob();
        checkFunctionsJob();
    }

    private File prepareResultFile() {
        String resultFileName = instance(SchedulerConfiguration.class).localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
        File resultFile = new File(resultFileName);
        resultFile.delete();
        return resultFile;
    }

    private ImmutableMap<String,String> getResultMap(File resultFile) throws IOException {
        ImmutableMap.Builder<String,String> result = ImmutableMap.builder();
        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
        for(String line : lines) {
            String[] arr = line.split("=", 2);
            if (arr.length != 2)
                throw new RuntimeException("Line in resultfile '" + resultFile + "' is not valid: " + line);
            result.put(arr[0], arr[1]);
        }
        return result.build();
    }

    @EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> e) {
        taskCount++;
        if (taskCount == jobNames.size())
            controller().terminateScheduler();
    }
    private void checkScriptOnlyJob() {
        assertObject("script_only", "script_only");
    }

    private void checkObjectsJob() {
        assertObject("spooler.variables.count", "3");
        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
        assertObject("spooler_job.name", "objects_noorder");
    }

    private void checkFunctionsJob() {
        assertObject("spooler_init", "1");
        assertObject("spooler_open", "1");
        assertObject("spooler_process", "1");
        assertObject("spooler_close", "1");
        assertObject("spooler_on_success", "1");
        assertObject("spooler_exit", "1");
        assertObject("spooler_task_before", "1");
        assertObject("spooler_task_after", "1");
        assertObject("spooler_process_before", "1");
        assertObject("spooler_process_after", "1");
    }

    /**
     * checks if an estimated object was given
     */
    private void assertObject(String name, String expected) {
        String value = resultMap.get(name);
        assertTrue(name + " is not set in scheduler variables", value != null);
        assertTrue(name +"="+ value + " is not valid - " + expected + " expected", value.equals(expected));
    }
}
