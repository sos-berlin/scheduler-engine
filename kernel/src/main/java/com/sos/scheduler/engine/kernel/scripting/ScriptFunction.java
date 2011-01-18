package com.sos.scheduler.engine.kernel.scripting;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * \file ScriptFunction.java
 * \brief class for handling function names
 *  
 * \class ScriptFunction
 * \brief class for handling function names
 * 
 * \details
 * This class provide different version of function names:
 * getFunctionName(): the name of the function followed by braces, e.g. myFunction()
 * getNativeFunctionName(): the pure name of the function, e.g. myFunction()
 * 
 * Additionally the method isFunction() is provided for testing the existence of the
 * function in a given script.
 * 
 * With getTypeId() you can get the COM-type of the function.
 * \code
  \endcode
 *
 * \author ss
 * \version 1.0 - 18.01.2011 11:20:08
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class ScriptFunction {

	private String	functionName	= "";
	private String	typeId			= "";
	
	public ScriptFunction(String fullname) {
		boolean propsToSet = true;
		if (fullname.contains(")") && !fullname.endsWith(")")) {
			Pattern p = Pattern.compile("^(.*)([^\\)])$");
			Matcher m = p.matcher(fullname);
			if (m.matches()) {
				functionName = m.group(1);
				typeId = m.group(2);
				propsToSet = false;
			}
		}
		if (propsToSet) {
			functionName = fullname;
			functionName = getNativeFunctionName() + "()";
			typeId = "V"; // void
		}
	}

	public String getFunctionName() {
		return functionName;
	}

	public String getNativeFunctionName() {
		return functionName.replaceAll("\\(.*\\)", "");
	}

	public String getTypeId() {
		return typeId;
	}
	
	/**
	 * \brief check i a function exists in the given code
	 * \detail
	 * the presence of a function is determined with the functionname followed by optional whitespaces 
	 * and an opened brace sign, that means ( or { or [.
	 * 
	 * @param scriptcode
	 * @return
	 */
	public boolean isFunction(String scriptcode) {
		final Pattern p = Pattern.compile( getNativeFunctionName() + "\\s*(\\(|\\{|\\[)");
		Matcher m = p.matcher(scriptcode); 
		return m.find();
	}

}
