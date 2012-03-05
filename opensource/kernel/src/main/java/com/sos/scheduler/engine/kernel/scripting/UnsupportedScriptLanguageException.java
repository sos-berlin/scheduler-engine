package com.sos.scheduler.engine.kernel.scripting;

/**
 * Exception for invalid script language keys
 * This exception will thrown if the key for the script language to use is not supported.
 * 
 * @author ss
 * @version 1.0 - 18.01.2011 11:26:01
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class UnsupportedScriptLanguageException extends Exception {

    private static final long serialVersionUID = 1245686761274577736L;

    public UnsupportedScriptLanguageException (Exception e, String message) {
        super(message);
        System.err.println(getMessage());
    }

}
