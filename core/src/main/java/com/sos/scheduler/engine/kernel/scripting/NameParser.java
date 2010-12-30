package com.sos.scheduler.engine.kernel.scripting;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class NameParser {

    private String functionName = "";
    private String scriptLanguage;
    private String typeId = "";
    
    public NameParser(String fullname, String scriptLanguage) {
        this.scriptLanguage = scriptLanguage.toLowerCase();
        Pattern p = Pattern.compile("^(.*)(.)$");
        Matcher m = p.matcher(fullname);
        if (m.matches()) {
            functionName = m.group(1);
            typeId = m.group(2);
        } else {
            functionName = fullname;
            typeId = "V";                    // void
        }
    }
    
    public String getFunctionName() {
        return functionName;
    }
    
    public String getNativeFunctionName() {
        return functionName.replaceAll("\\(.*\\)", "");
    }
    
    public String getFunctionCall() {
        String eol = (scriptLanguage.equals("javascript"))  ? ";" : "";
        return functionName + eol;
    }
    
    public String getTypeId() {
        return typeId;
    }

}
