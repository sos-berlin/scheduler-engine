package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file Script.java
 * \brief General Interface for javax.script package 
 *  
 * \class Script
 * \brief General Interface for javax.script package 
 * 
 * \details
 * This interface should be used from classes which implements a layer for
 * scripting via the javax.script package.
 *
 * \author ss
 * \version 1.0 - 18.01.2011 11:17:54
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public interface Script {
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
    public String getLanguageId();
	public String getSourcecode();
	public ScriptFunction getLastFunction();
}
