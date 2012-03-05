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

//import com.sos.JSHelper.Logging.JobSchedulerLog4JAppender;
//import org.apache.log4j.Appender;

import org.apache.log4j.Appender;
import sos.spooler.Log;
import sos.util.SOSSchedulerLogger;

import com.sos.JSHelper.Logging.JobSchedulerLog4JAppender;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class APIModuleInstance extends ScriptInstance implements APIModule {

	private final static String LANGUAGE_PREFIX = "javax.script:";
	private final static String SCHEDULER_PROCESS = "spooler_process";

	private final String schedulerLanguageId;
	private final JobSchedulerLog4JAppender jsAppender;

	@ForCpp public APIModuleInstance(String scriptLanguage, String sourcecode) throws UnsupportedScriptLanguageException {
		super(getScriptLanguage(scriptLanguage));
	
		JobSchedulerLog4JAppender bapn = null;
		Appender stdoutAppender = logger.getAppender("scheduler");
		if (stdoutAppender instanceof JobSchedulerLog4JAppender) {
			bapn = (JobSchedulerLog4JAppender) stdoutAppender;
			logger.info("LOG-I-0020: JobSchedulerLog4JAppender is configured as log4j-appender");
		}
		jsAppender = bapn;

		setSourceCode(sourcecode);
		schedulerLanguageId = scriptLanguage;
	}

	/**
	 * The script language to use
     * @return String
	 */
	private static String getScriptLanguage(String scriptlanguage) {
		return scriptlanguage.toLowerCase().replace(LANGUAGE_PREFIX, "");
	}

	/**
	 * The content from the language attribute of the script element. In case of javascript it will return 'javax.script:javascript'
	 * @return String
	 */
	public String getSchedulerLanguageId() {
		return schedulerLanguageId;
	}
	
	@Override
    @ForCpp
	public void addObject(Object object, String name) {
		if (object instanceof Log) {
			Log log = (Log) object;		// log object of the scheduler-task
			if (jsAppender != null) {
				try {
					SOSSchedulerLogger l = new SOSSchedulerLogger(log);
					jsAppender.setSchedulerLogger( l );
				} catch (Exception e) {
					logger.error("LOG-E-0120: job scheduler log object could not set in log4j properties");
					e.printStackTrace();
				}
			}
			super.addObject(logger, "logger");
		}
		super.addObject(object, name);
	}

	/**
	 * Call a script function
	 * It's just the same like the call method of the superclass, but the error handling is different. The JS
	 * calls the script for any function of the api (scheduler_init, scheduler_open etc.), but it is not necessary
     * the functions are present in the script.
	 * 
	 * scheduler_process has to be present. If not the whole script will run without functions.
	 * 
	 * @return Object - with the result of the script
	 * 
	 * @param rawFunctionName
     * @param params
	 */
	@Override
    @ForCpp
	public Object call(String rawFunctionName, Object[] params) {
		Object result = null;
		ScriptFunction functionObject = new ScriptFunction(rawFunctionName);
		String function = functionObject.getNativeFunctionName();
        logger.info("try to call function " + function);
		if ( functionObject.isFunction(getSourcecode())) {
			try {
                logger.info("call for function " + function + " (" + rawFunctionName + ")");
				result = super.call(function, params);
                logger.info("result is " + result);
			} catch (NoSuchMethodException e) {
				logger.error("the function " + function + " does not exist.");
			}
		} else {
            logger.info("not found");
			if (function.equals(SCHEDULER_PROCESS)) {
                logger.info("call for function " + function + " (" + rawFunctionName + ")");
				result = super.call();
				result = (result == null) ? Boolean.FALSE : result;
                logger.info("result is " + result);
			}
		}
		return result;
	}

	@Override
    @ForCpp
	public Object call(String rawFunctionName) {
		return call(rawFunctionName, new Object[] {});
	}

	@Override
    @ForCpp
	public Object call(String rawfunctionname, boolean param) {
		return call(rawfunctionname, new Object[] {Boolean.valueOf(param)});
	}

	/**
	 * \brief dummy for checking the existence of a function
	 * \details
	 * 
	 * @param rawFunctionName
	 * @return true
	 */
	@Override
	@ForCpp
    public boolean nameExists(String rawFunctionName) {
		ScriptFunction functionObject = new ScriptFunction(rawFunctionName);
		return functionObject.isFunction(getSourcecode());
	}

}
