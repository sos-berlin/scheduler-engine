package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;

import java.util.ArrayList;


/**
 *
 * @author Zschimmer.sos
 */
public class Order_subsystemCMock extends CppProxyImpl<Sister> implements Order_subsystemC
{
    @Override public boolean cppReferenceIsValid() { return true; }

    @Override public int finished_orders_count() { return 7; }

    @Override public Job_chainC java_file_based(String path) {
        //FIXME Implementierung fehlt
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock.file_based()");
    }

    @Override public Job_chainC java_file_based_or_null(String path) {
        //FIXME Implementierung fehlt
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock.file_based_or_null()");
    }

    @Override public Job_chainC java_active_file_based(String path) {
        //FIXME Implementierung fehlt
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock.active_file_based()");
    }

    @Override public ArrayList<JobChain> java_file_baseds() { return new ArrayList<>(0); }

    @Override public String[] file_based_paths(boolean visibleOnly) {
        //FIXME Implementierung fehlt
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock.file_based_paths()");
    }

    @Override public boolean is_empty() {
        //FIXME Implementierung fehlt
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemCMock.is_empty()");
    }
}
