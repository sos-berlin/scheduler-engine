package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Command;


public class PluginCommandCommand implements Command {
    private final String pluginClassName;
    private final Command subcommand;


    public PluginCommandCommand(String pluginClassName, Command subcommand) {
        this.pluginClassName = pluginClassName;
        this.subcommand = subcommand;
    }


    public final String getPluginClassName() {
        return pluginClassName;
    }

    
    public final Command getSubcommand() {
        return subcommand;
    }
}
