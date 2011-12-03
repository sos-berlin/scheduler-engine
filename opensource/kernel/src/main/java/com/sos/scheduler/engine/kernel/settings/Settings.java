package com.sos.scheduler.engine.kernel.settings;

import static com.google.common.collect.Maps.newHashMap;

import java.util.Map;

import com.sos.scheduler.engine.kernel.cppproxy.SettingsC;

public class Settings {
    private final Map<SettingName,String> values = newHashMap();

    public void set(SettingName name, String value) {
        values.put(name, value);
    }

    public void setSettingsInCpp(SettingsC cppProxy) {
        for (Map.Entry<SettingName,String> e: values.entrySet())
            cppProxy.set(e.getKey().getNumber(), e.getValue());
    }

    public void setAll(Settings o) {
        values.putAll(o.values);
    }

    public static Settings of(SettingName name, String value) {
        Settings result = new Settings();
        result.set(name, value);
        return result;
    }
}
