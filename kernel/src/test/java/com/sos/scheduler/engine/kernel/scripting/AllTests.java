package com.sos.scheduler.engine.kernel.scripting;

import org.junit.runner.RunWith;
import org.junit.runners.Suite;

@RunWith(Suite.class)
@Suite.SuiteClasses( 
		{ 
			com.sos.scheduler.engine.kernel.scripting.ScriptFunctionTest.class,
			com.sos.scheduler.engine.kernel.scripting.ModuleInstanceTest.class,
			com.sos.scheduler.engine.kernel.scripting.APIModuleInstanceTest.class
		}
)

public class AllTests {

	public AllTests() {
	}

}
