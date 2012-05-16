package com.sos.scheduler.engine.kernel.scripting;

import com.google.common.io.Files;
import com.google.common.io.Resources;
import com.sos.JSHelper.Exceptions.JobSchedulerException;
import com.sos.scheduler.engine.kernel.scheduler.LogMock;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.PrintStream;
import java.net.URISyntaxException;
import java.net.URL;

import static org.junit.Assert.assertEquals;

public class APIModuleInstanceTest {
	
	private static final Logger logger = Logger.getLogger(APIModuleInstanceTest.class);
    private static final ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        System.setOut(new PrintStream(outputStream));     // redirect stdout to the outputStream object
    }

	@Test
	public void javascriptApi() throws Exception {
		final String script = "var cnt;\n"
			+ "function spooler_init() {\n"
			+ "   cnt = 0;\n"
			+ "   return true;\n"
			+ "}"
			+ " "
			+ "function spooler_process() {\n"
			+ "  if (cnt < 5) {;\n"
			+ "    cnt++;\n"
			+ "    print('iteration no ' + cnt + '\\n');\n"
			+ "    return true;\n"
			+ "  }"
			+ "  return false;\n"
			+ "}";
        outputStream.reset();
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("spooler_init");
		while (module.callBoolean("spooler_process")) {}
        assertEquals(5,outputStream.toString().split("\n").length);
	}

	/**
	 * Spooler_process is the implicitly given function if no function body exists. This behavior is similiar to
	 * the current behavior for using the scripting API  
	 */
	@Test
	public void javascriptWithoutFunctions() throws Exception {
        outputStream.reset();
		String script =	"print('script ran successfully');";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("spooler_init");
		module.call("spooler_process");
		module.call("spooler_exit");
        assertEquals("script ran successfully",outputStream.toString());
	}

	/**
	 * \brief Executes a simple javascript
	 * \detail
	 * Calls a javascript snippet and gives them some objects via the addObject method.
	 * 
	 * The script does not contain any function, but calls the scheduler_process
	 * method to cause the executing of the script. This is a special behavior 
	 * of the JobScheduler api: The execution of call("scheduler_process") is just the 
	 * same like call() if the function scheduler_process is not defined in the script. 
	 * http://www.mozilla.org/rhino/
	 * jar-file: inherited in java
	 */
	@Test
	public void javascriptWithObjects() throws Exception {
        outputStream.reset();
		String script =	"print('hello my name is ' + name + '\\n');\n" +
				"log.debug('hello my name is ' + name)\n";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.addObject("Walter", "name");
		module.addObject(new LogMock(), "log");
		module.call("scheduler_process");
        
        String[] result = outputStream.toString().split("\n");
        String expected = "hello my name is Walter";
        assertEquals(2,result.length);      // expects 2 lines of output
        assertEquals(expected,result[0].trim());
        assertEquals(expected,result[1].trim());
	}

	/**
	 * \brief Executes a simple groovy script
	 * \detail
     * Calls a groovy script and gives them some objects via the addObject method.
     * The script contains some funtions called by the call method.
	 */
    @Test
	public void groovyScriptFromFile() throws Exception {
        File sourceFile = getResourceFile("com/sos/scheduler/engine/kernel/scripting/test.groovy");
        doScript("groovy", Files.toString(sourceFile, SchedulerConstants.defaultEncoding),"groovyScript");
	}

    /**
     * \brief Executes a simple java script
     * \detail
     * Calls a java script and gives them some objects via the addObject method.
     */
    @Test
    public void javaScriptFromFile() throws Exception {
        File sourceFile = getResourceFile("com/sos/scheduler/engine/kernel/scripting/test.js");
        doScript("javascript", Files.toString(sourceFile, SchedulerConstants.defaultEncoding),"javaScript");
    }

    private void doScript(String scriptLanguage, String script, String calledFrom) throws NoSuchMethodException {
        outputStream.reset();
        APIModuleInstance module = new APIModuleInstance(scriptLanguage,script);
        module.addObject(calledFrom, "name");
        module.call("spooler_init");
        while (module.callBoolean("spooler_process")) {
            ;
        }
        module.call("spooler_exit");

        logger.info(outputStream.toString());
        String[] result = outputStream.toString().split("\n");
        assertEquals(7,result.length);      // expects 7 lines of output
        assertEquals("spooler_init is called by " + calledFrom,result[0].trim());
        for(int i=1; i < 6; i++) {
            assertEquals("spooler_process is called by " + calledFrom,result[i].trim());
        }
        assertEquals("spooler_exit is called by " + calledFrom, result[6].trim());
    }

    private static String getResourceFolder() {
        String resourceRoot = APIModuleInstanceTest.class.getResource("/").getPath();
        return resourceRoot += APIModuleInstanceTest.class.getPackage().getName().replace(".","/");
    }
    
    private File getResourceFile(String resourceName) {
        URL url = Resources.getResource(resourceName);
        File result = null;
        try {
            result = new File( url.toURI() );
        } catch (URISyntaxException e) {
            throw new JobSchedulerException("invalid URI '" + url + "' :" + e,e);
        }
        return result;
    }


}