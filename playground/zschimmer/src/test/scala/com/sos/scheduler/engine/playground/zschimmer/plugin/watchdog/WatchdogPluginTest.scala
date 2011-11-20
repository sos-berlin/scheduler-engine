package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.test.ScalaSchedulerTest
import com.sos.scheduler.engine.kernel.log.SchedulerLogLevel
import org.hamcrest.Matchers
import org.hamcrest.MatcherAssert.assertThat
import org.junit._

class WatchdogPluginTest extends ScalaSchedulerTest {
    private val schedulerTimeout = Time.of(15)
    private val sleepTime = Time.of(11)

    @Test def test() {
        controller.startScheduler()
        Thread.sleep(schedulerTimeout.getMillis)
        assertThat(scheduler.log.lastByLevel(SchedulerLogLevel.warn), Matchers.startsWith("SCHEDULER-721"));
        controller.terminateScheduler()
    }

    @HotEventHandler def handleEvent(e: OrderFinishedEvent) {
        Thread.sleep(sleepTime.getMillis)   // Wir  blockieren den Scheduler
    }
}
