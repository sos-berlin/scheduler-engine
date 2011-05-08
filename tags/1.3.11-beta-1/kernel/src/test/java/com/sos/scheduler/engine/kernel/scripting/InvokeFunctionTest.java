package com.sos.scheduler.engine.kernel.scripting;


import javax.script.Bindings;
import javax.script.Invocable;
import javax.script.ScriptContext;
import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

public class InvokeFunctionTest {

	private static ScriptEngineManager manager;
	private static ScriptEngine engine;
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		manager = new ScriptEngineManager();
		engine = manager.getEngineByName("js");
	}
	
	@Test
	public void invokeFuntion() throws ScriptException, NoSuchMethodException {
		Bindings bindings = engine.getBindings(ScriptContext.ENGINE_SCOPE);
        bindings.put("parm1", "hello");
        
        engine.eval("function add (a, b) {c = a + b; return c; }", bindings);
		Invocable jsInvoke = (Invocable) engine;

		Object result1 = jsInvoke.invokeFunction("add", new Object[] { 10, 5 });
		System.out.println(result1);

		Adder adder = jsInvoke.getInterface(Adder.class);
		int result2 = adder.add(10, 5);
		System.out.println(result2);
	}

	@AfterClass
	public static void tearDownAfterClass() throws Exception {
	}


	interface Adder {
	  int add(int a, int b);
	}

}
