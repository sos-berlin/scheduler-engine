package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.log.SchedulerLogLevel
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.test.SchedulerTest
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers
import org.junit._

class WatchdogPluginTest extends SchedulerTest {
    private val schedulerTimeout = Time.of(15)
    private val sleepTime = Time.of(11)

    @Test def test() {
        controller.activateScheduler()
        Thread.sleep(schedulerTimeout.getMillis)
        assertThat(scheduler.log.lastByLevel(SchedulerLogLevel.warn), Matchers.startsWith("SCHEDULER-721"))
        controller.terminateScheduler()
    }

    @HotEventHandler def handleEvent(e: OrderFinishedEvent) {
        Thread.sleep(sleepTime.getMillis)   // Wir  blockieren den Scheduler
    }
}
