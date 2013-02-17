package com.sos.scheduler.engine.data.folder;

import java.io.File;

public class TypedPath extends AbsolutePath {
    private final FileBasedType typ;

    protected TypedPath(FileBasedType typ, AbsolutePath path) {
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
        return of(t, new AbsolutePath(absolutePath));
    }

    public static TypedPath of(FileBasedType t, AbsolutePath absolutePath) {
        switch (t) {
            case job: return new JobPath(absolutePath);
            case jobChain: return new JobChainPath(absolutePath);
            case folder:
            case lock:
            case order:
            case processClass:
            case schedule:
                return new TypedPath(t, absolutePath);  // Bis die anderen ...Path implementiert sind.
        }
        throw new IllegalArgumentException();  // Javac will das
    }
}
