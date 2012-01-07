package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.xmlQuoted;

/** Die Engine spricht das Plugin über diesen Adapter an. */
class PluginAdapter {
    private final Plugin plugin;
    private final String name;
    private final PrefixLog log;

    PluginAdapter(Plugin plugin, String name, PrefixLog log) {
        this.plugin = plugin;
        this.name = name;
        this.log = log;
    }

    final void activate() {
        plugin.activate();
    }

    final String getXmlState() {
        try {
            return plugin.getXmlState();
        } catch (Exception x) {
            return "<ERROR text=" + xmlQuoted(x.toString()) + "/>";
        }
    }

    final void tryActivate() {
        try {
            plugin.activate();
        } catch (Exception x) {
            logThrowable(x);
        }
    }

    final void tryClose() {
        try {
            plugin.close();
        } catch (Exception x) {
            logThrowable(x);
        }
    }

    private void logThrowable(Throwable t) {
        PluginSubsystem.logError(log, toString(), t);
    }

    final String getPluginClassName() {
        return plugin.getClass().getName();
    }

    final Plugin getPlugin() {
        return plugin;
    }

    @Override public String toString() {
        return "Plugin " + name;
    }
}
