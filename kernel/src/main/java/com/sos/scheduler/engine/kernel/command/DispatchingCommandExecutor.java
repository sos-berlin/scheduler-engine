package com.sos.scheduler.engine.kernel.command;

import com.google.common.collect.ImmutableMap;


class DispatchingCommandExecutor implements CommandExecutor {
    private final ImmutableMap<Class<?>,CommandExecutor> commandExecutors;

    
    DispatchingCommandExecutor(Iterable<CommandExecutor> executors) {
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
        CommandExecutor e = commandExecutor(c.getClass());
        return e.execute(c);
    }


    private CommandExecutor commandExecutor(Class<?> commandClass) {
        CommandExecutor result = commandExecutors.get(commandClass);
        if (result == null)  throw new UnknownCommandException(commandClass.getName());
        return result;
    }
}
