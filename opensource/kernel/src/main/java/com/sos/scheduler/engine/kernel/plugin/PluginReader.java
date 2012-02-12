package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.w3c.dom.Element;

import javax.annotation.Nullable;

import static com.sos.scheduler.engine.kernel.plugin.ActivationMode.activateOnStart;
import static com.sos.scheduler.engine.kernel.plugin.ActivationMode.dontActivate;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.booleanXmlAttribute;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.elementXPathOrNull;

public class PluginReader {
    private final PrefixLog log;

    PluginReader(PrefixLog log) {
        this.log = log;
    }

    @Nullable final PluginAdapter tryReadPlugin(Element e) {
        String className = e.getAttribute("java_class");
        if (className.isEmpty()) throw new SchedulerException("Missing attribute java_class in <plugin>");
        return tryReadPlugin(new PluginConfiguration(
                className,
                booleanXmlAttribute(e, "dont_activate", false)? dontActivate : activateOnStart ,
                elementXPathOrNull(e, "plugin.config")));
    }

    @Nullable private PluginAdapter tryReadPlugin(PluginConfiguration c) {
        try {
            return pluginAdapter(c);
        } catch (Exception x) {
            PluginSubsystem.logError(log, "Plugin "+ c.getClassName(), x);
            return null;
        }
    }

    private PluginAdapter pluginAdapter(PluginConfiguration c)  {
        return new PluginAdapter(c, log);
    }
}
