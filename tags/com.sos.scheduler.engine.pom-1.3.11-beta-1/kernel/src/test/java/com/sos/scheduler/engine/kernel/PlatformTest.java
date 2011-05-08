package com.sos.scheduler.engine.kernel;

import java.io.StringWriter;
import org.junit.Test;


public class PlatformTest
{
    private final StringWriter logBuffer = new StringWriter();
    private final Platform platform = PlatformMock.newInstance(logBuffer);

    @Test public void testDummy() {
        String line = "Zeile";
        platform.log().info(line);
        assert(logBuffer.toString().contains(line));
    }
}
