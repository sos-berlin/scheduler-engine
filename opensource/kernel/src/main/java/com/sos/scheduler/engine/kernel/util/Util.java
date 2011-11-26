package com.sos.scheduler.engine.kernel.util;

import static com.google.common.base.Strings.isNullOrEmpty;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.google.common.collect.ImmutableList;

public final class Util {
    private static final Logger logger = Logger.getLogger(Util.class);

    private Util() {}

    public static String stringOrException(Object o) {
        try {
            return o == null? "null" : o.toString();
        } catch (Throwable x) { return x.toString(); }
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

    public static <T> ImmutableList<T> optional(@Nullable T o) {
        if (o == null)
            return ImmutableList.of();
        else
            return ImmutableList.of(o);
    }
}
