package com.sos.scheduler.engine.kernel.util;

/**
 *
 * @author Zschimmer.sos
 */
public abstract class StringValue {
    private final String string;


    protected StringValue(String x) {
        if (x == null)  throw new NullPointerException(getClass().toString());
        string = x;
    }


    public final String getString() {
        return string;
    }

    
    public final boolean isEmpty() {
        return string.isEmpty();
    }


    @Override public final boolean equals(Object o) {
        if (o == null)  return false;
        if (getClass() == o.getClass())  return string.equals(((StringValue)o).string);
        if (o.getClass() == String.class)  return string.equals((String)o);
        return false;
    }


    @Override public final int hashCode() {
        return string.hashCode();
    }


    @Override public String toString() { 
        return getString();
    }
}
