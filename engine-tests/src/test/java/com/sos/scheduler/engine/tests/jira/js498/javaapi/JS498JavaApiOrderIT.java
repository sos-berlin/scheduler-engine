package com.sos.scheduler.engine.tests.jira.js498.javaapi;

import com.google.common.io.Files;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.order.OrderFinished;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
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
 * This is a test for scripting with a JavaScript engine. The jobchain started executes
 * <ul>
 *     <li>a job with all knowing API methods (including the monitors).</li>
 *     <li>a job with a range of api calls for the JobScheduler api.</li>
 * </ul>
 *
 * At the end of the test the resultfile will checked for the estimated functions calls.
 *
 * @author Stefan Schädlich
 * @version 1.0 - 16.12.2011 13:39:41
 */
public class JS498JavaApiOrderIT extends SchedulerTest {

    private static final String jobchain = "chain";

    private final CommandBuilder util = new CommandBuilder();
    private Map<String,String> resultMap;

    @Test
    public void test() throws IOException {
        // controller().activateScheduler(Arrays.asList("-e","-log-level=info"));
        controller().activateScheduler();
        File resultFile = prepareResultFile();
        controller().scheduler().executeXml(util.addOrder(jobchain).getCommand());
        controller().waitForTermination();
        resultMap = getResultMap(resultFile);
        checkScriptOnlyJob();
        checkJobObjects();
        checkJobFunctions();
    }

    private File prepareResultFile() {
        String resultFileName = instance(SchedulerConfiguration.class).localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
        File resultFile = new File(resultFileName);
        resultFile.delete();
        return resultFile;
    }

    private Map<String,String> getResultMap(File resultFile) throws IOException {
        Map<String,String> result = new HashMap<String, String>();
        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
        for(String line : lines) {
            String[] arr = line.split("=");
            if (arr.length != 2)
                throw new SchedulerException("line in resultfile '" + resultFile + "' is not valid: " + line);
            result.put(arr[0],arr[1]);
        }
        return result;
    }

    @EventHandler
    public void handleOrderFinished(KeyedEvent<OrderFinished> g)  {
        controller().terminateScheduler();
    }

    private void checkScriptOnlyJob() {
        assertObject("script_only", "script_only_order");
    }

    private void checkJobObjects() {
        assertObject("spooler.variables.count", "3");
        assertObject("spooler_task.order.job_chain.name", "chain");
        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
        assertObject("spooler_job.order_queue.length", "1");
        assertObject("spooler_task.order.id", "chain");
    }

    private void checkJobFunctions() {
        assertFunction("spooler_init");
        assertFunction("spooler_open");
        assertFunction("spooler_process");
        assertFunction("spooler_close");
        assertFunction("spooler_on_success");
        assertFunction("spooler_exit");
        assertFunction("spooler_task_before");
        assertFunction("spooler_task_after");
        assertFunction("spooler_process_before");
        assertFunction("spooler_process_after");
    }

    /**
     * checks if an estimated funtion was called.
     * @param function
     */
    private void assertFunction(String function) {
        assertObject(function,"1");		// any funtion should be called exactly one time
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
