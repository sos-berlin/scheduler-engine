package com.sos.scheduler.engine.kernel.plugin;

import com.google.common.base.Function;
import com.google.common.collect.Iterables;
import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.sos.scheduler.engine.kernel.command.CommandDispatcher;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import org.w3c.dom.Element;

import javax.annotation.Nullable;

import static com.google.common.base.Preconditions.checkState;
import static com.google.common.base.Throwables.propagate;
import static com.sos.scheduler.engine.kernel.util.Util.optional;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.xmlQuoted;

/** Die Engine spricht das Plugin über diesen Adapter an. */
class PluginAdapter {
    private final PluginConfiguration configuration;
    private final PrefixLog log;
    @Nullable private Plugin plugin = null;
    private boolean isActive = false;

    PluginAdapter(PluginConfiguration c, PrefixLog log) {
        this.configuration = c;
        this.log = log;
    }

    final void tryActivate(Injector injector) {
        try {
            activate(injector);
        } catch (Exception x) {
            logThrowable(x);
        }
    }

    final void activate(Injector injector) {
        checkState(!isActive, this +" is already active");
        if (plugin == null)
            plugin = newPlugin(injector);
        plugin.activate();
        isActive = true;
    }

    final String getXmlState() {
        try {
            return getPlugin().getXmlState();
        } catch (Exception x) {
            return "<ERROR text="+ xmlQuoted(x.toString()) +"/>";
        }
    }

    final void tryClose() {
        try {
            if (plugin != null)
                plugin.close();
            isActive = false;
        } catch (Exception x) {
            logThrowable(x);
        }
    }

    private Plugin newPlugin(Injector injector) {
        return newPluginByDI(injector, pluginClassForName(configuration.getClassName()), configuration.getConfigElementOrNull());
    }

    private static Class<? extends Plugin> pluginClassForName(String name) {
        try {
            Class<?> c = Class.forName(name);
            if (!Plugin.class.isAssignableFrom(c))
                throw new SchedulerException("Plugin does not implement " + Plugin.class.getName());

            @SuppressWarnings("unchecked")
            Class<? extends Plugin> result = (Class<? extends Plugin>)c;
            return result;
        } catch (ClassNotFoundException x) { throw propagate(x); }
    }

    private static Plugin newPluginByDI(Injector injector, Class<? extends Plugin> c, @Nullable Element elementOrNull) {
        Injector myInjector = injector.createChildInjector(Iterables.transform(optional(elementOrNull), elementToModule()));
        return myInjector.getInstance(c);
    }

    private static Function<Element,Module> elementToModule() {
        // Funktion, damit Abhängigkeit zu DI-Jars erst zur Laufzeit, bei Bedarf, verlangt werden.
        return new Function<Element,Module>() {
            @Override public Module apply(final Element e) {
                return new AbstractModule() {
                    @Override protected void configure() {
                        bind(Element.class).toInstance(e);
                    }
                };
            }
        };
    }

    final String getPluginClassName() {
        return configuration.getClassName();
    }

    final Plugin getPlugin() {
        if (plugin == null)  throw new IllegalStateException(this +" is not loaded");
        return plugin;
    }

    final CommandDispatcher getCommandDispatcher() {
        Plugin p = getPlugin();
        if (!(p instanceof CommandPlugin))
            throw new SchedulerException("Plugin is not a " + CommandPlugin.class.getSimpleName());
        return new CommandDispatcher(((CommandPlugin)p).getCommandHandlers());
    }

    public final PluginConfiguration getConfiguration() {
        return configuration;
    }

    private void logThrowable(Throwable t) {
        PluginSubsystem.logError(log, toString(), t);
    }

    @Override public String toString() {
        return "Plugin "+ configuration.getClassName();
    }
}
