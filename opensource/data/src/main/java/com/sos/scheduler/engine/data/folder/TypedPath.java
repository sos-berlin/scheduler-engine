package com.sos.scheduler.engine.data.folder;

import java.io.File;

public class TypedPath {
    private final FileBasedType typ;
    private final AbsolutePath path;

    public TypedPath(FileBasedType typ, AbsolutePath path) {
        this.typ = typ;
        this.path = path;
    }

    public final File file(File baseDirectory) {
        return typ.file(baseDirectory, path);
    }

    public final FileBasedType getTyp() {
        return typ;
    }

    public final AbsolutePath getPath() {
        return path;
    }

    public final String name() {
        return path.getName();
    }

    public String toString() {
        return typ+" "+path;
    }

    public static TypedPath of(FileBasedType t, String absolutePath) {
        return new TypedPath(t, new AbsolutePath(absolutePath));
    }
}
