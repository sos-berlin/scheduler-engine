package com.sos.scheduler.engine.kernel.order;

import static com.google.common.base.Objects.equal;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.Path;

public class OrderKey {
    private final Path jobChainPath;
    private final OrderId id;

    public OrderKey(Path jobChainPath, OrderId id) {
        this.id = id;
        this.jobChainPath = jobChainPath;
    }

    public final Path getJobChainPath() {
        return jobChainPath;
    }

    public final OrderId getId() {
        return id;
    }

    @Override public int hashCode() {
        return Objects.hashCode(jobChainPath, id);
    }

    @Override public boolean equals(Object o) {
        return o instanceof OrderKey && eq((OrderKey)o);
    }

    private boolean eq(OrderKey o) {
        return equal(jobChainPath, o.jobChainPath) && equal(id, o.id);
    }

    @Override public String toString() {
        return jobChainPath+":"+id;
    }

    public static OrderKey of(String jobChainPath, String id) {
        return new OrderKey(new AbsolutePath(jobChainPath), new OrderId(id));
    }
}
