package com.sos.scheduler.engine.kernel.command;

class BCommandExecutor extends GenericCommandExecutor<BCommand,BResult> {
    static final BCommandExecutor singleton = new BCommandExecutor();

    BCommandExecutor() {
        super(BCommand.class);
    }

    @Override protected final BResult doExecute(BCommand c) {
        return new BResult(c.value);
    }
}
