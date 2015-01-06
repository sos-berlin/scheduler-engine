package com.sos.scheduler.engine.kernel.util;

import static com.sos.scheduler.engine.kernel.util.Classes.directoryOfPackage;

public class ResourcePath {
    private final Package pack;
    private final String subPath;

    public ResourcePath(Package pack) {
        this(pack, "");
    }

    public ResourcePath(Package pack, String subPath) {
        this.pack = pack;
        this.subPath = subPath;
    }

    public final String path() {
        StringBuilder result = new StringBuilder();
        result.append(directoryOfPackage(pack));
        if (!subPath.isEmpty())
            result.append('/').append(subPath);
        return result.toString();
    }

    @Override public String toString() {
        return path();
    }
}
