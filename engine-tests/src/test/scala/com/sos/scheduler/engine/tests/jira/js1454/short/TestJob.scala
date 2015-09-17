package com.sos.scheduler.engine.tests.jira.js1454.short

import com.sos.scheduler.engine.common.time.ScalaJoda._
import org.joda.time.Instant.now

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    val until = now() + 5.s
    while (now() < until) {
      spooler.id
    }
    false
  }
}
