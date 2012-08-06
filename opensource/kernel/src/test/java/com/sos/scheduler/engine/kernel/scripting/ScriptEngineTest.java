package com.sos.scheduler.engine.kernel.scripting;

import org.junit.Test;

import javax.script.*;

import static junit.framework.Assert.assertEquals;

public class ScriptEngineTest {
	private final ScriptEngine engine = new ScriptEngineManager().getEngineByName("js");

    @Test public void testBinding() {
        Bindings bindings = engine.getBindings(ScriptContext.ENGINE_SCOPE);
        bindings.put("name", "hello");
        assertEquals("hello", engine.get("name"));
    }

	@Test public void testFunction() throws ScriptException, NoSuchMethodException {
        engine.eval("function add (a, b) { var c = a + b; return c; }");
		Invocable invocable = (Invocable)engine;
        assertEquals(15.0, invocable.invokeFunction("add", 10, 5));

        IntegerAdder adder = invocable.getInterface(IntegerAdder.class);
        assertEquals(15, adder.add(10, 5));
	}

	private interface IntegerAdder {
	    int add(int a, int b);
	}
}
