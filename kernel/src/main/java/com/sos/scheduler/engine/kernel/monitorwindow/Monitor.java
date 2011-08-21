package com.sos.scheduler.engine.kernel.monitorwindow;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.Writer;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.kernel.order.jobchain.OrderQueueNode;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

//TODO Test-Beispiel. Könnte über Annotation aktiviert werden, etwa @Monitor.

/** Beispiel, für die Beobachtung der Scheduler-Ereignisse. */
public class Monitor implements EventSubscriber {
    private final Scheduler scheduler;
    private final Writer output = new PrintWriter(System.err);


    public Monitor(Scheduler scheduler) {
        this.scheduler = scheduler;
    }


    public final void close() {
        scheduler.getEventSubsystem().unsubscribe(this);
    }


    public final void activate() {
        scheduler.getEventSubsystem().subscribe(this);
        showJobChains();
    }


    @Override public final void onEvent(Event e) throws IOException {
        if (e instanceof OrderStateChangedEvent)
            processOrderStateChangeEvent((OrderStateChangedEvent)e);
    }


    private void processOrderStateChangeEvent(OrderStateChangedEvent e) throws IOException {
        UnmodifiableOrder order = e.getOrder();
        UnmodifiableJobchain jobChain = order.unmodifiableJobchainOrNull();
        if (jobChain != null) {
            showJobChain(jobChain);
            output.flush();
        }
    }


    private void showJobChains() {
        for (UnmodifiableJobchain jobChain: scheduler.getOrderSubsystem().jobChains())  showJobChain(jobChain);
    }


    private void showJobChain(UnmodifiableJobchain jobChain) {
        try {
            output.write("***Monitor*** " + jobChain.getName() + ": " );
            for (Node node: jobChain.getNodes())  showNode(node);
            output.write("\n");
        }
        catch (IOException x) { throw new RuntimeException(x); }
    }


    private void showNode(Node node) throws IOException {
        output.write(node.getOrderState().getString());
        if (node instanceof OrderQueueNode) {
            OrderQueueNode n = (OrderQueueNode)node;
            output.write("(" + n.getOrderQueue().size() + " orders)");
        }
        output.write("  ");
    }
}
