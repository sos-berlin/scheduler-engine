package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Command;
import com.sos.scheduler.engine.kernel.command.SingleCommandXmlParser;
import org.w3c.dom.Element;

class ShowTaskHistoryCommandXmlParser extends SingleCommandXmlParser {
    public static final ShowTaskHistoryCommandXmlParser singleton = new ShowTaskHistoryCommandXmlParser();

    ShowTaskHistoryCommandXmlParser() {
        super("showTaskHistory");
    }

    @Override public final Command parse(Element e) {
        int limit = intAttribute(e, "limit", ShowTaskHistoryCommand.defaultLimit);
        return new ShowTaskHistoryCommand(limit);
    }

    private static int intAttribute(Element e, String name, int deflt) {
        String a = e.getAttribute(name);
        return a.isEmpty()? deflt : Integer.parseInt(a);
    }
}
