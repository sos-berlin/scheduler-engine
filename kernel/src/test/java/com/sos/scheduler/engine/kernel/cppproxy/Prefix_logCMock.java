package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import java.io.IOException;
import java.io.Writer;


/**
 *
 * @author Zschimmer.sos
 */
public class Prefix_logCMock extends CppProxyImpl<Sister> implements Prefix_logC {
    private final Writer writer;

    public Prefix_logCMock(Writer w) { 
        writer = w;
    }

    @Override public boolean cppReferenceIsValid() { 
        return true;
    }

    @Override public void error(String line) { 
        logLine("ERROR", line);
    }

    @Override public void warn(String line) { 
        logLine("WARN", line);
    }

    @Override public void info(String line) { 
        logLine("info", line);
    }

    @Override public void debug3(String line) { 
        logLine("debug3", line);
    }

    private void logLine(String level, String line) {
        try {
            writer.write(level + " " + line + "\n" );
        } catch(IOException x) { throw new RuntimeException(x); }
    }
}
