package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.util.StringValue;


public class Path extends StringValue {
    public Path(String p) {
        super(p);
    }


    public void assertIsAbsolute() {
        if (!isAbsolute())  throw new SchedulerException("Absolute path expected: " + this);
    }


    public void assertIsEmptyOrAbsolute() {
        boolean ok = isEmpty() || isAbsolute();
        if (!ok)  throw new SchedulerException("Absolute path expected: " + this);
    }


    public boolean isAbsolute() {
        return string().startsWith("/");
    }
}
