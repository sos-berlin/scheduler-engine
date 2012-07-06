package com.sos.scheduler.engine.data.order;

import com.google.common.base.Objects;
import com.sos.scheduler.engine.data.folder.JobChainPath;
import org.codehaus.jackson.annotate.JsonProperty;

import static com.google.common.base.Objects.equal;

public class OrderKey {
    private final JobChainPath jobChainPath;
    private final OrderId id;

    public OrderKey(@JsonProperty("jobChainPath") JobChainPath jobChainPath, @JsonProperty("id") OrderId id) {
        this.id = id;
        this.jobChainPath = jobChainPath;
    }

    public final JobChainPath getJobChainPath() {
        return jobChainPath;
    }

    public final String jobChainPathString() {
        return jobChainPath.asString();
    }

    public final OrderId getId() {
        return id;
    }

    public final String idString() {
        return id.asString();
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
        return new OrderKey(JobChainPath.of(jobChainPath), new OrderId(id));
    }
}
