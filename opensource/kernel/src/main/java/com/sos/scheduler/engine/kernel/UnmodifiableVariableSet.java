package com.sos.scheduler.engine.kernel;

import java.util.Collection;

import javax.annotation.Nullable;

public interface UnmodifiableVariableSet {
    @Nullable String tryGet(String name);
    String get(String name);
    Collection<String> getNames();
}