package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;
import com.sos.scheduler.engine.kernel.command.Result;


class PlugInCommandExecutor extends GenericCommandExecutor<PlugInCommandCommand,PluginCommandResult> {
    private final PlugInSubsystem subsystem;


    PlugInCommandExecutor(PlugInSubsystem subsystem) {
        super(PlugInCommandCommand.class);
        this.subsystem = subsystem;
    }


    @Override public final PluginCommandResult doExecute(PlugInCommandCommand c) {
        CommandPluginAdapter a = subsystem.commandPluginByClassName(c.getPluginClassName());
        Result pluginResult = a.getCommandDispatcher().execute(c.getSubcommand());
        return new PluginCommandResult(a.getPluginClassName(), pluginResult);
    }
}
