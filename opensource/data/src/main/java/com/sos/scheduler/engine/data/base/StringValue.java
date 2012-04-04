package com.sos.scheduler.engine.data.base;

public abstract class StringValue {
    private final String string;

    protected StringValue(String x) {
        if (x == null)  throw new NullPointerException(getClass().toString());
        string = x;
    }

    public final String asString() {
        return string;
    }

    public final boolean isEmpty() {
        return string.isEmpty();
    }

    @Override public boolean equals(Object o) {
        return o != null && getClass() == o.getClass() && string.equals(((StringValue)o).string);
    }

    @Override public final int hashCode() {
        return string.hashCode();
    }

    @Override public String toString() { 
        return asString();
    }
}
