package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.xmlQuoted;


class PlugInAdapter implements PlugIn {
    private final PlugIn plugIn;
    private final String name;
    private final PrefixLog log;


    PlugInAdapter(PlugIn plugIn, String name, PrefixLog log) {
        this.plugIn = plugIn;
        this.name = name;
        this.log = log;
    }


    @Override public void activate() {
        plugIn.activate();
    }


    @Override public void close() {
        plugIn.close();
    }


    @Override public String getXmlState() {
        try {
            return plugIn.getXmlState();
        } catch (Exception x) {
            return "<ERROR text=" + xmlQuoted(x.toString()) + "/>";
        }
    }


    public void tryActivate() {
        try {
            plugIn.activate();
        } catch (Exception x) {
            log.error(this + ": " + x);
        }
    }


    public void tryClose() {
        try {
            plugIn.close();
        } catch (Exception x) {
            log.error(this + ": " + x);
        }
    }


    @Override public String toString() {
        return "PlugIn " + name;
    }
}
