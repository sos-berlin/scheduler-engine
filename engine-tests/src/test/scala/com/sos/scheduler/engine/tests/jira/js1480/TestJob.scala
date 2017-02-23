package com.sos.scheduler.engine.tests.jira.js1480

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.tests.jira.js1480.TestJob._

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  private var remaining = 5

  override def spooler_open() = {
    spooler_log.info(LogLine)
    true
  }

  override def spooler_process() =
    remaining > 0 && {
      remaining -= 1
      sleep(1.s)
      true
    }
}

private[js1480] object TestJob {
  val LogLine = "TEST JOB IS RUNNING"
}
