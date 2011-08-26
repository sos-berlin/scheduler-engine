package com.sos.scheduler.engine.kernelcpptest.order.test1

import java.lang.Override
import org.junit.Ignore
import org.junit.Test
import com.sos.scheduler.engine.kernel.order.OrderEvent
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent
import com.sos.scheduler.engine.kernel.order.OrderStepBeginEvent
import com.sos.scheduler.engine.kernel.order.OrderStepEndEvent
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernelcpptest.ScalaEventThread
import com.sos.scheduler.engine.kernelcpptest.ScalaSchedulerTest


// Wie die Java-Klasse ExampleTest. scheduler.xml steht deshalb im Java-Verzeichnis, dasselbe Paket.
class ExampleScalaTest extends ScalaSchedulerTest {
    private val eventTimeout = Time.of(3)

//    @Ignore	//FIXME Test wieder laufen lassen, wenn OrderStepBeginEvent usw. laufen
//    @Test def test() {
//        strictSubscribeEvents(new MyEventThread)
//        runScheduler(shortTimeout)
//    }

    class MyEventThread extends ScalaEventThread {
        filter { case e: OrderEvent => e.getOrder.getId == "id.1" }

        @Override protected def runEventThread() {
            expectEvent(eventTimeout) { case e: OrderTouchedEvent      => e.getOrder.getState == "state.1" }
            expectEvent(eventTimeout) { case e: OrderStepBeginEvent    => e.getOrder.getState == "state.1" }
            expectEvent(eventTimeout) { case e: OrderStepEndEvent      => e.getOrder.getState == "state.1" }
            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == "state.2" }
            expectEvent(eventTimeout) { case e: OrderStepBeginEvent    => e.getOrder.getState == "state.2" }
            expectEvent(eventTimeout) { case e: OrderStepEndEvent      => e.getOrder.getState == "state.2" }
            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == "state.end" }
            expectEvent(eventTimeout) { case e: OrderFinishedEvent     => e.getOrder.getState == "state.end" }
        }
    }
}
