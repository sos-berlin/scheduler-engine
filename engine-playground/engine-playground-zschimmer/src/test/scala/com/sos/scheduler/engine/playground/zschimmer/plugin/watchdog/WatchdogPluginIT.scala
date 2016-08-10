package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.test.SchedulerTest
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers
import org.junit._

class WatchdogPluginIT extends SchedulerTest {
    private val schedulerTimeout = 15.s
    private val sleepTime = 11.s

    @Test def test(): Unit = {
        controller.activateScheduler()
        sleep(schedulerTimeout)
        assertThat(instance(classOf[PrefixLog]).lastByLevel(SchedulerLogLevel.warning), Matchers.startsWith("SCHEDULER-721"))
        controller.terminateScheduler()
    }

    @HotEventHandler def handleEvent(e: OrderFinished): Unit = {
        sleep(sleepTime)   // Wir  blockieren den Scheduler
    }
}
