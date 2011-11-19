package com.sos.scheduler.engine.kernel.util;

import java.io.File;

public final class Hostware {
    private Hostware() {}

    public static String h2DatabasePath(File file) {
        String classOption = " -class=org.h2.Driver";
        String jdbcUrl = quoted("jdbc:h2:"+file);
        return "jdbc" + classOption + " " + jdbcUrl;
    }

    public static String quoted(String s) {
        return '"' + s.replaceAll("\"", "\"\"") + '"';
    }
}
