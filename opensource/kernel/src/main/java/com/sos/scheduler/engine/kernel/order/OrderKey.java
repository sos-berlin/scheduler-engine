package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.folder.Path;

public class OrderKey {
    private final Path jobChainPath;
    private final OrderId id;

    public OrderKey(Path jobChainPath, OrderId id) {
        this.id = id;
        this.jobChainPath = jobChainPath;
    }

    public Path getJobChainPath() {
        return jobChainPath;
    }

    public OrderId getId() {
        return id;
    }
}
