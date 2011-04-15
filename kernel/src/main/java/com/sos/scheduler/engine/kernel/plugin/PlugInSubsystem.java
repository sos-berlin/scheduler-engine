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
    private final List<PlugInAdapter> plugIns = new ArrayList<PlugInAdapter>();

    
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
            plugIns.add(new PlugInAdapter(p, className, log()));
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
        for (PlugInAdapter p: plugIns)  p.tryActivate();
    }


    public void close() {
        for (PlugInAdapter p: plugIns)  p.tryClose();
    }
}
