package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.SingleCommandXmlParser;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.command.Command;
import java.util.Collection;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


class PlugInCommandCommandXmlParser extends SingleCommandXmlParser {
    private final PlugInSubsystem subsystem;


    PlugInCommandCommandXmlParser(PlugInSubsystem subsystem) {
        super("plugin.command");
        this.subsystem = subsystem;
    }


    @Override public final Command parse(Element e) {
        String className = e.getAttribute("plugin_class");
        CommandPluginAdapter a = subsystem.commandPluginByClassName(className);
        Command subcommand = a.getCommandDispatcher().parse(singleSubcommandElement(e));
        return new PlugInCommandCommand(className, subcommand);
    }


    private Element singleSubcommandElement(Element commandElement) {
        Collection<Element> childElements = elementsXPath(commandElement, "*");
        if (childElements.size() != 1)
            throw new NotASingleSubcommandException(commandElement.getNodeName());
        return childElements.iterator().next();
    }


    static class NotASingleSubcommandException extends SchedulerException {
        private NotASingleSubcommandException(String cmdName) {
            super("Command " + cmdName + " has not exactly one subcommand");
        }
    }
}
