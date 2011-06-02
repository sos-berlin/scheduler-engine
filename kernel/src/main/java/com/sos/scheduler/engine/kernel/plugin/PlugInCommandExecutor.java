package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;


public class PlugInCommandExecutor extends GenericCommandExecutor<PlugInCommandCommand,PlugInCommandResult> {
    private final PlugInSubsystem plugInSubsystem;


    PlugInCommandExecutor(PlugInSubsystem p) {
        super(PlugInCommandCommand.class);
        plugInSubsystem = p;
    }


    @Override public final PlugInCommandResult doExecute(PlugInCommandCommand c) {
        return plugInSubsystem.plugInByClassName(c.getPluginClassName()).executeCommand(c);
    }
}
