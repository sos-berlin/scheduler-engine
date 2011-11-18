package com.sos.scheduler.engine.test.excluded;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class LoggerTest {
	
    private static Logger logger = LoggerFactory.getLogger(LoggerTest.class);

    @Test public void test1() throws Exception {
        try {
        	logger.debug("debug");
        	logger.info("info");
        	logger.warn("warn");
        	logger.error("error");
        }
        catch (Exception x) {
            logger.debug("OK: " + x);
        }
    }
}
