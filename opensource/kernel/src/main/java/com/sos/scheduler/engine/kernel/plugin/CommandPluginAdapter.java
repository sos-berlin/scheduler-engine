package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.CommandDispatcher;
import com.sos.scheduler.engine.kernel.log.PrefixLog;


class CommandPluginAdapter extends PluginAdapter {
    private final CommandDispatcher commandDispatcher;


    CommandPluginAdapter(CommandPlugin plugin, String name, PrefixLog log) {
        super(plugin, name, log);
        this.commandDispatcher = new CommandDispatcher(plugin.getCommandHandlers());
    }


    CommandDispatcher getCommandDispatcher() {
        return commandDispatcher;
    }
}
