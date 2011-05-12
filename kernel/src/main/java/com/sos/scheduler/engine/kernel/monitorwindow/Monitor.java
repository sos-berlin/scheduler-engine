package com.sos.scheduler.engine.kernel.monitorwindow;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.jobchain.OrderQueueNode;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.Writer;

//TODO Test-Beispiel. Könnte über Annotation aktiviert werden, etwa @Monitor.

/** Beispiel, für die Beobachtung der Scheduler-Ereignisse. */
public class Monitor implements EventSubscriber {
    private final Scheduler scheduler;
    private final Writer output = new PrintWriter(System.err);


    public Monitor(Scheduler scheduler) {
        this.scheduler = scheduler;
    }


    public void close() {
        scheduler.getEventSubsystem().unsubscribe(this);
    }


    public void activate() {
        scheduler.getEventSubsystem().subscribe(this);
        showJobChains();
    }


    @Override public void onEvent(Event e) throws IOException {
        if (e instanceof OrderStateChangedEvent)
            processOrderStateChangeEvent((OrderStateChangedEvent)e);
    }


    private void processOrderStateChangeEvent(OrderStateChangedEvent e) throws IOException {
        Order order = e.getObject();
        JobChain jobChain = order.jobChainOrNull();
        if (jobChain != null) {
            showJobChain(jobChain);
            output.flush();
        }
    }


    private void showJobChains() {
        for (JobChain jobChain: scheduler.getOrderSubsystem().jobChains())  showJobChain(jobChain);
    }


    private void showJobChain(JobChain jobChain) {
        try {
            output.write("***Monitor*** " + jobChain.name() + ": " );
            for (Node node: jobChain.nodes())  showNode(node);
            output.write("\n");
        }
        catch (IOException x) { throw new RuntimeException(x); }
    }


    private void showNode(Node node) throws IOException {
        output.write(node.orderState().string());
        if (node instanceof OrderQueueNode) {
            OrderQueueNode n = (OrderQueueNode)node;
            output.write("(" + n.orderQueue().size() + " orders)");
        }
        output.write("  ");
    }
}
