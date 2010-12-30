/**
 * \file ModuleInterface.java
 * \brief Interface f�r die Implementierung von Scriptsprachen
 *  
 * \class ModuleInterface 
 * \brief Interface f�r die Implementierung von Scriptsprachen
 * 
 * \details
 * Mit dem package javax.script steht eine allgeimeine Implementierung f�r Scriptsprachen in Java zur Verf�gung.
 * Dieses Interface bildet die Beschreibung einer allgemeing�ltigen Schnittstelle f�r alle Scriptsprachen zum
 * Jobscheduler.
 * 
 * \author Stefan Sch�dlich
 * \version 1.0 - 2010-06-03
 * <div class="sos_branding">
 *   <p>� 2010 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
package com.sos.scheduler.engine.kernel.scripting;

public interface ModuleInterface  {
    public void addObject(Object object, String name);    // f�gt dem Script ein Objekt (eine Variable) hinzu
    public boolean callBoolean(String name);
    public String callString(String name);
    public double callDouble(String name);
    public Object call(String name);
    public void call();
    public boolean nameExists(String name);                // pr�ft das Vorhandensein einer Methode
}
