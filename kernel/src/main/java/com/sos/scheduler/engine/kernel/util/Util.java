package com.sos.scheduler.engine.kernel.util;

import static com.google.common.base.Strings.isNullOrEmpty;

import java.io.PrintWriter;
import java.io.StringWriter;

import org.apache.log4j.Logger;

public final class Util {
    private static final Logger logger = Logger.getLogger(Util.class);

    private Util() {}

    public static void throwUnchecked(Throwable x) {
        if (x instanceof Error)  throw (Error)x;
        if (x instanceof RuntimeException)  throw (RuntimeException)x;
        throw new WrappedThrowable(x);
    }

    public static String stringOrException(Object o) {
        try {
            return o == null? "null" : o.toString();
        } catch (Throwable x) { return x.toString(); }
    }

    public static String stackTrace(Throwable t) {
        //TODO Durch Guava ersetzbar?
        StringWriter s = new StringWriter();
        t.printStackTrace(new PrintWriter(s));
        return s.toString();
    }

    private static final class WrappedThrowable extends RuntimeException {
        private WrappedThrowable(Throwable t) {
            super(t);
        }
    }

    public static boolean booleanOf(String s, boolean nullDefault, boolean emptyDefault) {
        return s == null? nullDefault : booleanOf(s, emptyDefault);
    }

    public static boolean booleanOf(String s, boolean deflt) {
        if (isNullOrEmpty(s))  return deflt;
        else
        if (s.equals("true")) return true;
        else
        if (s.equals("false")) return false;
        else
            throw new RuntimeException("Invalid boolean value: " + s);
    }


    public static void ignore(@SuppressWarnings("unused") Object unusedVariable) {}

    /** @return true, wenn {@link InterruptedException} abgefangen worden ist. */
    public static boolean sleepUntilInterrupted(long ms) {
        try {
            Thread.sleep(ms);
            return false;
        } catch (InterruptedException x) {
            logger.trace(x, x);
            return true;
        }
    }
}
