package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl;

public class OrderCMock extends CppProxyImpl<Order> implements OrderC {
    private String id = "TESTORDER-ID";
    private String state = "TESTORDER-STATE";
    private String title =" TESTORDER-TITLE";
    private String endState = "";
    private String filePath = "TESTORDER-PATH";

    @Override public boolean cppReferenceIsValid() { return true; }

    @Override public String string_id() { return id; }

    @Override public void set_id(String id) { this.id = id; }

    @Override public String job_chain_path_string() {
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.OrderCMock.job_chain_path_string()");
    }

    @Override public Job_chainC job_chain() {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    @Override public String string_state() { return state; }

    @Override public void set_end_state(String state) { endState = state; }

    @Override public String string_end_state() { return endState; }

    @Override public Prefix_logC log() {
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.OrderCMock.log()");
    }

    @Override public String title() { return title; }

    @Override public String path() {
        throw new UnsupportedOperationException("com.sos.scheduler.engine.kernel.cppproxy.OrderCMock.path()");
    }

    @Override public String file_path() {	return filePath; }

    @Override public Variable_setC params() {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}
