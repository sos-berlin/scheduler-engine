package com.sos.scheduler.engine.core_cpp_test.order.test1

import com.sos.scheduler.engine.kernel.order.OrderEvent
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.order.OrderStateChangeEvent
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.core_cpp_test._
import org.junit._


// Wie die Java-Klasse ExampleTest. scheduler.xml steht deshalb im Java-Verzeichnis, dasselbe Paket.
class ExampleScalaTest extends ScalaSchedulerTest {
    private val eventTimeout = Time.of(3)

    @Test def test() {
        strictSubscribeEvents(new MyEventThread)
        runScheduler(shortTimeout)
    }

    class MyEventThread extends ScalaEventThread {
        filter { case e: OrderEvent => e.getOrder.getId == "id.1" }

        @Override protected def runEventThread() {
            expectEvent(eventTimeout) { case e: OrderTouchedEvent     => e.getOrder.getState == "state.1" }
            expectEvent(eventTimeout) { case e: OrderStateChangeEvent => e.getOrder.getState == "state.2" }
            expectEvent(eventTimeout) { case e: OrderStateChangeEvent => e.getOrder.getState == "state.end" }
            expectEvent(eventTimeout) { case e: OrderFinishedEvent    => e.getOrder.getState == "state.end" }
        }
    }
}
