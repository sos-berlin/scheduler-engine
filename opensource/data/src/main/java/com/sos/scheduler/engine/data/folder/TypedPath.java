package com.sos.scheduler.engine.data.folder;

import java.io.File;

public class TypedPath extends AbsolutePath {
    private final FileBasedType typ;

    public TypedPath(FileBasedType typ, AbsolutePath path) {
        super(path);
        this.typ = typ;
    }

    public final File file(File baseDirectory) {
        return typ.file(baseDirectory, this);
    }

    public final FileBasedType typ() {
        return typ;
    }

//    public int hashCode() {
//        return 31*typ.hashCode() + super.hashCode();
//    }

    @Override public boolean equals(Object o) {
        return o instanceof TypedPath && eq((TypedPath)o);
    }

    private boolean eq(TypedPath o) {
        return typ == o.typ && super.equals(o);
    }

    @Override public String toString() {
        return typ+" "+super.toString();
    }

    public static TypedPath of(FileBasedType t, String absolutePath) {
        return new TypedPath(t, new AbsolutePath(absolutePath));
    }
}
