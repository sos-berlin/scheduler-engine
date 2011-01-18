package com.sos.scheduler.engine.kernel.scripting;

/**
 * \file UnsupportedScriptLanguageException.java
 * \brief Exception for invalid script language keys 
 *  
 * \class UnsupportedScriptLanguageException
 * \brief Exception for invalid script language keys 
 * 
 * \details
 * This exception will thrown if the key for the script language to use is not supported. 
 * 
 * \author ss
 * \version 1.0 - 18.01.2011 11:26:01
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class UnsupportedScriptLanguageException extends RuntimeException {

    private static final long serialVersionUID = 1245686761274577736L;
    private String scriptlanguage;

    public UnsupportedScriptLanguageException (Exception e, String message, String scriptlanguage) {
        super(message);
        this.scriptlanguage = scriptlanguage;
        System.err.println(getScriptlanguage() + ": " + getMessage());
    }
    
    public String getScriptlanguage() {
        return this.scriptlanguage;
    }
    
}
