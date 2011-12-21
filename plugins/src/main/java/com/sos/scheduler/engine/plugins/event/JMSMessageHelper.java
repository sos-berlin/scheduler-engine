package com.sos.scheduler.engine.plugins.event;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
import javax.jms.Message;

import org.apache.log4j.Logger;

/**
 * \file JMSPlugIn.java
 * \brief JS Plugin to connect the JMS
 *  
 * \class JMSPlugIn
 * \brief JS Plugin to connect the JMS
 * 
 * \details
 * \code
  \endcode
 *
 * \version 1.0 - 12.04.2011 11:54:06
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class JMSMessageHelper {

	private static Logger logger = Logger.getLogger(JMSMessageHelper.class);
	private final Message message;

    
    JMSMessageHelper(Message m) {
    	message = m;
    }
    
    public void setJMSHeaderProperties(Properties props) throws Exception {
    	Iterator<Map.Entry<Object, Object>> it = props.entrySet().iterator();
    	while(it.hasNext()) {
    		Map.Entry<Object,Object> prop = it.next();
    		if (prop.getKey() instanceof String) {
    	    	logger.debug("setProperty " + (String)prop.getKey() + "=" + prop.getValue() );
    			setJMSHeaderProperty(message, (String)prop.getKey(), prop.getValue());
    		}
    	}
    }
    
    public void setJMSHeaderProperty(String propertyName, Object propertyValue) throws IllegalArgumentException, IllegalAccessException, InvocationTargetException {
    	setJMSHeaderProperty(message, propertyName, propertyValue);
    }
    
    public void setJMSProperty(String propertyName, Object propertyValue) throws IllegalArgumentException, IllegalAccessException, InvocationTargetException {
    	setJMSHeaderProperty(message, propertyName, propertyValue);
    }
    
    public static void setJMSHeaderProperty(Message m, String propertyName, Object propertyValue) throws IllegalArgumentException, IllegalAccessException, InvocationTargetException {
    	String searchMethod = "set" + propertyName;
    	try {
        	for(Method method : m.getClass().getMethods() ) {
        		if (method.getName().equals(searchMethod)) {
        			method.invoke(m, propertyValue);
        	    	logger.debug("method invoke: " + searchMethod + "(" + propertyValue + ")");
        		}
        	}
    	} catch(Exception e) {
	    	logger.error("error invoking method " + searchMethod + "(" + propertyValue + "): - " + e.getMessage());
    	}
    }

	public static Properties defaultEventProperties() {
		Properties result = new Properties();
		result.put("JMSPriority", 20);
		return result;
	}
    
}
