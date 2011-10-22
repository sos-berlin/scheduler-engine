package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import org.w3c.dom.Element;

import static com.google.common.base.Throwables.getStackTraceAsString;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static java.util.Arrays.asList;

//TODO Eigenes PrefixLog einführen

public class PlugInSubsystem extends AbstractHasPlatform implements Subsystem, HasCommandHandlers {
    private static final String staticFactoryMethodName = "factory";
    private final Scheduler scheduler;
    private final Map<String,PluginAdapter> plugIns = new HashMap<String,PluginAdapter>();
    private final CommandHandler[] commandHandlers = { 
        new PlugInCommandExecutor(this), 
        new PlugInCommandCommandXmlParser(this),
        new PluginCommandResultXmlizer(this) };

    
    public PlugInSubsystem(Scheduler scheduler) {
        super(scheduler.getPlatform());
        this.scheduler = scheduler;
    }


    public final void load(Element root) {
        Element plugInsElement = elementXPathOrNull(root, "config/plugins");
        if (plugInsElement != null) {
            for (Element e: elementsXPath(plugInsElement, "plugin"))  tryAddPlugIn(e);
        }
    }


    private void tryAddPlugIn(Element e) {
        String className = e.getAttribute("java_class");
        if (className.isEmpty())  throw new SchedulerException("Missing attribute java_class in <plugin>");
        Element contentElement = elementXPathOrNull(e, "plugin.config");
        tryAddPlugIn(className, contentElement);
    }
    

    private void tryAddPlugIn(String className, Element elementOrNull) {
        try {
            PlugIn p = newPlugIn(className, elementOrNull);
            PluginAdapter a = p instanceof CommandPlugin? new CommandPluginAdapter((CommandPlugin)p, className, log())
                : new PluginAdapter(p, className, log());
            plugIns.put(className, a);
            log().info(a + " added");
        } catch (Exception x) {
            logError(log(), "Plug-in " + className, x);
        }
    }


    private PlugIn newPlugIn(String className, Element elementOrNull) throws Exception {
        Class<?> c = Class.forName(className);
        PlugInFactory f = (PlugInFactory)c.getMethod(staticFactoryMethodName).invoke(null);
        return f.newInstance(scheduler, elementOrNull);
    }


    public final void activate() {
        for (PluginAdapter p: plugIns.values())  p.tryActivate();
    }


    public final void close() {
        for (PluginAdapter p: plugIns.values())  p.tryClose();
    }


    final CommandPluginAdapter commandPluginByClassName(String className) {
        PluginAdapter a = pluginByClassName(className);
        if (!(a instanceof CommandPluginAdapter))
            throw new SchedulerException("Plugin is not a " + CommandPlugin.class.getSimpleName());
        return (CommandPluginAdapter)a;
    }


    private PluginAdapter pluginByClassName(String className) {
        PluginAdapter result = plugIns.get(className);
        if (result == null)
            throw new SchedulerException("Unknown plugin '" + className + "'");
        return result;
    }


    @Override public final Collection<CommandHandler> getCommandHandlers() {
        return asList(commandHandlers);
    }


    static void logError(PrefixLog log, String pluginName, Throwable t) {
        log.error(pluginName + ": " + t + "\n" + getStackTraceAsString(t));
    }
}
