package com.sos.scheduler.engine.kernel.scripting;

public interface Module {
    public Object call(String name) throws NoSuchMethodException;
    public Object call(String rawfunctionname, Object[] params) throws NoSuchMethodException;
    public Object call();
    public boolean callBoolean(String name) throws NoSuchMethodException;
    public boolean callBoolean(String name, Object[] params) throws NoSuchMethodException;
    public String callString(String name) throws NoSuchMethodException;
    public String callString(String name, Object[] params) throws NoSuchMethodException;
    public double callDouble(String name) throws NoSuchMethodException;
    public double callDouble(String name, Object[] params) throws NoSuchMethodException;
    public void addObject(Object object, String name);
//    public void log(String message);
    public String getLanguageId();
	public String getSourcecode();
	public ScriptFunction getLastFunction();
}
