package com.sos.scheduler.engine.kernel.scripting;

import com.sos.scheduler.engine.kernel.scripting.ModuleInstance;
import com.sos.scheduler.engine.kernel.scripting.APIModuleInstance;
import org.junit.Test;

import com.sos.scheduler.engine.kernel.LogMock;

import sos.spooler.Log;

public class APIModuleInstanceTest {
	
	private final static String script_root = "./src/test/scripts/";

	@Test
	public void javascriptApi() throws NoSuchMethodException {
		String script = "var cnt;\n"
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
		System.out.println("START javascriptApi ---------------------------------------------------------------------");
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("spooler_init");
		while (module.callBoolean("spooler_process")) {
			;
		}
		System.out.println("END javascriptApi -----------------------------------------------------------------------");
	}

	@Test
	public void javascriptWithoutFunctions() {
		System.out.println("START javascriptWithoutFunctions ---------------------------------------------------------------------");
		String script =	"print('script ran successfully\\n');\n";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.call("spooler_init");
		module.call("spooler_process");
		module.call("spooler_exit");
		System.out.println("END javascriptWithoutFunctions -----------------------------------------------------------------------");
	}

	/*
	 * \brief Executes a simple javascript
	 * \detail
	 * Calls a javascript snippet and gives them some objects
	 * via the addObject method.
	 * The script does not contain any function, but calls the spooler_process
	 * method to cause the executing of the script. This is a special behavior 
	 * of the JobScheduler api: The execution of call("spooler_process") is just the 
	 * same like call() if the function spooler_process is not defined in the script. 
	 * http://www.mozilla.org/rhino/
	 * jar-file: inherited in java
	 */
	@Test
	public void javascriptWithObjects() {
		System.out.println("START javascriptWithObjects ---------------------------------------------------------------------");
		String script =	"print('hello, my name ist ' + name + '\\n');\n" +
				"log.debug('hello my name ist ' + name + '\\n')\n";
		APIModuleInstance module = new APIModuleInstance("javascript",script);
		module.addObject("walter", "name");
		module.addObject(new LogMock(), "log");
		module.call("spooler_process");
		System.out.println("END javascriptWithObjects -----------------------------------------------------------------------");
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
	 @Test
	//	maven dependencies not found for bean
	public void jythonScriptFromFile() {
		System.out.println("START jythonScriptFromFile ---------------------------------------------------------------------");
		APIModuleInstance module = new APIModuleInstance("jython","");
		module.setSourceFile(script_root + "test.py");
		module.addObject("jythonScriptFromFile", "name");
		module.addObject(new LogMock(), "log");
		module.call("spooler_init");
		try {
			while ( module.callBoolean("spooler_process")) {
				;
			}
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		module.call("spooler_exit");
		System.out.println("END jythonScriptFromFile -----------------------------------------------------------------------");
	}

	/*
	 * \brief Executes a simple groovy script
	 * \detail
	 * Calls a grrovy script and gives them some objects via the addObject method.
	 * The script contains some funtions called by the call method.
	 * http://groovy.codehaus.org/
	 * jar-file: groovy-all-1.5.6.jar
	 * jar-file: groovy-engine.jar
	 */
	// @Test
	//	maven dependencies not found for bean
	public void groovyScriptFromFile() throws NoSuchMethodException {
		System.out.println("START groovyScriptFromFile --------------------------------------------------------------------");
		ModuleInstance module = new ModuleInstance("groovy");
		module.setSourceFile(script_root + "test.groovy");
		module.addObject("groovyScriptFromFile", "name");
		module.call("spooler_init");
		while (module.callBoolean("spooler_process")) {
			;
		}
		module.call("spooler_exit");
		System.out.println("END groovyScriptFromFile -----------------------------------------------------------------------");
	}

}
