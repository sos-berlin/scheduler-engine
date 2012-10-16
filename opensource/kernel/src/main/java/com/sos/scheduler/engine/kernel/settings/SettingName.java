package com.sos.scheduler.engine.kernel.settings;

public enum SettingName {
    /** Wie -db= */
    dbName(1),

    /** Erweitert den Class-Path für einen Java-Job. */
    jobJavaClassPath(2),

    /** Default für factory.ini [spooler] html_dir */
    htmlDir(3),

    orderDistributedBalanced(7);

    /** Die Zahl muss mit der Zahl im C++-Code Settings.cxx übereinstimmen. */
    private final int number;

    SettingName(int number) {
        this.number = number;
    }

    public int getNumber() {
        return number;
    }
}
