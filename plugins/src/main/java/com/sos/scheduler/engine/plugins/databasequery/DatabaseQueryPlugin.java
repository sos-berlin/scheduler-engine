package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import java.util.Collection;
import org.w3c.dom.Element;
import static java.util.Arrays.asList;


public class DatabaseQueryPlugin extends AbstractPlugin implements CommandPlugin {
    private final DatabaseSubsystem databaseSubsystem;
    private final CommandHandler[] commandHandlers;


    DatabaseQueryPlugin(DatabaseSubsystem db) {
        this.databaseSubsystem = db;
        commandHandlers = new CommandHandler[]{
            new ShowTaskHistoryCommandExecutor(databaseSubsystem),
            ShowTaskHistoryCommandXmlParser.singleton,
            TaskHistoryEntriesResultXmlizer.singleton };
    }


    @Override public Collection<CommandHandler> getCommandHandlers() {
        return asList(commandHandlers);
    }


	public static PlugInFactory factory() {
    	return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
            	return new DatabaseQueryPlugin(scheduler.getDatabaseSubsystem());
            }
        };
    }
}
