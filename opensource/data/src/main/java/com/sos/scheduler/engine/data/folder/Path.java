package com.sos.scheduler.engine.data.folder;

import com.sos.scheduler.engine.data.base.StringValue;
import org.codehaus.jackson.annotate.JsonIgnore;

public class Path extends StringValue {
    public Path(String p) {
        super(p);
    }

    public final void assertIsAbsolute() {
        if (!isAbsolute())  throw new RuntimeException("Absolute path expected: " + this);
    }

    public final void assertIsEmptyOrAbsolute() {
        boolean ok = isEmpty() || isAbsolute();
        if (!ok)  throw new RuntimeException("Absolute path expected: " + this);
    }

    @JsonIgnore
    public final boolean isAbsolute() {
        return asString().startsWith("/");
    }

    @Override public boolean equals(Object o) {
        // Path und AbsolutePath sind vergleichbar
        return o instanceof Path && asString().equals(((Path)o).asString());
    }
}
