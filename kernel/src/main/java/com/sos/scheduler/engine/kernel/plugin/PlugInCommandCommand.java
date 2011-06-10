package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Command;


public class PlugInCommandCommand implements Command {
    private final String pluginClassName;
    private final Command subcommand;


    public PlugInCommandCommand(String pluginClassName, Command subcommand) {
        this.pluginClassName = pluginClassName;
        this.subcommand = subcommand;
    }


    @Override public final String getName() {
        return "plugin.command";
    }


    public final String getPluginClassName() {
        return pluginClassName;
    }

    
    public final Command getSubcommand() {
        return subcommand;
    }
}
