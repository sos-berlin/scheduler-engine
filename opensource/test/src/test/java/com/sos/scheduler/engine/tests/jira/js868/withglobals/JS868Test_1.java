package com.sos.scheduler.engine.tests.jira.js868.withglobals;

import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.engine.tests.jira.js868.JS868Base;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

/**
 * This class is working with a scheduler.xml sith some parameter deklaration. It is for test to get the correct
 * parameter values in a shell job.
 *
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 10.07.2012
 *
 */
public class JS868Test_1 extends JS868Base {
    
    @Test
	public void test() throws InterruptedException, IOException {
        final CommandBuilder util = new CommandBuilder();
        File resultFile = getTempFile(JS868Test_1.class,"result.txt");
//        controller().activateScheduler("-e","-log-level=info");
        controller().activateScheduler();
        String cmd = util.modifyOrder("test_chain","order")
                .addParam("RESULT_FILE",resultFile.getAbsolutePath())
                .getCommand();
		controller().scheduler().executeXml(cmd);
		controller().waitForTermination(shortTimeout);

        resultMap = getResultMap(resultFile);
        testAssertions();
    }

    public void testAssertions() {
        assertObject("ORDER_JOB_GLOBAL","order");   // is defined in all objects (scheduler.xml, order, job) - order overwrites all
        assertObject("ORDER_JOB","order");          // is defined in order and job - order overwrites job
        assertObject("ORDER_GLOBAL","order");       // is defined in order and scheduler.xml - order overwrites global
        assertObject("ORDER", "order");              // is defined in the order only
        assertObject("JOB","job");                  // is defined in the job only
        assertObject("JOB_GLOBAL","job");           // is defined in job and scheduler.xml - job order overwrites global
        assertObject("GLOBAL", "global");            // is defined in scheduler.xml only
    }

}
