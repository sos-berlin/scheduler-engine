package com.sos.scheduler.engine.kernel.plugin;

import com.google.inject.Injector;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.util.Lazy;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import org.w3c.dom.Element;

import static com.google.common.base.Throwables.getStackTraceAsString;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;
import static java.util.Arrays.asList;

//TODO Eigenes PrefixLog einführen

public final class PluginSubsystem extends AbstractHasPlatform implements Subsystem, HasCommandHandlers {
    private final CommandHandler[] commandHandlers = {
            new PluginCommandExecutor(this),
            new PluginCommandCommandXmlParser(this),
            new PluginCommandResultXmlizer(this) };
    private final Scheduler scheduler;
    private final Lazy<Injector> lazyInjector;
    private final Map<String,PluginAdapter> plugins = new HashMap<String,PluginAdapter>();
    private final EventBus eventBus;

    public PluginSubsystem(Scheduler scheduler, Lazy<Injector> injector, EventBus eventBus) {
        super(scheduler.getPlatform());
        this.scheduler = scheduler;
        this.lazyInjector = injector;
        this.eventBus = eventBus;
    }

    public void load(Element root) {
        PluginReader pluginReader = new PluginReader(log(), lazyInjector, scheduler);
        Element pluginsElement = elementXPathOrNull(root, "config/plugins");
        if (pluginsElement != null) {
            for (Element e: elementsXPath(pluginsElement, "plugin")) {
                PluginAdapter a = pluginReader.tryReadPlugin(e);
                if (a != null) {
                    plugins.put(a.getPluginClassName(), a);
                    log().info(a + " added");
                }
            }
        }
    }

    public void activate() {
        for (PluginAdapter p: plugins.values()) {
            eventBus.registerAnnotated(p.getPlugin());
            p.tryActivate();
        }
    }

    public void close() {
        for (PluginAdapter p: plugins.values()) {
            eventBus.unregisterAnnotated(p.getPlugin());
            p.tryClose();
        }
    }

    CommandPluginAdapter commandPluginByClassName(String className) {
        PluginAdapter a = pluginByClassName(className);
        if (!(a instanceof CommandPluginAdapter))
            throw new SchedulerException("Plugin is not a " + CommandPlugin.class.getSimpleName());
        return (CommandPluginAdapter)a;
    }

    private PluginAdapter pluginByClassName(String className) {
        PluginAdapter result = plugins.get(className);
        if (result == null)
            throw new SchedulerException("Unknown plugin '" + className + "'");
        return result;
    }

    @Override public Collection<CommandHandler> getCommandHandlers() {
        return asList(commandHandlers);
    }

    static void logError(PrefixLog log, String pluginName, Throwable t) {
        log.error(pluginName + ": " + t + "\n" + getStackTraceAsString(t));
    }
}
