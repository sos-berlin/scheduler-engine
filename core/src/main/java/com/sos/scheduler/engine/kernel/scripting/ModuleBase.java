package com.sos.scheduler.engine.kernel.scripting;

import javax.script.Bindings;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;

import sos.spooler.Log;

public abstract class ModuleBase {
    
    private final String sourceCode;
    private final String languageId;
    private final String rawLanguageId;
    private Log log = null;
    
    ScriptEngineManager scriptman;;
    ScriptEngine scriptengine;
    Bindings scriptbindings;
    
    public ModuleBase(String scriptlanguage, String sourcecode) {
        rawLanguageId = scriptlanguage;
        languageId = scriptlanguage;
        sourceCode = sourcecode;
        try {
	        scriptman = new ScriptEngineManager();
	        scriptengine = scriptman.getEngineByName(getLanguageId());
	        scriptbindings = scriptengine.createBindings();
        } catch (Exception e) {
        	throw new UnsupportedScriptLanguageException(e, "Scriptlanguage " + getLanguageId() + " is not supported.", rawLanguageId );
        }
    }
    
    public void addObject(Object object, String name) {
        if (object instanceof Log) {
            log = (Log)object;
            log("scriptlanguage is " + getLanguageId() + " (" + rawLanguageId + ")");
        }
        log("addObject " + name + " = " + object.toString());
        scriptbindings.put(name, object);
    }

    public void call() {
    	String code = getSourcecode();
        try {
        	if (code != null) {
        		scriptengine.eval(code,scriptbindings);
        	}
        } catch (ScriptException e) {
            throw new InvalidScriptException(e, "error in script",code);
        }
    }

    public String getLanguageId() {
        return this.languageId;
    }

	public String getSourcecode() {
		return sourceCode;
	}

    public void log(String message) {
        String output = this.getClass().getName() + " - " + message;
        if (log == null)
            System.out.println(output);
        else
            log.debug(output);
    }
}
