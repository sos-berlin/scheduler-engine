package com.sos.scheduler.engine.kernel.scripting;

import com.sos.scheduler.engine.kernel.scripting.ScriptFunction;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;


public class ScriptFunctionTest {

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
	}

	@Test
	public void functionTest() {
		ScriptFunction function = new ScriptFunction("myFunction");
		printOut(function);
	}

	@Test
	public void functionWithBracesTest() {
		ScriptFunction function = new ScriptFunction("myFunction()");
		printOut(function);
	}

	@Test
	public void functionWithTypeTest() {
		ScriptFunction function = new ScriptFunction("myFunction()V");
		printOut(function);
	}
	
	private void printOut(ScriptFunction f) {
		System.out.println("functionname: " + f.getFunctionName());
		System.out.println("native functionname: " + f.getNativeFunctionName());
		System.out.println("functiontype: " + f.getTypeId());
	}
	
	@AfterClass
	public static void tearDownAfterClass() throws Exception {
	}

}
