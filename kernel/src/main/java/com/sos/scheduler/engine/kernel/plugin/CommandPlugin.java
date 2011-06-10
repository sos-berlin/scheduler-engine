package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;

/** Wenn ein Plugin dieses Interface implementiert, können ihm Kommandos gegeben werden. */
public interface CommandPlugin extends PlugIn, HasCommandHandlers {}
