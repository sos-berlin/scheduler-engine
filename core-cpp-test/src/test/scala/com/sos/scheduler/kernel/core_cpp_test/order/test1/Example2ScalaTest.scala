package com.sos.scheduler.kernel.core_cpp_test.order.test1

import com.sos.scheduler.kernel.core.order.OrderState
import com.sos.scheduler.kernel.core.order.OrderStateChangeEvent
import com.sos.scheduler.kernel.core.util.Time
import com.sos.scheduler.kernel.core_cpp_test._
import org.junit._


// Wie die Java-Klasse Example2Text. scheduler.xml siehe Java-Verzeichnis, dasselbe Paket.
class Example2ScalaTest extends ScalaSchedulerTest {
    private val eventTimeout = Time.of(3)

    @Test def test() {
        strictSubscribeEvents(new MyEventThread)
        runScheduler(shortTimeout)
    }

    class MyEventThread extends ScalaEventThread {
        filter { case e: OrderStateChangeEvent => e.getOrder.getId == "id.1" }

        @Override protected def runEventThread() {
            expect(new OrderState("state.2"))
            expect(new OrderState("state.end"))
            Thread.sleep(1000)
        }

        def expect(state: OrderState) {
            expectEvent(eventTimeout) { case e: OrderStateChangeEvent => e.getOrder.getState == state }
        }
    }
}
