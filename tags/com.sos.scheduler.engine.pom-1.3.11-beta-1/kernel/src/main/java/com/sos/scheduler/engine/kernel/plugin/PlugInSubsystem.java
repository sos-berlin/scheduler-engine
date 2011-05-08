package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Scheduler;
import java.util.ArrayList;
import java.util.List;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.Util.stackTrace;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;

//TODO Eigenes PrefixLog einführen

public class PlugInSubsystem extends AbstractHasPlatform {
    private static final String staticFactoryMethodName = "factory";
    
    private final Scheduler scheduler;
    private final List<PlugInAdapter> plugIns = new ArrayList<PlugInAdapter>();

    
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
            PlugInAdapter p = new PlugInAdapter(newPlugIn(className, elementOrNull), className, log());
            plugIns.add(p);
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
        for (PlugInAdapter p: plugIns)  p.tryActivate();
    }


    public void close() {
        for (PlugInAdapter p: plugIns)  p.tryClose();
    }


    public static void logError(PrefixLog log, String plugInName, Throwable t) {
        log.error(plugInName + ": " + t + "\n" + stackTrace(t));
    }
}
