package com.sos.scheduler.engine.data.folder;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderKey;

public final class JobChainPath extends TypedPath {
    public JobChainPath(@JsonProperty("absolutePath") AbsolutePath path) {
        super(FileBasedType.jobChain, path);
    }

    public OrderKey orderKey(OrderId o) {
        return new OrderKey(this, o);
    }

    @JsonCreator
    public static JobChainPath of(String absolutePath) {
        return new JobChainPath(new AbsolutePath(absolutePath));
    }
}
