package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.folder.AbsolutePath;
import com.sos.scheduler.engine.data.folder.FileBasedType;
import com.sos.scheduler.engine.data.folder.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.data.order.OrderState;
import com.sos.scheduler.engine.eventbus.HasUnmodifiableDelegate;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.cppproxy.OrderC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.util.Lazy;
import com.sos.scheduler.engine.kernel.variable.VariableSet;

import javax.annotation.Nullable;

@ForCpp
public final class Order extends FileBased implements UnmodifiableOrder, HasUnmodifiableDelegate<UnmodifiableOrder>, Sister {
    private final OrderC cppProxy;
    private final Lazy<UnmodifiableOrder> unmodifiableDelegate = new Lazy<UnmodifiableOrder>() {
        @Override protected UnmodifiableOrder compute() {
            return new UnmodifiableOrderDelegate(Order.this);
        }
    };

    Order(OrderC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {}

    public void remove() {
        cppProxy.java_remove();
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
        cppProxy.set_end_state(s.asString());
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

    @Override public FileBasedType getFileBasedType() {
        return FileBasedType.order;
    }

    @Override public AbsolutePath getPath() {
        return new AbsolutePath(cppProxy.path());
    }

    public JobChainPath getJobChainPath() {
        return JobChainPath.of(cppProxy.job_chain_path_string());
    }

    @Override public JobChain getJobChain() {
        JobChain result = getJobChainOrNull();
        if (result == null)  throw new SchedulerException("Order is not in a job chain: "+this);
        return result;
    }

    @Override @Nullable public JobChain getJobChainOrNull() {
        Job_chainC jobChainC = cppProxy.job_chain();
        return jobChainC == null? null : jobChainC.getSister();
    }

    @Override public VariableSet getParameters() {
        return cppProxy.params().getSister();
    }

    @Override public PrefixLog getLog() {
        return cppProxy.log().getSister();
    }

    @Override public UnmodifiableOrder unmodifiableDelegate() {
        return unmodifiableDelegate.get();
    }

    @Override public String toString() {
        String result = getClass().getSimpleName();
        if (cppProxy.cppReferenceIsValid())  result += " " + getId().toString();
        return result;
    }


    public static class Type implements SisterType<Order, OrderC> {
        @Override public Order sister(OrderC proxy, Sister context) {
            return new Order(proxy);
        }
    }
}
