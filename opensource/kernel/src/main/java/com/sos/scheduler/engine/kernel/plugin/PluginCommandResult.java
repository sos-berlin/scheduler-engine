package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.command.Result;


class PluginCommandResult implements Result {
    private final String pluginClassName;
    private final Result pluginResult;


    PluginCommandResult(String pluginClassName, Result r) {
        this.pluginClassName = pluginClassName;
        this.pluginResult = r;
    }


    String getPluginClassName() {
        return pluginClassName;
    }

    
    Result getPluginResult() {
        return pluginResult;
    }
}
