package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.command.CommandExecutor;
import com.sos.scheduler.engine.kernel.command.CommandSuite;
import com.sos.scheduler.engine.kernel.command.HasCommandSuite;
import com.sos.scheduler.engine.kernel.command.ResultXmlizer;
import com.sos.scheduler.engine.kernel.command.XmlCommandParser;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import java.util.HashMap;
import java.util.Map;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.Util.stackTrace;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;

//TODO Eigenes PrefixLog einführen

public class PlugInSubsystem extends AbstractHasPlatform implements Subsystem, HasCommandSuite {
    private static final String staticFactoryMethodName = "factory";
    private final Scheduler scheduler;
    private final Map<String,PluginAdapter> plugIns = new HashMap<String,PluginAdapter>();
    private final CommandSuite commandSuite = new CommandSuite(
        new CommandExecutor[]{ new PlugInCommandExecutor(this) },
        new XmlCommandParser[]{ new PlugInCommandParser() },
        new ResultXmlizer[]{ new PlugInResultXmlizer() });

    
    public PlugInSubsystem(Scheduler scheduler) {
        super(scheduler.getPlatform());
        this.scheduler = scheduler;
    }


    public void load(Element root) {
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
            PluginAdapter p = new PluginAdapter(newPlugIn(className, elementOrNull), className, log());
            plugIns.put(className, p);
            log().info(p + " added");
        } catch (Exception x) {
            logError(log(), "Plug-in " + className, x);
        }
    }


    private PlugIn newPlugIn(String className, Element elementOrNull) throws Exception {
        Class<?> c = Class.forName(className);
        PlugInFactory f = (PlugInFactory)c.getMethod(staticFactoryMethodName).invoke(null);
        return f.newInstance(scheduler, elementOrNull);
    }


    public void activate() {
        for (PluginAdapter p: plugIns.values())  p.tryActivate();
    }


    public void close() {
        for (PluginAdapter p: plugIns.values())  p.tryClose();
    }


    PluginAdapter plugInByClassName(String className) {
        PluginAdapter result = plugIns.get(className);
        if (result == null)  throw new SchedulerException("Unknown plugin '" + className + "'");
        return result;
    }


    public XmlCommandParser getXmlCommandParser() {
        return new PlugInCommandParser();
    }


    @Override public CommandSuite getCommandSuite() {
        return commandSuite;
    }


    public static void logError(PrefixLog log, String plugInName, Throwable t) {
        log.error(plugInName + ": " + t + "\n" + stackTrace(t));
    }
}
