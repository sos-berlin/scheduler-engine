package com.sos.scheduler.engine.plugins.databasequery;

import javax.persistence.EntityManager;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import java.util.Collection;
import org.w3c.dom.Element;
import static java.util.Arrays.asList;


public class DatabaseQueryPlugin extends AbstractPlugin implements CommandPlugin {
    private final CommandHandler[] commandHandlers;


    DatabaseQueryPlugin(EntityManager em, String schedulerId, String clusterMemberId) {
        commandHandlers = new CommandHandler[]{
            new ShowTaskHistoryCommandExecutor(em, schedulerId, clusterMemberId),
            ShowTaskHistoryCommandXmlParser.singleton,
            TaskHistoryEntriesResultXmlizer.singleton };
    }


    @Override public final Collection<CommandHandler> getCommandHandlers() {
        return asList(commandHandlers);
    }


	public static PlugInFactory factory() {
    	return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
            	return new DatabaseQueryPlugin(scheduler.getDatabaseSubsystem().getEntityManager(),
                        scheduler.getSchedulerId(), scheduler.getClusterMemberId());
            }
        };
    }
}
