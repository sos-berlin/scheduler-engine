package com.sos.scheduler.engine.kernel.event;

import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;

/** Erleichtert die Implementierung eines EventPlugin. */
public abstract class AbstractEventPlugin extends AbstractPlugin implements EventSubscriber {
    protected final Scheduler scheduler;
    protected final Element pluginElement;

    protected AbstractEventPlugin(Scheduler scheduler, Element pluginElement) {
        this.scheduler = scheduler;
        this.pluginElement = pluginElement;
    }

    @Override public void activate() {
        scheduler.getEventSubsystem().subscribe(this);			// register the plugin in the JS
    }

    @Override public void close() {
        scheduler.getEventSubsystem().unsubscribe(this);		// unregister the plugin in the JS
    }
}
