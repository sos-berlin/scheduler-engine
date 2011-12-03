package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.util.StringValue;

public class Path extends StringValue {
    public Path(String p) {
        super(p);
    }

    public final void assertIsAbsolute() {
        if (!isAbsolute())  throw new SchedulerException("Absolute path expected: " + this);
    }

    public final void assertIsEmptyOrAbsolute() {
        boolean ok = isEmpty() || isAbsolute();
        if (!ok)  throw new SchedulerException("Absolute path expected: " + this);
    }

    public final boolean isAbsolute() {
        return getString().startsWith("/");
    }

    public boolean equals(Object o) {
        // Path und AbsolutePath sind vergleichbar
        return o instanceof Path && getString().equals(((Path)o).getString());
    }
}
