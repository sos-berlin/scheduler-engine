package com.sos.scheduler.engine.kernel.scripting;

public interface APIModule {
    public Object call(String name) throws NoSuchMethodException;
    public boolean nameExists(String name);
    public void addObject(Object object, String name);
}
