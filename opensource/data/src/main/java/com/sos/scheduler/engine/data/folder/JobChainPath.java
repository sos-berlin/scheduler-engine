package com.sos.scheduler.engine.data.folder;

import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

public class JobChainPath extends TypedPath {
    public JobChainPath(@JsonProperty("absolutePath") AbsolutePath path) {
        super(FileBasedType.jobChain, path);
    }

    @JsonCreator
    public static JobChainPath of(String absolutePath) {
        return new JobChainPath(new AbsolutePath(absolutePath));
    }
}
