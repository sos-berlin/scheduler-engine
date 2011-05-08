package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.*;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import java.util.Collection;

//TODO Aufteilen in JobChainSubsystem und OrderSubsystem?

public class OrderSubsystem extends AbstractHasPlatform
{
    private final Order_subsystemC order_subsystemC;


    public OrderSubsystem(Platform platform, Order_subsystemC order_subsystemC) {
        super(platform);
        this.order_subsystemC = order_subsystemC;
    }


    public Collection<JobChain> jobChains() { return order_subsystemC.java_file_baseds(); }

    public JobChain jobChain(AbsolutePath path) {
        return order_subsystemC.java_file_based_or_null(path.toString());
    }
}
