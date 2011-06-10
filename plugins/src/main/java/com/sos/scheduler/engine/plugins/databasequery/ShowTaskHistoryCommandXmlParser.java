package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Command;
import com.sos.scheduler.engine.kernel.command.SingleCommandXmlParser;
import org.w3c.dom.Element;


public class ShowTaskHistoryCommandXmlParser extends SingleCommandXmlParser {
    public static final ShowTaskHistoryCommandXmlParser singleton = new ShowTaskHistoryCommandXmlParser();


    public ShowTaskHistoryCommandXmlParser() {
        super("showTaskHistory");
    }
    
    
    @Override public final Command parse(Element e) {
        return new ShowTaskHistoryCommand();
    }
}
