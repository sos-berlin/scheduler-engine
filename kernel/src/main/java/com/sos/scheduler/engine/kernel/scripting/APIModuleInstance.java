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
 *   <p>2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import java.util.Arrays;
import java.util.List;

@ForCpp
public class APIModuleInstance extends ModuleInstance implements APIModule {

	private final static String LANGUAGE_PREFIX = "java:";

	private final static String SCHEDULER_INIT = "scheduler_init";
	private final static String SCHEDULER_OPEN = "scheduler_open";
	private final static String SCHEDULER_CLOSE = "scheduler_close";
	private final static String SCHEDULER_ON_SUCCESS = "scheduler_on_success";
	private final static String SCHEDULER_EXIT = "scheduler_exit";
	private final static String SCHEDULER_ON_ERROR = "scheduler_on_error";
	private final static String SCHEDULER_PROCESS = "scheduler_process";

	private final String schedulerLanguageId;

	/**
	 * These are the optional methods of the scheduler script api.
	 */
	List<String> optionalMethods = Arrays.asList(SCHEDULER_INIT, SCHEDULER_OPEN,
			SCHEDULER_CLOSE, SCHEDULER_ON_SUCCESS, SCHEDULER_EXIT, SCHEDULER_ON_ERROR);

	public APIModuleInstance(String scriptlanguage, String sourcecode) {
		super(getScriptLanguage(scriptlanguage));
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
	 * \brief the content from the langauge attribute of the script element
	 * \details In case of javascript it will return 'java:javascript'
	 * 
	 * \return String
	 */
	public String getSchedulerLanguageId() {
		return schedulerLanguageId;
	}

	/**
	 * \brief call a script funtion \details It's just the same like the call
	 * method of the superclass, but the error handling is different. The JS
	 * calls the script for any funtion of the api (spooler_init, spooler_open
	 * etc.), but it is not neccessary the functions are present in the script.
	 * 
	 * spooler_process has to be present. If not the whole script will run
	 * without functions.
	 * 
	 * \return Object - with the result of the script
	 * 
	 * @param functionname
	 */
	@Override
	public Object call(String rawfunctionname, Object[] params) {
		Object result = null;
		String function = new APIScriptFunction(rawfunctionname).getNativeFunctionName();
//		log("call for function " + rawfunctionname);
		boolean isFunction = (getSourcecode().contains(function)) ? true : false;
		if (isFunction) {
			try {
				result = super.call(function, params);
			} catch (NoSuchMethodException e) {
				log("the function " + function + " does not exist.");
			}
//			function = getLastFunction().getNativeFunctionName();
		} else {
			if (function.equals(SCHEDULER_PROCESS)) {
				result = super.call();
			}
		}
		return check_result(function, result);
	}

	@Override
	public Object call(String rawfunctionname) {
		return call(rawfunctionname, new Object[] {});
	}

	/**
	 * \brief check and modify the result of the script call \details The
	 * scheduler need a boolean as a result from a script call. Sometimes the
	 * script do not implement an explicit 'return' command, in this case the
	 * script result will changed depending on the called function.
	 * 
	 * @param function
	 *            - the called function
	 * @param ret
	 *            - the result from the script
	 * @return Object
	 */
	private Object check_result(String function, Object ret) {
		Object result = ret;
		if (result == null || !(ret instanceof Boolean)) {
			result = true;
			if (function.equals(SCHEDULER_PROCESS))
				result = false;
			if (function.equals(SCHEDULER_EXIT))
				result = false;
		}
		return result;
	}

	/**
	 * \brief dummy for checking the existence of a funtion \details
	 * 
	 * @param name
	 * @return true
	 */
	@Override
	public boolean nameExists(String name) {
		return true;
	}

}
