package com.sos.scheduler.engine.kernel.scheduler;

import com.google.inject.Module;

/** TODO Provisorisch für MyJettyServer. */
public interface HasGuiceModule {
    Module getGuiceModule();
}
