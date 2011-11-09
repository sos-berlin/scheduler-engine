package com.sos.scheduler.engine.kernel.test;

import javax.annotation.Nullable;

public interface ThreadTerminationHandler {
    void onThreadTermination(@Nullable Throwable t);
}
