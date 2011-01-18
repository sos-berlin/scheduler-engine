package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file APIScriptFunction.java
 * \brief class for handling function names (scheduler version)
 *  
 * \class APIScriptFunction
 * \brief class for handling different function names (scheduler version)
 * 
 * \details
 * This class is the same like com.sos.scheduler.engine.kernel.scripting.ScriptFunction but
 * "spooler" will be replaced with "scheduler" if the name begins with "spooler".
 *
 * \author ss
 * \version 1.0 - 18.01.2011 11:08:49
 * <div class="sos_branding">
 *   <p>© 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class APIScriptFunction extends ScriptFunction {

	public APIScriptFunction(String fullname) {
		super( (fullname.startsWith("spooler")) ? fullname.replaceFirst("spooler", "scheduler") : fullname);
	}

}
