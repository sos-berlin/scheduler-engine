package com.sos.scheduler.engine.kernel.log;

public interface LogSubscription {
    void onLogged();
    void onClosed();
}
