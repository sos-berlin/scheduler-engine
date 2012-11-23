package com.sos.scheduler.engine.plugins.databasequery;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.inject.Injector;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin;

import javax.inject.Inject;

public class DatabaseQueryPlugin extends AbstractPlugin implements CommandPlugin {
    private final CommandHandler[] commandHandlers;

    @Inject public DatabaseQueryPlugin(Injector injector) {
        commandHandlers = new CommandHandler[]{
            injector.getInstance(ShowTaskHistoryCommandExecutor.class),
            ShowTaskHistoryCommandXmlParser.singleton,
            TaskHistoryEntriesResultXmlizer.singleton };
    }

    @Override public final ImmutableCollection<CommandHandler> getCommandHandlers() {
        return ImmutableList.copyOf(commandHandlers);
    }
}
