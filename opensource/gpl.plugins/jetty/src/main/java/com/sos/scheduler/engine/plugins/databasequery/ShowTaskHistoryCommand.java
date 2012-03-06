package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Command;


public class ShowTaskHistoryCommand implements Command {
    public static final int defaultLimit = 50;
    
    private final int limit;

    
    public ShowTaskHistoryCommand(int limit) {
        if (limit < 1) throw new IllegalArgumentException(getClass().getName() + " limit is not >= 1");
        this.limit = limit;
    }


    public final int getLimit() {
        return limit;
    }
}
