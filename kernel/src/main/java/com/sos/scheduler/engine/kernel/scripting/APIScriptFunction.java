package com.sos.scheduler.engine.kernel.scripting;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * \file class for handling different function calls
 * \brief 
 *  
 * \class class for handling different function calls 
 * \brief 
 * 
 * \details
 *
 * \author Stefan Schaedlich
 * \version 17.12.2010 12:01:08
 * <div class="sos_branding">
 *   <p>© 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class APIScriptFunction extends ScriptFunction {

	private final static String SPOOLER_PREFIX = "spooler_";
	private final static String SCHEDULER_PREFIX = "scheduler_";

	public APIScriptFunction(String fullname) {
		super( replaceSpoolerPrefix(fullname) );
	}
	
	public static String replaceSpoolerPrefix(String name) {
		return name.replaceFirst(SPOOLER_PREFIX, SCHEDULER_PREFIX);
	}

}
