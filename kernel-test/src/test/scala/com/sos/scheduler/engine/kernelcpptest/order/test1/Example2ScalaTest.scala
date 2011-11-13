package com.sos.scheduler.engine.kernelcpptest.order.test1

import com.sos.scheduler.engine.kernel.order.OrderState
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernelcpptest._
import org.junit._

// Wie die Java-Klasse Example2Text. scheduler.xml siehe Java-Verzeichnis, dasselbe Paket.
class Example2ScalaTest extends ScalaSchedulerTest {
    private val eventTimeout = Time.of(3)

    @Test def test() {
        controller.strictSubscribeEvents(new MyEventThread)
        controller.runScheduler(shortTimeout)
    }

    class MyEventThread extends ScalaEventThread(controller) {
        filter { case e: OrderStateChangedEvent => e.getOrder.getId == "id.1" }

        @Override protected def runEventThread() {
            expect(new OrderState("state.2"))
            expect(new OrderState("state.end"))
            Thread.sleep(1000)
        }

        def expect(state: OrderState) {
            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == state }
        }
    }
}
