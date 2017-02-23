package com.sos.scheduler.engine.tests.jira.js498.scriptapi;

import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.job.TaskEnded;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Test;
import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with a JavaScript engine. The test starts different standalone jobs.
 */
public final class JS498JobIT extends SchedulerTest {

    private static final ImmutableList<String> jobs = ImmutableList.of(
            "script_only",
            "objects_noorder",
            "functions_noorder"
    );

    private final CommandBuilder util = new CommandBuilder();
    private Map<String,String> resultMap;
    private int taskCount = 0;

    @Test
    public void test() throws IOException {
        // if (Bitness.is32Bit()) {    // SpiderMonkey stellen wir nur für 32bit bereit
            controller().activateScheduler();
            File resultFile = prepareResultFile();
            for (String jobName : jobs) {
                controller().scheduler().executeXml(util.startJobImmediately(jobName).getCommand());
            }
            controller().waitForTermination();
            resultMap = getResultMap(resultFile);
            checkScriptOnlyJob();
            checkObjectsJob();
            checkFunctionsJob();
        // }
    }

    private File prepareResultFile() {
        String resultFileName = instance(SchedulerConfiguration.class).localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
        File resultFile = new File(resultFileName);
        resultFile.delete();
        return resultFile;
    }

    private Map<String,String> getResultMap(File resultFile) throws IOException {
        Map<String,String> result = new HashMap<>();
        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
        for(String line : lines) {
            String[] arr = line.split("=", 2);
            if (arr.length != 2)
                throw new RuntimeException("Line in resultfile '" + resultFile + "' is not valid: " + line);
            result.put(arr[0], arr[1]);
        }
        return result;
    }

    @EventHandler
    public void handleEvent(KeyedEvent<TaskEnded> e) {
        taskCount++;
        if (taskCount == jobs.size())
            controller().terminateScheduler();
    }

    private void checkScriptOnlyJob() {
        assertObject("script_only", "script_only");
    }

    private void checkObjectsJob() {
        assertObject("spooler.variables.count", "2");
        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
        assertObject("spooler_job.name", "objects_noorder");
    }

    private void checkFunctionsJob() {
        assertObject("spooler_init","1");
        assertObject("spooler_open","1");
        assertObject("spooler_process","1");
        assertObject("spooler_close","1");
        assertObject("spooler_on_success","1");
        assertObject("spooler_exit","1");
        assertObject("spooler_task_before","1");
        assertObject("spooler_task_after","1");
        assertObject("spooler_process_before","1");
        assertObject("spooler_process_after","1");
    }

    /**
     * checks if an estimated object was given
     */
    private void assertObject(String varname, String expected) {
        String value = resultMap.get(varname);
        assertTrue(varname + " is not set in scheduler variables", value != null);
        assertTrue(value + " is not valid - " + expected + " expected", value.equals(expected));
    }
}
