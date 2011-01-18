package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file APIModule.java
 * \brief interface for an API script job from the jobscheduler 
 *  
 * \class APIModule
 * \brief interface for an API script job from the jobscheduler 
 * 
 * \details
 * A class for supporting the script API of the JobScheduler has to implement this interface.
 *
 * \author SS
 * \version 1.0 - 18.01.2011 11:06:09
 * <div class="sos_branding">
 *   <p>© 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public interface APIModule {
    public Object call(String name) throws NoSuchMethodException;
    public boolean nameExists(String name);
    public void addObject(Object object, String name);
}
