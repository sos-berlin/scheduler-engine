package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.Scheduler;
import org.w3c.dom.Element;


public interface PlugInFactory {
    PlugIn newInstance(Scheduler scheduler, Element configurationOrNull);
}
