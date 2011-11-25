package com.sos.scheduler.engine.tests.excluded.ss.js606;

import java.util.HashMap;

public class JS606Environment {

	private static Environment js606Env;

	public static final synchronized Environment getInstance(String variableNamePrefix) {
		if (js606Env == null) 
			new JS606Environment(variableNamePrefix);
		return js606Env;
	}

	private JS606Environment(String variableNamePrefix) {
		HashMap<String,String> e = new HashMap<String,String>();
		e.put("SCHEDULER_VARIABLE_NAME_PREFIX", variableNamePrefix);
		js606Env = Environment.getInstance(e);
	}

}
