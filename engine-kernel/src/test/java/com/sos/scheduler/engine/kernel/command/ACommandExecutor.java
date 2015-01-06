package com.sos.scheduler.engine.kernel.command;


class ACommandExecutor extends GenericCommandExecutor<ACommand,AResult> {
    static final ACommandExecutor singleton = new ACommandExecutor();

    ACommandExecutor() {
        super(ACommand.class);
    }

    @Override protected final AResult doExecute(ACommand c) {
        return new AResult(c.value);
    }
}
