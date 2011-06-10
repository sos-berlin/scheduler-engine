package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.*;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import java.util.Collection;

//Aufteilen in JobChainSubsystem und OrderSubsystem?

public class OrderSubsystem extends AbstractHasPlatform implements Subsystem
{
    private final Order_subsystemC cppproxy;


    public OrderSubsystem(Platform platform, Order_subsystemC cppproxy) {
        super(platform);
        this.cppproxy = cppproxy;
    }


    public final Collection<JobChain> jobChains() {
        return cppproxy.java_file_baseds();
    }

    
    public final JobChain jobChain(AbsolutePath path) {
        return cppproxy.java_file_based_or_null(path.toString());
    }
}
