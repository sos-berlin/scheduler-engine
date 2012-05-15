package com.sos.scheduler.engine.data.folder;

public class JobChainPath extends TypedPath {
    public JobChainPath(AbsolutePath path) {
        super(FileBasedType.jobChain, path);
    }

    public static JobChainPath of(String absolutePath) {
        return new JobChainPath(new AbsolutePath(absolutePath));
    }
}
