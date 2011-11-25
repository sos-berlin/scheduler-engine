package com.sos.scheduler.engine.tests.excluded.ss.js606;

import java.util.HashMap;
import java.util.Map;

public class Environment extends HashMap<String, String> {

	private static final long serialVersionUID = -8156620842146208406L;

	// singleton
	private static Environment env;

	public static synchronized Environment getInstance(Map<String,String> personalEnvironment) {
		if (env == null) {
			HashMap<String,String> e = new HashMap<String,String>(System.getenv());
			e.putAll(personalEnvironment);
			env = new Environment(e);
		}
		return env;
	}

	private Environment(Map<String, String> copy) {
		super(copy);
	}

}
