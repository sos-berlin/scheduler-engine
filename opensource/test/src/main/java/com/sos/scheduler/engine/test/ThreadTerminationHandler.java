package com.sos.scheduler.engine.test;

import javax.annotation.Nullable;

public interface ThreadTerminationHandler {
    void onThreadTermination(@Nullable Throwable t);
}
