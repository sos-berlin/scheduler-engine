//package com.sos.scheduler.engine.kernelcpptest.plugin.previousstate;
//
//import com.sos.scheduler.engine.kernel.Scheduler;
//import com.sos.scheduler.engine.kernel.event.Event;
//import com.sos.scheduler.engine.kernel.event.EventSubscriber;
//import com.sos.scheduler.engine.kernel.order.OrderStateChangeEvent;
//import com.sos.scheduler.engine.kernel.plugin.PlugIn;
//import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
//import org.w3c.dom.Element;
//
//public class PreviousStatePlugIn implements PlugIn {
//    private final Scheduler scheduler;
//    private final EventSubscriber eventSubscriber = new MyEventSubscriber();
//
//
//    PreviousStatePlugIn(Scheduler scheduler, Element plugInElement) {
//        this.scheduler = scheduler;
//        scheduler.getEventSubsystem().subscribe(eventSubscriber);
//    }
//
//    public void close() {
//        scheduler.getEventSubsystem().unSubscribe(eventSubscriber);
//    }
//
//    private static class MyEventSubscriber implements EventSubscriber {
//        @Override
//        public EventResult onEvent(Event e) throws Exception {
//            if (e instanceof OrderStateChangeEvent) {
//                OrderStateChangeEvent a =(OrderStateChangeEvent)e;
//                a.getObject().getParams().setVar("scheduler.previousState", a.getPreviousState().toString());
//            }
//        }
//    }
//
//    public static PlugInFactory factory() {
//        return new PlugInFactory() {
//            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
//                return new PreviousStatePlugIn(scheduler, plugInElement);
//            }
//        };
//    }
//}
