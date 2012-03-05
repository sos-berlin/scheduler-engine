package com.sos.scheduler.engine.kernel.scripting;

import org.apache.log4j.Logger;
import org.junit.Ignore;
import org.junit.Test;

import com.sos.scheduler.engine.kernel.scheduler.LogMock;

public class APIModuleInstanceTest {
	
	private final static String script_root = "./src/test/scripts/";

	private static final Logger logger = Logger.getLogger(APIModuleInstanceTest.class);

	@Test
	public void javascriptApi() throws Exception {
		String script = "var cnt;\n"
			+ "function spooler_init() {\n"
			+ "   cnt = 0;\n"
			+ "   return true;\n"
			+ "}"
			+ " "
			+ "function scheduler_process() {\n"
			+ "  if (cnt < 5) {;\n"
			+ "    cnt++;\n"
			+ "    print('iteration no ' + cnt + '\\n');\n"
			+ "    return true;\n"
			+ "  }"
			+ "  return false;\n"
			+ "}";
		logger.debug("START javascriptApi ---------------------------------------------------------------------");
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("scheduler_init");
		while (module.callBoolean("scheduler_process")) {;}
		logger.debug("END javascriptApi -----------------------------------------------------------------------");
	}

	/**
	 * Spooler_process is the implicitly given function if no function body exists. This behavior is similiar to
	 * the current behavior for using the scripting API  
	 */
	@Test
	public void javascriptWithoutFunctions() throws Exception {
		logger.debug("START javascriptWithoutFunctions ---------------------------------------------------------------------");
		String script =	"print('script ran successfully\\n');\n";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("spooler_init");
		module.call("spooler_process");
		module.call("spooler_exit");
		logger.debug("END javascriptWithoutFunctions -----------------------------------------------------------------------");
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
		logger.debug("START javascriptWithObjects ---------------------------------------------------------------------");
		String script =	"print('hello, my name ist ' + name + '\\n');\n" +
				"log.debug('hello my name ist ' + name + '\\n')\n";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.addObject("walter", "name");
		module.addObject(new LogMock(), "log");
		module.call("scheduler_process");
		logger.debug("END javascriptWithObjects -----------------------------------------------------------------------");
	}

	/*
	 * \brief Executes a simple jython script
	 * \detail
	 * Calls the jython script test.py and gives them some objects
	 * via the addObject method.
	 * The script contains some funtions called by the call method.
	 * http://www.jython.org/
	 * jar-file: jython.jar from the jython 2.5.2 RC 2 installation
	 * jar-file: jython-engine.jar
	 */
//        @Ignore //TODO 2010-12-30 Zschimmer com.sos.scheduler.engine.kernel.scripting.UnsupportedScriptLanguageException: Scriptlanguage jython is not supported
//	 @Test
	//	maven dependencies not found for bean
//	public void jythonScriptFromFile() {
//		logger.debug("START jythonScriptFromFile ---------------------------------------------------------------------");
//		APIModuleInstance module = new APIModuleInstance("jython","");
//		module.setSourceFile(script_root + "test.py");
//		module.addObject("jythonScriptFromFile", "name");
//		module.addObject(new LogMock(), "log");
//		module.call("scheduler_init");
//		try {
//			while ( module.callBoolean("scheduler_process")) {
//				;
//			}
//		} catch (NoSuchMethodException e) {
//			e.printStackTrace();
//		}
//		module.call("scheduler_exit");
//		logger.debug("END jythonScriptFromFile -----------------------------------------------------------------------");
//	}

	/*
	 * \brief Executes a simple groovy script
	 * \detail
	 * Calls a grrovy script and gives them some objects via the addObject method.
	 * The script contains some funtions called by the call method.
	 * http://groovy.codehaus.org/
	 * jar-file: groovy-all-1.5.6.jar
	 * jar-file: groovy-engine.jar
	 */
//	 @Test
	//	maven dependencies not found for groovy
	@Ignore
	public void groovyScriptFromFile() throws Exception {
		logger.debug("START groovyScriptFromFile --------------------------------------------------------------------");
		ScriptInstance module = new ScriptInstance("groovy");
		module.setSourceFile(script_root + "test.groovy");
		module.addObject("groovyScriptFromFile", "name");
		module.call("scheduler_init");
		while (module.callBoolean("scheduler_process")) {
			;
		}
		module.call("scheduler_exit");
		logger.debug("END groovyScriptFromFile -----------------------------------------------------------------------");
	}

}
