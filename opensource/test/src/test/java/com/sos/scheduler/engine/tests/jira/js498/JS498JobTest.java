package com.sos.scheduler.engine.tests.jira.js498;

import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.List;

import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with the Rhino engine. The test starts different standalone jobs.
 *
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *
 * @author ss
 * @version 1.0 - 12.07.2012 12:03:22
 *
 */
public class JS498JobTest extends SchedulerTest {
    
    private static final Logger logger = Logger.getLogger(JS498JobTest.class);


    private static final ImmutableList<String> jobs = ImmutableList.of ("script_only","rhino_objects_standalone","rhino_functions_standalone");
	private final CommandBuilder util = new CommandBuilder();

	private HashMap<String,String> resultMap;
    private int taskCount = 0;

    @Test
    public void test() throws InterruptedException, IOException {
        //controller().activateScheduler("-e","-ignore-process-classes","-log-level=info","-log=" + logFile);
        controller().activateScheduler("-e","-log-level=info");
        File resultFile = prepareResultFile();
        for (String jobName : jobs) {
            controller().scheduler().executeXml(util.startJobImmediately(jobName).getCommand());
        }
        controller().waitForTermination(shortTimeout);
        resultMap = getResultMap(resultFile);
        testScript();
        testObjectsJob();
        testFunctions();
    }
    
    private File prepareResultFile() {
        String resultFileName = scheduler().getConfiguration().localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
        File resultFile = new File(resultFileName);
        resultFile.delete();
        return resultFile;
    }

    private HashMap<String,String> getResultMap(File resultFile) throws IOException {
        HashMap<String,String> result = new HashMap<String, String>();
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
	public void handleOrderEnd(TaskEndedEvent e) throws IOException {
        taskCount++;
        if (taskCount == jobs.size())
            controller().terminateScheduler();
	}

    // result of job script_only
    public void testScript() throws IOException {
        assertObject("script_only", "script_only");
    }

    // result of job rhino_objects
    public void testObjectsJob() throws IOException {
        assertObject("spooler.variables.count", "2");
        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
    }

    // result of job rhino_functions
    public void testFunctions() throws IOException {
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
