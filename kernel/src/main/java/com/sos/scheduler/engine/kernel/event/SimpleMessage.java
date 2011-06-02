package com.sos.scheduler.engine.kernel.event;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableList;


public class SimpleMessage implements Message {
    private final String code;
    private final ImmutableList<String> insertions;


    public SimpleMessage(String code, Object... insertions) {
        this.code = code;
        this.insertions = listOfInsertions(insertions);
    }


    private static ImmutableList<String> listOfInsertions(Object[] insertions) {
        ImmutableList.Builder<String> result = new ImmutableList.Builder<String>(); //insertions.length);
        for (Object o: insertions)  result.add(failsafeToString(o));
        return result.build();
    }

    
    @Override public String getCode() { return code; }

    
    @Override public String toString() { 
        return insertions.isEmpty()? code : code + " " + Joiner.on(", ").join(insertions);
    }


    private static String failsafeToString(Object o) {
        try {
            return o.toString();
        } catch (Throwable x) {
            return "[" + o.getClass().getName() + ": ERROR " + x + "]";
        }
    }
}
