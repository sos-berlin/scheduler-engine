package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.kernel.SchedulerException;


public class DispatchingCommandExecutor implements CommandExecutor {
    private final ImmutableMap<Class<?>,CommandExecutor> commandExecutors;

    
    public DispatchingCommandExecutor(Iterable<CommandExecutor> executors) {
        commandExecutors = mapOfExecutors(executors);
    }


    private static ImmutableMap<Class<?>,CommandExecutor> mapOfExecutors(Iterable<CommandExecutor> executors) {
        ImmutableMap.Builder<Class<?>,CommandExecutor> b = new ImmutableMap.Builder<Class<?>,CommandExecutor>();
        for (CommandExecutor ex: executors) b.put(ex.getCommandClass(), ex);
        return b.build();
    }


    @Override public Class<? extends Command> getCommandClass() {
        return Command.class;   // Dummy
    }

    
    @Override public Result execute(Command c) {
        CommandExecutor ex = commandExecutor(c.getClass());
        return ex.execute(c);
    }


    private CommandExecutor commandExecutor(Class<?> commandClass) {
        CommandExecutor result = commandExecutors.get(commandClass);
        if (result == null)  throw new UnknownCommandException(commandClass.getName());
        return result;
    }
}
