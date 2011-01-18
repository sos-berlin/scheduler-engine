package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file APIModuleInstance.java
 * \brief implementation of the script api from the job scheduler
 *  
 * \class ModuleInstance 
 * \brief implementation of the script api from the job scheduler
 * 
 * \details
 * This class is a general implementation of the scheduler script api for using with
 * all script languages supported by the javax interface for scripting.
 * 
 * see http://java.sun.com/developer/technicalArticles/J2SE/Desktop/scripting/
 * see https://scripting.dev.java.net/
 *
 * \author Stefan Schaedlich
 * \version 1.0 - 2010-12-17
 * <div class="sos_branding">
 *   <p>(c) 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

//import com.sos.JSHelper.Logging.JobSchedulerLog4JAppender;
//import org.apache.log4j.Appender;

import java.util.Arrays;
import java.util.List;

import org.apache.log4j.Appender;
import org.apache.log4j.Logger;

import sos.spooler.Log;
import sos.util.SOSSchedulerLogger;

import com.sos.JSHelper.Logging.JobSchedulerLog4JAppender;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class APIModuleInstance extends ScriptInstance implements APIModule {

	private final static String LANGUAGE_PREFIX = "javax.script:";

	private final static String SCHEDULER_INIT = "scheduler_init";
	private final static String SCHEDULER_OPEN = "scheduler_open";
	private final static String SCHEDULER_CLOSE = "scheduler_close";
	private final static String SCHEDULER_ON_SUCCESS = "scheduler_on_success";
	private final static String SCHEDULER_EXIT = "scheduler_exit";
	private final static String SCHEDULER_ON_ERROR = "scheduler_on_error";
	private final static String SCHEDULER_PROCESS = "scheduler_process";

	private final String schedulerLanguageId;
	private final JobSchedulerLog4JAppender jsAppender;
	private final static Logger logger = Logger.getLogger(APIModuleInstance.class);

	/**
	 * These are the optional methods of the scheduler script api.
	 */
	List<String> optionalMethods = Arrays.asList(SCHEDULER_INIT, SCHEDULER_OPEN,
			SCHEDULER_CLOSE, SCHEDULER_ON_SUCCESS, SCHEDULER_EXIT, SCHEDULER_ON_ERROR);

	public APIModuleInstance(String scriptlanguage, String sourcecode) {
		super(getScriptLanguage(scriptlanguage));
		
//		Appender apn = logger.getAppender("scheduler");
//		if (apn == null) {
//			// @TODO sollte Bestandteil von Log4JHelper werden ...
//			SimpleLayout layout = new SimpleLayout();
//			apn = new BufferedJobSchedulerLog4JAppender(layout);
//			Appender consoleAppender = apn; // JobSchedulerLog4JAppender(layout);
//			logger.addAppender(consoleAppender);
//			logger.setLevel(Level.DEBUG);
//			logger.debug("LOG-I-0010: Log4j configured programmatically");
//		}
//		stdoutAppender = apn;
		
		JobSchedulerLog4JAppender bapn = null;
		Appender stdoutAppender = logger.getAppender("scheduler");
		if (stdoutAppender instanceof JobSchedulerLog4JAppender) {
			bapn = (JobSchedulerLog4JAppender) stdoutAppender;
			logger.info("LOG-I-0020: JobSchedulerLog4JAppender is configured as log4j-appender");
		}
		jsAppender = bapn;

		setSourceCode(sourcecode);
		schedulerLanguageId = scriptlanguage;
	}

	/**
	 * \brief the script language to use \return String
	 */
	private static String getScriptLanguage(String scriptlanguage) {
		return scriptlanguage.toLowerCase().replace(LANGUAGE_PREFIX, "");
	}

	/**
	 * \brief the content from the language attribute of the script element
	 * \details In case of javascript it will return 'javax.script:javascript'
	 * 
	 * \return String
	 */
	public String getSchedulerLanguageId() {
		return schedulerLanguageId;
	}
	
	@Override
	public void addObject(Object object, String name) {
		String object_name = new APIScriptFunction(name).getNativeFunctionName();
		if (object instanceof Log && jsAppender != null) {
			Log log = (Log) object;
			try {
				SOSSchedulerLogger l = new SOSSchedulerLogger(log);
				jsAppender.setSchedulerLogger( l );
			} catch (Exception e) {
				logger.error("LOG-E-0120: job scheduler log object could not set in log4j properties");
				e.printStackTrace();
			}
		}
		super.addObject(object, object_name);
	}

	/**
	 * \brief call a script function \details It's just the same like the call
	 * method of the superclass, but the error handling is different. The JS
	 * calls the script for any function of the api (scheduler_init, scheduler_open
	 * etc.), but it is not necessary the functions are present in the script.
	 * 
	 * scheduler_process has to be present. If not the whole script will run
	 * without functions.
	 * 
	 * \return Object - with the result of the script
	 * 
	 * @param functionname
	 */
	@Override
	public Object call(String rawfunctionname, Object[] params) {
		Object result = null;
		APIScriptFunction fobj = new APIScriptFunction(rawfunctionname);
		String function = fobj.getNativeFunctionName();
		logger.info("call for function " + function);
		if ( fobj.isFunction(getSourcecode())) {
			try {
				result = super.call(function, params);
			} catch (NoSuchMethodException e) {
				logger.error("the function " + function + " does not exist.");
			}
		} else {
			if (function.equals(SCHEDULER_PROCESS)) {
				result = super.call();
				result = (result == null) ? false : result;
			}
		}
		return result;
	}

	@Override
	public Object call(String rawfunctionname) {
		return call(rawfunctionname, new Object[] {});
	}

	/**
	 * \brief dummy for checking the existence of a function
	 * \details
	 * 
	 * @param name
	 * @return true
	 */
	@Override
	public boolean nameExists(String name) {
		return true;
	}

}
