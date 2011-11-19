package com.sos.scheduler.engine.kernel.command;

import java.util.Collection;


public interface HasCommandHandlers {
    Collection<CommandHandler> getCommandHandlers();
}
