package com.sos.scheduler.engine.kernel.util;


public final class Util {
    private Util() {}

    
    public static void throwUnchecked(Throwable x) {
        if (x instanceof Error)  throw (Error)x;
        if (x instanceof RuntimeException)  throw (RuntimeException)x;
        throw new RuntimeException(x);
    }


    public static String stringOrException(Object o) {
        try {
            return o == null? "null" : o.toString();
        } catch (Throwable x) { return x.toString(); }
    }
}
