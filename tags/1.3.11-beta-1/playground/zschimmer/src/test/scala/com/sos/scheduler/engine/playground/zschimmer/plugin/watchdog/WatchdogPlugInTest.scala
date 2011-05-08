package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.event.Event
import com.sos.scheduler.engine.kernel.event.EventSubscriber
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernelcpptest.ScalaSchedulerTest
import org.junit._


class WatchdogPlugInTest extends ScalaSchedulerTest {
    private val schedulerTimeout = Time.of(15)
    private val sleepTime = Time.of(11)
    private val watchdogPeriod = Time.of(1)

    @Test def test() {
        strictSubscribeEvents(new MyEventSubscriber)
        runScheduler(schedulerTimeout, "-e")
    }

    class MyEventSubscriber extends EventSubscriber {
        override def onEvent(e: Event) {
            e match {
                case e: OrderFinishedEvent => Thread.sleep(sleepTime.getMillis)
                case _ =>
            }
        }
    }
}
