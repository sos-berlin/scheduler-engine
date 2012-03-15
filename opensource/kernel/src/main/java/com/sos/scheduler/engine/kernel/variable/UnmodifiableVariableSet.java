package com.sos.scheduler.engine.kernel.variable;

import com.google.common.collect.ImmutableCollection;

import javax.annotation.Nullable;

public interface UnmodifiableVariableSet {
    @Nullable String tryGet(String name);
    String get(String name);
    ImmutableCollection<String> getNames();
}