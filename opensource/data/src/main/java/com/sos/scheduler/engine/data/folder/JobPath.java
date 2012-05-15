package com.sos.scheduler.engine.data.folder;

public class JobPath extends TypedPath {
    public JobPath(AbsolutePath path) {
        super(FileBasedType.job, path);
    }

    public static JobPath of(String absolutePath) {
        return new JobPath(new AbsolutePath(absolutePath));
    }
}
