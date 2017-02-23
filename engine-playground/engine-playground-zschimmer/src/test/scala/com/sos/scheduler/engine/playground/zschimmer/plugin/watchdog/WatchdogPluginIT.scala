package com.sos.scheduler.engine.playground.zschimmer.plugin.watchdog

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.kernel.log.PrefixLog
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.hamcrest.MatcherAssert.assertThat
import org.hamcrest.Matchers
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class WatchdogPluginIT extends FreeSpec with ScalaSchedulerTest {
  private val schedulerTimeout = 15.s
  private val sleepTime = 11.s

  "test" in {
    sleep(schedulerTimeout)
    assertThat(instance[PrefixLog].lastByLevel(SchedulerLogLevel.warning), Matchers.startsWith("SCHEDULER-721"))
    controller.terminateScheduler()
  }

  eventBus.onHot[OrderFinished] { case _ ⇒
    sleep(sleepTime) // Wir blockieren den Scheduler
  }
}
