package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.kernelcpptest.ScalaSchedulerTest
import org.junit._
import com.sos.scheduler.engine.eventbus.EventHandler

class WatchdogPluginTest extends ScalaSchedulerTest {
    private val schedulerTimeout = Time.of(15)
    private val sleepTime = Time.of(11)

    @Test def test() {
        controller.startScheduler("-e")
        Thread.sleep(schedulerTimeout.getMillis)
        //TODO SCHEDULER-721 soll gemeldet sein und Plugin soll Warnungen ausgegeben haben.   assertThat(scheduler.log().last(SchedulerLogLevel.warn), Matchers.stringStartsWith("SCHEDULER-721"));
        controller.terminateScheduler()
    }

    @EventHandler def handleEvent(e: OrderFinishedEvent) {
        Thread.sleep(sleepTime.getMillis)   // Wir  blockieren den Scheduler
    }
}
