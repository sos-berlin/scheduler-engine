package com.sos.scheduler.engine.tests.jira.js868.withoutglobals;


import com.sos.scheduler.engine.common.system.OperatingSystem;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.engine.tests.jira.js868.JS868Base;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

/**
 * This class is working with a scheduler.xml without any parameter declaration. It is for test to get the correct
 * parameter values in a shell job.
 */
public class JS868NoGlobalParamsIT extends JS868Base {

    private final static String orderName = "test_chain_" + (OperatingSystem.isWindows ? "windows" : "unix");

    @Test
	public void test() throws InterruptedException, IOException {
        CommandBuilder util = new CommandBuilder();
        File resultFile = getTempFile("result.txt");
        // controller().activateScheduler("-e","-log-level=info");
        controller().activateScheduler();
        String cmd = util.modifyOrder(orderName,"order")
                .addParam("RESULT_FILE",resultFile.getAbsolutePath())
                .getCommand();
		controller().scheduler().executeXml(cmd);
		controller().waitForTermination(shortTimeout);

        resultMap = getResultMap(resultFile);
        testAssertions();
    }

    public void testAssertions() {
        assertObject("JOB","job");              // is defined in the job only
        assertObject("ORDER","order");          // is defined in the order only
        assertObject("ORDER_JOB","order");      // is defined in all objects (order, job) - order overwrites job
    }

}
