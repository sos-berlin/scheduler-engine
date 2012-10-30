package com.sos.scheduler.engine.kernel.util;

import java.io.File;

public final class Hostware {
    private Hostware() {}

    public static String h2DatabasePath(File file) {
        return h2DatabasePath(file.toString());
    }

    public static String h2DatabasePath(String name) {
        String classOption = " -class=org.h2.Driver";
        String jdbcUrl = quoted("jdbc:h2:"+name);
        return "jdbc" + classOption + " " + jdbcUrl;
    }

    public static String quoted(String s) {
        return '"' + s.replaceAll("\"", "\"\"") + '"';
    }
}
