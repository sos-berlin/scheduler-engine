package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Command;


public class ShowTaskHistoryCommand implements Command {
    public ShowTaskHistoryCommand() {
        // Das Kommando hat (noch) keine Parameter
    }


    @Override public final String getName() {
        return "show_task_history";
    }
}
