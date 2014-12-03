package com.sos.scheduler.engine.kernel.log;

public interface LogSubscription {
    void onStarted();

    void onClosed();

    void onLogged();
}
