package com.sos.scheduler.engine.tests.jira.js1145

import TestJob._

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    spooler_task.params.value(DelayName) match {
      case "" ⇒
      case o ⇒ spooler_task.set_delay_spooler_process(o.toDouble)
    }
    false
  }
}

private[js1145] object TestJob {
  val DelayName = "TEST-DELAY"
}
