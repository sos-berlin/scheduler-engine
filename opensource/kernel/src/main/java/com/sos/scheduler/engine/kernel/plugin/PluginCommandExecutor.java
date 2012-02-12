package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;
import com.sos.scheduler.engine.kernel.command.Result;

class PluginCommandExecutor extends GenericCommandExecutor<PluginCommandCommand,PluginCommandResult> {
    private final PluginSubsystem subsystem;

    PluginCommandExecutor(PluginSubsystem subsystem) {
        super(PluginCommandCommand.class);
        this.subsystem = subsystem;
    }

    @Override public final PluginCommandResult doExecute(PluginCommandCommand c) {
        PluginAdapter a = subsystem.pluginAdapterByClassName(c.getPluginClassName());
        Result pluginResult = a.getCommandDispatcher().execute(c.getSubcommand());
        return new PluginCommandResult(a.getPluginClassName(), pluginResult);
    }
}
