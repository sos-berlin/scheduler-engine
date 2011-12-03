package com.sos.scheduler.engine.kernel.settings;

public enum SettingName {
    /** Wie -db= */
    dbName(1),

    /** Erweitert den Class-Path für einen Java-Job. */
    jobJavaClassPath(2);

    /** Die Zahl muss mit der Zahl im C++-Code übereinstimmen. */
    private final int number;

    SettingName(int number) {
        this.number = number;
    }

    int getNumber() {
        return number;
    }
}
