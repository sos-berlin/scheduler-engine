package com.sos.scheduler.engine.kernel.plugin;

import org.w3c.dom.Element;

import javax.annotation.Nullable;

public class PluginConfiguration {
    private final String className;
    private final ActivationMode activationMode;
    @Nullable private final Element configElementOrNull;

    public PluginConfiguration(String className, ActivationMode activationMode, @Nullable Element configElementOrNull) {
        this.activationMode = activationMode;
        this.configElementOrNull = configElementOrNull;
        this.className = className;
    }

    public final String getClassName() {
        return className;
    }

    public final ActivationMode getActivationMode() {
        return activationMode;
    }

    @Nullable public final Element getConfigElementOrNull() {
        return configElementOrNull;
    }
}
