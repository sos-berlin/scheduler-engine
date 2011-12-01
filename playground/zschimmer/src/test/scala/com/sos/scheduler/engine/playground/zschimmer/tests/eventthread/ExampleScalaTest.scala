//package com.sos.scheduler.engine.tests.order.test1
//
//import com.sos.scheduler.engine.kernel.order._
//import com.sos.scheduler.engine.kernel.util.Time
//import com.sos.scheduler.engine.test.{ScalaEventThread, ScalaSchedulerTest}
//import org.junit.Test
//
//// Wie die Java-Klasse ExampleTest. scheduler.xml steht deshalb im Java-Verzeichnis, dasselbe Paket.
//class ExampleScalaTest extends ScalaSchedulerTest {
//    private val eventTimeout = Time.of(3)
//
//    @Test def test() {
//        controller.subscribeEvents(new MyEventThread)
//        controller.runScheduler(shortTimeout)
//    }
//
//    class MyEventThread extends ScalaEventThread(controller) {
//        filter { case e: OrderEvent => e.getOrder.getId == "id.1" }
//
//        override protected def runEventThread() {
//            expectEvent(eventTimeout) { case e: OrderTouchedEvent      => e.getOrder.getState == "state.1" }
//            expectEvent(eventTimeout) { case e: OrderStepStartedEvent  => e.getOrder.getState == "state.1" }
//            expectEvent(eventTimeout) { case e: OrderStepEndedEvent    => e.getOrder.getState == "state.1" }
//            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == "state.2" }
//            expectEvent(eventTimeout) { case e: OrderStepStartedEvent  => e.getOrder.getState == "state.2" }
//            expectEvent(eventTimeout) { case e: OrderStepEndedEvent    => e.getOrder.getState == "state.2" }
//            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == "state.end" }
//            expectEvent(eventTimeout) { case e: OrderFinishedEvent     => e.getOrder.getState == "state.end" }
//        }
//    }
//}
