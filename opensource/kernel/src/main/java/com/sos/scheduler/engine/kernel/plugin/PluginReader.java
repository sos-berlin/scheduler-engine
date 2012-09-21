package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.w3c.dom.Element;

import javax.annotation.Nullable;

import static com.sos.scheduler.engine.common.xml.XmlUtils.booleanXmlAttribute;
import static com.sos.scheduler.engine.common.xml.XmlUtils.elementXPathOrNull;
import static com.sos.scheduler.engine.kernel.plugin.ActivationMode.activateOnStart;
import static com.sos.scheduler.engine.kernel.plugin.ActivationMode.dontActivate;

class PluginReader {
    private final PrefixLog log;

    PluginReader(PrefixLog log) {
        this.log = log;
    }

    final PluginAdapter readPlugin(Element e) {
        String className = e.getAttribute("java_class");
        if (className.isEmpty()) throw new SchedulerException("Missing attribute java_class in <plugin>");
        return pluginAdapter(new PluginConfiguration(
                className,
                booleanXmlAttribute(e, "dont_activate", false)? dontActivate : activateOnStart,
                elementXPathOrNull(e, "plugin.config")));
    }

    private PluginAdapter pluginAdapter(PluginConfiguration c)  {
        return new PluginAdapter(c, log);
    }
}
