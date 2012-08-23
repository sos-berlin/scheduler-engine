package com.sos.scheduler.engine.tests.jira.js868;


import com.google.common.io.Files;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.SchedulerTest;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.List;

import static org.junit.Assert.assertTrue;

/**
 * This class is working with a scheduler.xml without any parameter deklaration. It is for test to get the correct
 * parameter values in a shell job.
 *
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 16.12.2011 13:39:41
 *
 */
public abstract class JS868Base extends SchedulerTest {
    
	protected HashMap<String,String> resultMap;

    protected HashMap<String,String> getResultMap(File resultFile) throws IOException {
        HashMap<String,String> result = new HashMap<String, String>();
        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
        for(String line : lines) {
            String[] arr = line.trim().split("=");
            if (arr.length != 2)
                throw new SchedulerException("line in resultfile '" + resultFile + "' is not valid: " + line);
            result.put(arr[0],arr[1]);
        }
        return result;
    }

    @EventHandler
    public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        controller().terminateScheduler();
    }

    public abstract void testAssertions();

    /**
     * checks if an estimated object was given
     */
    protected void assertObject(String varname, String expected) {
        String value = resultMap.get(varname);
        assertTrue("parameter " + varname + " is not set", value != null);
        assertTrue(varname + "=" + value + " is not valid - " + varname + "=" + expected + " expected", value.equals(expected));
    }
    
}
