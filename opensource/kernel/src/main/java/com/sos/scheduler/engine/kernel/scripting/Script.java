package com.sos.scheduler.engine.kernel.scripting;

/**
 * General Interface for javax.script package
 *  
 * This interface should be used from classes which implements a layer for
 * scripting via the javax.script package.
 */
public interface Script {
    Object call(String name) throws NoSuchMethodException;
    Object call(String rawfunctionname, Object[] params) throws NoSuchMethodException;
    Object call(String rawfunctionname, boolean param) throws NoSuchMethodException;
    Object call();
    boolean callBoolean(String name) throws NoSuchMethodException;
    boolean callBoolean(String name, Object[] params) throws NoSuchMethodException;
    String callString(String name) throws NoSuchMethodException;
    String callString(String name, Object[] params) throws NoSuchMethodException;
    double callDouble(String name) throws NoSuchMethodException;
    double callDouble(String name, Object[] params) throws NoSuchMethodException;
    void addObject(Object object, String name);
    String getLanguageId();
	String getSourcecode();
}
