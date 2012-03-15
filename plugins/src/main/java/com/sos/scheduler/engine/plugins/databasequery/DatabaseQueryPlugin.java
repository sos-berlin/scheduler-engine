package com.sos.scheduler.engine.plugins.databasequery;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;

import javax.inject.Inject;
import javax.persistence.EntityManager;

public class DatabaseQueryPlugin extends AbstractPlugin implements CommandPlugin {
    private final CommandHandler[] commandHandlers;

    @Inject public DatabaseQueryPlugin(EntityManager em, SchedulerConfiguration schedulerConfiguration) {
        commandHandlers = new CommandHandler[]{
            new ShowTaskHistoryCommandExecutor(em, schedulerConfiguration.schedulerId(), schedulerConfiguration.clusterMemberId()),
            ShowTaskHistoryCommandXmlParser.singleton,
            TaskHistoryEntriesResultXmlizer.singleton };
    }

    @Override public final ImmutableCollection<CommandHandler> getCommandHandlers() {
        return ImmutableList.copyOf(commandHandlers);
    }
}
