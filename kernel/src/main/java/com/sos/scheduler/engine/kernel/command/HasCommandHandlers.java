package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableList;

public interface HasCommandHandlers {
    ImmutableList<CommandHandler> commandHandlers();
}
