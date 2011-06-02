package com.sos.scheduler.engine.kernel.command;


class Executor2 extends GenericCommandExecutor<Command2,Result2> {
    static Executor2 singleton = new Executor2();


    public Executor2() {
        super(Command2.class);
    }


    @Override protected final Result2 doExecute(Command2 c) {
        return new Result2(c.value);
    }
}
