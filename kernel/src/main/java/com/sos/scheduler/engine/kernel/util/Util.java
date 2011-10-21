package com.sos.scheduler.engine.kernel.util;

import static com.google.common.base.Strings.isNullOrEmpty;

import java.io.PrintWriter;
import java.io.StringWriter;


public final class Util {
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

    @SuppressWarnings("unused")
    public static void unused(Object unusedVariable) {}
}
