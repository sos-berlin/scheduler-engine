package com.sos.scheduler.engine.kernel.command;


/** Typsichere abstrakte Implementierung von CommandExecutor. */
public abstract class GenericCommandExecutor<C extends Command, R extends Result> implements CommandExecutor {
    private final Class<C> commandClass;


    protected GenericCommandExecutor(Class<C> commandClass) {
        this.commandClass = commandClass;
    }


    @Override public final Class<? extends Command> getCommandClass() {
        return commandClass;
    }


    @SuppressWarnings("unchecked")
    @Override public final Result execute(Command c) {
        assert commandClass.isInstance(c);
        return doExecute((C)c);
    }
    
    protected abstract R doExecute(C c);
}
