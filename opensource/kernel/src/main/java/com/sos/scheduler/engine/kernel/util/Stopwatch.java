package com.sos.scheduler.engine.kernel.util;

import static java.lang.System.currentTimeMillis;

public class Stopwatch {
    private static final double ms = 0.001;
    private final long startMs = currentTimeMillis();

    public final long elapsedMs() {
        return currentTimeMillis() - startMs;
    }

    @Override public String toString() {
        return elapsedMs()*ms + "s";
    }
}
