package com.sos.scheduler.engine.kernel.command;


public abstract class GenericCommandExecutor<C extends Command, R extends Result> implements CommandExecutor {
    private final Class<C> commandClass;


    protected GenericCommandExecutor(Class<C> commandClass) {
        this.commandClass = commandClass;
    }


    @Override public final Class<? extends Command> getCommandClass() {
        return commandClass;
    }


    @Override public final Result execute(Command c) {
        assert commandClass.isInstance(c);
        return doExecute((C)c);
    }
    
    protected abstract R doExecute(C c);
}
