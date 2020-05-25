package com.sos.scheduler.engine.tests.jira.jitl606

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class Jitl606IT extends FreeSpec with ScalaSchedulerTest
{
  private val orderKey = JobChainPath("/test") orderKey "ORDER"

  "JITL-606" in {
    for (i <- 1 to 3) {
      withEventPipe { eventPipe =>
        if (i > 1) sleep(100.ms)  // JobScheduler needs some time after OrderFinished
        scheduler executeXml <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} at="now"/>
        eventPipe.next[OrderFinished](orderKey)
      }
    }
  }
}
