package com.sos.scheduler.engine.tests.jira.js1026;

import com.google.common.collect.ImmutableMap;
import com.google.common.io.Files;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static java.util.Arrays.asList;
import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with the Rhino engine. The jobchain started executes
 * <ul>
 *     <li>a job with all knowing API methods (including the monitors).</li>
 *     <li>a job with a range of api calls for the JobScheduler api.</li>
 * </ul>
 *
 * At the end of the test the resultfile will checked for the estimated functions calls.
 *
 * @author Florian Schreiber
 * @version 1.0 - 24.09.2014 13:39:41
 */
public class JS1026ShellOrderIT extends SchedulerTest {

//    private static final Logger logger = LoggerFactory.getLogger(JS1026ShellOrderIT.class);
    private static final String jobchain = "chain";
    private final CommandBuilder util = new CommandBuilder();
//    private Map<String,String> resultMap;

    @Test
    public void test() throws IOException {
//        controller().activateScheduler(asList("-log-dir=" + logDir.getAbsolutePath(), "-log=" + logFile.getAbsolutePath()));
        controller().activateScheduler( asList("-e", "-log-level=debug9") );
//        controller().activateScheduler();
//        File resultFile = prepareResultFile();
        controller().scheduler().executeXml(util.addOrder(jobchain).getCommand());
        controller().waitForTermination(shortTimeout);

//        resultMap = getResultMap(resultFile);
//        checkJobObjects();
//        checkJobFunctions();
    }

//    private File prepareResultFile() {
//        String resultFileName = instance(SchedulerConfiguration.class).localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
//        File resultFile = new File(resultFileName);
//        resultFile.delete();
//        return resultFile;
//    }
//
//    private Map<String,String> getResultMap(File resultFile) throws IOException {
//        Map<String,String> result = new HashMap<String, String>();
//        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
//        for(String line : lines) {
//            String[] arr = line.split("=");
//            if (arr.length != 2)
//                throw new SchedulerException("line in resultfile '" + resultFile + "' is not valid: " + line);
//            result.put(arr[0],arr[1]);
//        }
//        return result;
//    }

//    @EventHandler
//    public void handleOrderEnd(OrderFinishedEvent e)  {
//        controller().terminateScheduler();
//    }

    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder order) throws IOException, InterruptedException {

//        System.err.println("ORDERFINISHED: " + order.getParameters().getNames().size());
        ImmutableMap<String,String> map = order.getParameters().toGuavaMap();

//        assertObject( map, "testvar1", "value1" );
//        assertObject( map, "testvar2", "newvalue2" );
//        assertObject( map, "testvar3", "value3" );

        controller().terminateScheduler();
    }

//    private void checkJobObjects() {
//        assertObject("spooler.variables.count", "3");
//        assertObject("spooler_task.order.job_chain.name", "chain");
//        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
//        assertObject("spooler_job.order_queue.length", "1");
//        assertObject("spooler_task.order.id", "chain");
//    }
//
//    private void checkJobFunctions() {
//        assertFunction("spooler_init");
//        assertFunction("spooler_open");
//        assertFunction("spooler_process");
//        assertFunction("spooler_close");
//        assertFunction("spooler_on_success");
//        assertFunction("spooler_exit");
//        assertFunction("spooler_task_before");
//        assertFunction("spooler_task_after");
//        assertFunction("spooler_process_before");
//        assertFunction("spooler_process_after");
//    }
//
//    /**
//     * checks if an estimated funtion was called.
//     * @param function
//     */
//    private void assertFunction(String function) {
//        assertObject(function,"1");		// any funtion should be called exactly one time
//    }
//
    /**
     * checks if an estimated object was given
     */
    private void assertObject(ImmutableMap<String,String> map, String key, String expectedValue) {
        assertTrue(key + " is no valid key", map.containsKey(key));
        String val = map.get(key);
        assertTrue(val + " is no valid value - " + expectedValue + " expected", val.equals(expectedValue));
    }
}