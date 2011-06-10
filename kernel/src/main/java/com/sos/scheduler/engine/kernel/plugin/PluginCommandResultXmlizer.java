package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.GenericResultXmlizer;
import org.w3c.dom.Element;


class PluginCommandResultXmlizer extends GenericResultXmlizer<PluginCommandResult>{
    private final PlugInSubsystem subsystem;


    PluginCommandResultXmlizer(PlugInSubsystem subsystem) {
        super(PluginCommandResult.class);
        this.subsystem = subsystem;

    }


    @Override protected Element doToElement(PluginCommandResult r) {
        CommandPluginAdapter a = subsystem.commandPluginByClassName(r.getPluginClassName());
        return a.getCommandDispatcher().elementOfResult(r.getPluginResult());
    }
}
