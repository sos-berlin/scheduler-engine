package com.sos.scheduler.engine.kernel.order;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.cppproxy.OrderC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.util.Lazy;

@ForCpp
public final class Order extends FileBased implements UnmodifiableOrder, Sister {
    private final OrderC cppProxy;
    private final Lazy<UnmodifiableOrder> unmodifiableDelegate = new Lazy<UnmodifiableOrder>() {
        @Override protected UnmodifiableOrder compute() {
            return new UnmodifiableOrderDelegate(Order.this);
        }
    };

    Order(Platform platform, OrderC cppProxy) {
        super(platform);
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {
    }

    @Override public OrderKey getKey() {
        return new OrderKey(getJobChainPath(), getId());
    }

    @Override public OrderId getId() {
        return new OrderId(cppProxy.string_id());
    }

    @Override public OrderState getState() {
        return new OrderState(cppProxy.string_state());
    }

    public void setEndState(OrderState s) {
        cppProxy.set_end_state(s.getString());
    }

//	public String getFilePath() {
//        return cppProxy.file_path();
//    }

    @Override public OrderState getEndState() {
        return new OrderState(cppProxy.string_end_state());
    }

    @Override public String getTitle() {
        return cppProxy.title();
    }

    public AbsolutePath getJobChainPath() {
        return new AbsolutePath(cppProxy.job_chain_path_string());
    }

    public JobChain getJobChain() {
        JobChain result = getJobChainOrNull();
        if (result == null)  throw new SchedulerException("Order is not in a job chain: "+this);
        return result;
    }

    @Nullable public JobChain getJobChainOrNull() {
        Job_chainC jobChainC = cppProxy.job_chain();
        return jobChainC == null? null : jobChainC.getSister();
    }

    public VariableSet getParameters() {
        return cppProxy.params().getSister();
    }

    public UnmodifiableOrder unmodifiableDelegate() {
        return unmodifiableDelegate.get();
    }

    @Override public String toString() {
        String result = getClass().getSimpleName();
        if (cppProxy.cppReferenceIsValid())  result += " " + getId().toString();
        return result;
    }


    public static class Type implements SisterType<Order, OrderC> {
        @Override public Order sister(OrderC proxy, Sister context) {
            return new Order(Platform.of(context), proxy);
        }
    }
}
