package com.sos.scheduler.engine.kernel.scripting;


import org.junit.BeforeClass;
import org.junit.Test;

import javax.script.*;

import static junit.framework.Assert.assertEquals;

public class InvokeFunctionTest {

	private static ScriptEngineManager manager;
	private static ScriptEngine engine;
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		manager = new ScriptEngineManager();
		engine = manager.getEngineByName("js");
	}
	
	@Test
	public void invokeFunction() throws ScriptException, NoSuchMethodException {
		Bindings bindings = engine.getBindings(ScriptContext.ENGINE_SCOPE);
        bindings.put("parm1", "hello");
        assertEquals("hello",engine.get("parm1"));
        
        engine.eval("function add (a, b) {c = a + b; return c; }", bindings);
		Invocable jsInvoke = (Invocable) engine;

		Double result1 = (Double)jsInvoke.invokeFunction("add", new Object[] { 10, 5 });
        assertEquals(15, result1.intValue());

		Adder adder = jsInvoke.getInterface(Adder.class);
		int result2 = adder.add(10, 5);
        assertEquals(15, result2);
	}

	interface Adder {
	  int add(int a, int b);
	}

}
