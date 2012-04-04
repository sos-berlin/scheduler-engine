package com.sos.scheduler.engine.data.folder;

public class TypedPath {
    private final FileBasedType typ;
    private final AbsolutePath path;

    public TypedPath(FileBasedType typ, AbsolutePath path) {
        this.typ = typ;
        this.path = path;
    }

    public FileBasedType getTyp() {
        return typ;
    }

    public AbsolutePath getPath() {
        return path;
    }
}
