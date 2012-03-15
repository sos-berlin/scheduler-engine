package com.sos.scheduler.engine.kernel.plugin;

import com.google.common.collect.ImmutableCollection;
import com.sos.scheduler.engine.kernel.command.Command;
import com.sos.scheduler.engine.kernel.command.SingleCommandXmlParser;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.w3c.dom.Element;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.elementsXPath;

class PluginCommandCommandXmlParser extends SingleCommandXmlParser {
    private final PluginSubsystem subsystem;

    PluginCommandCommandXmlParser(PluginSubsystem subsystem) {
        super("plugin.command");
        this.subsystem = subsystem;
    }

    @Override public final Command parse(Element e) {
        String className = e.getAttribute("plugin_class");
        PluginAdapter a = subsystem.pluginAdapterByClassName(className);
        Command subcommand = a.getCommandDispatcher().parse(singleSubcommandElement(e));
        return new PluginCommandCommand(className, subcommand);
    }

    private static Element singleSubcommandElement(Element commandElement) {
        ImmutableCollection<Element> childElements = elementsXPath(commandElement, "*");
        if (childElements.size() != 1)
            throw new NotASingleSubcommandException(commandElement.getNodeName());
        return childElements.iterator().next();
    }

    static final class NotASingleSubcommandException extends SchedulerException {
        private NotASingleSubcommandException(String cmdName) {
            super("Command " + cmdName + " has not exactly one subcommand");
        }
    }
}
