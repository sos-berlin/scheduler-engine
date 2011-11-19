package com.sos.scheduler.engine.kernel.util;

public final class Classes {
    private Classes() {}

    public String directoryOfClass(Class<?> c) {
        return directoryOfPackage(c.getPackage());
    }

    public static String directoryOfPackage(Package p) {
        return p.getName().replace('.', '/');
    }

    /** Für {@link org.springframework.core.io.support.PathMatchingResourcePatternResolver}.*/
    public static String springPattern(Package p) {
        return springPattern(p, "");
    }

    /** Für {@link org.springframework.core.io.support.PathMatchingResourcePatternResolver}.*/
    public static String springPattern(Package p, String pattern) {
        return "classpath*:"+ directoryOfPackage(p) +"/" + pattern;
    }
}
