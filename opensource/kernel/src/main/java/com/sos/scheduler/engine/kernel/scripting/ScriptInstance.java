package com.sos.scheduler.engine.kernel.scripting;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.script.*;

import java.util.regex.Pattern;

import static com.google.common.base.Throwables.propagate;
import static javax.script.ScriptContext.ENGINE_SCOPE;

/**
 * General wrapper for the javax.script interface
 *
 * This class provides a general mechanism to call script in different languages.
 *    
 * {@code
ScriptInstance module = new ScriptInstance("javascript");
module.setSourceCode("print('Hello ' + name + '\\n');");
module.addObject("nick", "name");
module.call();
 * }
 */
public class ScriptInstance {
    private static final String languagePrefix = "javax.script:";
    private static final Logger logger = LoggerFactory.getLogger(ScriptInstance.class);

    private final ScriptEngine engine;

    public ScriptInstance(String language) {
        this.engine = newScriptEngine(normalizeLanguageName(language));
    }

    private static String normalizeLanguageName(String language) {
        return language.replaceFirst("^" + Pattern.quote(languagePrefix), "");   //.toLowerCase();
    }

    private static ScriptEngine newScriptEngine(String language) {
        ScriptEngine result = new ScriptEngineManager().getEngineByName(language);
        if (result == null) throw throwUnknownLanguage(language);
        if (logger.isDebugEnabled())
            logger.debug(result.getFactory().getEngineName() +" "+ result.getFactory().getEngineVersion());
        return result;
    }

    private static RuntimeException throwUnknownLanguage(String language) {
        String availableLanguages = Joiner.on(", ").join(new ScriptEngineManager().getEngineFactories());
        throw new SchedulerException("Script language "+ language +" is unknown. Available languages are "+availableLanguages);
    }

    public void loadScript(ImmutableMap<String,Object> bindingMap, String script) {
        try {
            for (ImmutableMap.Entry<String,Object> e: bindingMap.entrySet())
                engine.put(e.getKey(), e.getValue());
            engine.eval(script);
        }
        catch (ScriptException e) { throw propagate(e); }
    }

    public boolean callBooleanWhenExists(String name, boolean defaultResult) {
        try {
            return callBooleanWithDefault(name, defaultResult);
        } catch (NoSuchMethodException e) {
            logger.debug(e +", method="+ name);
            return defaultResult;
        }
    }

    public boolean callBooleanWithDefault(String functionName, boolean deflt) throws NoSuchMethodException {
        return resultToBoolean(call(functionName), deflt);
    }

    private static boolean resultToBoolean(Object result, boolean deflt) {
        if (result instanceof Boolean) return (Boolean)result;
        else
        if (result == null) return deflt;
        else
        //if (result instanceof Integer) return (Integer)result != 0;
        throw new RuntimeException("The function has not returned a Boolean: "+ result);
    }

    public void callWhenExists(String name) {
        try {
            call(name);
        } catch (NoSuchMethodException e) {
            logger.trace(e +", function="+name);
        }
    }

    public Object call(String functionName, Object... parameters) throws NoSuchMethodException {
		try {
			logger.trace("Call function " + functionName);
			Invocable invocableEngine = (Invocable)engine;
			Object result = invocableEngine.invokeFunction(functionName, parameters);
            logger.trace("Result is " + result);
            return (Boolean)result;
        }
        catch (ScriptException e) { throw propagate(e); }
    }

    public void close() {
        engine.setBindings(engine.createBindings(), ENGINE_SCOPE);
    }
}
