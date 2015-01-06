package com.sos.scheduler.engine.kernel.util;

public final class Hostware {
    private Hostware() {}

    public static String databasePath(String className, String jdbcUrl) {
        String classOption = " -class="+className;
        return "jdbc" + classOption + " " + quoted(jdbcUrl);
    }

    public static String quoted(String s) {
        return '"' + s.replaceAll("\"", "\"\"") + '"';
    }
}
