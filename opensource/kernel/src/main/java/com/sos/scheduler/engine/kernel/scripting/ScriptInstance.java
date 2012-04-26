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
 *
 * \author ss
 * \version 12.03.2012 10:22:17
 * <div class="sos_branding">
 *   <p>(c) 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

import com.google.common.io.Files;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.apache.log4j.Logger;

import javax.script.*;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.List;

public class ScriptInstance implements Script {

	private final Logger 		logger = Logger.getLogger(ScriptInstance.class);
	
	private final String		languageId;
	private final ScriptEngine	scriptengine;
	private final Bindings		scriptbindings;
	private final String		sourceCode;

    private boolean isEvaluated = false;
    private Object scriptResult = null;


    public ScriptInstance(String scriptLanguage, String sourceCode) {
        this.languageId = scriptLanguage;
        this.sourceCode = sourceCode;
        this.scriptengine = getScriptEngine();
        this.scriptbindings = this.scriptengine.getBindings(ScriptContext.ENGINE_SCOPE);
        logger.debug("the language id is " + scriptLanguage);
    }

    public ScriptInstance(String scriptLanguage, File sourceFile) {
        try {
            this.sourceCode = Files.toString(sourceFile, Charset.defaultCharset());
        } catch (IOException e) {
            throw new SchedulerException("error reading file " + sourceFile.getAbsolutePath(),e);
        }
        this.languageId = scriptLanguage;
        this.scriptengine = getScriptEngine();
        this.scriptbindings = this.scriptengine.getBindings(ScriptContext.ENGINE_SCOPE);
        logger.debug("the language id is " + scriptLanguage);
    }

    private ScriptEngine getScriptEngine() {
        ScriptEngineManager sm = new ScriptEngineManager();
        ScriptEngine se = sm.getEngineByName(getLanguageId());
        if (se == null) {
            List<ScriptEngineFactory> factories = sm.getEngineFactories();
            for(ScriptEngineFactory factory : factories) {
                logger.info(factory.getEngineName() + " is available (" + factory.getNames().toString() + ")");
            }
            throw new SchedulerException("script language " + getLanguageId() + " not valid.");
        }
        return se;
    }
    
    private Object evalScript() {
        try {
            if (!isEvaluated) {
                scriptResult = scriptengine.eval(sourceCode, scriptbindings);
                isEvaluated = true;
            }
        }
        catch (ScriptException e) {
            throw new SchedulerException(e + ": error in script code " + sourceCode, e);
        }
        return scriptResult;
    }

	public final Object getObject(String name) {
		return scriptbindings.get(name); 		
	}
	
	@Override
	public final void addObject(Object object, String name) {
        logger.debug("add object " + name + " to script");
        scriptbindings.put(name, object);
	}
	
	public Object call() {
        return evalScript();
	}

	@Override
	public Object call(String functionname, Object[] params) throws NoSuchMethodException {
        evalScript();
		try {
			logger.debug("executing function " + functionname);
			Invocable invocableEngine = (Invocable) scriptengine;
			return invocableEngine.invokeFunction(functionname, params);
		}
		catch (ScriptException e) {
			throw new SchedulerException("error in script code " + sourceCode + ": " + e, e);
		}
	}

	@Override
	public Object call(String rawfunctionname) throws NoSuchMethodException {
		return call(rawfunctionname, new Object[] {});
	}

	@Override
	public Object call(String rawfunctionname, boolean param) throws NoSuchMethodException {
		return call(rawfunctionname, new Object[] {param});
	}

	@Override
	public boolean callBoolean(String functionname, Object[] params) throws NoSuchMethodException {
		Object obj = call(functionname, params);
		if (obj instanceof Boolean) return (Boolean) obj; 
		if (obj instanceof Integer) return (Integer)obj == 1;
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
	public final String getLanguageId() {
		return this.languageId;
	}

	@Override
	public final String getSourcecode() {
		return sourceCode;
	}

}
