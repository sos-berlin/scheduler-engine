package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file InvalidScriptException.java
 * \brief Exception if the script code is not valid 
 *  
 * \class InvalidScriptException
 * \brief Exception if the script code is not valid 
 * 
 * \details
 * This exception will thrown if the script code is empty or not valid.
 *
 * \author ss
 * \version 1.0 - 18.01.2011 11:15:13
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class InvalidScriptException extends RuntimeException {

    private static final long serialVersionUID = 1245686761274588736L;
    private String script;

    public InvalidScriptException (Exception e, String message, String scriptcode) {
        super(message);
        this.script = scriptcode;
        String[] sa = this.script.split("\n");
        e.printStackTrace();
        for (int i = 0; i < sa.length; i++) {
            System.err.println((i+1) + ": " + sa[i]);
        }
    }

    public InvalidScriptException (Exception e, String message) {
        super(message);
        e.printStackTrace();
    }

    public InvalidScriptException (String message) {
        super(message);
    }
    
    public String getScriptcode() {
        return this.script;
    }
    
}
