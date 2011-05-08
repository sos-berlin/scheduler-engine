package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import java.util.ArrayList;


/**
 *
 * @author Zschimmer.sos
 */
public class Order_subsystemCMock extends CppProxyImpl<Sister> implements Order_subsystemC
{
    @Override public boolean cppReferenceIsValid() { return true; }

    @Override public int finished_orders_count() { return 7; }

    @Override public ArrayList<JobChain> java_file_baseds() { return new ArrayList<JobChain>(0); }

    @Override public JobChain java_file_based_or_null(String p) { return null; }
}
