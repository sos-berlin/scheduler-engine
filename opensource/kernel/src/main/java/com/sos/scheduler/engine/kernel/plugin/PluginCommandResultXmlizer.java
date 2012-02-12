package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.GenericResultXmlizer;
import org.w3c.dom.Element;

class PluginCommandResultXmlizer extends GenericResultXmlizer<PluginCommandResult>{
    private final PluginSubsystem subsystem;

    PluginCommandResultXmlizer(PluginSubsystem subsystem) {
        super(PluginCommandResult.class);
        this.subsystem = subsystem;
    }

    @Override protected final Element doToElement(PluginCommandResult r) {
        PluginAdapter a = subsystem.pluginAdapterByClassName(r.getPluginClassName());
        return a.getCommandDispatcher().elementOfResult(r.getPluginResult());
    }
}
