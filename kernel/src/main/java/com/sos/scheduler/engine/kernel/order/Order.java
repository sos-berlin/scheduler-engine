package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.*;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.cppproxy.OrderC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class Order extends FileBased implements Sister {
    private final OrderC orderC;

    public Order(Platform platform, OrderC orderC) {
        super(platform);
        this.orderC = orderC;
    }


    @Override public void onCppProxyInvalidated() {}


    public OrderId getId() { return new OrderId(orderC.string_id()); }

    public OrderState getState() { return new OrderState(orderC.string_state()); }

    public void setEndState(OrderState s) { orderC.set_end_state(s.string()); }

    public void getFilePath() { orderC.file_path(); }

    public OrderState getEndState() { return new OrderState(orderC.string_end_state()); }

    public String getTitle() { return orderC.title(); }

    
    public JobChain jobChain() {
        JobChain result = jobChainOrNull();
        if (result == null)  throw new NullPointerException(this + ".jobChain");
        return result;
    }


    public JobChain jobChainOrNull() {
        Job_chainC job_chainC = orderC.job_chain();
        return job_chainC == null? null : job_chainC.getSister();
    }

    
    @Override public String toString() { 
        String result = getClass().getSimpleName();
        if (orderC.cppReferenceIsValid())  result += " " + getId().toString();
        return result;
    }


    public static class Type implements SisterType<Order, OrderC> {
        @Override public Order sister(OrderC proxy, Sister context) { return new Order(Platform.of(context), proxy); }
    }
}
