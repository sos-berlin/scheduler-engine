package com.sos.scheduler.engine.kernel.plugin;

import com.google.common.base.Function;
import com.google.common.collect.Iterables;
import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.util.Lazy;
import org.w3c.dom.Element;

import javax.annotation.Nullable;

import static com.sos.scheduler.engine.kernel.util.Util.optional;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.elementXPathOrNull;

public class PluginReader {
    private final PrefixLog log;
    private final Lazy<Injector> lazyInjector;

    PluginReader(PrefixLog log, Lazy<Injector> lazyInjector) {
        this.log = log;
        this.lazyInjector = lazyInjector;
    }

    @Nullable final PluginAdapter tryReadPlugin(Element e) {
        String className = e.getAttribute("java_class");
        if (className.isEmpty())  throw new SchedulerException("Missing attribute java_class in <plugin>");
        return tryReadPlugin(className, elementXPathOrNull(e, "plugin.config"));
    }

    @Nullable private PluginAdapter tryReadPlugin(String className, Element elementOrNull) {
        try {
            return pluginAdapter(className, elementOrNull);
        } catch (Exception x) {
            PluginSubsystem.logError(log, "Plugin "+ className, x);
            return null;
        }
    }

    private PluginAdapter pluginAdapter(String className, @Nullable Element elementOrNull) throws ClassNotFoundException {
        Plugin p = newPlugin(className, elementOrNull);
        return p instanceof CommandPlugin? new CommandPluginAdapter((CommandPlugin)p, className, log)
            : new PluginAdapter(p, className, log);
    }

    private Plugin newPlugin(String className, @Nullable Element elementOrNull) throws ClassNotFoundException {
        return newPluginByDI(pluginClassForName(className), elementOrNull);
    }

    private static Class<? extends Plugin> pluginClassForName(String name) throws ClassNotFoundException {
        Class<?> c = Class.forName(name);
        if (!Plugin.class.isAssignableFrom(c))
            throw new SchedulerException("Plugin does not implement " + Plugin.class.getName());

        @SuppressWarnings("unchecked")
        Class<? extends Plugin> result = (Class<? extends Plugin>)c;
        return result;
    }

    private Plugin newPluginByDI(Class<? extends Plugin> c, @Nullable final Element elementOrNull) {
        Injector myInjector = lazyInjector.get().createChildInjector(Iterables.transform(optional(elementOrNull), elementToModule()));
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
}
