package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Scheduler;
import java.util.ArrayList;
import java.util.List;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


public class PlugInSubsystem extends AbstractHasPlatform {
    private static final String staticFactoryMethodName = "factory";
    
    private final Scheduler scheduler;
    private final List<PlugIn> plugIns = new ArrayList<PlugIn>();

    
    public PlugInSubsystem(Scheduler scheduler) {
        super(scheduler.getPlatform());
        this.scheduler = scheduler;
    }


    public void load(Element root) {
        for (Element e: elementsXPath(root, "config/plugin"))
            tryAddPlugIn(e);
    }


    private void tryAddPlugIn(Element e) {
        String className = e.getAttribute("java_class");
        if (className.isEmpty())  throw new SchedulerException("Missing attribute java_class in <plugin>");
        tryAddPlugIn(className, e);
    }


    private void tryAddPlugIn(String className, Element e) {
        try {
            PlugIn p = newPlugIn(className, e);
            plugIns.add(p);
            log().info(p + " added");
        } catch (Exception x) {
            log().error("Plug-in " + className + ": " + x.toString());
        }
    }


    private PlugIn newPlugIn(String className, Element e) throws Exception {
        Class<?> c = Class.forName(className);
        PlugInFactory f = (PlugInFactory)c.getMethod(staticFactoryMethodName).invoke(null);
        return f.newInstance(scheduler, e);
    }


    public void activate() {
        for (PlugIn p: plugIns)  tryActivatePlugIn(p);
    }


    private void tryActivatePlugIn(PlugIn p) {
        try {
           p.activate();
           log().info(p + " activated");
        } catch (Exception x) {
            log().error(p + ": " + x);
        }
    }


    public void close() {
        for (PlugIn p: plugIns)  tryClosePlugIn(p);
    }


    private void tryClosePlugIn(PlugIn p) {
        try {
           p.close();
           log().info(p + " closed");
        } catch (Exception x) {
            log().error(x.toString());
        }
    }
}
