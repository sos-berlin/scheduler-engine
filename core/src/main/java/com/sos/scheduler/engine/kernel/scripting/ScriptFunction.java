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

}
