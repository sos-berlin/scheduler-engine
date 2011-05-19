package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.*;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.cppproxy.OrderC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.VariableSet;
import com.sos.scheduler.engine.kernel.cppproxy.Variable_setC;


@ForCpp
public class Order extends FileBased implements Sister {
    private final OrderC cppProxy;

    
    public Order(Platform platform, OrderC cppProxy) {
        super(platform);
        this.cppProxy = cppProxy;
    }


    @Override public void onCppProxyInvalidated() {}


    public OrderId getId() { return new OrderId(cppProxy.string_id()); }

    public OrderState getState() { return new OrderState(cppProxy.string_state()); }

    public void setEndState(OrderState s) { cppProxy.set_end_state(s.string()); }

    public void getFilePath() { cppProxy.file_path(); }

    public OrderState getEndState() { return new OrderState(cppProxy.string_end_state()); }

    public String getTitle() { return cppProxy.title(); }

    
    public JobChain jobChain() {
        JobChain result = jobChainOrNull();
        if (result == null)  throw new NullPointerException(this + ".jobChain");
        return result;
    }


    public JobChain jobChainOrNull() {
        Job_chainC job_chainC = cppProxy.job_chain();
        return job_chainC == null? null : job_chainC.getSister();
    }


    public final VariableSet getParameters() {
        return Variable_setC.sisterType.sister(cppProxy.params(), (Sister)null);
    }
    
    
    @Override public String toString() { 
        String result = getClass().getSimpleName();
        if (cppProxy.cppReferenceIsValid())  result += " " + getId().toString();
        return result;
    }


    public static class Type implements SisterType<Order, OrderC> {
        @Override public Order sister(OrderC proxy, Sister context) { return new Order(Platform.of(context), proxy); }
    }
}
