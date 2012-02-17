package com.sos.scheduler.engine.kernel.folder;

public class AbsolutePath extends Path {
    public AbsolutePath(String p) {
        super(p);
        assertIsEmptyOrAbsolute();
    }

    public final String getName() {
        return getString().substring(getString().lastIndexOf('/') + 1);
    }

    public final String withTrailingSlash() {
        String result = getString();
        return result.endsWith("/")? result : result +"/";
    }

    /** @param path ist absolut oder relativ zur Wurzel. */
    public static AbsolutePath of(String path) {
        return new AbsolutePath(path.startsWith("/")? path : "/" + path);
    }

    public static AbsolutePath of(AbsolutePath parentPath, String subpath) {
        StringBuilder a = new StringBuilder();
        a.append(stripTrailingSlash(parentPath.getString()));
        a.append('/');
        a.append(subpath);
        return of(a.toString());
    }

    private static String stripTrailingSlash(String a) {
        return a.endsWith("/")? a.substring(0, a.length() - 1) : a;
    }
}
