package com.sos.scheduler.engine.plugins.databasequery;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.Plugin;

import javax.inject.Inject;

public class DatabaseQueryPlugin extends AbstractPlugin implements Plugin, HasCommandHandlers {
    private final CommandHandler[] commandHandlers;

    @Inject public DatabaseQueryPlugin(ShowTaskHistoryCommandExecutor showTaskHistoryCommandExecutor) {
        commandHandlers = new CommandHandler[]{
            showTaskHistoryCommandExecutor,
            ShowTaskHistoryCommandXmlParser.singleton,
            TaskHistoryEntriesResultXmlizer.singleton };
    }

    @Override public final ImmutableList<CommandHandler> commandHandlers() {
        return ImmutableList.copyOf(commandHandlers);
    }
}
