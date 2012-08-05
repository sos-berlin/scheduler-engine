package com.sos.scheduler.engine.kernel.scripting;

/**
 * Implementation of the script api from the job scheduler.
 *
 * This class is a general implementation of the scheduler script api for using with
 * all script languages supported by the javax interface for scripting.
 * 
 * see http://java.sun.com/developer/technicalArticles/J2SE/Desktop/scripting/
 * see https://scripting.dev.java.net/
 *
 * @author Stefan Schaedlich
 * @version 1.0 - 2010-12-17
 * <div class="sos_branding">
 *   <p>(c) 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

// import org.apache.log4j.Appender;
import org.apache.log4j.Logger;
// import com.sos.JSHelper.Logging.JobSchedulerLog4JAppender;

public final class APIModuleInstance extends ScriptInstance {

    private final Logger logger = Logger.getLogger(APIModuleInstance.class);
	private static final String LANGUAGE_PREFIX = "javax.script:";
	private static final String SCHEDULER_PROCESS = "spooler_process";

	public APIModuleInstance(String scriptLanguage, String sourcecode) {
		super(getScriptLanguage(scriptLanguage),sourcecode);
	}

    @Override public Object call() {
        return super.call();    //To change body of overridden methods use File | Settings | File Templates.
    }

    /**
	 * The script language to use
     * @return String
	 */
	private static String getScriptLanguage(String scriptlanguage) {
		return scriptlanguage.toLowerCase().replace(LANGUAGE_PREFIX, "");
	}

	@Override
	public Object call(String functionName) {
		return callFunction(functionName, new Object[] {});
	}

	public boolean callBoolean(String functionname, boolean defaultResult) {
		boolean result = defaultResult;
        try {
            result = callBoolean(functionname, new Object[] {defaultResult});
        } catch (NoSuchMethodException e) {
            result = defaultResult;
        }
        return result;
    }

    /**
     * Call a script function
     * It's just the same like the call method of the superclass, but the error handling is different. The JS
     * calls the script for any function of the api (scheduler_init, scheduler_open etc.), but it is not necessary
     * that the functions are present in the script.
     *
     * scheduler_process has to be present. If not the whole script will run without functions.
     *
     * @return Object - with the result of the script
     *
     * @param functionName
     * @param params
     */
    private Object callFunction(String functionName, Object[] params) {
        Object result = null;
        try {
            logger.debug("call for function " + functionName + " (" + functionName + ")");
            result = super.call(functionName, params);
            logger.debug("result is " + result);
        } catch (NoSuchMethodException e) {
            logger.debug("function " + functionName + " not found");
            if (functionName.equals(SCHEDULER_PROCESS)) {
                super.call();
                result = Boolean.FALSE;
            }
        }
        return result;
    }

}
