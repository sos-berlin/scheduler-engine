package com.sos.scheduler.engine.kernel.scripting;

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
