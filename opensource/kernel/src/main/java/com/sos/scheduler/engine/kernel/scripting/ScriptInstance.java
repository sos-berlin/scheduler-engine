package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file ScriptInstance.java
 * \brief general wrapper for the javax.script interface 
 *  
 * \class ScriptInstance 
 * \brief general wrapper for the javax.script interface 
 * 
 * \details
 * This class provides a general mechanism to call script in different languages.
 *    
 * \code
	ScriptInstance module = new ScriptInstance("javascript");
	module.setSourceCode("print('Hello ' + name + '\\n');");
	module.addObject("nick", "name");
	module.call();
  \endcode
 *
 * \author ss
 * \version 17.12.2010 12:04:34
 * <div class="sos_branding">
 *   <p>(c) 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import javax.script.Bindings;
import javax.script.Invocable;
import javax.script.ScriptContext;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;

import org.apache.log4j.Logger;

public class ScriptInstance implements Script {

	protected final Logger 		logger = Logger.getLogger(ScriptInstance.class.getPackage().getName());
	
	private final String		languageId;
	private final ScriptEngine	scriptengine;
	private final Bindings		scriptbindings;

	private String				sourceCode;
	private ScriptFunction		lastFunction	= null;

	public ScriptInstance(String scriptLanguage) throws UnsupportedScriptLanguageException {
		languageId = scriptLanguage;
		logger.info("the language id is " + scriptLanguage);

		ScriptEngineManager sm;
		ScriptEngine se;
		Bindings sb;
		try {
			sm = new ScriptEngineManager();
			se = sm.getEngineByName(getLanguageId());
			sb = se.getBindings(ScriptContext.ENGINE_SCOPE);
		}
		catch (Exception e) {
            String message = "Scriptlanguage " + languageId + " is not supported.";
            logger.error(message);
			throw new UnsupportedScriptLanguageException(e, message);
		}
		scriptengine = se;
		scriptbindings = sb;
	}

	public void setSourceCode(String sourcecode) {
		this.sourceCode = sourcecode;
	}

	public void setSourceFile(String filename) {
		this.sourceCode = readFile(filename);
	}
	
	private String readFile(String filename) {
		StringBuffer sb = null;
		FileReader fr = null;
		logger.info("reading script from file " + filename);
		try {
			fr = new FileReader(filename);
			BufferedReader br = new BufferedReader(fr);
			String line;
			sb = new StringBuffer();
			while ((line = br.readLine()) != null) {
				sb.append(line);
				sb.append('\n');
			}
		}
		catch (FileNotFoundException e) {
			throw new InvalidScriptException(e, "the file " + filename + "does not exist.");
		}
		catch (IOException e) {
			throw new InvalidScriptException(e, "error reading the file " + filename);
		}
		return (sb != null) ? sb.toString() : "";
	}

	public Object getObject(String name) {
		return scriptbindings.get(name); 		
	}
	
	@Override
	public void addObject(Object object, String name) {
		String object_name = name;
		if (object == null) {
			logger.error("the object '" + name + "' is not set");
		} else {
			logger.debug("add object " + name + " to script");
			scriptbindings.put(object_name, object);
		}
	}
	
	public Object call() {
		Object result = true;
		String code = getSourcecode();
		lastFunction = null;
		if (code == null) {
			throw new InvalidScriptException("scriptcode is missing - it seems neither setSourceCode nor SetSourceFile was called first.");
		}

		try {
			result = scriptengine.eval(code, scriptbindings);
		}
		catch (ScriptException e) {
			throw new InvalidScriptException(e, "error in script", code);
		}
		return result;
	}

	@Override
	public Object call(String rawfunctionname, Object[] params) throws NoSuchMethodException {
		Object result = true;
		lastFunction = new ScriptFunction(rawfunctionname);
		String functionname = lastFunction.getNativeFunctionName();
		String code = this.getSourcecode();
		if (code == null) {
			throw new InvalidScriptException("scriptcode is missing - it seems neither setSourceCode nor SetSourceFile was called first.");
		}
		
		try {
			logger.debug("executing function " + functionname);
			scriptengine.eval(code, scriptbindings);
			Invocable invocableEngine = (Invocable) scriptengine;
			result = invocableEngine.invokeFunction(functionname, params);
		}
		catch (ScriptException e) {
			throw new InvalidScriptException(e, "error in script", code);
		}
		return result;
	}

	@Override
	public Object call(String rawfunctionname) throws NoSuchMethodException {
		return call(rawfunctionname, new Object[] {});
	}

	@Override
	public Object call(String rawfunctionname, boolean param) throws NoSuchMethodException {
		return call(rawfunctionname, new Object[] {Boolean.valueOf(param)});
	}

	@Override
	public boolean callBoolean(String functionname, Object[] params) throws NoSuchMethodException {
		Object obj = call(functionname, params);
		if (obj instanceof Boolean) return (Boolean) obj; 
		if (obj instanceof Integer) return ((Integer)obj == 1) ? true : false;
		throw new ClassCastException("the result of function " + functionname + " could not cast to " + Boolean.class.getName());
	}

	@Override
	public boolean callBoolean(String functionname) throws NoSuchMethodException {
		return callBoolean(functionname, new Object[]{});
	}

	@Override
	public String callString(String functionname, Object[] params) throws NoSuchMethodException {
		Object obj = call(functionname, params);
		if (obj instanceof String) return (String) obj;
		throw new ClassCastException("the result of function " + functionname + " could not cast to " + String.class.getName());
	}

	@Override
	public String callString(String functionname) throws NoSuchMethodException {
		return callString(functionname, new Object[]{});
	}

	@Override
	public double callDouble(String functionname, Object[] params) throws NoSuchMethodException {
		Object obj = call(functionname, params);
		if (obj instanceof Double) return (Double) obj;
		throw new ClassCastException("the result of function " + functionname + " could not cast to " + Double.class.getName());
	}

	@Override
	public double callDouble(String functionname) throws NoSuchMethodException {
		return callDouble(functionname, new Object[]{});
	}

	@Override
	public String getLanguageId() {
		return this.languageId;
	}

	@Override
	public String getSourcecode() {
		return sourceCode;
	}

	@Override
	public ScriptFunction getLastFunction() {
		return lastFunction;
	}

}
