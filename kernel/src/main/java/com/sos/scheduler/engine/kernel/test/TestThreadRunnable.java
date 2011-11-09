package com.sos.scheduler.engine.kernel.test;

import static com.google.common.base.Throwables.propagate;

final class TestThreadRunnable implements Runnable {
    private final ThreadTerminationHandler terminationHandler;
    private final Runnable runnable;

    TestThreadRunnable(Runnable runnable, ThreadTerminationHandler terminationHandler) {
        this.runnable = runnable;
        this.terminationHandler = terminationHandler;
    }

    @Override public void run() {
        try {
            runnable.run();
            terminationHandler.onThreadTermination(null);
        } catch (Throwable t) {
            terminationHandler.onThreadTermination(t);
            throw propagate(t);
        }
    }
}
