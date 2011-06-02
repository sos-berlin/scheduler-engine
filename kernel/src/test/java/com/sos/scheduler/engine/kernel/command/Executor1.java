package com.sos.scheduler.engine.kernel.command;


class Executor1 extends GenericCommandExecutor<Command1,Result1> {
    static Executor1 singleton = new Executor1();


    public Executor1() {
        super(Command1.class);
    }
    

    @Override protected final Result1 doExecute(Command1 c) {
        return new Result1(c.value);
    }
}
