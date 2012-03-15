package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableCollection;

public interface HasCommandHandlers {
    ImmutableCollection<CommandHandler> getCommandHandlers();
}
