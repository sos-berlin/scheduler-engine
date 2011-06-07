package com.sos.scheduler.engine.kernel.command;


class BCommandExecutor extends GenericCommandExecutor<BCommand,BResult> {
    static BCommandExecutor singleton = new BCommandExecutor();


    public BCommandExecutor() {
        super(BCommand.class);
    }


    @Override protected final BResult doExecute(BCommand c) {
        return new BResult(c.value);
    }
}
