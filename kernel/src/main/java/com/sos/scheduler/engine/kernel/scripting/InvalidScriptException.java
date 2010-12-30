package com.sos.scheduler.engine.kernel.scripting;

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
