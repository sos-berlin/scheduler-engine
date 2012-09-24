package com.sos.scheduler.engine.data.scheduler;

import com.sos.scheduler.engine.data.base.StringValue;

public class ClusterMemberId extends StringValue {
    public static final ClusterMemberId empty = new ClusterMemberId("");

    public ClusterMemberId(String x) {
        super(x);
    }
}
